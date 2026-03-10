import random
import math

random.seed(1337)  # Fixed seed for reproducibility

# New scene
scene = """# New Scene - Raytracer
# Format:
# sphere x y z radius material r g b [extra]
# cylinder x y z radius height material r g b [extra]
# xy_rect x0 x1 y0 y1 z material r g b [extra]
# yz_rect y0 y1 z0 z1 x material r g b [extra]
# xz_rect x0 x1 z0 z1 y material r g b [extra]

# Floor - XY plane (z=0), chessboard
xy_rect -5 5 -5 5 0 checker 0.05 0.05 0.05 0.95 0.95 0.95

# Left wall - YZ plane (x=-5), chessboard
yz_rect -5 5 0 3 -5 checker 0.05 0.05 0.05 0.95 0.95 0.95

# Right wall - YZ plane (x=5), chessboard
yz_rect -5 5 0 3 5 checker 0.05 0.05 0.05 0.95 0.95 0.95

# Back wall - XZ plane (y=5), chessboard
xz_rect -5 5 0 3 5 checker 0.05 0.05 0.05 0.95 0.95 0.95

# Small mirror on back wall, lower position
xz_rect -1.5 1.5 0.5 2.0 5 metal 0.95 0.95 0.95 0.0

# Spheres:
# - 10 metal spheres
# - 10 glass spheres
# - 3 red rubber spheres
# - 3 blue rubber spheres
# - 3 yellow rubber spheres
# - Total: 29 spheres
"""

# Keep track of existing spheres to avoid overlapping
spheres = []

def is_overlapping(x, y, r):
    """Check if new sphere overlaps with any existing spheres"""
    for (sx, sy, sr) in spheres:
        dx = x - sx
        dy = y - sy
        dist_sq = dx * dx + dy * dy
        min_dist = r + sr
        if dist_sq < min_dist * min_dist:
            return True
    return False

# Generate 10 metal spheres
for i in range(10):
    attempts = 0
    while True:
        x = random.uniform(-4, 4)
        y = random.uniform(0, 4)
        r = random.uniform(0.1, 0.3)
        attempts += 1
        if not is_overlapping(x, y, r) or attempts > 1000:
            spheres.append((x, y, r))
            scene += f"sphere {x:.4f} {y:.4f} {r:.4f} {r:.4f} metal 0.8 0.8 0.8 0.0\n"
            break

# Generate 10 glass spheres
for i in range(10):
    attempts = 0
    while True:
        x = random.uniform(-4, 4)
        y = random.uniform(0, 4)
        r = random.uniform(0.1, 0.3)
        attempts += 1
        if not is_overlapping(x, y, r) or attempts > 1000:
            spheres.append((x, y, r))
            scene += f"sphere {x:.4f} {y:.4f} {r:.4f} {r:.4f} dielectric 1 1 1 1.5\n"
            break

# Generate 3 red rubber spheres
for i in range(3):
    attempts = 0
    while True:
        x = random.uniform(-4, 4)
        y = random.uniform(0, 4)
        r = random.uniform(0.1, 0.3)
        attempts += 1
        if not is_overlapping(x, y, r) or attempts > 1000:
            spheres.append((x, y, r))
            scene += f"sphere {x:.4f} {y:.4f} {r:.4f} {r:.4f} lambertian 0.7 0.2 0.1\n"
            break

# Generate 3 blue rubber spheres
for i in range(3):
    attempts = 0
    while True:
        x = random.uniform(-4, 4)
        y = random.uniform(0, 4)
        r = random.uniform(0.1, 0.3)
        attempts += 1
        if not is_overlapping(x, y, r) or attempts > 1000:
            spheres.append((x, y, r))
            scene += f"sphere {x:.4f} {y:.4f} {r:.4f} {r:.4f} lambertian 0.1 0.2 0.7\n"
            break

# Generate 3 yellow rubber spheres
for i in range(3):
    attempts = 0
    while True:
        x = random.uniform(-4, 4)
        y = random.uniform(0, 4)
        r = random.uniform(0.1, 0.3)
        attempts += 1
        if not is_overlapping(x, y, r) or attempts > 1000:
            spheres.append((x, y, r))
            scene += f"sphere {x:.4f} {y:.4f} {r:.4f} {r:.4f} lambertian 0.8 0.6 0.2\n"
            break

with open('examples/scene_new.txt', 'w') as f:
    f.write(scene)

print("生成新场景: examples/scene_new.txt")
print("- 地板和左右墙: 国际象棋棋盘")
print("- 后墙: 国际象棋棋盘，中间下方有小镜子")
print("- 10个金属球, 10个玻璃球")
print("- 3个红球, 3个蓝球, 3个黄球")
print(f"- 所有球不重叠！共{len(spheres)}个球")
