#ifndef MESH_H
#define MESH_H

#include "rt_utils.h"
#include "hittable.h"
#include "bvh.h"
#include "material.h"
#include <fstream>
#include <sstream>

class triangle : public hittable {
public:
    triangle() {}
    triangle(point3 v0, point3 v1, point3 v2, shared_ptr<material> m)
        : v0(v0), v1(v1), v2(v2), mat_ptr(m) {}

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
            vec3 outward_normal = unit_vector(cross(edge1, edge2));
            rec.set_face_normal(r, outward_normal);
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
    shared_ptr<material> mat_ptr;
};

class mesh : public hittable {
public:
    mesh() {}
    mesh(const std::string& filename, shared_ptr<material> m, float scale = 1.0, point3 offset = point3(0,0,0), float rotate_y = 0) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open mesh file: " << filename << std::endl;
            return;
        }

        std::vector<point3> vertices;
        std::string line;
        
        float cos_r = cos(rotate_y * PI / 180.0);
        float sin_r = sin(rotate_y * PI / 180.0);

        while (std::getline(file, line)) {
            if (line.substr(0, 2) == "v ") {
                std::istringstream s(line.substr(2));
                float x, y, z;
                s >> x >> y >> z;
                
                // Apply scaling
                x *= scale;
                y *= scale;
                z *= scale;
                
                // Apply rotation around Y axis
                float nx = x * cos_r + z * sin_r;
                float nz = -x * sin_r + z * cos_r;
                x = nx;
                z = nz;

                // Apply offset
                vertices.push_back(point3(x + offset.x(), y + offset.y(), z + offset.z()));
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

                for (size_t i = 1; i < v_indices.size() - 1; i++) {
                    triangles.add(make_shared<triangle>(
                        vertices[v_indices[0]],
                        vertices[v_indices[i]],
                        vertices[v_indices[i+1]],
                        m
                    ));
                }
            }
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
