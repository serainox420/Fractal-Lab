# Example Shaders

---

## **1. ORGANIC FUSION (DEFAULT BLOB)**

<img width="500" height="250" alt="image" src="https://github.com/user-attachments/assets/a890814b-70be-4aa8-b313-dbcaa6968552" />

**Method:** Fuses a sphere and a box using a smooth minimum (`smin`) function. Applies domain warping and trigonometric displacement to the coordinate space.

**Parameters:**

* **Shape Speed:** Modifies the temporal variable (`t`), controlling orbital rotation and displacement animation speed.
* **Amp (Amplitude):** Determines the scalar depth of the trigonometric displacement.
* **Freq (Frequency):** Determines the frequency multiplier for the trigonometric displacement, increasing surface detail density.
* **Warp Strength:** Controls the vector magnitude by which the coordinate space (`q`) is offset by the displacement value.

```glsl
// THIS IS YOUR GEOMETRY CODE - FEEL FREE TO BREAK IT
float map(vec3 p) {
    // 1. External Volume Rotation
    p.xy *= rot(u_rotZ);
    p.xz *= rot(u_rotY);
    p.yz *= rot(u_rotX);

    float t = u_time * u_shapeSpd;
    
    // 2. BASE CREATION (Fusing a Sphere and a Box with smin!)
    float sphere = sdSphere(p - vec3(sin(t), 0.0, 0.0), 1.2);
    float box = sdBox(p + vec3(sin(t), 0.0, 0.0), vec3(0.8));
    float d = smin(sphere, box, 0.8); // 0.8 is the blending power (liquid metal effect)

    // 3. TEXTURE AND SCULPTING (Domain Warping and Noise)
    vec3 q = p;
    float amp = u_amp;  
    float freq = u_freq; 

    for(int i = 0; i < 4; i++) {
        q.xy *= rot(0.5); q.yz *= rot(0.7); // Break spatial symmetry
        
        vec3 offset = vec3(sin(t*0.5), cos(t*0.3), sin(t*0.4));
        float disp = sin(q.x * freq + offset.x) * sin(q.y * freq + offset.y) * sin(q.z * freq + offset.z);

        d += amp * disp;
        q += disp * u_warp; // Distort the tissue back into itself
        
        amp *= 0.5; freq *= 2.0;
    }
    
    return d * 0.4; // Ray protection multiplier so it doesn't clip through steep waves
}

```

---

## **2. INFINITE INDUSTRIAL MATRIX**

<img width="500" height="250" alt="image" src="https://github.com/user-attachments/assets/200f6753-0cd9-4a3d-b9ee-3acd611f8446" />


**Method:** Utilizes a modulo operator (`mod`) to repeat local coordinate space infinitely across all axes. Constructs a grid of intersecting cylinders and subtracts a dynamic sphere to create an unobstructed camera path.

**Parameters:**

* **Shape Speed:** Controls the Z-axis translation rate of the coordinate space (forward flight speed).
* **Amp (Amplitude):** Defines the scalar distance (`spacing`) between modulo grid repetitions.
* **Freq (Frequency):** Determines the radius of the `sdCylinder` primitives.
* **Warp Strength:** Controls the amplitude of the sine-based spatial displacement applied to the X-axis.

```glsl
// INFINITE INDUSTRIAL MATRIX
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;

    // 1. Flight through space (Shifting the Z position over time)
    p.z -= t * 5.0; 
    
    // Non-linear spatial earthquake on the X-axis
    p.x += sin(t + p.z * 0.1) * u_warp * 2.0;

    // 2. CREATING THE INFINITE GRID (Modulo function)
    // 'spacing' determines how densely the objects are packed
    vec3 spacing = vec3(3.0 + u_amp * 2.0);
    vec3 q = mod(p + 0.5 * spacing, spacing) - 0.5 * spacing;

    // 3. Build the lattice from infinite cylinders on all axes
    float c1 = sdCylinder(q.xyz, vec3(0.0, 0.0, u_freq * 0.3)); // Z-Axis
    float c2 = sdCylinder(q.yzx, vec3(0.0, 0.0, u_freq * 0.3)); // X-Axis
    float c3 = sdCylinder(q.zxy, vec3(0.0, 0.0, u_freq * 0.3)); // Y-Axis

    // Union (combine) the three pipes into a single cross-node
    float grid = min(min(c1, c2), c3);

    // 4. SUBTRACTION OPERATION (Carving a hole in the grid)
    // Create a giant sphere that travels along with the camera
    float cavern = sdSphere(p - vec3(0.0, 0.0, t * 5.0), 3.0);
    
    // max(-a, b) in GLSL means: "Subtract shape A from shape B"
    float final_shape = max(-cavern, grid);

    // Reduce ray step for safety because the structure has sharp 90-degree angles
    return final_shape * 0.5; 
}

```

