import random

random.seed(42)  # Fixed seed for reproducibility

# Basic scene
scene = """# Chess Scene - Raytracer with random small balls
# Format:
# sphere x y z radius material r g b [extra]
# cylinder x y z radius height material r g b [extra]
# xy_rect x0 x1 y0 y1 z material r g b [extra]
# yz_rect y0 y1 z0 z1 x material r g b [extra]
# xz_rect x0 x1 z0 z1 y material r g b [extra]

# Floor - XY plane (z=0)
xy_rect -5 5 -5 5 0 checker 0.05 0.05 0.05 0.95 0.95 0.95

# Left wall - YZ plane (x=-5)
yz_rect -5 5 0 3 -5 checker 0.05 0.05 0.05 0.95 0.95 0.95

# Right wall - YZ plane (x=5)
yz_rect -5 5 0 3 5 checker 0.05 0.05 0.05 0.95 0.95 0.95

# Back wall - XZ plane (y=5)
xz_rect -5 5 0 3 5 checker 0.05 0.05 0.05 0.95 0.95 0.95

# Glass spheres
sphere -1.2 2.5 0.4 0.4 dielectric 1 1 1 1.5
sphere -0.6 3.0 0.12 0.12 dielectric 1 1 1 1.5
sphere 0.2 2.8 0.28 0.28 dielectric 1 1 1 1.5
sphere 0.8 3.2 0.15 0.15 dielectric 1 1 1 1.5
sphere 1.5 2.7 0.35 0.35 dielectric 1 1 1 1.5
sphere -1.5 3.5 0.18 0.18 dielectric 1 1 1 1.5
sphere -0.8 3.8 0.45 0.45 dielectric 1 1 1 1.5
sphere 1.0 3.6 0.32 0.32 dielectric 1 1 1 1.5

# Metal spheres
sphere -2.5 2.0 0.35 0.35 metal 0.8 0.8 0.8 0.0
sphere 2.5 2.2 0.3 0.3 metal 0.8 0.8 0.8 0.0

# Metal cylinder
cylinder -3.2 3.5 0.375 0.4 0.75 metal 0.8 0.8 0.8 0.0

# Colored spheres
sphere -4.0 2.0 0.3 0.3 lambertian 0.7 0.2 0.1
sphere 4.0 2.2 0.3 0.3 lambertian 0.1 0.2 0.7
sphere 0 1.5 0.35 0.35 metal 0.8 0.8 0.8 0.0
sphere -4.0 3.5 0.28 0.28 lambertian 0.8 0.6 0.2
sphere 4.0 3.8 0.28 0.28 lambertian 0.1 0.6 0.2
sphere 0 4.2 0.32 0.32 lambertian 0.6 0.1 0.6

# Random small balls (50)
"""

# Add 50 random small balls
for i in range(50):
    x = random.uniform(-3, 3)
    y = random.uniform(-2, 3)
    r = random.uniform(0.05, 0.15)
    
    if random.random() < 0.5:
        scene += f"sphere {x:.4f} {y:.4f} {r:.4f} {r:.4f} dielectric 1 1 1 1.5\n"
    else:
        scene += f"sphere {x:.4f} {y:.4f} {r:.4f} {r:.4f} metal 0.8 0.8 0.8 0.0\n"

with open('scene_random.txt', 'w') as f:
    f.write(scene)

print("生成随机场景: scene_random.txt")
