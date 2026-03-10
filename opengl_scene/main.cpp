#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Vec3 {
    float x;
    float y;
    float z;
};

Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 operator*(const Vec3& a, float s) { return {a.x * s, a.y * s, a.z * s}; }

float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float length(const Vec3& v) { return std::sqrt(dot(v, v)); }

Vec3 normalize(const Vec3& v) {
    float len = length(v);
    if (len == 0.0f) return {0, 0, 0};
    return {v.x / len, v.y / len, v.z / len};
}

struct Mat4 {
    float m[16];
};

Mat4 identity() {
    Mat4 r{};
    r.m[0] = 1.0f; r.m[5] = 1.0f; r.m[10] = 1.0f; r.m[15] = 1.0f;
    return r;
}

Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int c = 0; c < 4; ++c) {
        for (int r0 = 0; r0 < 4; ++r0) {
            r.m[c * 4 + r0] =
                a.m[0 * 4 + r0] * b.m[c * 4 + 0] +
                a.m[1 * 4 + r0] * b.m[c * 4 + 1] +
                a.m[2 * 4 + r0] * b.m[c * 4 + 2] +
                a.m[3 * 4 + r0] * b.m[c * 4 + 3];
        }
    }
    return r;
}

Mat4 translate(const Vec3& t) {
    Mat4 r = identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
}

Mat4 scale(float s) {
    Mat4 r = identity();
    r.m[0] = s;
    r.m[5] = s;
    r.m[10] = s;
    return r;
}

Mat4 perspective(float fov, float aspect, float znear, float zfar) {
    float tanHalf = std::tan(fov * 0.5f);
    Mat4 r{};
    r.m[0] = 1.0f / (aspect * tanHalf);
    r.m[5] = 1.0f / tanHalf;
    r.m[10] = -(zfar + znear) / (zfar - znear);
    r.m[11] = -1.0f;
    r.m[14] = -(2.0f * zfar * znear) / (zfar - znear);
    return r;
}

Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = normalize(center - eye);
    Vec3 s = normalize(cross(f, up));
    Vec3 u = cross(s, f);
    Mat4 r = identity();
    r.m[0] = s.x; r.m[4] = s.y; r.m[8] = s.z;
    r.m[1] = u.x; r.m[5] = u.y; r.m[9] = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -dot(s, eye);
    r.m[13] = -dot(u, eye);
    r.m[14] = dot(f, eye);
    return r;
}

struct Mat3 {
    float m[9];
};

Mat3 mat3FromMat4(const Mat4& a) {
    Mat3 r{};
    r.m[0] = a.m[0]; r.m[1] = a.m[1]; r.m[2] = a.m[2];
    r.m[3] = a.m[4]; r.m[4] = a.m[5]; r.m[5] = a.m[6];
    r.m[6] = a.m[8]; r.m[7] = a.m[9]; r.m[8] = a.m[10];
    return r;
}

struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
};

struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
};

Mesh createMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    Mesh mesh;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
    mesh.indexCount = static_cast<GLsizei>(indices.size());
    return mesh;
}

Mesh createPlane(float size, const Vec3& normal, const Vec3& center, const Vec3& uDir, const Vec3& vDir) {
    Vec3 u = uDir * size;
    Vec3 v = vDir * size;
    Vec3 p0 = center - u - v;
    Vec3 p1 = center + u - v;
    Vec3 p2 = center + u + v;
    Vec3 p3 = center - u + v;
    std::vector<Vertex> vertices = {
        {p0.x, p0.y, p0.z, normal.x, normal.y, normal.z},
        {p1.x, p1.y, p1.z, normal.x, normal.y, normal.z},
        {p2.x, p2.y, p2.z, normal.x, normal.y, normal.z},
        {p3.x, p3.y, p3.z, normal.x, normal.y, normal.z}
    };
    std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};
    return createMesh(vertices, indices);
}