---

## **3. CRYSTAL KALEIDOSCOPE (IFS FRACTAL)**

<img width="500" height="500" alt="image" src="https://github.com/user-attachments/assets/f087fe2b-aed2-4df1-9a1e-150f60b2a521" />

**Method:** Implements an Iterated Function System (IFS). Relies on the absolute value function (`abs()`) to aggressively fold space symmetrically across origin planes. Combines primitives via boolean intersection/union inside the iteration loop.

**Parameters:**

* **Shape Speed:** Controls the temporal input (`t`), modifying the internal rotation matrices of the folded space.
* **Amp (Amplitude):** Controls the offset subtracted from the absolute coordinates, governing structural expansion/separation.
* **Freq (Frequency):** Specifies the base angles applied to the internal `rot()` functions during each iteration.
* **Warp Strength:** Determines the linear extrusion offset applied to coordinates at the end of each iteration.

```glsl
// CRYSTAL KALEIDOSCOPE (IFS FRACTAL)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;

    vec3 q = p;
    float d = 1000.0; // Initialize distance as extremely huge
    float scale = 1.0;

    // Fractal Kaleidoscope Loop
    for(int i = 0; i < 6; i++) {
        // Space Folding - creates symmetrical mirror reflections
        q = abs(q) - (u_amp * 1.5);
        
        // Internal mirror rotation
        q.xy *= rot(u_freq + t * 0.2);
        q.xz *= rot(u_freq * 0.8 - t * 0.1);
        
        // Calculate shape (Mix of a Sphere and a Box) for the current reflection
        float shape = max(sdBox(q, vec3(0.5)), sdSphere(q, 0.7));
        
        // Save the smallest distance from all iterations
        d = min(d, shape * scale); 
        
        // Scale down for fractal depth
        scale *= 0.8; 
        q *= 1.25; 
        
        // Aggressive tissue extrusion (creates needles and shards)
        q -= u_warp * 0.5; 
    }
    
    // 0.4 multiplier prevents rendering glitches on such aggressive edges
    return d * 0.4;
}

```

---

## **4. BIOMECHANICAL CORE (THE TWIST)**

<img width="500" height="350" alt="image" src="https://github.com/user-attachments/assets/bc4d9c97-5db2-48a8-91f6-e9e257b16df8" />

**Method:** Applies non-linear spatial distortion by rotating the XY plane based on the Z-coordinate. Base geometry is an `sdTorus`. Employs spatial noise for Boolean subtraction. Rendered geometry is constrained by an `sdSphere` intersection to prevent ray-marching arithmetic errors at infinity.

**Parameters:**

* **Shape Speed:** Modifies the temporal input (`t`) applied to the twist angle and rotation variables.
* **Amp (Amplitude):** Determines the subtraction depth/magnitude of the trigonometric rib structures.
* **Freq (Frequency):** Sets the frequency multiplier for the trigonometric sine/cosine subtraction grid.
* **Warp Strength:** Serves as the magnitude multiplier for the Z-dependent spatial twist equation.

```glsl
// BIOMECHANICAL CORE (THE TWIST)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;

    vec3 q = p;
    
    // 1. SPATIAL DISTORTION (Screw Twist)
    // The rotation angle on XY axes depends on the Z position.
    // This makes the solid get "wrung out" like a towel.
    float twist_force = u_warp * 2.5 * sin(t * 0.5);
    q.xy *= rot(q.z * twist_force); 

    // 2. Base is a massive, thick Torus (Donut)
    float core = sdTorus(q, vec2(2.2, 0.9));

    // 3. Adding biomechanical "ribs" using trig functions
    // Spatial noise hitting the tissue layout
    float ribs = sin(q.x * u_freq * 4.0) * sin(q.y * u_freq * 4.0) * cos(q.z * u_freq * 4.0);
    
    // Pull the ribs out of the core
    core -= ribs * u_amp * 0.4;

    // 4. GARBAGE CLIPPING (Bounding Volume)
    // The "Twist" distortion works non-linearly into infinity, which breaks the renderer.
    // So we lock the mutated core inside an invisible barrier with a 3.8 radius,
    // cutting off math that escapes it (intersect operation).
    float bounding_sphere = length(p) - 3.8;
    
    // max() forces the structure to return ONLY where it fits inside the sphere
    return max(core, bounding_sphere) * 0.4;
}

```
