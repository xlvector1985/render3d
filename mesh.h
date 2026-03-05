#ifndef MESH_H
#define MESH_H

#include "rt_utils.h"
#include "hittable.h"
#include "bvh.h"
#include "material.h"
#include <fstream>
#include <sstream>
#include <vector>

class triangle : public hittable {
public:
    triangle() {}
    triangle(point3 v0, point3 v1, point3 v2, vec3 n0, vec3 n1, vec3 n2, shared_ptr<material> m)
        : v0(v0), v1(v1), v2(v2), n0(n0), n1(n1), n2(n2), mat_ptr(m) {}

    virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
        vec3 edge1 = v1 - v0;
        vec3 edge2 = v2 - v0;
        vec3 h = cross(r.direction(), edge2);
        float a = dot(edge1, h);

        if (a > -1e-8 && a < 1e-8)
            return false;    // This ray is parallel to this triangle.

        float f = 1.0 / a;
        vec3 s = r.origin() - v0;
        float u = f * dot(s, h);

        if (u < 0.0 || u > 1.0)
            return false;

        vec3 q = cross(s, edge1);
        float v = f * dot(r.direction(), q);

        if (v < 0.0 || u + v > 1.0)
            return false;

        float t = f * dot(edge2, q);

        if (t > t_min && t < t_max) {
            rec.t = t;
            rec.p = r.at(t);
            
            // Interpolate normal using barycentric coordinates
            // w = 1 - u - v
            float w = 1.0 - u - v;
            vec3 interpolated_normal = unit_vector(w * n0 + u * n1 + v * n2);
            
            rec.set_face_normal(r, interpolated_normal);
            rec.mat_ptr = mat_ptr;
            rec.u = u;
            rec.v = v;
            return true;
        }
        else
            return false;
    }

    virtual bool bounding_box(float time0, float time1, aabb& output_box) const override {
        float min_x = fmin(v0.x(), fmin(v1.x(), v2.x()));
        float min_y = fmin(v0.y(), fmin(v1.y(), v2.y()));
        float min_z = fmin(v0.z(), fmin(v1.z(), v2.z()));

        float max_x = fmax(v0.x(), fmax(v1.x(), v2.x()));
        float max_y = fmax(v0.y(), fmax(v1.y(), v2.y()));
        float max_z = fmax(v0.z(), fmax(v1.z(), v2.z()));

        output_box = aabb(point3(min_x - 0.001, min_y - 0.001, min_z - 0.001),
                          point3(max_x + 0.001, max_y + 0.001, max_z + 0.001));
        return true;
    }

private:
    point3 v0, v1, v2;
    vec3 n0, n1, n2;
    shared_ptr<material> mat_ptr;
};

class mesh : public hittable {
public:
    mesh() {}
    mesh(const std::string& filename, shared_ptr<material> m, float scale = 1.0, point3 offset = point3(0,0,0), float rx = 0, float ry = 0, float rz = 0) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open mesh file: " << filename << std::endl;
            return;
        }

        std::vector<point3> vertices;
        std::vector<vec3> vertex_normals;
        std::vector<std::vector<int>> face_indices; // Stores vertex indices for each face
        
        std::string line;
        
        auto radians = [](float deg) { return deg * PI / 180.0; };
        float sin_x = sin(radians(rx)); float cos_x = cos(radians(rx));
        float sin_y = sin(radians(ry)); float cos_y = cos(radians(ry));
        float sin_z = sin(radians(rz)); float cos_z = cos(radians(rz));

        while (std::getline(file, line)) {
            if (line.substr(0, 2) == "v ") {
                std::istringstream s(line.substr(2));
                float x, y, z;
                s >> x >> y >> z;
                
                // Apply scaling
                x *= scale;
                y *= scale;
                z *= scale;
                
                // Rotate around X
                float y1 = y * cos_x - z * sin_x;
                float z1 = y * sin_x + z * cos_x;
                y = y1; z = z1;
                
                // Rotate around Y
                float x1 = x * cos_y + z * sin_y;
                float z2 = -x * sin_y + z * cos_y;
                x = x1; z = z2;
                
                // Rotate around Z
                float x2 = x * cos_z - y * sin_z;
                float y2 = x * sin_z + y * cos_z;
                x = x2; y = y2;

                // Apply offset
                vertices.push_back(point3(x + offset.x(), y + offset.y(), z + offset.z()));
                
                // Initialize normal for this vertex
                vertex_normals.push_back(vec3(0,0,0));
            } else if (line.substr(0, 2) == "f ") {
                std::string vertex_string;
                std::istringstream s(line.substr(2));
                std::vector<int> v_indices;
                
                while (s >> vertex_string) {
                    size_t pos = vertex_string.find('/');
                    if (pos != std::string::npos) {
                        v_indices.push_back(std::stoi(vertex_string.substr(0, pos)) - 1);
                    } else {
                        v_indices.push_back(std::stoi(vertex_string) - 1);
                    }
                }
                
                // Triangulate polygon
                for (size_t i = 1; i < v_indices.size() - 1; i++) {
                    std::vector<int> triangle_indices;
                    triangle_indices.push_back(v_indices[0]);
                    triangle_indices.push_back(v_indices[i]);
                    triangle_indices.push_back(v_indices[i+1]);
                    face_indices.push_back(triangle_indices);
                }
            }
        }
        
        // Compute vertex normals by averaging face normals
        for (const auto& indices : face_indices) {
            point3 v0 = vertices[indices[0]];
            point3 v1 = vertices[indices[1]];
            point3 v2 = vertices[indices[2]];
            
            vec3 edge1 = v1 - v0;
            vec3 edge2 = v2 - v0;
            vec3 normal = cross(edge1, edge2); // Weighted by area
            
            vertex_normals[indices[0]] += normal;
            vertex_normals[indices[1]] += normal;
            vertex_normals[indices[2]] += normal;
        }
        
        // Normalize all vertex normals
        for (auto& n : vertex_normals) {
            if (n.length_squared() > 0) {
                n = unit_vector(n);
            } else {
                n = vec3(0,1,0); // Default up if degenerate
            }
        }
        
        // Create triangles with smooth normals
        for (const auto& indices : face_indices) {
            triangles.add(make_shared<triangle>(
                vertices[indices[0]],
                vertices[indices[1]],
                vertices[indices[2]],
                vertex_normals[indices[0]],
                vertex_normals[indices[1]],
                vertex_normals[indices[2]],
                m
            ));
        }
        
        if (triangles.objects.empty()) {
            std::cerr << "Warning: Mesh " << filename << " has no triangles." << std::endl;
            bvh_root = nullptr;
        } else {
            bvh_root = make_shared<bvh_node>(triangles, 0, 1);
        }
    }

    virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
        if (!bvh_root) return false;
        return bvh_root->hit(r, t_min, t_max, rec);
    }

    virtual bool bounding_box(float time0, float time1, aabb& output_box) const override {
        if (!bvh_root) return false;
        return bvh_root->bounding_box(time0, time1, output_box);
    }

private:
    hittable_list triangles;
    shared_ptr<bvh_node> bvh_root;
};

#endif