Mesh createSphere(float radius, int stacks, int slices) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    for (int i = 0; i <= stacks; ++i) {
        float v = float(i) / stacks;
        float phi = v * 3.1415926f;
        for (int j = 0; j <= slices; ++j) {
            float u = float(j) / slices;
            float theta = u * 2.0f * 3.1415926f;
            float x = std::sin(phi) * std::cos(theta);
            float y = std::sin(phi) * std::sin(theta);
            float z = std::cos(phi);
            vertices.push_back({x * radius, y * radius, z * radius, x, y, z});
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int row1 = i * (slices + 1);
            int row2 = (i + 1) * (slices + 1);
            indices.push_back(row1 + j);
            indices.push_back(row2 + j);
            indices.push_back(row2 + j + 1);
            indices.push_back(row1 + j);
            indices.push_back(row2 + j + 1);
            indices.push_back(row1 + j + 1);
        }
    }
    return createMesh(vertices, indices);
}

Mesh createTorus(float R, float r, int segments, int sides) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    for (int i = 0; i <= segments; ++i) {
        float u = float(i) / segments * 2.0f * 3.1415926f;
        float cu = std::cos(u);
        float su = std::sin(u);
        for (int j = 0; j <= sides; ++j) {
            float v = float(j) / sides * 2.0f * 3.1415926f;
            float cv = std::cos(v);
            float sv = std::sin(v);
            float x = (R + r * cv) * cu;
            float y = (R + r * cv) * su;
            float z = r * sv;
            float nx = cv * cu;
            float ny = cv * su;
            float nz = sv;
            vertices.push_back({x, y, z, nx, ny, nz});
        }
    }
    int stride = sides + 1;
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < sides; ++j) {
            int a = i * stride + j;
            int b = (i + 1) * stride + j;
            int c = (i + 1) * stride + j + 1;
            int d = i * stride + j + 1;
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);
            indices.push_back(a);
            indices.push_back(c);
            indices.push_back(d);
        }
    }
    return createMesh(vertices, indices);
}

Mesh createCylinder(float radius, float height, int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    float half = height * 0.5f;
    for (int i = 0; i <= segments; ++i) {
        float t = float(i) / segments * 2.0f * 3.1415926f;
        float x = std::cos(t);
        float y = std::sin(t);
        vertices.push_back({x * radius, y * radius, -half, x, y, 0});
        vertices.push_back({x * radius, y * radius, half, x, y, 0});
    }
    for (int i = 0; i < segments; ++i) {
        int base = i * 2;
        indices.push_back(base);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 3);
        indices.push_back(base + 2);
    }
    int centerBottom = static_cast<int>(vertices.size());
    vertices.push_back({0, 0, -half, 0, 0, -1});
    int centerTop = static_cast<int>(vertices.size());
    vertices.push_back({0, 0, half, 0, 0, 1});
    for (int i = 0; i <= segments; ++i) {
        float t = float(i) / segments * 2.0f * 3.1415926f;
        float x = std::cos(t) * radius;
        float y = std::sin(t) * radius;
        vertices.push_back({x, y, -half, 0, 0, -1});
        vertices.push_back({x, y, half, 0, 0, 1});
    }
    int start = centerTop + 1;
    for (int i = 0; i < segments; ++i) {
        indices.push_back(centerBottom);
        indices.push_back(start + i * 2);
        indices.push_back(start + ((i + 1) * 2) % ((segments + 1) * 2));
        indices.push_back(centerTop);
        indices.push_back(start + ((i + 1) * 2 + 1) % ((segments + 1) * 2));
        indices.push_back(start + i * 2 + 1);
    }
    return createMesh(vertices, indices);
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint compileShader(GLenum type, const std::string& src) {
    GLuint shader = glCreateShader(type);
    const char* cstr = src.c_str();
    glShaderSource(shader, 1, &cstr, nullptr);
    glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, info);
        std::cerr << info << std::endl;
    }
    return shader;
}

GLuint createProgram(const std::string& vsPath, const std::string& fsPath) {
    std::string vsSource = readFile(vsPath);
    std::string fsSource = readFile(fsPath);
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSource);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

