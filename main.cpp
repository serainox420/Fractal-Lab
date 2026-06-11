#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

// ImGui
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

// --- STRUKTURA PARAMETRU Z AUTO-EWOLUCJĄ ---
struct DynamicParam {
    float val;
    float min_val;
    float max_val;
    bool auto_evolve;
    float speed;

    DynamicParam(float v, float min, float max, float spd = 0.5f) 
        : val(v), min_val(min), max_val(max), auto_evolve(false), speed(spd) {}

    void update(float dt) {
        if (auto_evolve) {
            val += speed * dt;
            if (val > max_val) { val = max_val; speed *= -1.0f; } // Odbicie od sufitu
            if (val < min_val) { val = min_val; speed *= -1.0f; } // Odbicie od podłogi
        }
    }
};

// Funkcja rysująca suwak z ewolucją w ImGui
void DrawParamUI(const char* label, DynamicParam& p) {
    ImGui::PushID(label);
    ImGui::Text("%s", label);
    ImGui::SliderFloat("##val", &p.val, p.min_val, p.max_val, "%.3f");
    ImGui::SameLine();
    ImGui::Checkbox("Auto", &p.auto_evolve);
    if (p.auto_evolve) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::SliderFloat("Speed", &p.speed, -5.0f, 5.0f, "%.1f");
    }
    ImGui::Separator();
    ImGui::PopID();
}

// --- SHADER BAZOWY ---
const char* vertexShaderSource = 
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main() { gl_Position = vec4(aPos, 1.0); }\n";

// HEADER: Definiuje Uniformy, podstawową matmę, SDF (Kształty)
const std::string fragHeader = R"(
#version 330 core
out vec4 FragColor;
uniform vec2 u_resolution;
uniform float u_time;

// Twoje podpięte parametry
uniform float u_rotX; uniform float u_rotY; uniform float u_rotZ;
uniform float u_shapeSpd; uniform float u_colorSpd;
uniform float u_amp; uniform float u_freq; uniform float u_warp;
uniform float u_fresnel; uniform float u_camDist;

// --- FUNKCJE POMOCNICZE (MOŻESZ ICH UŻYWAĆ W SWOIM KODZIE) ---
mat2 rot(float a) { float s=sin(a), c=cos(a); return mat2(c,-s,s,c); }

// Smooth Minimum (Zlewa ze sobą kształty jak woda)
float smin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5*(b-a)/k, 0.0, 1.0);
    return mix(b, a, h) - k*h*(1.0-h);
}

// SDF Primitives (Kształty podstawowe)
float sdSphere(vec3 p, float s) { return length(p) - s; }
float sdBox(vec3 p, vec3 b) { vec3 q = abs(p)-b; return length(max(q,0.0))+min(max(q.x,max(q.y,q.z)),0.0); }
float sdTorus(vec3 p, vec2 t) { vec2 q = vec2(length(p.xz)-t.x,p.y); return length(q)-t.y; }
float sdCylinder(vec3 p, vec3 c) { return length(p.xz-c.xy)-c.z; }

)";

// DOMYŚLNA FUNKCJA UŻYTKOWNIKA (To ląduje w oknie edytora)
char userCodeBuffer[8192] = R"(// TO JEST TWÓJ KOD GEOMETRII - MOŻESZ GO MODYFIKOWAĆ
float map(vec3 p) {
    // 1. Zewnętrzna Rotacja bryły
    p.xy *= rot(u_rotZ);
    p.xz *= rot(u_rotY);
    p.yz *= rot(u_rotX);

    float t = u_time * u_shapeSpd;
    
    // 2. TWORZENIE BAZY (Łączymy Kulę i Sześcian za pomocą smin!)
    float kula = sdSphere(p - vec3(sin(t), 0.0, 0.0), 1.2);
    float pudlo = sdBox(p + vec3(sin(t), 0.0, 0.0), vec3(0.8));
    float d = smin(kula, pudlo, 0.8); // 0.8 to siła złączenia (rtęć)

    // 3. FAKTURA I RZEŹBA (Domain Warping i Szum)
    vec3 q = p;
    float amp = u_amp;  
    float freq = u_freq; 

    for(int i = 0; i < 4; i++) {
        q.xy *= rot(0.5); q.yz *= rot(0.7); // Łamanie symetrii
        
        vec3 offset = vec3(sin(t*0.5), cos(t*0.3), sin(t*0.4));
        float disp = sin(q.x * freq + offset.x) * sin(q.y * freq + offset.y) * sin(q.z * freq + offset.z);

        d += amp * disp;
        q += disp * u_warp; // Odkształcenie tkanki wewnątrz samej siebie
        
        amp *= 0.5; freq *= 2.0;
    }
    
    return d * 0.4; // Zabezpieczenie promienia przed przebiciem fal
}
)";

