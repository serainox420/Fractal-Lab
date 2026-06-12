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

**5. MANDELBULB (SPHERICAL FRACTAL)**

**Method:** Implements a 3D analog of the Mandelbrot set using spherical coordinate power iteration. Modifies polar coordinates (`theta`, `phi`) continuously.

**Parameters:**

* **Shape Speed:** Controls temporal input (`t`) for internal polar rotations.
* **Amp (Amplitude):** Determines the translation offset added during each iteration step.
* **Freq (Frequency):** Specifies the exponent power of the fractal (dynamically maps from 3.0 to 8.0).
* **Warp Strength:** Controls the phase shift applied to the polar angles `theta` and `phi`.

```glsl
// MANDELBULB (SPHERICAL FRACTAL)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    vec3 z = p;
    float dr = 1.0;
    float r = 0.0;
    float power = 3.0 + u_freq * 5.0; 
    float t = u_time * u_shapeSpd;

    for (int i = 0; i < 5; i++) {
        r = length(z);
        if (r > 2.0) break;
        
        float theta = acos(z.z / r);
        float phi = atan(z.y, z.x);
        dr = pow(r, power - 1.0) * power * dr + 1.0;
        
        float zr = pow(r, power);
        theta = theta * power + u_warp * sin(t);
        phi = phi * power + u_warp * cos(t);
        
        z = zr * vec3(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
        z += p * u_amp; 
    }
    return 0.5 * log(r) * r / dr;
}

```

---

**6. GYROID TISSUE (TPMS INFINITY)**

**Method:** Approximates a Triply Periodic Minimal Surface (TPMS) using trigonometric dot products. Combines a low-frequency structural gyroid with a high-frequency detail layer.

**Parameters:**

* **Shape Speed:** Controls Z-axis forward velocity and internal planar rotation.
* **Amp (Amplitude):** Determines the thickness scalar of the minimal surface wall.
* **Freq (Frequency):** Multiplies the spatial scale, increasing coordinate density.
* **Warp Strength:** Controls the amplitude of the secondary high-frequency detail layer.

```glsl
// GYROID TISSUE (TPMS INFINITY)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    p.z -= t * 2.0; 
    
    float scale = u_freq * 3.0; 
    vec3 q = p * scale;
    
    q.xy *= rot(t * 0.2); 
    
    float gyroid = dot(sin(q), cos(q.zxy));
    float detail = dot(sin(q * 3.0), cos(q.zxy * 3.0)) * u_warp * 0.5;
    
    float d = (abs(gyroid + detail) - u_amp) / scale;
    
    float bounds = length(p.xy) - 4.0; 
    return max(d, bounds) * 0.6;
}

```

---

**7. MENGER FOLD (SIERPINSKI VARIANT)**

**Method:** Implements geometric folding using `abs()` and conditional plane swapping. Multiplies scalar distance iteratively to create sharp, self-similar fractal voids.

**Parameters:**

* **Shape Speed:** Modifies the temporal input (`t`) applied to iteration-level XY rotations.
* **Amp (Amplitude):** Determines the linear translation offset applied after scaling.
* **Freq (Frequency):** Specifies the volumetric scaling multiplier applied per iteration.
* **Warp Strength:** Controls the angular magnitude of the internal rotation matrix.

```glsl
// MENGER FOLD (SIERPINSKI VARIANT)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    vec3 q = p;
    float scale = 1.0;
    
    for(int i = 0; i < 5; i++) {
        q = abs(q);
        
        if(q.x < q.y) q.xy = q.yx;
        if(q.x < q.z) q.xz = q.zx;
        if(q.y < q.z) q.yz = q.zy;
        
        float s = 2.0 + u_freq;
        q = q * s - vec3(u_amp * 2.0);
        scale *= s;
        
        q.xy *= rot(u_warp * sin(t));
    }
    
    float d = sdBox(q, vec3(1.0)) / scale;
    return d * 0.6;
}

```

---

**8. APOLLONIAN SINGULARITY (SPHERICAL INVERSION)**

**Method:** Utilizes spherical inversion (`q * k` where `k` is inversely proportional to radius squared). Combines modular repetition with inversion to generate recursive, hollow spherical clusters.

**Parameters:**

