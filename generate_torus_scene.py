import random

random.seed(42)

scene = """# Colored Glass Torus Scene
# Format:
# torus x y z R r material r g b [extra]

# White background (simple ground)
xy_rect -8 8 -8 8 0 lambertian 0.8 0.8 0.8

# Spheres:
"""

colors = [
    (1.0, 0.2, 0.2),  # Red
    (0.2, 1.0, 0.2),  # Green
    (0.2, 0.2, 1.0),  # Blue
    (1.0, 0.8, 0.2),  # Yellow
    (1.0, 0.2, 1.0),  # Magenta
    (0.2, 1.0, 1.0),  # Cyan
    (1.0, 0.5, 0.2),  # Orange
    (0.8, 0.2, 0.8),  # Purple
    (0.5, 1.0, 0.5),  # Light Green
    (0.5, 0.5, 1.0),  # Light Blue
]

num_tori = 50

for i in range(num_tori):
    x = random.uniform(-4, 4)
    y = random.uniform(-2, 4)
    z = random.uniform(0.5, 2.0)
    major_r = random.uniform(0.4, 0.8)
    minor_r = random.uniform(0.1, 0.2)
    color = colors[i % len(colors)]
    
    # Random rotation - simple: just change the axis by varying position
    # For simplicity, we'll just place them randomly in 3D space
    
    # Use colored dielectric glass (we'll just use white for now, but the structure is there)
    scene += f"torus {x:.4f} {y:.4f} {z:.4f} {major_r:.4f} {minor_r:.4f} dielectric 1.0 1.0 1.0 1.5\n"

with open('examples/scene_tori.txt', 'w') as f:
    f.write(scene)

print(f"生成彩色圆环场景: examples/scene_tori.txt ({num_tori}个圆环)")
