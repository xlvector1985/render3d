#include "rt_utils.h"
#include "hittable.h"
#include "material.h"
#include "bvh.h"
#include "mesh.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <string>

using namespace std;

// -- Scene Settings --
struct SceneSettings {
    point3 lookfrom{13, 2, 3};
    point3 lookat{0, 0, 0};
    vec3 vup{0, 1, 0};
    float vfov = 20;
    float aspect_ratio = 16.0/9.0;
    float aperture = 0.0;
    float focus_dist = 10.0;
    int image_width = 400;
    int samples_per_pixel = 100;
    int max_depth = 50;
    color background{0.7, 0.8, 1.0};
};

// -- Primitive Classes (Sphere, Rects, Cylinder, Torus) --
// (Mesh is in mesh.h)

class sphere : public hittable {
public:
    sphere() {}
    sphere(point3 cen, float r, shared_ptr<material> m)
        : center(cen), radius(r), mat_ptr(m) {}

    virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
        vec3 oc = r.origin() - center;
        float a = r.direction().length_squared();
        float half_b = dot(oc, r.direction());
        float c = oc.length_squared() - radius*radius;

        float discriminant = half_b*half_b - a*c;
        if (discriminant < 0) return false;
        float sqrtd = sqrt(discriminant);

        float root = (-half_b - sqrtd) / a;
        if (root < t_min || t_max < root) {
            root = (-half_b + sqrtd) / a;
            if (root < t_min || t_max < root)
                return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat_ptr = mat_ptr;

        return true;
    }

    virtual bool bounding_box(float time0, float time1, aabb& output_box) const override {
        output_box = aabb(
            center - vec3(radius, radius, radius),
            center + vec3(radius, radius, radius)
        );
        return true;
    }

private:
    point3 center;
    float radius;
    shared_ptr<material> mat_ptr;
};

class xy_rect : public hittable {
public:
    xy_rect() {}
    xy_rect(float x0, float x1, float y0, float y1, float k, shared_ptr<material> mat)
        : x0(x0), x1(x1), y0(y0), y1(y1), k(k), mp(mat) {}

    virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
        float t = (k - r.origin().z()) / r.direction().z();
        if (t < t_min || t > t_max)
            return false;
        float x = r.origin().x() + t*r.direction().x();
        float y = r.origin().y() + t*r.direction().y();
        if (x < x0 || x > x1 || y < y0 || y > y1)
            return false;
        rec.u = (x - x0) / (x1 - x0);
        rec.v = (y - y0) / (y1 - y0);
        rec.t = t;
        vec3 outward_normal = vec3(0, 0, 1);
        rec.set_face_normal(r, outward_normal);
        rec.mat_ptr = mp;
        rec.p = r.at(t);
        return true;
    }

    virtual bool bounding_box(float time0, float time1, aabb& output_box) const override {
        output_box = aabb(
            point3(x0, y0, k - 0.0001),
            point3(x1, y1, k + 0.0001)
        );
        return true;
    }

private:
    float x0, x1, y0, y1, k;
    shared_ptr<material> mp;
};

class xz_rect : public hittable {
public:
    xz_rect() {}
    xz_rect(float x0, float x1, float z0, float z1, float k, shared_ptr<material> mat)
        : x0(x0), x1(x1), z0(z0), z1(z1), k(k), mp(mat) {}

    virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
        float t = (k - r.origin().y()) / r.direction().y();
        if (t < t_min || t > t_max)
            return false;
        float x = r.origin().x() + t*r.direction().x();
        float z = r.origin().z() + t*r.direction().z();
        if (x < x0 || x > x1 || z < z0 || z > z1)
            return false;
        rec.u = (x - x0) / (x1 - x0);
        rec.v = (z - z0) / (z1 - z0);
        rec.t = t;
        vec3 outward_normal = vec3(0, 1, 0);
        rec.set_face_normal(r, outward_normal);
        rec.mat_ptr = mp;
        rec.p = r.at(t);
        return true;
    }

    virtual bool bounding_box(float time0, float time1, aabb& output_box) const override {
        output_box = aabb(
            point3(x0, k - 0.0001, z0),
            point3(x1, k + 0.0001, z1)
        );
        return true;
    }