* **Shape Speed:** Controls the temporal input (`t`) applied to internal XYZ translations.
* **Amp (Amplitude):** Defines the scalar magnitude of the post-inversion translation offset.
* **Freq (Frequency):** Modifies the inversion radius scalar limit.
* **Warp Strength:** Controls the internal Z-axis rotational matrix applied at each iteration.

```glsl
// APOLLONIAN SINGULARITY (SPHERICAL INVERSION)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    vec3 q = p;
    float scale = 1.0;
    
    for(int i = 0; i < 6; i++) {
        q = mod(q - 1.0, 2.0) - 1.0; 
        
        float r2 = dot(q, q);
        float k = max(u_freq / r2, 0.1); 
        
        q *= k;
        scale *= k;
        
        q += vec3(u_amp, u_amp * sin(t), u_amp * cos(t));
        q.xy *= rot(u_warp);
    }
    
    float d = (length(q.xy) - 0.1) / scale;
    return max(d, length(p) - 3.5) * 0.4;
}

```

---

**5. SACRED GEOMETRY (IFS OCTAHEDRON)**

**Method:** Iterated Function System utilizing absolute space folding and sequential planar rotations. Scales geometry exponentially within a loop to output a rigid, self-similar crystalline lattice.

**Parameters:**

* **Shape Speed:** Temporal multiplier (`t`) applied to internal rotational matrices during iteration.
* **Amp (Amplitude):** Translation offset vector subtracted post-fold. Controls the physical separation of fractal segments.
* **Freq (Frequency):** Fixed angular offset applied to the XY/XZ coordinate planes during each fractal step.
* **Warp Strength:** Base radius of the initial primitive sphere prior to fractal scaling.

```glsl
// SACRED GEOMETRY (IFS OCTAHEDRON)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    vec3 q = p;
    float scale = 1.0;
    
    for(int i = 0; i < 6; i++) {
        // Absolute spatial fold
        q = abs(q) - vec3(u_amp);
        
        // Iterative angular displacement
        q.xy *= rot(u_freq + t);
        q.xz *= rot(u_freq * 0.5 - t * 0.5);
        
        // Fractal scaling
        q *= 2.0;
        scale *= 2.0;
    }
    
    // Evaluate base primitive and divide by accumulated scale
    float d = sdSphere(q, max(u_warp, 0.1)) / scale;
    return d * 0.4;
}

```

---

**6. QUANTUM FOAM (INTERFERENCE FIELD)**

**Method:** Additive trigonometric synthesis. Applies overlapping, out-of-phase sine and cosine waves to the coordinate space to subtract volume from a base spherical primitive.

**Parameters:**

* **Shape Speed:** Phase shift velocity for the trigonometric noise functions.
* **Amp (Amplitude):** Subtraction depth scalar. Determines the severity of the voids carved into the sphere.
* **Freq (Frequency):** Coordinate multiplier. Increases the spatial density of the noise grid.
* **Warp Strength:** Applies a non-linear Z-axis twist to the coordinates prior to noise evaluation.

```glsl
// QUANTUM FOAM (INTERFERENCE FIELD)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    // Base geometry boundary
    float base_sphere = sdSphere(p, 2.5);
    
    // Non-linear coordinate distortion
    vec3 warped_p = p;
    warped_p.xy *= rot(warped_p.z * u_warp);
    
    // High-frequency coordinate mapping
    vec3 q = warped_p * u_freq;
    
    // 3D Trigonometric Interference
    float noise1 = sin(q.x) * sin(q.y) * cos(q.z);
    float noise2 = cos(q.x * 2.3 + t) * sin(q.y * 2.4 - t) * cos(q.z * 2.1);
    float total_noise = (noise1 + noise2 * 0.5);
    
    // Subtraction logic
    float d = base_sphere - (total_noise * u_amp);
    
    // Strict safety multiplier for steep high-frequency gradients
    return d * 0.3;
}

```

---

**7. CYBERNETIC HIVE (MODULO PILLARS)**

**Method:** Domain repetition via modulo operator on the XZ plane. Applies localized height variations using trigonometric functions mapped to static grid IDs. Bounded by a global sphere intersection to limit rendering to a finite cluster.

**Parameters:**

