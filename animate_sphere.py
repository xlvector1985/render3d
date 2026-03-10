import os
import subprocess
from PIL import Image

def create_scene_file(frame_idx, x_pos, filename):
    content = f"""resolution 480 270
camera 0 -10 3 0 0 1 0 0 1 30 1.777

# Floor - XY plane (z=0), chessboard
xy_rect -10 10 -10 10 0 checker 0.1 0.1 0.1 0.9 0.9 0.9

# Back wall - XZ plane (y=20), chessboard
xz_rect -10 10 0 20 20 checker 0.1 0.1 0.1 0.9 0.9 0.9

# Left wall - YZ plane (x=-10), chessboard
yz_rect -10 10 0 10 -10 checker 0.1 0.1 0.1 0.9 0.9 0.9

# Right wall - YZ plane (x=10), chessboard
yz_rect -10 10 0 10 10 checker 0.1 0.1 0.1 0.9 0.9 0.9

# Light Source
xz_rect -4 4 4 6 9.9 light 15 15 15

# Rolling Glass Sphere
# Radius 1.0, z=1.0 (on floor)
# dielectric: r g b ir
sphere {x_pos:.2f} 0 1.0 1.0 dielectric 0.9 0.9 0.9 1.5

# Add a small metal ball inside to visualize rotation? 
# If it's rolling, x moves, and it rotates around Y axis.
# A point at (r, 0, 0) relative to center rotates.
# But sphere is symmetric. Let's just move it first.
# Adding an inner inclusion to see "rolling" would be cool but requires math for the inclusion's position.
# Let's keep it simple: just a sliding glass sphere for now.
"""
    with open(filename, 'w') as f:
        f.write(content)

def generate_animation():
    output_dir = "anim_frames"
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    num_frames = 24
    start_x = -4.0
    end_x = 4.0
    
    samples = 200 # Low samples for speed
    max_depth = 20
    
    images = []

    print(f"Generating {num_frames} frames...")

    for i in range(num_frames):
        t = i / (num_frames - 1)
        x = start_x + (end_x - start_x) * t
        
        scene_filename = os.path.join(output_dir, f"scene_{i:03d}.txt")
        ppm_filename = os.path.join(output_dir, f"frame_{i:03d}.ppm")
        png_filename = os.path.join(output_dir, f"frame_{i:03d}.png")
        
        # 1. Create Scene
        create_scene_file(i, x, scene_filename)
        
        # 2. Render
        print(f"Rendering frame {i+1}/{num_frames}...")
        cmd = ["./raytracer_bvh", str(samples), str(max_depth), scene_filename]
        with open(ppm_filename, "w") as outfile:
            subprocess.run(cmd, stdout=outfile, check=True)
            
        # 3. Convert to PNG
        # We can use Pillow directly here instead of calling the script
        try:
            with Image.open(ppm_filename) as im:
                im.save(png_filename)
                images.append(im.copy()) # Keep in memory for GIF
        except Exception as e:
            print(f"Error converting frame {i}: {e}")

    # 4. Save GIF
    print("Creating GIF...")
    if images:
        images[0].save(
            "rolling_sphere.gif",
            save_all=True,
            append_images=images[1:],
            duration=100, # 100ms per frame = 10fps
            loop=0
        )
        print("Done! Saved to rolling_sphere.gif")
    else:
        print("No images generated.")

if __name__ == "__main__":
    generate_animation()