bool savePPM(const std::string& path, int width, int height) {
    std::vector<unsigned char> pixels(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << width << " " << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y) {
        const unsigned char* row = pixels.data() + y * width * 3;
        out.write(reinterpret_cast<const char*>(row), width * 3);
    }
    return true;
}

void setMat4(GLuint program, const char* name, const Mat4& m) {
    GLint loc = glGetUniformLocation(program, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, m.m);
}

void setMat3(GLuint program, const char* name, const Mat3& m) {
    GLint loc = glGetUniformLocation(program, name);
    glUniformMatrix3fv(loc, 1, GL_FALSE, m.m);
}

void setVec3(GLuint program, const char* name, const Vec3& v) {
    GLint loc = glGetUniformLocation(program, name);
    glUniform3f(loc, v.x, v.y, v.z);
}

void setInt(GLuint program, const char* name, int v) {
    GLint loc = glGetUniformLocation(program, name);
    glUniform1i(loc, v);
}

void setFloat(GLuint program, const char* name, float v) {
    GLint loc = glGetUniformLocation(program, name);
    glUniform1f(loc, v);
}

int main() {
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    const int width = 1280;
    const int height = 720;
    GLFWwindow* window = glfwCreateWindow(width, height, "OpenGL Scene", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    GLuint program = createProgram("shaders/vertex.glsl", "shaders/fragment.glsl");
    glUseProgram(program);

    Mesh floorMesh = createPlane(5.0f, {0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {0, 1, 0});
    Mesh backWall = createPlane(5.0f, {0, -1, 0}, {0, 5, 2.0f}, {1, 0, 0}, {0, 0, 1});

    Mesh sphere = createSphere(1.0f, 64, 64);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Vec3 cameraPos = {0, -8, 3};
    Vec3 cameraTarget = {0, 0, 1};
    Vec3 up = {0, 0, 1};
    Mat4 view = lookAt(cameraPos, cameraTarget, up);
    Mat4 proj = perspective(35.0f * 3.1415926f / 180.0f, float(width) / float(height), 0.1f, 100.0f);
    Vec3 lightPos = {3, -4, 6};
    bool saved = false;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.75f, 0.82f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        setMat4(program, "uView", view);
        setMat4(program, "uProj", proj);
        setVec3(program, "uCameraPos", cameraPos);
        setVec3(program, "uLightPos", lightPos);
        setFloat(program, "uCheckerScale", 3.0f);

        setInt(program, "uMaterialType", 0);
        setFloat(program, "uAlpha", 1.0f);
        setFloat(program, "uMetalness", 0.0f);
        setFloat(program, "uRoughness", 0.6f);

        Mat4 model = identity();
        setMat4(program, "uModel", model);
        setMat3(program, "uNormalMat", mat3FromMat4(model));
        setInt(program, "uCheckerMode", 1);
        glBindVertexArray(floorMesh.vao);
        glDrawElements(GL_TRIANGLES, floorMesh.indexCount, GL_UNSIGNED_INT, 0);

        setInt(program, "uCheckerMode", 2);
        setMat4(program, "uModel", identity());
        setMat3(program, "uNormalMat", mat3FromMat4(identity()));
        glBindVertexArray(backWall.vao);
        glDrawElements(GL_TRIANGLES, backWall.indexCount, GL_UNSIGNED_INT, 0);

        setInt(program, "uCheckerMode", 0);

        model = multiply(translate({0.0f, 0.0f, 1.0f}), scale(1.0f));
        setMat4(program, "uModel", model);
        setMat3(program, "uNormalMat", mat3FromMat4(model));
        setVec3(program, "uColor", {0.9f, 0.95f, 1.0f});
        setInt(program, "uMaterialType", 2);
        setFloat(program, "uAlpha", 0.2f);
        setFloat(program, "uRoughness", 0.02f);
        glBindVertexArray(sphere.vao);
        glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);

        if (!saved) {
            glFinish();
            savePPM("image_opengl_balls_behind_torii_300.ppm", width, height);
            saved = true;
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