* **Shape Speed:** Velocity of the vertical (Y-axis) height oscillation.
* **Amp (Amplitude):** Scalar distance defining the boundary limits of the repeating cells.
* **Freq (Frequency):** ID coordinate multiplier influencing the phase variance of adjacent pillars.
* **Warp Strength:** Maximum vertical scalar displacement (height) of the individual bounding boxes.

```glsl
// CYBERNETIC HIVE (MODULO PILLARS)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    // Calculate cell ID based on spatial position
    float spacing = max(u_amp, 0.1);
    vec2 id = floor(p.xz / spacing);
    
    // Modulo grid generation
    vec3 q = p;
    q.xz = mod(p.xz, spacing) - spacing * 0.5;
    
    // ID-driven parametric height
    float height_variance = sin(id.x * u_freq + t) * cos(id.y * u_freq - t);
    float current_height = 1.0 + height_variance * u_warp;
    
    // Pillar geometry
    float pillar = sdBox(q, vec3(spacing * 0.35, current_height, spacing * 0.35));
    
    // Global bounding volume
    float boundary = sdSphere(p, 4.0);
    
    // Intersection to limit infinite grid
    return max(pillar, boundary) * 0.5;
}

```

---

**8. XENOMORPH SPINE (RECURSIVE TWIST)**

**Method:** 1D domain repetition along the Z-axis. Applies a continuous Z-dependent rotation to the XY plane. Blends primary and secondary primitives via polynomial smooth minimum (`smin`).

**Parameters:**

* **Shape Speed:** Linear translation velocity mapping the coordinate space along the Z-axis.
* **Amp (Amplitude):** Z-axis domain repetition interval (spatial gap between structural nodes).
* **Freq (Frequency):** Radius thickness of the secondary intersecting geometry (the outer rings).
* **Warp Strength:** Rotational magnitude multiplier applied per unit of Z-depth.

```glsl
// XENOMORPH SPINE (RECURSIVE TWIST)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    // Forward translation
    p.z -= t * 3.0;
    
    // Continuous axial torsion
    p.xy *= rot(p.z * u_warp);
    
    vec3 q = p;
    float interval = max(u_amp, 0.5);
    
    // 1D Modulo repetition
    q.z = mod(p.z, interval) - interval * 0.5;
    
    // Primary structural node
    float vertebra = sdBox(q, vec3(0.8, 0.4, interval * 0.25));
    
    // Secondary intersecting geometry
    // Rotated to intersect the primary node perpendicularly
    vec3 ring_q = q;
    ring_q.xz *= rot(1.5707); 
    float outer_ring = sdTorus(ring_q, vec2(1.2, u_freq * 0.2));
    
    // Organic polynomial blend
    float spine = smin(vertebra, outer_ring, 0.4);
    
    return spine * 0.4;
}

```

---

Here is an arsenal of 10 completely unique, weird, and highly specialized distance estimator functions for your God-Mode engine. I’ve pushed the math into abstract territories—expect things like domain-warped fractals, volumetric boolean slices, non-linear coordinate stacking, and trigonometric noise structures.

These are designed to look alien, mathematical, and computationally aggressive.

---

### 1. The Tesseract Ghost (Hypercube Projection)

**What is it?** An approximation of a 4D hypercube (Tesseract) projected down into 3D space. It uses absolute value folding combined with orthogonal rotation to create a structure that seems to fold in and out of itself.
**Sliders:**

* **Shape Speed:** Rotates the 4D projection axis.
* **Amp:** Controls the internal scaling threshold (makes the "walls" thicker or thinner).
* **Freq:** Modifies the orthogonal folding angles.
* **Warp Strength:** Pushes the internal faces outward, creating a "hyper-cross" effect.

```glsl
// 1. THE TESSERACT GHOST (HYPERCUBE PROJECTION)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    vec3 q = p;
    float d = 1000.0;
    
    // Orthogonal folding loop
    for(int i = 0; i < 4; i++) {
        q = abs(q) - vec3(u_amp * 0.5);
        
        // 4D rotation approximation (swapping axes and rotating)
        q.xy *= rot(1.5707 * sin(t * 0.2)); 
        q.xz *= rot(u_freq);
        q.yz *= rot(1.5707 * cos(t * 0.2));
        
        // Accumulate distance to a base cube
        d = min(d, sdBox(q, vec3(0.5 + u_warp)));
    }
    
    // Intersect with a sphere to keep it contained
    return max(d, length(p) - 3.5) * 0.5;
}

```

