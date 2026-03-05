import sys
from PIL import Image

def ppm_to_png(ppm_file, png_file):
    try:
        with Image.open(ppm_file) as img:
            img.save(png_file)
        print(f"Converted {ppm_file} to {png_file}")
    except Exception as e:
        print(f"Error converting {ppm_file}: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python ppm_to_png.py <input.ppm> <output.png>")
    else:
        ppm_to_png(sys.argv[1], sys.argv[2])