private:
    float x0, x1, z0, z1, k;
    shared_ptr<material> mp;
};

class yz_rect : public hittable {
public:
    yz_rect() {}
    yz_rect(float y0, float y1, float z0, float z1, float k, shared_ptr<material> mat)
        : y0(y0), y1(y1), z0(z0), z1(z1), k(k), mp(mat) {}

    virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
        float t = (k - r.origin().x()) / r.direction().x();
        if (t < t_min || t > t_max)
            return false;
        float y = r.origin().y() + t*r.direction().y();
        float z = r.origin().z() + t*r.direction().z();
        if (y < y0 || y > y1 || z < z0 || z > z1)
            return false;
        rec.u = (y - y0) / (y1 - y0);
        rec.v = (z - z0) / (z1 - z0);
        rec.t = t;
        vec3 outward_normal = vec3(1, 0, 0);
        rec.set_face_normal(r, outward_normal);
        rec.mat_ptr = mp;
        rec.p = r.at(t);
        return true;
    }

    virtual bool bounding_box(float time0, float time1, aabb& output_box) const override {
        output_box = aabb(
            point3(k - 0.0001, y0, z0),
            point3(k + 0.0001, y1, z1)
        );
        return true;
    }

private:
    float y0, y1, z0, z1, k;
    shared_ptr<material> mp;
};

class cylinder : public hittable {
public:
    cylinder() {}
    cylinder(point3 cen, float r, float h, shared_ptr<material> m)
        : center(cen), radius(r), height(h), mat_ptr(m) {}

    virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
        bool hit_anything = false;
        float closest_so_far = t_max;

        vec3 oc = r.origin() - center;
        float a = r.direction().x()*r.direction().x() + r.direction().y()*r.direction().y();
        float b = 2.0 * (oc.x()*r.direction().x() + oc.y()*r.direction().y());
        float c = oc.x()*oc.x() + oc.y()*oc.y() - radius*radius;

        float discriminant = b*b - 4*a*c;
        if (discriminant >= 0) {
            float sqrtd = sqrt(discriminant);
            float root = (-b - sqrtd) / (2*a);
            
            if (root >= t_min && root < closest_so_far) {
                point3 p = r.at(root);
                float z = p.z() - center.z();
                if (z >= 0 && z <= height) {
                    closest_so_far = root;
                    hit_anything = true;
                    rec.t = root;
                    rec.p = p;
                    vec3 outward_normal = vec3((p.x() - center.x())/radius, (p.y() - center.y())/radius, 0);
                    rec.set_face_normal(r, outward_normal);
                    rec.mat_ptr = mat_ptr;
                }
            }
            
            root = (-b + sqrtd) / (2*a);
            if (root >= t_min && root < closest_so_far) {
                point3 p = r.at(root);
                float z = p.z() - center.z();
                if (z >= 0 && z <= height) {
                    closest_so_far = root;
                    hit_anything = true;
                    rec.t = root;
                    rec.p = p;
                    vec3 outward_normal = vec3((p.x() - center.x())/radius, (p.y() - center.y())/radius, 0);
                    rec.set_face_normal(r, outward_normal);
                    rec.mat_ptr = mat_ptr;
                }
            }
        }

        float bottom_z = center.z();
        if (fabs(r.direction().z()) > 1e-8) {
            float t = (bottom_z - r.origin().z()) / r.direction().z();
            if (t >= t_min && t < closest_so_far) {
                point3 p = r.at(t);
                float dx = p.x() - center.x();
                float dy = p.y() - center.y();
                if (dx*dx + dy*dy <= radius*radius) {
                    closest_so_far = t;
                    hit_anything = true;
                    rec.t = t;
                    rec.p = p;
                    vec3 outward_normal = vec3(0, 0, -1);
                    rec.set_face_normal(r, outward_normal);
                    rec.mat_ptr = mat_ptr;
                }
            }
        }

