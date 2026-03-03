import math

def generate_sphere(radius=1.0, stacks=20, slices=20, filename="sphere.obj"):
    vertices = []
    faces = []

    # Generate vertices
    for i in range(stacks + 1):
        lat = math.pi * i / stacks
        sin_lat = math.sin(lat)
        cos_lat = math.cos(lat)

        for j in range(slices + 1):
            lon = 2 * math.pi * j / slices
            sin_lon = math.sin(lon)
            cos_lon = math.cos(lon)

            x = radius * sin_lat * cos_lon
            y = radius * sin_lat * sin_lon
            z = radius * cos_lat
            vertices.append((x, z, -y)) # Swap Y and Z for typical OBJ orientation if needed, but let's stick to Y-up

    # Generate faces
    for i in range(stacks):
        for j in range(slices):
            p1 = i * (slices + 1) + j + 1
            p2 = p1 + 1
            p3 = (i + 1) * (slices + 1) + j + 1
            p4 = p3 + 1

            # Two triangles per quad
            faces.append((p1, p3, p2))
            faces.append((p2, p3, p4))

    with open(filename, 'w') as f:
        f.write("# Simple Sphere Mesh\n")
        for v in vertices:
            f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
        
        for face in faces:
            f.write(f"f {face[0]} {face[1]} {face[2]}\n")

    print(f"Generated {filename} with {len(vertices)} vertices and {len(faces)} faces.")

if __name__ == "__main__":
    generate_sphere()
