import math
import sys

def generate_klein_bottle(filename, segments_u, segments_v):
    vertices = []
    faces = []

    # Helper to add vertex and return its 1-based index
    def add_vertex(x, y, z):
        vertices.append((x, y, z))
        return len(vertices)

    # Parametric equations for Classic Klein Bottle
    # u in [0, 2*pi], v in [0, 2*pi]
    # Source: http://paulbourke.net/geometry/klein/
    
    grid = []
    
    # Ensure even segments_v for the twist logic
    if segments_v % 2 != 0:
        segments_v += 1
    
    for i in range(segments_u):
        u = 2 * math.pi * i / segments_u
        row = []
        for j in range(segments_v):
            v = 2 * math.pi * j / segments_v
            
            cos_u = math.cos(u)
            sin_u = math.sin(u)
            cos_v = math.cos(v)
            sin_v = math.sin(v)
            
            # r term depends on u
            # r = 4 * (1 - cos(u)/2)
            r = 4 * (1 - cos_u / 2)
            
            if u < math.pi:
                # Bottom half (bulbous part)
                x = 6 * cos_u * (1 + sin_u) + r * cos_u * cos_v
                y = 16 * sin_u + r * sin_u * cos_v
                z = r * sin_v
            else:
                # Top half (neck part)
                x = 6 * cos_u * (1 + sin_u) - r * cos_v
                y = 16 * sin_u
                z = r * sin_v
                
            # Coordinate mapping:
            # Equation Y is the vertical axis (length of bottle).
            # Equation X, Z are cross section.
            # Renderer is Z-up.
            # Map: Eq_Y -> Renderer_Z
            #      Eq_X -> Renderer_X
            #      Eq_Z -> Renderer_Y
            
            scale = 0.2
            
            vx = x * scale
            vy = z * scale
            vz = y * scale
            
            # Center/Position
            # Y ranges roughly -16 to 16. scaled: -3.2 to 3.2.
            # Shift Z up to sit on ground.
            vz += 3.5 
            
            idx = add_vertex(vx, vy, vz)
            row.append(idx)
        grid.append(row)

    # Generate Faces
    for i in range(segments_u):
        for j in range(segments_v):
            next_j = (j + 1) % segments_v
            
            v1 = grid[i][j]
            v4 = grid[i][next_j]
            
            if i == segments_u - 1:
                # Connect last row to first row with a TWIST
                # The parameterization at u=2pi matches u=0 only if v is flipped (v' = pi - v)
                # Index mapping: k = (N/2 - j) % N
                
                k = (segments_v // 2 - j) % segments_v
                k_next = (segments_v // 2 - next_j) % segments_v
                
                # The first row is grid[0]
                v2 = grid[0][k]
                v3 = grid[0][k_next]
                
                faces.append((v1, v2, v3, v4))
            else:
                next_i = i + 1
                v2 = grid[next_i][j]
                v3 = grid[next_i][next_j]
                
                faces.append((v1, v2, v3, v4))

    with open(filename, 'w') as f:
        f.write(f"# Classic Klein Bottle\n")
        for v in vertices:
            f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
        for face in faces:
            f.write("f")
            for idx in face:
                f.write(f" {idx}")
            f.write("\n")
    
    print(f"Generated {filename} with {len(vertices)} vertices and {len(faces)} faces.")

if __name__ == "__main__":
    generate_klein_bottle("klein.obj", segments_u=80, segments_v=40)