        float top_z = center.z() + height;
        if (fabs(r.direction().z()) > 1e-8) {
            float t = (top_z - r.origin().z()) / r.direction().z();
            if (t >= t_min && t < closest_so_far) {
                point3 p = r.at(t);
                float dx = p.x() - center.x();
                float dy = p.y() - center.y();
                if (dx*dx + dy*dy <= radius*radius) {
                    closest_so_far = t;
                    hit_anything = true;
                    rec.t = t;
                    rec.p = p;
                    vec3 outward_normal = vec3(0, 0, 1);
                    rec.set_face_normal(r, outward_normal);
                    rec.mat_ptr = mat_ptr;
                }
            }
        }

        return hit_anything;
    }

    virtual bool bounding_box(float time0, float time1, aabb& output_box) const override {
        output_box = aabb(
            point3(center.x() - radius, center.y() - radius, center.z()),
            point3(center.x() + radius, center.y() + radius, center.z() + height)
        );
        return true;
    }

private:
    point3 center;
    float radius;
    float height;
    shared_ptr<material> mat_ptr;
};

class torus : public hittable {
public:
    torus() {}
    torus(point3 cen, float major_radius, float minor_radius, shared_ptr<material> m, float rx=0, float ry=0, float rz=0)
        : center(cen), r_major(major_radius), r_minor(minor_radius), mat_ptr(m), rot_x(rx), rot_y(ry), rot_z(rz) {
            init_rotation();
        }

    virtual bool hit(const ray& r_in, float t_min, float t_max, hit_record& rec) const override {
        vec3 origin = r_in.origin() - center;
        origin = rotate_inverse(origin);
        vec3 direction = rotate_inverse(r_in.direction());
        
        vec3 ro = origin;
        vec3 rd = direction;
        
        float R = r_major;
        float r = r_minor;
        
        float a = dot(rd, rd);
        float b = 2.0 * dot(ro, rd);
        float c = dot(ro, ro) + R*R - r*r;
        float d = 4.0 * R*R * (rd.x()*rd.x() + rd.y()*rd.y());
        float e = 8.0 * R*R * (ro.x()*rd.x() + ro.y()*rd.y());
        float f = 4.0 * R*R * (ro.x()*ro.x() + ro.y()*ro.y());
        
        float coeffs[5];
        coeffs[4] = a*a;
        coeffs[3] = 2*a*b;
        coeffs[2] = 2*a*c + b*b - d;
        coeffs[1] = 2*b*c - e;
        coeffs[0] = c*c - f;
        
        vector<float> roots;
        int n = solve_quartic(coeffs, roots);
        
        bool hit_anything = false;
        float closest_so_far = t_max;
        
        for (int i = 0; i < n; i++) {
            float t = roots[i];
            
            if (t < t_min - 1e-5 || t > closest_so_far)
                continue;
            
            if (t < t_min) t = t_min;
            
            vec3 p_local = ro + t * rd;
            
            vec3 p_xy = vec3(p_local.x(), p_local.y(), 0);
            float len_xy = p_xy.length();
            if (len_xy < 1e-6)
                continue;
            
            vec3 center_tube = (p_xy / len_xy) * R;
            vec3 normal_local = p_local - center_tube;
            normal_local = unit_vector(normal_local);
            
            vec3 p_world = rotate(p_local) + center;
            vec3 normal_world = rotate(normal_local);
            
            closest_so_far = t;
            hit_anything = true;
            rec.t = t;
            rec.p = p_world;
            rec.set_face_normal(r_in, normal_world);
            rec.mat_ptr = mat_ptr;
        }
        
        return hit_anything;
    }

    virtual bool bounding_box(float time0, float time1, aabb& output_box) const override {
        float max_dim = r_major + r_minor;
        output_box = aabb(
            center - vec3(max_dim, max_dim, max_dim),
            center + vec3(max_dim, max_dim, max_dim)
        );
        return true;
    }

private:
    point3 center;
    float r_major;
    float r_minor;
    shared_ptr<material> mat_ptr;
    float rot_x, rot_y, rot_z;
    float sin_x, cos_x, sin_y, cos_y, sin_z, cos_z;

    void init_rotation() {
        auto radians = [](float deg) { return deg * PI / 180.0; };
        sin_x = sin(radians(rot_x)); cos_x = cos(radians(rot_x));
        sin_y = sin(radians(rot_y)); cos_y = cos(radians(rot_y));
        sin_z = sin(radians(rot_z)); cos_z = cos(radians(rot_z));
    }