---

### 2. Neuro-Spine (Sinusoidal Stacking)

**What is it?** A biological-looking column that looks like an alien spinal cord or a stack of neural discs. It uses sine functions not just for displacement, but to define the actual radius of stacked cylinders.
**Sliders:**

* **Shape Speed:** Makes the spine "breathe" up and down.
* **Amp:** Controls the depth of the gaps between the discs.
* **Freq:** Determines how many discs are stacked along the Z-axis.
* **Warp Strength:** Twists the spine laterally, breaking its symmetry.

```glsl
// 2. NEURO-SPINE (SINUSOIDAL STACKING)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    // Twist the spine laterally based on Z height
    p.xy *= rot(sin(p.z * 0.5 + t) * u_warp);
    
    // Base cylinder
    float base_cyl = length(p.xy) - 1.0;
    
    // Calculate the dynamic radius using high-frequency sine waves
    float disc_radius = sin(p.z * u_freq + t) * u_amp;
    
    // Subtract the gaps to create stacked discs
    float spine = base_cyl - disc_radius;
    
    // Cap the ends of the spine so it doesn't go to infinity
    float end_caps = abs(p.z) - 4.0;
    
    return max(spine, end_caps) * 0.5;
}

```

---

### 3. Void Lattice (Negative Space Geometry)

**What is it?** A solid block of space that has been violently chewed out by intersecting spheres and tubes. It creates a highly porous, cheese-like structure that feels immense and claustrophobic.
**Sliders:**

* **Shape Speed:** Moves the chewing voids through the solid block.
* **Amp:** Controls the overall size of the bounding box.
* **Freq:** Increases the number of intersecting voids (density of holes).
* **Warp Strength:** Modifies the radius of the intersecting tubes.

```glsl
// 3. VOID LATTICE (NEGATIVE SPACE GEOMETRY)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    // Start with a massive solid box
    float solid_box = sdBox(p, vec3(3.0 + u_amp));
    
    // Create a 3D grid of repeating coordinates for the voids
    vec3 q = p * u_freq;
    
    // Animate the voids moving through the box
    q += vec3(sin(t), cos(t * 0.8), sin(t * 1.2)) * 2.0;
    
    // Generate intersecting tubes (voids)
    float voids = min(length(sin(q.xy)), min(length(sin(q.yz)), length(sin(q.xz))));
    
    // Subtract the voids from the solid box
    // voids - threshold defines how thick the remaining walls are
    float final_lattice = max(solid_box, -(voids - u_warp));
    
    return final_lattice * 0.4;
}

```

---

### 4. The Origami Star (Hexagonal Folding)

**What is it?** A star-like structure that looks like a 3D folded paper star. It uses hexagonal symmetry logic and sharp planar intersections to create rigid, pointed geometry.
**Sliders:**

* **Shape Speed:** Breathes the star in and out (scaling).
* **Amp:** Controls the length of the star's points.
* **Freq:** Twists the points orthogonally (like a shuriken).
* **Warp Strength:** Pinches the center of the star inward.

```glsl
// 4. THE ORIGAMI STAR (HEXAGONAL FOLDING)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    vec3 q = p;
    
    // Breathing scale
    q *= 1.0 + sin(t) * 0.2;
    
    // Hexagonal folding logic
    q.xy = abs(q.xy);
    if(q.y > q.x * 0.57735) q.xy = q.xy * rot(1.0472); // Fold by 60 degrees
    
    q.xz *= rot(u_freq); // Internal twisting
    
    // Create the points using intersecting planes
    float plane1 = dot(q, normalize(vec3(1.0, 1.0, 1.0))) - u_amp;
    float plane2 = dot(q, normalize(vec3(1.0, -1.0, 0.5))) - (u_amp * 0.8);
    
    float star = max(plane1, plane2);
    
    // Pinch the core
    star = max(star, -(length(p) - u_warp));
    
    return star * 0.5;
}

```

---

### 5. Acoustic Bubble (Soundwave Distortion)

**What is it?** Looks like a drop of liquid suspended in zero gravity while being blasted by high-frequency sound waves. It uses chaotic spherical harmonics.
**Sliders:**