// FOOTER: Silnik Ray Marching i Oświetlenie
const std::string fragFooter = R"(
vec3 getNormal(vec3 p) {
    vec2 e = vec2(0.002, 0.0);
    return normalize(vec3(map(p+e.xyy)-map(p-e.xyy), map(p+e.yxy)-map(p-e.yxy), map(p+e.yyx)-map(p-e.yyx)));
}
float getAO(vec3 p, vec3 n) {
    float ao = 0.0; float step = 0.1;
    for(int i=1; i<=4; i++) { ao += max(0.0, (step*i - map(p + n * step*i)) / (step*i)); }
    return clamp(1.0 - ao*0.3, 0.0, 1.0);
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * u_resolution.xy) / u_resolution.y;
    vec3 ro = vec3(0.0, 0.0, -u_camDist);
    vec3 rd = normalize(vec3(uv, 1.0));
    
    float t = 0.0; float max_d = 25.0;
    vec3 col = vec3(0.015, 0.01, 0.025);
    
    for (int i = 0; i < 150; i++) {
        vec3 p = ro + rd * t;
        float d = map(p);
        if (d < 0.001) {
            vec3 n = getNormal(p);
            vec3 l1 = normalize(vec3(1.0, 1.5, -2.0));
            vec3 l2 = normalize(vec3(-1.0, -0.5, -1.0));
            vec3 v = normalize(ro - p);
            
            float dif1 = max(dot(n, l1), 0.0);
            float dif2 = max(dot(n, l2), 0.0) * 0.3;
            float spec = pow(max(dot(n, normalize(l1+v)), 0.0), 32.0);
            float fresnel = pow(1.0 - max(dot(n, v), 0.0), u_fresnel);
            float ao = getAO(p, n);
            
            vec3 objCol = vec3(0.4, 0.1, 0.3);
            objCol = mix(objCol, vec3(0.1, 0.5, 0.6), sin(length(p)*2.0 + u_time*u_colorSpd)*0.5+0.5);
            
            col = objCol * (dif1 + dif2 + 0.1) * ao;
            col += vec3(1.0, 0.9, 0.8) * spec * ao;
            col += vec3(0.5, 0.2, 0.8) * fresnel * ao;
            break;
        }
        t += d;
        if (t > max_d) break;
    }
    col *= 1.0 - 0.4 * length(uv);
    FragColor = vec4(pow(col, vec3(0.4545)), 1.0);
}
)";

// Globalne identyfikatory Shadera i Uniformów
GLuint shaderProgram;
GLint locRes, locTime, locRotX, locRotY, locRotZ, locShapeSpd, locColorSpd, locAmp, locFreq, locWarp, locFresnel, locCamDist;
std::string lastCompilerError = "";