    vec3 rotate(vec3 v) const {
        float y = v.y() * cos_x - v.z() * sin_x;
        float z = v.y() * sin_x + v.z() * cos_x;
        v = vec3(v.x(), y, z);
        
        float x = v.x() * cos_y + v.z() * sin_y;
        z = -v.x() * sin_y + v.z() * cos_y;
        v = vec3(x, v.y(), z);
        
        x = v.x() * cos_z - v.y() * sin_z;
        y = v.x() * sin_z + v.y() * cos_z;
        return vec3(x, y, v.z());
    }

    vec3 rotate_inverse(vec3 v) const {
        float x = v.x() * cos_z + v.y() * sin_z;
        float y = -v.x() * sin_z + v.y() * cos_z;
        v = vec3(x, y, v.z());
        
        x = v.x() * cos_y - v.z() * sin_y;
        float z = v.x() * sin_y + v.z() * cos_y;
        v = vec3(x, v.y(), z);
        
        y = v.y() * cos_x + v.z() * sin_x;
        z = -v.y() * sin_x + v.z() * cos_x;
        return vec3(v.x(), y, z);
    }

    static int solve_quadratic(double coeffs[3], vector<double>& roots) {
        double a = coeffs[2];
        double b = coeffs[1];
        double c = coeffs[0];
        double discriminant = b*b - 4*a*c;
        if (discriminant < 0) return 0;
        if (discriminant == 0) {
            roots.push_back(-b / (2*a));
            return 1;
        }
        double sqrt_d = sqrt(discriminant);
        roots.push_back((-b - sqrt_d) / (2*a));
        roots.push_back((-b + sqrt_d) / (2*a));
        return 2;
    }

    static int solve_cubic(double coeffs[4], vector<double>& roots) {
        double a = coeffs[3];
        double b = coeffs[2] / a;
        double c = coeffs[1] / a;
        double d = coeffs[0] / a;
        double Q = (3*c - b*b) / 9;
        double R = (9*b*c - 27*d - 2*b*b*b) / 54;
        double D = Q*Q*Q + R*R;
        if (D >= 0) {
            double S = cbrt(R + sqrt(D));
            double T = cbrt(R - sqrt(D));
            roots.push_back(S + T - b/3.0);
            return 1;
        } else {
            double argument = R / sqrt(-Q*Q*Q);
            if (argument < -1.0) argument = -1.0;
            if (argument > 1.0) argument = 1.0;
            double theta = acos(argument);
            double sqrt_minus_Q = sqrt(-Q);
            roots.push_back(2.0 * sqrt_minus_Q * cos(theta/3.0) - b/3.0);
            roots.push_back(2.0 * sqrt_minus_Q * cos((theta + 2.0*PI)/3.0) - b/3.0);
            roots.push_back(2.0 * sqrt_minus_Q * cos((theta + 4.0*PI)/3.0) - b/3.0);
            return 3;
        }
    }