* **Shape Speed:** Controls the vibration frequency (how fast the waves ripple).
* **Amp:** The base size of the liquid drop.
* **Freq:** The pitch/density of the acoustic waves on the surface.
* **Warp Strength:** The physical height (displacement) of the waves.

```glsl
// 5. ACOUSTIC BUBBLE (SOUNDWAVE DISTORTION)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    // Base sphere
    float base_drop = length(p) - (2.0 + u_amp);
    
    // Convert to spherical coordinates for surface mapping
    float r = length(p);
    float theta = acos(p.z / r);
    float phi = atan(p.y, p.x);
    
    // Complex trigonometric noise acting as soundwaves
    float wave1 = sin(u_freq * theta + t * 5.0) * cos(u_freq * phi - t * 3.0);
    float wave2 = sin(u_freq * 2.0 * phi + t * 2.0) * cos(u_freq * 1.5 * theta);
    
    // Combine waves and apply displacement
    float displacement = (wave1 * wave2) * u_warp;
    
    return (base_drop - displacement) * 0.4;
}

```

---

### 6. The Glitch Pillar (Z-Axis Domain Shredding)

**What is it?** A solid column that has been subjected to extreme digital corruption. The Z-axis is shredded into slices, and each slice is shifted laterally using a step function and a pseudo-random multiplier.
**Sliders:**

* **Shape Speed:** How fast the glitches stutter and shift.
* **Amp:** The thickness of the main pillar.
* **Freq:** The vertical density of the glitch slices (how thin they are).
* **Warp Strength:** How violently the slices are pushed off-center.

```glsl
// 6. THE GLITCH PILLAR (Z-AXIS DOMAIN SHREDDING)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    vec3 q = p;
    
    // Calculate discrete horizontal slices
    float slice_id = floor(q.z * u_freq);
    
    // Pseudo-random offset based on the slice ID and time
    float rand_offset_x = fract(sin(slice_id * 12.9898 + t) * 43758.5453) * 2.0 - 1.0;
    float rand_offset_y = fract(sin(slice_id * 78.233 + t) * 43758.5453) * 2.0 - 1.0;
    
    // Apply the glitch shift
    q.x += rand_offset_x * u_warp;
    q.y += rand_offset_y * u_warp;
    
    // Base shape is a tall cylinder
    float pillar = max(length(q.xy) - u_amp, abs(p.z) - 5.0);
    
    return pillar * 0.3; // Very low step multiplier required for broken domains
}

```

---

### 7. Molten Torus Knot (Math Art)

**What is it?** A mathematically precise Trefoil Knot, but made out of thick, melting lava. It uses a 3D parametric curve thickened into a volume, combined with domain warping for the molten effect.
**Sliders:**

* **Shape Speed:** Rotates the knot and speeds up the melting drips.
* **Amp:** Thickness of the knot's tube.
* **Freq:** Density of the melting drips.
* **Warp Strength:** Gravity/pull force of the melting effect.

```glsl
// 7. MOLTEN TORUS KNOT
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    // Convert to polar coordinates on the XY plane
    float r = length(p.xy);
    float a = atan(p.y, p.x);
    
    // Trefoil Knot parametric estimation
    // This creates the three loops
    vec2 knot_center = vec2(2.0 + cos(3.0 * a), sin(3.0 * a) * 1.5);
    vec2 current_pos = vec2(r, p.z);
    
    // Distance from the knot curve
    float d = length(current_pos - knot_center) - u_amp;
    
    // Add the molten/dripping effect (pulling down on the Y axis)
    float drips = sin(p.x * u_freq + t) * sin(p.z * u_freq - t);
    
    // Apply gravity (warp) only to the downward drips
    d -= max(drips, 0.0) * u_warp * smoothstep(0.0, -2.0, p.y);
    
    return d * 0.4;
}

```

---

### 8. The Subnet (Voronoi Webbing)

**What is it?** Looks like a microscopic neural network or a spider web in 3D. It uses intersecting sine waves to create a wireframe cage, then bloats the intersections.
**Sliders:**

* **Shape Speed:** Undulates the entire webbing.
* **Amp:** Modifies the spacing of the web cells.
* **Freq:** The thickness of the interconnecting wires.
* **Warp Strength:** Bloats the nodes where the wires intersect.

