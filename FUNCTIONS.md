> # Default Blob
>
> <img width="1000" height="500" alt="image" src="https://github.com/user-attachments/assets/a890814b-70be-4aa8-b313-dbcaa6968552" />
>
```glsl
// TO JEST TWÓJ KOD GEOMETRII - MOŻESZ GO MODYFIKOWAĆ
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
```

---

> # NIESKOŃCZONA MATRYCA INDUSTRIALNA
>
> <img width="1000" height="500" alt="image" src="https://github.com/user-attachments/assets/200f6753-0cd9-4a3d-b9ee-3acd611f8446" />

```glsl
// NIESKOŃCZONA MATRYCA INDUSTRIALNA
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;

    // 1. Lot przez przestrzeń (Zmieniamy pozycję Z w czasie)
    p.z -= t * 5.0; 
    
    // Nieliniowe trzęsienie przestrzeni na boki
    p.x += sin(t + p.z * 0.1) * u_warp * 2.0;

    // 2. TWORZENIE NIESKOŃCZONEJ SIATKI (Funkcja mod)
    // spacing determinuje jak gęsto ułożone są obiekty
    vec3 spacing = vec3(3.0 + u_amp * 2.0);
    vec3 q = mod(p + 0.5 * spacing, spacing) - 0.5 * spacing;

    // 3. Budujemy kratownicę z nieskończonych cylindrów na wszystkich osiach
    float c1 = sdCylinder(q.xyz, vec3(0.0, 0.0, u_freq * 0.3)); // Oś Z
    float c2 = sdCylinder(q.yzx, vec3(0.0, 0.0, u_freq * 0.3)); // Oś X
    float c3 = sdCylinder(q.zxy, vec3(0.0, 0.0, u_freq * 0.3)); // Oś Y

    // Unia (połączenie) trzech rur w jeden krzyżak
    float grid = min(min(c1, c2), c3);

    // 4. OPERACJA SUBTRAKCJI (Wycinanie dziury w siatce)
    // Tworzymy wielką sferę, która podróżuje razem z kamerą
    float cavern = sdSphere(p - vec3(0.0, 0.0, t * 5.0), 3.0);
    
    // max(-a, b) oznacza w GLSL: "Z kształtu B, wytnij kształt A"
    float final_shape = max(-cavern, grid);

    // Zmniejszamy krok promienia dla bezpieczeństwa, bo struktura ma ostre kąty
    return final_shape * 0.5; 
}
```

---

> # KRYSZTAŁOWY KALEJDOSKOP (FRAKTAL IFS)
>
> <img width="777" height="777" alt="image" src="https://github.com/user-attachments/assets/f087fe2b-aed2-4df1-9a1e-150f60b2a521" />

```glsl
// KRYSZTAŁOWY KALEJDOSKOP (FRAKTAL IFS)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;

    vec3 q = p;
    float d = 1000.0; // Inicjujemy dystans jako bardzo duży
    float scale = 1.0;

    // Fraktalna Pętla Kalejdoskopu
    for(int i = 0; i < 6; i++) {
        // Zaginanie przestrzeni - tworzy symetryczne odbicia lustrzane
        q = abs(q) - (u_amp * 1.5);
        
        // Rotacja wewnętrznych luster
        q.xy *= rot(u_freq + t * 0.2);
        q.xz *= rot(u_freq * 0.8 - t * 0.1);
        
        // Obliczamy kształt (Mieszanka Kuli i Pudła) dla aktualnego odbicia
        float shape = max(sdBox(q, vec3(0.5)), sdSphere(q, 0.7));
        
        // Zapisujemy najmniejszy dystans ze wszystkich iteracji
        d = min(d, shape * scale); 
        
        // Skalowanie dla fraktalnej głębi
        scale *= 0.8; 
        q *= 1.25; 
        
        // Agresywne wypychanie tkanki (tworzy igły i odłamki)
        q -= u_warp * 0.5; 
    }
    
    // Mnożnik 0.4 zapobiega błędom renderowania przy tak agresywnych krawędziach
    return d * 0.4;
}
```

---

> # Biomechaniczny Rdzeń
>
> <img width="720" height="580" alt="image" src="https://github.com/user-attachments/assets/bc4d9c97-5db2-48a8-91f6-e9e257b16df8" />

```glsl
// BIOMECHANICZNY RDZEŃ (THE TWIST)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;

    vec3 q = p;
    
    // 1. ZNIEKSZTAŁCENIE PRZESTRZENI (Śrubowy Twist)
    // Kąt obrotu na osiach XY jest zależny od pozycji na osi Z
    // To sprawia, że bryła zostaje "ukręcona"
    float twist_force = u_warp * 2.5 * sin(t * 0.5);
    q.xy *= rot(q.z * twist_force); 

    // 2. Baza to wielki, gruby Torus (Pączek)
    float rdzen = sdTorus(q, vec2(2.2, 0.9));

    // 3. Dodajemy biomechaniczne "żebra" za pomocą funkcji trygonometrycznych
    // Szum przestrzenny uderzający w ułożenie tkanki
    float zeberka = sin(q.x * u_freq * 4.0) * sin(q.y * u_freq * 4.0) * cos(q.z * u_freq * 4.0);
    
    // Wyciągamy żebra z rdzenia
    rdzen -= zeberka * u_amp * 0.4;

    // 4. ODCIĘCIE ŚMIECI (Bounding Volume)
    // Zniekształcenie "Twist" działa nieliniowo w nieskończoność, co rozwala renderer.
    // Zamykamy więc zmutowany rdzeń w niewidzialnej barierze o promieniu 3.8,
    // ucinając matematykę, która wychodzi poza nią (operacja intersect).
    float bounding_sphere = length(p) - 3.8;
    
    // max() wymusza zwrócenie struktury tylko tam, gdzie mieści się w kuli
    return max(rdzen, bounding_sphere) * 0.4;
}
```