    static int solve_quartic(float coeffs[5], vector<float>& roots) {
        double a = coeffs[4];
        double b = coeffs[3] / a;
        double c = coeffs[2] / a;
        double d = coeffs[1] / a;
        double e = coeffs[0] / a;

        double b2 = b * b;
        double P = c - 0.375 * b2;
        double Q = d + 0.125 * b2 * b - 0.5 * b * c;
        double R = e - 0.01171875 * b2 * b2 + 0.0625 * b2 * c - 0.25 * b * d;

        if (fabs(Q) < 1e-9) {
            double quad_coeffs[3] = {R, P, 1};
            vector<double> y2_roots;
            solve_quadratic(quad_coeffs, y2_roots);
            for (double y2 : y2_roots) {
                if (y2 >= 0) {
                    double y = sqrt(y2);
                    roots.push_back((float)(y - b/4.0));
                    roots.push_back((float)(-y - b/4.0));
                }
            }
        } else {
            double cubic_coeffs[4] = {-Q * Q, P * P - 4 * R, 2 * P, 1};
            vector<double> u_roots;
            solve_cubic(cubic_coeffs, u_roots);
            
            double u = -1.0;
            bool found_u = false;
            for (double r : u_roots) {
                if (r > 1e-9) {
                    if (!found_u || r > u) {
                        u = r;
                        found_u = true;
                    }
                }
            }
            if (!found_u) {
                for (double r : u_roots) {
                    if (r > -1e-6) {
                        if (!found_u || r > u) {
                            u = 0.0;
                            found_u = true;
                        }
                    }
                }
            }
            if (!found_u) return 0;

            double k = sqrt(u);
            if (k < 1e-9) k = 1e-9;
            double m = (P + u - Q/k) / 2.0;
            double n = (P + u + Q/k) / 2.0;
            double quad1[3] = {m, k, 1};
            double quad2[3] = {n, -k, 1};
            vector<double> q1_roots, q2_roots;
            solve_quadratic(quad1, q1_roots);
            solve_quadratic(quad2, q2_roots);
            for (double y : q1_roots) roots.push_back((float)(y - b/4.0));
            for (double y : q2_roots) roots.push_back((float)(y - b/4.0));
        }
        
        double A = coeffs[4], B = coeffs[3], C = coeffs[2], D = coeffs[1], E = coeffs[0];
        for (float& r : roots) {
            double t = (double)r;
            for (int i = 0; i < 2; i++) {
                double f = (((A*t + B)*t + C)*t + D)*t + E;
                double df = ((4.0*A*t + 3.0*B)*t + 2.0*C)*t + D;
                if (std::abs(df) < 1e-8) break;
                double step = f / df;
                t -= step;
                if (std::abs(step) < 1e-6) break;
            }
            r = (float)t;
        }
        return (int)roots.size();
    }
    
    static int solve_quadratic(float coeffs[3], vector<float>& roots) {
        float a = coeffs[2], b = coeffs[1], c = coeffs[0];
        float discriminant = b*b - 4*a*c;
        if (discriminant < 0) return 0;
        float sqrt_d = sqrt(discriminant);
        roots.push_back((-b - sqrt_d) / (2*a));
        roots.push_back((-b + sqrt_d) / (2*a));
        return 2;
    }
};

class camera {
public:
    camera(
        point3 lookfrom,
        point3 lookat,
        vec3 vup,
        float vfov,
        float aspect_ratio,
        float aperture,
        float focus_dist
    ) {
        float theta = degrees_to_radians(vfov);
        float h = tan(theta/2);
        float viewport_height = 2.0 * h;
        float viewport_width = aspect_ratio * viewport_height;

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        origin = lookfrom;
        horizontal = focus_dist * viewport_width * u;
        vertical = focus_dist * viewport_height * v;
        lower_left_corner = origin - horizontal/2 - vertical/2 - focus_dist * w;

        lens_radius = aperture / 2;
    }

    ray get_ray(float s, float t) const {
        vec3 rd = lens_radius * random_in_unit_disk();
        vec3 offset = u * rd.x() + v * rd.y();
        return ray(
            origin + offset,
            lower_left_corner + s*horizontal + t*vertical - origin - offset
        );
    }

private:
    point3 origin;
    point3 lower_left_corner;
    vec3 horizontal;
    vec3 vertical;
    vec3 u, v, w;
    float lens_radius;
};

color ray_color(const ray& r, const hittable& world, int depth, const color& background) {
    hit_record rec;

    if (depth <= 0)
        return color(0, 0, 0);

    if (world.hit(r, 0.001, INF, rec)) {
        ray scattered;
        color attenuation;
        color emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
        if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            return emitted + attenuation * ray_color(scattered, world, depth-1, background);
        return emitted;
    }

    // Use background color if no hit
    return background;
}

void write_color(ostream &out, color pixel_color, int samples_per_pixel) {
    float r = pixel_color.x();
    float g = pixel_color.y();
    float b = pixel_color.z();

    float scale = 1.0 / samples_per_pixel;
    r = sqrt(scale * r);
    g = sqrt(scale * g);
    b = sqrt(scale * b);

    out << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';
}

