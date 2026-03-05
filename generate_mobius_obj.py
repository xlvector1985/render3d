import math
import sys

def generate_mobius_strip(filename, segments_u, segments_v):
    vertices = []
    faces = []

    # Helper to add vertex and return its 1-based index
    def add_vertex(x, y, z):
        vertices.append((x, y, z))
        return len(vertices)

    # Parametric equations for a "Solid" Mobius Strip (with thickness)
    # This is essentially a twisted tube with a flattened elliptical cross-section.
    
    R = 2.0       # Major radius of the loop
    w = 1.0       # Width of the strip (major axis of cross-section ellipse)
    thickness = 0.2 # Thickness of the strip (minor axis of cross-section ellipse)
    
    # We sweep an elliptical cross section.
    # u in [0, 2*pi] is the angle along the main loop.
    # v in [0, 2*pi] is the angle around the cross-section.
    
    grid = []
    
    for i in range(segments_u):
        u = 2 * math.pi * i / segments_u
        row = []
        for j in range(segments_v):
            v = 2 * math.pi * j / segments_v
            
            # 1. Define cross-section in local 2D frame (x', y')
            # x' is width direction, y' is thickness direction
            x_prime = (w / 2.0) * math.cos(v)
            y_prime = (thickness / 2.0) * math.sin(v)
            
            # 2. Twist the local frame by u/2
            # We rotate (x', y') by u/2 in the plane perpendicular to the curve tangent.
            # For a circle in XY plane, tangent is azimuthal. Normal is radial. Binormal is Z.
            # We want the strip to lie "flat" initially.
            # At u=0, Normal=(1,0,0), Binormal=(0,0,1).
            # The strip width is along Normal? No, usually along Binormal for a vertical strip, 
            # or along Normal for a flat washer.
            # Mobius strip usually lies "flat" locally but twists up.
            # Let's align width along Normal (Radial) initially.
            
            # Let n be the coordinate along Radial (Normal) direction.
            # Let b be the coordinate along Z (Binormal) direction.
            
            # Rotate by u/2
            angle = u / 2.0
            n = x_prime * math.cos(angle) - y_prime * math.sin(angle)
            b = x_prime * math.sin(angle) + y_prime * math.cos(angle)
            
            # 3. Map to global coordinates
            # Center of cross section: (R*cos(u), R*sin(u), 0)
            # Radial vector N: (cos(u), sin(u), 0)
            # Z vector B: (0, 0, 1)
            
            cx = R * math.cos(u)
            cy = R * math.sin(u)
            cz = 0
            
            x = cx + n * math.cos(u)
            y = cy + n * math.sin(u)
            z = cz + b
            
            # Shift up to sit on floor
            z += 1.5
            
            idx = add_vertex(x, y, z)
            row.append(idx)
        grid.append(row)

    # Generate Faces
    for i in range(segments_u):
        for j in range(segments_v):
            next_j = (j + 1) % segments_v
            
            v1 = grid[i][j]
            v4 = grid[i][next_j]
            
            if i == segments_u - 1:
                # Connection at the seam (u=2pi to u=0)
                # At u=2pi, angle is pi. Rotation by pi means (n, b) -> (-n, -b).
                # (x', y') -> (-x', -y').
                # In the ellipse parameterization:
                # x'(v) = (w/2)cos(v), y'(v) = (t/2)sin(v).
                # -x' = (w/2)cos(v+pi), -y' = (t/2)sin(v+pi).
                # So the point at v maps to the point at v + pi.
                # Index mapping: k = (j + segments_v/2) % segments_v.
                
                k = (j + segments_v // 2) % segments_v
                k_next = (next_j + segments_v // 2) % segments_v
                
                v2 = grid[0][k]
                v3 = grid[0][k_next]
                
                faces.append((v1, v2, v3, v4))
            else:
                next_i = i + 1
                v2 = grid[next_i][j]
                v3 = grid[next_i][next_j]
                
                faces.append((v1, v2, v3, v4))

    with open(filename, 'w') as f:
        f.write(f"# Solid Mobius Strip\n")
        for v in vertices:
            f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
        for face in faces:
            f.write("f")
            for idx in face:
                f.write(f" {idx}")
            f.write("\n")
    
    print(f"Generated {filename} with {len(vertices)} vertices and {len(faces)} faces.")

if __name__ == "__main__":
    generate_mobius_strip("mobius.obj", segments_u=120, segments_v=40)
