> # Fractal Lab
> ### Real-time 3D non-Euclidean geometry ray-marcher written in C++ (OpenGL 3.3 Core) with ImGui. 

<img width="1000" height="500" alt="image" src="https://github.com/user-attachments/assets/60de4d31-c2a0-4b00-94e2-adc0f092c461" />

> ### Features live GLSL recompilation and automated parameter modulation for creative coding.

## Technical Specs

* **Live GLSL Recompiler:** Edit the `map(vec3 p)` Distance Estimator function directly via the ImGui interface. Compiles and hot-reloads the Fragment Shader at runtime without dropping frames.
* **Parametric Automation:** Delta-time based ping-pong modulation for rotation, amplitude, frequency, domain warping, and lighting variables.
* **SDF Integration:** Native support for Signed Distance Field primitives (`sdSphere`, `sdBox`, `sdTorus`, `sdCylinder`) and smooth minimum (`smin`) polynomial blending.
* **Rendering Pipeline:** Real-time normal estimation, iteration-based Ambient Occlusion (AO), specular highlights, and Fresnel rim lighting.

## Dependencies

* `g++`, `make`
* OpenGL 3.3 Core
* GLFW 3, GLEW

**Arch Linux:**
```bash
sudo pacman -S glfw-x11 glew mesa base-devel git
```

## Build Instructions

Clone this repository and the required Dear ImGui submodule (Docking branch):

```bash
git clone https://github.com/serainox420/Fractal-Lab.git && cd Fractal-Lab
```

Get dependencies (imgui) & build
```bash
git clone -b docking https://github.com/ocornut/imgui.git
make
```

Manual build
```bash
g++ main.cpp \
imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp \
imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp \
-I./imgui -I./imgui/backends \
-lGL -lglfw -lGLEW -lm \
-o fractal_lab
```

## Run

```bash
make run
```

Or run manually (Wayland compositor)
```bash
env -u WAYLAND_DISPLAY ./fractal_lab
```

### Wayland Configuration Note

If you are running a Wayland compositor (e.g., Hyprland, Sway), GLEW will crash on startup with a `No GLX display` error. The `make run` command handles this automatically by executing `env -u WAYLAND_DISPLAY ./fractal_lab`, forcing a fallback to XWayland to satisfy GLEW's legacy GLX dependency.