shared_ptr<hittable> load_scene_from_file(const string& filename, SceneSettings& settings) {
    hittable_list world;
    
    ifstream file(filename);
    if (!file) {
        cerr << "Error: Could not open scene file: " << filename << endl;
        return make_shared<bvh_node>(world, 0, 1);
    }
    
    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string type;
        iss >> type;
        
        if (type.empty() || type[0] == '#')
            continue;
        
        if (type == "camera") {
            float lx, ly, lz, ax, ay, az, ux, uy, uz, fov, aspect;
            iss >> lx >> ly >> lz >> ax >> ay >> az >> ux >> uy >> uz >> fov >> aspect;
            settings.lookfrom = point3(lx, ly, lz);
            settings.lookat = point3(ax, ay, az);
            settings.vup = vec3(ux, uy, uz);
            settings.vfov = fov;
            settings.aspect_ratio = aspect;
        } else if (type == "resolution") {
            int w, h;
            iss >> w >> h;
            settings.image_width = w;
            settings.aspect_ratio = (float)w / (float)h;
        } else if (type == "samples") {
            int s;
            iss >> s;
            settings.samples_per_pixel = s;
        } else if (type == "max_depth") {
            int d;
            iss >> d;
            settings.max_depth = d;
        } else if (type == "background") {
            float r, g, b;
            iss >> r >> g >> b;
            settings.background = color(r, g, b);
        } else if (type == "sphere") {
            float x, y, z, r, cr, cg, cb;
            string mat_type;
            iss >> x >> y >> z >> r >> mat_type >> cr >> cg >> cb;
            
            shared_ptr<material> mat;
            if (mat_type == "lambertian") {
                mat = make_shared<lambertian>(color(cr, cg, cb));
            } else if (mat_type == "metal") {
                float fuzz;
                iss >> fuzz;
                mat = make_shared<metal>(color(cr, cg, cb), fuzz);
            } else if (mat_type == "dielectric") {
                float ir;
                iss >> ir;
                mat = make_shared<dielectric>(ir, color(cr, cg, cb));
            } else if (mat_type == "light") {
                mat = make_shared<diffuse_light>(color(cr, cg, cb));
            } else {
                continue;
            }
            
            world.add(make_shared<sphere>(point3(x, y, z), r, mat));
        } else if (type == "cylinder") {
            float x, y, z, r, h, cr, cg, cb;
            string mat_type;
            iss >> x >> y >> z >> r >> h >> mat_type >> cr >> cg >> cb;
            
            shared_ptr<material> mat;
            if (mat_type == "lambertian") {
                mat = make_shared<lambertian>(color(cr, cg, cb));
            } else if (mat_type == "metal") {
                float fuzz;
                iss >> fuzz;
                mat = make_shared<metal>(color(cr, cg, cb), fuzz);
            } else if (mat_type == "dielectric") {
                float ir;
                iss >> ir;
                mat = make_shared<dielectric>(ir, color(cr, cg, cb));
            } else if (mat_type == "light") {
                mat = make_shared<diffuse_light>(color(cr, cg, cb));
            } else {
                continue;
            }
            
            world.add(make_shared<cylinder>(point3(x, y, z), r, h, mat));
        } else if (type == "torus") {
            float x, y, z, R, r, cr, cg, cb;
            string mat_type;
            iss >> x >> y >> z >> R >> r >> mat_type >> cr >> cg >> cb;
            
            shared_ptr<material> mat;
            if (mat_type == "lambertian") {
                mat = make_shared<lambertian>(color(cr, cg, cb));
            } else if (mat_type == "metal") {
                float fuzz;
                iss >> fuzz;
                mat = make_shared<metal>(color(cr, cg, cb), fuzz);
            } else if (mat_type == "dielectric") {
                float ir;
                iss >> ir;
                mat = make_shared<dielectric>(ir, color(cr, cg, cb));
            } else if (mat_type == "light") {
                mat = make_shared<diffuse_light>(color(cr, cg, cb));
            } else {
                continue;
            }
            
            float rx = 0, ry = 0, rz = 0;
            if (iss) iss >> rx;
            if (iss) iss >> ry;
            if (iss) iss >> rz;
            
            world.add(make_shared<torus>(point3(x, y, z), R, r, mat, rx, ry, rz));
        } else if (type == "mesh") {
            string filename;
            float scale, x, y, z, ry, cr, cg, cb;
            string mat_type;
            iss >> filename >> scale >> x >> y >> z >> ry >> mat_type >> cr >> cg >> cb;
            
            shared_ptr<material> mat;
            if (mat_type == "lambertian") {
                mat = make_shared<lambertian>(color(cr, cg, cb));
            } else if (mat_type == "metal") {
                float fuzz;
                iss >> fuzz;
                mat = make_shared<metal>(color(cr, cg, cb), fuzz);
            } else if (mat_type == "dielectric") {
                float ir;
                iss >> ir;
                mat = make_shared<dielectric>(ir, color(cr, cg, cb));
            } else {
                // Default to lambertian if unknown or just continue
                mat = make_shared<lambertian>(color(0.5, 0.5, 0.5));
            }
            
            world.add(make_shared<mesh>(filename, mat, scale, point3(x, y, z), ry));
        } else if (type == "xy_rect") {
            float x0, x1, y0, y1, k, cr, cg, cb;
            string mat_type;
            iss >> x0 >> x1 >> y0 >> y1 >> k >> mat_type >> cr >> cg >> cb;
            
            shared_ptr<material> mat;
            if (mat_type == "lambertian") {
                mat = make_shared<lambertian>(color(cr, cg, cb));
            } else if (mat_type == "metal") {
                float fuzz;
                iss >> fuzz;
                mat = make_shared<metal>(color(cr, cg, cb), fuzz);
            } else if (mat_type == "dielectric") {
                float ir;
                iss >> ir;
                mat = make_shared<dielectric>(ir);
            } else if (mat_type == "checker") {
                float r1, g1, b1, r2, g2, b2;
                iss >> r1 >> g1 >> b1 >> r2 >> g2 >> b2;
                auto checker = make_shared<checker_texture>(color(r1, g1, b1), color(r2, g2, b2), checker_texture::plane_type::XY);
                mat = make_shared<lambertian>(checker);
            } else if (mat_type == "light") {
                mat = make_shared<diffuse_light>(color(cr, cg, cb));
            } else {
                continue;
            }
            
            world.add(make_shared<xy_rect>(x0, x1, y0, y1, k, mat));
        } else if (type == "yz_rect") {
            float y0, y1, z0, z1, k, cr, cg, cb;
            string mat_type;
            iss >> y0 >> y1 >> z0 >> z1 >> k >> mat_type >> cr >> cg >> cb;
            
            shared_ptr<material> mat;
            if (mat_type == "lambertian") {
                mat = make_shared<lambertian>(color(cr, cg, cb));
            } else if (mat_type == "metal") {
                float fuzz;
                iss >> fuzz;
                mat = make_shared<metal>(color(cr, cg, cb), fuzz);
            } else if (mat_type == "dielectric") {
                float ir;
                iss >> ir;
                mat = make_shared<dielectric>(ir);
            } else if (mat_type == "checker") {
                float r1, g1, b1, r2, g2, b2;
                iss >> r1 >> g1 >> b1 >> r2 >> g2 >> b2;
                auto checker = make_shared<checker_texture>(color(r1, g1, b1), color(r2, g2, b2), checker_texture::plane_type::YZ);
                mat = make_shared<lambertian>(checker);
            } else if (mat_type == "light") {
                mat = make_shared<diffuse_light>(color(cr, cg, cb));
            } else {
                continue;
            }
            
            world.add(make_shared<yz_rect>(y0, y1, z0, z1, k, mat));
        } else if (type == "xz_rect") {
            float x0, x1, z0, z1, k, cr, cg, cb;
            string mat_type;
            iss >> x0 >> x1 >> z0 >> z1 >> k >> mat_type >> cr >> cg >> cb;
            
            shared_ptr<material> mat;
            if (mat_type == "lambertian") {
                mat = make_shared<lambertian>(color(cr, cg, cb));
            } else if (mat_type == "metal") {
                float fuzz;
                iss >> fuzz;
                mat = make_shared<metal>(color(cr, cg, cb), fuzz);
            } else if (mat_type == "dielectric") {
                float ir;
                iss >> ir;
                mat = make_shared<dielectric>(ir);
            } else if (mat_type == "checker") {
                float r1, g1, b1, r2, g2, b2;
                iss >> r1 >> g1 >> b1 >> r2 >> g2 >> b2;
                auto checker = make_shared<checker_texture>(color(r1, g1, b1), color(r2, g2, b2), checker_texture::plane_type::XZ);
                mat = make_shared<lambertian>(checker);
            } else if (mat_type == "light") {
                mat = make_shared<diffuse_light>(color(cr, cg, cb));
            } else {
                continue;
            }
            
            world.add(make_shared<xz_rect>(x0, x1, z0, z1, k, mat));
        }
    }
    
    return make_shared<bvh_node>(world, 0, 1);
}

