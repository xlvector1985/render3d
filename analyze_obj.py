
import sys

def analyze_obj(filename):
    min_x, max_x = float('inf'), float('-inf')
    min_y, max_y = float('inf'), float('-inf')
    min_z, max_z = float('inf'), float('-inf')
    
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith('v '):
                parts = line.split()
                x = float(parts[1])
                y = float(parts[2])
                z = float(parts[3])
                
                min_x = min(min_x, x)
                max_x = max(max_x, x)
                min_y = min(min_y, y)
                max_y = max(max_y, y)
                min_z = min(min_z, z)
                max_z = max(max_z, z)

    print(f"X range: {min_x} to {max_x} (width: {max_x - min_x})")
    print(f"Y range: {min_y} to {max_y} (height: {max_y - min_y})")
    print(f"Z range: {min_z} to {max_z} (depth: {max_z - min_z})")
    print(f"Center: {(min_x+max_x)/2}, {(min_y+max_y)/2}, {(min_z+max_z)/2}")

if __name__ == "__main__":
    analyze_obj("bunny.obj")