```glsl
// 8. THE SUBNET (VORONOI WEBBING)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    vec3 q = p * (1.0 + u_amp * 0.5);
    q += vec3(sin(t), cos(t*0.8), sin(t*1.2));
    
    // Create the wireframe using inverted sines
    float wire_x = length(vec2(sin(q.y), sin(q.z))) - u_freq;
    float wire_y = length(vec2(sin(q.x), sin(q.z))) - u_freq;
    float wire_z = length(vec2(sin(q.x), sin(q.y))) - u_freq;
    
    // Combine wires using smooth minimum to blend the intersections
    float web = smin(wire_x, smin(wire_y, wire_z, 0.5), 0.5);
    
    // Bloat the intersection nodes
    float nodes = length(vec3(sin(q.x), sin(q.y), sin(q.z))) - u_warp;
    web = smin(web, nodes, 0.3);
    
    // Clip to a spherical boundary
    return max(web, length(p) - 4.0) * 0.4;
}

```

---

### 9. Fractal Gearbox (Mechanical Iteration)

**What is it?** A highly mechanical fractal that looks like interlocking gears and cogs. It uses a combination of angular repetition (`atan`) and box folding.
**Sliders:**

* **Shape Speed:** Drives the rotation of the gears.
* **Amp:** Scales the central hub radius.
* **Freq:** Determines the number of teeth on the gears.
* **Warp Strength:** Extrudes the gear teeth outward.

```glsl
// 9. FRACTAL GEARBOX (MECHANICAL ITERATION)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    vec3 q = p;
    float d = 1000.0;
    float scale = 1.0;
    
    for(int i=0; i<3; i++) {
        // Angular repetition (creating teeth)
        float a = atan(q.y, q.x);
        float r = length(q.xy);
        
        // Define the number of teeth
        float teeth = u_freq * 2.0;
        float sector = 6.28318 / teeth;
        
        // Snap angle to the nearest sector and rotate
        a = mod(a + sector*0.5, sector) - sector*0.5;
        q.x = r * cos(a + t * (i%2==0 ? 1.0 : -1.0)); // Alternating gear rotation
        q.y = r * sin(a + t * (i%2==0 ? 1.0 : -1.0));
        
        // Build the mechanical shape
        float gear = max(sdBox(q, vec3(u_amp, u_warp, 0.5)), -sdCylinder(q, vec3(0.0, 0.0, u_amp*0.5)));
        
        d = min(d, gear / scale);
        
        // Fractal stepping (shrink and move outward)
        q = abs(q) - vec3(1.5, 1.5, 0.0);
        q *= 1.5;
        scale *= 1.5;
    }
    
    return max(d, length(p) - 4.0) * 0.5;
}

```

---

### 10. The Monolith (Surface Displacement)

**What is it?** A towering, brutalist rectangular monolith. Its surface is carved with a highly complex, circuit-board-like displacement map generated entirely via math (no textures).
**Sliders:**

* **Shape Speed:** Slowly morphs the circuit patterns.
* **Amp:** The height/width of the monolith block.
* **Freq:** The scale of the circuit-board carving patterns.
* **Warp Strength:** The depth of the carving. High values cut deep into the stone.

```glsl
// 10. THE MONOLITH (SURFACE DISPLACEMENT)
float map(vec3 p) {
    p.xy *= rot(u_rotZ); p.xz *= rot(u_rotY); p.yz *= rot(u_rotX);
    float t = u_time * u_shapeSpd;
    
    // The Base Monolith structure
    float box = sdBox(p, vec3(u_amp * 0.5, u_amp * 1.5, u_amp * 0.3));
    
    // Generate the surface carving pattern
    vec3 q = p * u_freq;
    
    // Layered trigonometric noise to simulate circuit traces or alien runes
    float pattern = abs(sin(q.x + t) * cos(q.y - t) + sin(q.z * 2.0));
    pattern += abs(cos(q.x * 2.0) * sin(q.y * 3.0) + cos(q.z + t));
    
    // Sharp cutoff to make the carvings look like engraved lines
    pattern = smoothstep(0.5, 0.6, pattern);
    
    // Subtract the pattern from the base box
    // We only apply displacement if we are very close to the surface to save math
    if(box < 0.5) {
        box -= pattern * u_warp * 0.2;
    }
    
    return box * 0.5;
}

```