int main(int argc, char** argv) {
    SceneSettings settings;
    string scene_file = "";
    
    // Simple argument parsing
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg.find(".txt") != string::npos) {
            scene_file = arg;
        } else {
            // Try to parse as samples or depth? 
            // Better to rely on scene file for settings now, or strictly positional
            // If it looks like a number, maybe override samples?
            try {
                int val = stoi(arg);
                // If we haven't set samples via CLI yet (default is 100), maybe this is it?
                // For safety, let's just say CLI args are: [samples] [depth] [file] OR just [file]
                if (i == 1 && arg.find(".txt") == string::npos) settings.samples_per_pixel = val;
                if (i == 2 && arg.find(".txt") == string::npos) settings.max_depth = val;
            } catch (...) {
                // Ignore non-number args that aren't .txt
            }
        }
    }
    
    shared_ptr<hittable> world;
    if (!scene_file.empty()) {
        cerr << "Loading scene from: " << scene_file << endl;
        world = load_scene_from_file(scene_file, settings);
    } else {
        // Fallback to a hardcoded default scene if needed, but for now we expect a file
        cerr << "No scene file provided. Exiting." << endl;
        return 1;
    }

    const int image_width = settings.image_width;
    const int image_height = static_cast<int>(image_width / settings.aspect_ratio);
    
    cerr << "Image: " << image_width << "x" << image_height << endl;
    cerr << "Samples: " << settings.samples_per_pixel << ", Max Depth: " << settings.max_depth << endl;
    
    cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    point3 lookfrom = settings.lookfrom;
    point3 lookat = settings.lookat;
    vec3 vup = settings.vup;
    float dist_to_focus = settings.focus_dist; // (lookfrom - lookat).length();
    float aperture = settings.aperture;

    camera cam(lookfrom, lookat, vup, settings.vfov, settings.aspect_ratio, aperture, dist_to_focus);

    vector<color> framebuffer(image_width * image_height);

    const int num_threads = thread::hardware_concurrency();
    vector<thread> threads;
    atomic<int> scanlines_remaining(image_height);
    mutex cout_mutex;

    auto render_chunk = [&](int start_j, int end_j) {
        for (int j = start_j; j < end_j; ++j) {
            {
                lock_guard<mutex> lock(cout_mutex);
                cerr << "\rScanlines remaining: " << --scanlines_remaining << ' ' << flush;
            }
            for (int i = 0; i < image_width; ++i) {
                color pixel_color(0, 0, 0);
                for (int s = 0; s < settings.samples_per_pixel; ++s) {
                    float u = (i + random_double()) / (image_width-1);
                    float v = (j + random_double()) / (image_height-1);
                    ray r = cam.get_ray(u, v);
                    pixel_color += ray_color(r, *world, settings.max_depth, settings.background);
                }
                framebuffer[(image_height-1-j) * image_width + i] = pixel_color;
            }
        }
    };

    int chunk_size = image_height / num_threads;
    for (int t = 0; t < num_threads; ++t) {
        int start_j = t * chunk_size;
        int end_j = (t == num_threads - 1) ? image_height : (t + 1) * chunk_size;
        threads.emplace_back(render_chunk, start_j, end_j);
    }

    for (auto& t : threads) {
        t.join();
    }

    for (int j = 0; j < image_height; ++j) {
        for (int i = 0; i < image_width; ++i) {
            write_color(cout, framebuffer[j * image_width + i], settings.samples_per_pixel);
        }
    }

    cerr << "\nDone.\n";
    return 0;
}
