from PIL import Image
import sys

def main():
    input_file = 'image.ppm'
    output_file = 'image.png'
    
    if len(sys.argv) > 1:
        input_file = sys.argv[1]
    if len(sys.argv) > 2:
        output_file = sys.argv[2]
    
    try:
        img = Image.open(input_file)
        img.save(output_file)
        print(f"成功转换为 {output_file}")
    except Exception as e:
        print(f"错误: {e}")
        print("尝试使用另一种方法...")
        
        # 如果Pillow不可用，尝试手动读取PPM
        try:
            with open(input_file, 'r') as f:
                lines = [line.strip() for line in f if line.strip() and not line.startswith('#')]
                
            if lines[0] != 'P3':
                print("不支持的PPM格式")
                sys.exit(1)
                
            width, height = map(int, lines[1].split())
            max_val = int(lines[2])
            
            pixels = list(map(int, ' '.join(lines[3:]).split()))
            
            # 创建图像
            from array import array
            import struct
            
            # 尝试使用matplotlib
            try:
                import numpy as np
                import matplotlib.pyplot as plt
                
                img_data = np.array(pixels).reshape((height, width, 3))
                plt.imsave(output_file, img_data.astype(np.uint8))
                print(f"成功转换为 {output_file} (使用matplotlib)")
            except ImportError:
                print("请安装Pillow或matplotlib: pip install pillow")
        except Exception as e2:
            print(f"转换失败: {e2}")

if __name__ == "__main__":
    main()