// Funkcja w locie kompilująca Shader
bool RecompileShader(const char* userCode) {
    std::string fullFragSource = fragHeader + userCode + fragFooter;
    const char* fSrc = fullFragSource.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fSrc, NULL);
    glCompileShader(fs);

    GLint success;
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(fs, 1024, NULL, infoLog);
        lastCompilerError = std::string(infoLog);
        glDeleteShader(vs); glDeleteShader(fs);
        return false;
    }

    GLuint newProgram = glCreateProgram();
    glAttachShader(newProgram, vs);
    glAttachShader(newProgram, fs);
    glLinkProgram(newProgram);

    glDeleteShader(vs); glDeleteShader(fs);

    // Sukces! Podmieniamy stary program
    if (shaderProgram) glDeleteProgram(shaderProgram);
    shaderProgram = newProgram;
    lastCompilerError = "Sukces! Shader skompilowany poprawnie.";

    // Musimy odświeżyć lokalizacje uniformów dla nowego programu!
    locRes = glGetUniformLocation(shaderProgram, "u_resolution");
    locTime = glGetUniformLocation(shaderProgram, "u_time");
    locRotX = glGetUniformLocation(shaderProgram, "u_rotX");
    locRotY = glGetUniformLocation(shaderProgram, "u_rotY");
    locRotZ = glGetUniformLocation(shaderProgram, "u_rotZ");
    locShapeSpd = glGetUniformLocation(shaderProgram, "u_shapeSpd");
    locColorSpd = glGetUniformLocation(shaderProgram, "u_colorSpd");
    locAmp = glGetUniformLocation(shaderProgram, "u_amp");
    locFreq = glGetUniformLocation(shaderProgram, "u_freq");
    locWarp = glGetUniformLocation(shaderProgram, "u_warp");
    locFresnel = glGetUniformLocation(shaderProgram, "u_fresnel");
    locCamDist = glGetUniformLocation(shaderProgram, "u_camDist");

    return true;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    // Okno teraz wystartuje wielkie
    GLFWwindow* window = glfwCreateWindow(1600, 900, "God-Mode Fractal Engine", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (GLEW_OK != glewInit()) return -1;
    glGetError();

    // ImGui Init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::StyleColorsDark();

    // Pierwsza kompilacja
    RecompileShader(userCodeBuffer);

    // Kształty Geometryczne - pełna kontrola na starcie
    float quadVertices[] = { -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f };
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // INSTANCJE PARAMETRÓW (start, min, max, szybkosc_ewolucji)
    DynamicParam p_rotX(0.0f, -3.14f, 3.14f, 0.5f);
    DynamicParam p_rotY(0.0f, -3.14f, 3.14f, 0.3f);
    DynamicParam p_rotZ(0.0f, -3.14f, 3.14f, 0.1f);
    DynamicParam p_shapeSpd(0.5f, 0.0f, 3.0f, 0.2f);
    DynamicParam p_colorSpd(0.2f, 0.0f, 5.0f, 0.5f);
    DynamicParam p_amp(0.6f, 0.0f, 2.0f, 0.3f);
    DynamicParam p_freq(1.5f, 0.1f, 5.0f, 0.8f);
    DynamicParam p_warp(0.4f, 0.0f, 1.5f, 0.2f);
    DynamicParam p_fresnel(4.0f, 1.0f, 10.0f, 1.0f);
    DynamicParam p_camDist(5.0f, 2.0f, 15.0f, 1.0f); // Zoom in/out!

    // Zmienne do kontroli czasu
    float lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Obliczenia fizyki czasu (Delta Time)
        float current_time = glfwGetTime();
        float dt = current_time - lastTime;
        lastTime = current_time;

        // Auto-ewolucja wartości (update wszystkich suwaków)
        p_rotX.update(dt); p_rotY.update(dt); p_rotZ.update(dt);
        p_shapeSpd.update(dt); p_colorSpd.update(dt);
        p_amp.update(dt); p_freq.update(dt); p_warp.update(dt);
        p_fresnel.update(dt); p_camDist.update(dt);

        // Ustalanie prawdziwej rozdzielczości klatki
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        // GUI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // OKNO 1: ZARZĄDZANIE SUWAKAMI
        ImGui::Begin("Centrum Dowodzenia");
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Separator();
        
        if (ImGui::CollapsingHeader("Rotacja & Kamera", ImGuiTreeNodeFlags_DefaultOpen)) {
            DrawParamUI("Zoom (Kamera)", p_camDist);
            DrawParamUI("Obrot X", p_rotX);
            DrawParamUI("Obrot Y", p_rotY);
            DrawParamUI("Obrot Z", p_rotZ);
        }
        if (ImGui::CollapsingHeader("Dynamika Czasu", ImGuiTreeNodeFlags_DefaultOpen)) {
            DrawParamUI("Predkosc Morfowania", p_shapeSpd);
            DrawParamUI("Predkosc Koloru", p_colorSpd);
        }
        if (ImGui::CollapsingHeader("Tkanka & Geometria", ImGuiTreeNodeFlags_DefaultOpen)) {
            DrawParamUI("Glebokosc (Amp)", p_amp);
            DrawParamUI("Gestosc (Freq)", p_freq);
            DrawParamUI("Sila Skrecania", p_warp);
            DrawParamUI("Efekt Krawedzi", p_fresnel);
        }
        ImGui::End();

        // OKNO 2: EDYTOR KODU (SHADER LIVE RECOMPILER)
        ImGui::Begin("Live GLSL Editor (map function)");
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Dostepne Ksztalty: sdSphere(p, r), sdBox(p, vec3(w,h,d)), sdTorus(p, vec2(r, grubość))");
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Mieszanie Ksztaltow: smin(ksztaltA, ksztaltB, moc_zlepienia)");
        
        // Główne pole tekstowe (Rozszerzalne, używające czcionki systemowej)
        ImGui::InputTextMultiline("##source", userCodeBuffer, IM_ARRAYSIZE(userCodeBuffer), 
                                  ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 20), 
                                  ImGuiInputTextFlags_AllowTabInput);
        
        if (ImGui::Button("KOMPILUJ I ZASTOSUJ", ImVec2(-FLT_MIN, 40))) {
            RecompileShader(userCodeBuffer);
        }
        
        // Pole błędów/statusu
        if(lastCompilerError.find("Sukces") != std::string::npos) {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", lastCompilerError.c_str());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "BŁĄD KOMPILACJI:\n%s", lastCompilerError.c_str());
        }
        ImGui::End();

        // Render Frame
        glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (shaderProgram) {
            glUseProgram(shaderProgram);
            
            // Wysłanie wszystkich uniformów do GPU
            glUniform2f(locRes, (float)display_w, (float)display_h);
            glUniform1f(locTime, (float)glfwGetTime());
            glUniform1f(locRotX, p_rotX.val);
            glUniform1f(locRotY, p_rotY.val);
            glUniform1f(locRotZ, p_rotZ.val);
            glUniform1f(locShapeSpd, p_shapeSpd.val);
            glUniform1f(locColorSpd, p_colorSpd.val);
            glUniform1f(locAmp, p_amp.val);
            glUniform1f(locFreq, p_freq.val);
            glUniform1f(locWarp, p_warp.val);
            glUniform1f(locFresnel, p_fresnel.val);
            glUniform1f(locCamDist, p_camDist.val);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // Nakładka UI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    glDeleteVertexArrays(1, &VAO); glDeleteBuffers(1, &VBO); glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window); glfwTerminate();
    return 0;
}
