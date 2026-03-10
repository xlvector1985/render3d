# 渲染实验工程

这个仓库包含一个 C++ 光线追踪器、若干测试场景、以及一个使用 OpenGL 绘制的实时示例场景。

## 目录结构

- examples/：测试场景与批量运行脚本
- opengl_scene/：C++ OpenGL 示例工程
- raytracer_bvh：光线追踪器可执行文件
- ppm_to_png.py：PPM 转 PNG 工具

## 光线追踪器使用

渲染单个场景：

```bash
./raytracer_bvh 300 50 examples/scene_simple.txt > output.ppm
python3 ppm_to_png.py output.ppm output.png
```

批量运行 examples：

```bash
bash examples/run_all_examples.sh
```

输出默认在 `examples/output/`。

## OpenGL 示例

依赖安装：

```bash
brew install cmake glfw
```

构建与运行：

```bash
cmake -S opengl_scene -B opengl_scene/build
cmake --build opengl_scene/build
cd opengl_scene
./build/opengl_scene
```

运行后会生成 `image_opengl_balls_behind_torii_300.ppm`，可以转换为 PNG：

```bash
python3 ../ppm_to_png.py image_opengl_balls_behind_torii_300.ppm ../image_opengl_balls_behind_torii_300.png
```
