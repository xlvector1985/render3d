import math
import sys

def generate_beaker(filename, R, r, H, d, segments):
    vertices = []
    faces = []

    # Helper to add vertex and return its 1-based index
    def add_vertex(x, y, z):
        vertices.append((x, y, z))
        return len(vertices)

    # Generate vertices
    # We need 4 rings of vertices:
    # 0: Bottom Outer (z=0, radius=R)
    # 1: Top Outer (z=H, radius=R)
    # 2: Top Inner (z=H, radius=r)
    # 3: Bottom Inner (z=d, radius=r)
    
    # Also center points for bottom caps if we want to use triangle fans, 
    # but quads are better. Let's stick to rings for side walls and fans/rings for caps.
    # Actually, for the bottom, we can just use a center point.
    
    # Ring 0: Bottom Outer
    ring0_start = 1
    for i in range(segments):
        angle = 2 * math.pi * i / segments
        add_vertex(R * math.cos(angle), R * math.sin(angle), 0)
        
    # Ring 1: Top Outer
    ring1_start = len(vertices) + 1
    for i in range(segments):
        angle = 2 * math.pi * i / segments
        add_vertex(R * math.cos(angle), R * math.sin(angle), H)
        
    # Ring 2: Top Inner
    ring2_start = len(vertices) + 1
    for i in range(segments):
        angle = 2 * math.pi * i / segments
        add_vertex(r * math.cos(angle), r * math.sin(angle), H)
        
    # Ring 3: Bottom Inner
    ring3_start = len(vertices) + 1
    for i in range(segments):
        angle = 2 * math.pi * i / segments
        add_vertex(r * math.cos(angle), r * math.sin(angle), d)

    # Center points for caps
    center_bottom_outer = add_vertex(0, 0, 0)
    center_bottom_inner = add_vertex(0, 0, d)

    # Generate Faces
    
    # 1. Outer Wall (Ring 0 to Ring 1)
    for i in range(segments):
        current = i
        next_i = (i + 1) % segments
        
        # Quad: 0_curr -> 0_next -> 1_next -> 1_curr
        # Face indices
        v1 = ring0_start + current
        v2 = ring0_start + next_i
        v3 = ring1_start + next_i
        v4 = ring1_start + current
        faces.append((v1, v2, v3, v4))

    # 2. Inner Wall (Ring 3 to Ring 2) - Normals pointing inward means face winding reversed?
    # Actually, for OBJ, CCW is front. Inner wall faces center.
    # So we look from center: 3_curr -> 2_curr -> 2_next -> 3_next is CCW?
    # Let's visualize: 
    # Outer: 0->1 is Up. 0->0_next is Right. Up x Right = Inward? No.
    # Let's trace CCW on outer wall: Bottom-Right -> Top-Right -> Top-Left -> Bottom-Left
    # (v2, v3, v4, v1) -> (0_next, 1_next, 1_curr, 0_curr)
    # Normal = (1_next - 0_next) x (1_curr - 1_next) = Up x Left = Back (In). Wait.
    # Standard CCW: v1, v2, v3. (v2-v1) x (v3-v1).
    # Bottom-Left(0_c), Bottom-Right(0_n), Top-Right(1_n).
    # Right x Up = Out (Normal). Correct.
    # So Outer Wall: 0_c, 0_n, 1_n, 1_c.
    
    # Inner Wall: Needs to face INWARD (towards axis).
    # So if we look from axis, it should be CCW.
    # Ring 3 (Bottom) to Ring 2 (Top).
    # Vertices: 3_c, 3_n, 2_n, 2_c.
    # Right x Up = Out (towards wall, away from axis). This is Normal pointing OUT of the glass volume (into the liquid).
    # Correct. The normal of a solid always points OUT of the solid.
    # For the inner wall, "Out of the solid" means "Into the hollow space".
    # So: 3_c, 3_n, 2_n, 2_c.
    for i in range(segments):
        current = i
        next_i = (i + 1) % segments
        
        v1 = ring3_start + current
        v2 = ring3_start + next_i
        v3 = ring2_start + next_i
        v4 = ring2_start + current
        faces.append((v1, v2, v3, v4))

    # 3. Top Rim (Ring 1 to Ring 2) - Flat ring at z=H
    # Normal points Up (z+).
    # 1_c, 1_n, 2_n, 2_c.
    for i in range(segments):
        current = i
        next_i = (i + 1) % segments
        
        v1 = ring1_start + current
        v2 = ring1_start + next_i
        v3 = ring2_start + next_i
        v4 = ring2_start + current
        faces.append((v1, v2, v3, v4))

    # 4. Bottom Cap Outer (Center to Ring 0) - at z=0
    # Normal points Down (z-).
    # Center, 0_n, 0_c. (Clockwise if looked from top, CCW from bottom)
    for i in range(segments):
        current = i
        next_i = (i + 1) % segments
        
        v1 = center_bottom_outer
        v2 = ring0_start + next_i
        v3 = ring0_start + current
        faces.append((v1, v2, v3))
        
    # 5. Bottom Cap Inner (Ring 3 to Center) - at z=d
    # Normal points Up (z+, into the hollow space).
    # Center, 3_c, 3_n.
    for i in range(segments):
        current = i
        next_i = (i + 1) % segments
        
        v1 = center_bottom_inner
        v2 = ring3_start + current
        v3 = ring3_start + next_i
        faces.append((v1, v2, v3))

    with open(filename, 'w') as f:
        f.write(f"# Beaker: R={R}, r={r}, H={H}, d={d}\n")
        for v in vertices:
            f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
        for face in faces:
            f.write("f")
            for idx in face:
                f.write(f" {idx}")
            f.write("\n")
    
    print(f"Generated {filename} with {len(vertices)} vertices and {len(faces)} faces.")

if __name__ == "__main__":
    # Default parameters for a beaker
    # R=1.0, r=0.9, H=2.0, d=0.2
    generate_beaker("beaker.obj", R=1.0, r=0.85, H=2.2, d=0.2, segments=64)
