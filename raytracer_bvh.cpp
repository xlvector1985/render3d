#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <random>
#include <memory>
#include <limits>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <string>

using namespace std;

const float INF = numeric_limits<float>::infinity();
const float PI = acos(-1.0f);

inline float degrees_to_radians(float degrees) {
    return degrees * PI / 180.0f;
}

inline float random_double() {
    thread_local unsigned int x = 123456789;
    thread_local unsigned int y = 362436069;
    thread_local unsigned int z = 521288629;
    thread_local unsigned int w = 88675123;
    
    unsigned int t = x ^ (x << 11);
    x = y; y = z; z = w;
    w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
    return (float)w / (float)UINT_MAX;
}

inline float random_double(float min, float max) {
    return min + (max - min) * random_double();
}

inline float clamp(float x, float min_val, float max_val) {
    if (x < min_val) return min_val;
    if (x > max_val) return max_val;
    return x;
}

class vec3 {
public:
    float e[3];

    vec3() : e{0, 0, 0} {}
    vec3(float e0, float e1, float e2) : e{e0, e1, e2} {}

    float x() const { return e[0]; }
    float y() const { return e[1]; }
    float z() const { return e[2]; }

    vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
    float operator[](int i) const { return e[i]; }
    float& operator[](int i) { return e[i]; }

    vec3& operator+=(const vec3 &v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    vec3& operator*=(float t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    vec3& operator/=(float t) {
        return *this *= 1/t;
    }

    float length() const {
        return sqrt(length_squared());
    }

    float length_squared() const {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }

    bool near_zero() const {
        const float s = 1e-8f;
        return (fabs(e[0]) < s) && (fabs(e[1]) < s) && (fabs(e[2]) < s);
    }

    static vec3 random() {
        return vec3(random_double(), random_double(), random_double());
    }

    static vec3 random(float min, float max) {
        return vec3(random_double(min, max), random_double(min, max), random_double(min, max));
    }
};

using point3 = vec3;
using color = vec3;

inline vec3 operator+(const vec3 &u, const vec3 &v) {
    return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

inline vec3 operator-(const vec3 &u, const vec3 &v) {
    return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

inline vec3 operator*(const vec3 &u, const vec3 &v) {
    return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

inline vec3 operator*(float t, const vec3 &v) {
    return vec3(t*v.e[0], t*v.e[1], t*v.e[2]);
}

inline vec3 operator*(const vec3 &v, float t) {
    return t * v;
}

inline vec3 operator/(const vec3 &v, float t) {
    return (1/t) * v;
}

inline float dot(const vec3 &u, const vec3 &v) {
    return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}

inline vec3 cross(const vec3 &u, const vec3 &v) {
    return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                u.e[2] * v.e[0] - u.e[0] * v.e[2],
                u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

inline vec3 unit_vector(const vec3 &v) {
    return v / v.length();
}

inline vec3 random_in_unit_sphere() {
    while (true) {
        vec3 p = vec3::random(-1, 1);
        if (p.length_squared() < 1)
            return p;
    }
}

inline vec3 random_unit_vector() {
    return unit_vector(random_in_unit_sphere());
}

inline vec3 random_in_unit_disk() {
    while (true) {
        vec3 p = vec3(random_double(-1, 1), random_double(-1, 1), 0);
        if (p.length_squared() < 1)
            return p;
    }
}

inline vec3 reflect(const vec3 &v, const vec3 &n) {
    return v - 2*dot(v, n)*n;
}

inline vec3 refract(const vec3 &uv, const vec3 &n, float etai_over_etat) {
    float cos_theta = fmin(dot(-uv, n), 1.0f);
    vec3 r_out_perp = etai_over_etat * (uv + cos_theta*n);
    vec3 r_out_parallel = -sqrt(fabs(1.0f - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}

class ray {
public:
    ray() {}
    ray(const point3& origin, const vec3& direction)
        : orig(origin), dir(direction)
    {}

    point3 origin() const { return orig; }
    vec3 direction() const { return dir; }

    point3 at(float t) const {
        return orig + t*dir;
    }

private:
    point3 orig;
    vec3 dir;
};

class material;

struct hit_record {
    point3 p;
    vec3 normal;
    shared_ptr<material> mat_ptr;
    float t;
    float u, v;
    bool front_face;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class aabb {
public:
    aabb() {}
    aabb(const point3& a, const point3& b) { minimum = a; maximum = b; }

    point3 min() const { return minimum; }
    point3 max() const { return maximum; }

    bool hit(const ray& r, float t_min, float t_max) const {
        float inv_d0 = 1.0f / r.direction().x();
        float t0 = (minimum.x() - r.origin().x()) * inv_d0;
        float t1 = (maximum.x() - r.origin().x()) * inv_d0;
        if (inv_d0 < 0.0f) {
            float temp = t0;
            t0 = t1;
            t1 = temp;
        }
        t_min = t0 > t_min ? t0 : t_min;
        t_max = t1 < t_max ? t1 : t_max;
        if (t_max <= t_min)
            return false;

        float inv_d1 = 1.0f / r.direction().y();
        t0 = (minimum.y() - r.origin().y()) * inv_d1;
        t1 = (maximum.y() - r.origin().y()) * inv_d1;
        if (inv_d1 < 0.0f) {
            float temp = t0;
            t0 = t1;
            t1 = temp;
        }
        t_min = t0 > t_min ? t0 : t_min;
        t_max = t1 < t_max ? t1 : t_max;
        if (t_max <= t_min)
            return false;

        float inv_d2 = 1.0f / r.direction().z();
        t0 = (minimum.z() - r.origin().z()) * inv_d2;
        t1 = (maximum.z() - r.origin().z()) * inv_d2;
        if (inv_d2 < 0.0f) {
            float temp = t0;
            t0 = t1;
            t1 = temp;
        }
        t_min = t0 > t_min ? t0 : t_min;
        t_max = t1 < t_max ? t1 : t_max;
        if (t_max <= t_min)
            return false;

        return true;
    }

    point3 minimum;
    point3 maximum;
};

inline aabb surrounding_box(aabb box0, aabb box1) {
    point3 small(
        fmin(box0.min().x(), box1.min().x()),
        fmin(box0.min().y(), box1.min().y()),
        fmin(box0.min().z(), box1.min().z())
    );

    point3 big(
        fmax(box0.max().x(), box1.max().x()),
        fmax(box0.max().y(), box1.max().y()),
        fmax(box0.max().z(), box1.max().z())
    );

    return aabb(small, big);
}

class hittable {
public:
    virtual ~hittable() = default;
    virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const = 0;
    virtual bool bounding_box(float time0, float time1, aabb& output_box) const = 0;
};

class hittable_list : public hittable {
public:
    vector<shared_ptr<hittable>> objects;

    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }
    void add(shared_ptr<hittable> object) { objects.push_back(object); }

    bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
        hit_record temp_rec;
        bool hit_anything = false;
        float closest_so_far = t_max;

        for (const auto& object : objects) {
            if (object->hit(r, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        return hit_anything;
    }

    bool bounding_box(float time0, float time1, aabb& output_box) const override {
        if (objects.empty()) return false;

        aabb temp_box;
        bool first_box = true;

        for (const auto& object : objects) {
            if (!object->bounding_box(time0, time1, temp_box)) return false;
            output_box = first_box ? temp_box : surrounding_box(output_box, temp_box);
            first_box = false;
        }

        return true;
    }
};

class sphere : public hittable {
public:
    sphere() {}
    sphere(point3 cen, float r, shared_ptr<material> m)
        : center(cen), radius(r), mat_ptr(m) {}

    bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
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

    bool bounding_box(float time0, float time1, aabb& output_box) const override {
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

    bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
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

    bool bounding_box(float time0, float time1, aabb& output_box) const override {
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

class cylinder : public hittable {
public:
    cylinder() {}
    cylinder(point3 cen, float r, float h, shared_ptr<material> m)
        : center(cen), radius(r), height(h), mat_ptr(m) {}

    bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
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

    bool bounding_box(float time0, float time1, aabb& output_box) const override {
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

    bool hit(const ray& r_in, float t_min, float t_max, hit_record& rec) const override {
        // Transform ray to object space
        vec3 origin = r_in.origin() - center;
        origin = rotate_inverse(origin);
        vec3 direction = rotate_inverse(r_in.direction());
        
        // Use local origin and direction for intersection test
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
            
            // Allow a small epsilon for intersection tests
            if (t < t_min - 1e-5 || t > closest_so_far)
                continue;
            
            // If slightly less than t_min but within epsilon, clamp it
            if (t < t_min) t = t_min;
            
            // Intersection point in object space
            vec3 p_local = ro + t * rd;
            
            vec3 p_xy = vec3(p_local.x(), p_local.y(), 0);
            float len_xy = p_xy.length();
            if (len_xy < 1e-6)
                continue;
            
            vec3 center_tube = (p_xy / len_xy) * R;
            vec3 normal_local = p_local - center_tube;
            normal_local = unit_vector(normal_local);
            
            // Transform intersection point and normal back to world space
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

    bool bounding_box(float time0, float time1, aabb& output_box) const override {
        // A simple bounding box that covers the torus in any orientation
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
        // Rotate around X
        float y = v.y() * cos_x - v.z() * sin_x;
        float z = v.y() * sin_x + v.z() * cos_x;
        v = vec3(v.x(), y, z);
        
        // Rotate around Y
        float x = v.x() * cos_y + v.z() * sin_y;
        z = -v.x() * sin_y + v.z() * cos_y;
        v = vec3(x, v.y(), z);
        
        // Rotate around Z
        x = v.x() * cos_z - v.y() * sin_z;
        y = v.x() * sin_z + v.y() * cos_z;
        return vec3(x, y, v.z());
    }

    vec3 rotate_inverse(vec3 v) const {
        // Inverse rotation order: Z -> Y -> X, and negative angles
        
        // Inverse Z
        float x = v.x() * cos_z + v.y() * sin_z;
        float y = -v.x() * sin_z + v.y() * cos_z;
        v = vec3(x, y, v.z());
        
        // Inverse Y
        x = v.x() * cos_y - v.z() * sin_y;
        float z = v.x() * sin_y + v.z() * cos_y;
        v = vec3(x, v.y(), z);
        
        // Inverse X
        y = v.y() * cos_x + v.z() * sin_x;
        z = -v.y() * sin_x + v.z() * cos_x;
        return vec3(v.x(), y, z);
    }

    static int solve_quadratic(double coeffs[3], vector<double>& roots) {
        double a = coeffs[2];
        double b = coeffs[1];
        double c = coeffs[0];
        
        double discriminant = b*b - 4*a*c;
        if (discriminant < 0)
            return 0;
        
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

        // Depressed quartic: y^4 + Py^2 + Qy + R = 0
        // Substitution: x = y - b/4
        double b2 = b * b;
        double P = c - 0.375 * b2;
        double Q = d + 0.125 * b2 * b - 0.5 * b * c;
        double R = e - 0.01171875 * b2 * b2 + 0.0625 * b2 * c - 0.25 * b * d;

        if (fabs(Q) < 1e-9) {
            // Biquadratic case: y^4 + Py^2 + R = 0
            double quad_coeffs[3];
            quad_coeffs[2] = 1;
            quad_coeffs[1] = P;
            quad_coeffs[0] = R;
            
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
            // Descartes method
            // Resolvent cubic: u^3 + 2Pu^2 + (P^2 - 4R)u - Q^2 = 0
            double cubic_coeffs[4];
            cubic_coeffs[3] = 1;
            cubic_coeffs[2] = 2 * P;
            cubic_coeffs[1] = P * P - 4 * R;
            cubic_coeffs[0] = -Q * Q;

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
                // Try to relax the condition slightly
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
            if (k < 1e-9) k = 1e-9; // Avoid division by zero
            
            double m = (P + u - Q/k) / 2.0;
            double n = (P + u + Q/k) / 2.0;

            double quad1[3] = {m, k, 1}; // y^2 + ky + m = 0
            double quad2[3] = {n, -k, 1}; // y^2 - ky + n = 0

            vector<double> q1_roots, q2_roots;
            solve_quadratic(quad1, q1_roots);
            solve_quadratic(quad2, q2_roots);

            for (double y : q1_roots) roots.push_back((float)(y - b/4.0));
            for (double y : q2_roots) roots.push_back((float)(y - b/4.0));
        }
        
        // Newton-Raphson Refinement
        // Original equation: f(t) = at^4 + bt^3 + ct^2 + dt + e = 0
        // f'(t) = 4at^3 + 3bt^2 + 2ct + d
        double A = coeffs[4];
        double B = coeffs[3];
        double C = coeffs[2];
        double D = coeffs[1];
        double E = coeffs[0];

        for (float& r : roots) {
            double t = (double)r;
            // Perform 2 iterations of Newton's method
            for (int i = 0; i < 2; i++) {
                double f = (((A*t + B)*t + C)*t + D)*t + E;
                double df = ((4.0*A*t + 3.0*B)*t + 2.0*C)*t + D;
                
                if (std::abs(df) < 1e-8) break; // Avoid division by zero
                double step = f / df;
                t -= step;
                if (std::abs(step) < 1e-6) break; // Convergence check
            }
            r = (float)t;
        }
        
        return (int)roots.size();
    }
    
    static int solve_cubic(float coeffs[4], vector<float>& roots) {
        float a = coeffs[3];
        float b = coeffs[2] / a;
        float c = coeffs[1] / a;
        float d = coeffs[0] / a;
        
        float Q = (3*c - b*b) / 9;
        float R = (9*b*c - 27*d - 2*b*b*b) / 54;
        float D = Q*Q*Q + R*R;
        
        if (D >= 0) {
            float S = cbrt(R + sqrt(D));
            float T = cbrt(R - sqrt(D));
            roots.push_back(S + T - b/3);
            return 1;
        } else {
            float argument = R / sqrt(-Q*Q*Q);
            if (argument < -1.0) argument = -1.0;
            if (argument > 1.0) argument = 1.0;
            float theta = acos(argument);
            roots.push_back(2 * sqrt(-Q) * cos(theta/3) - b/3);
            roots.push_back(2 * sqrt(-Q) * cos((theta + 2*PI)/3) - b/3);
            roots.push_back(2 * sqrt(-Q) * cos((theta + 4*PI)/3) - b/3);
            return 3;
        }
    }
    
    static int solve_quadratic(float coeffs[3], vector<float>& roots) {
        float a = coeffs[2];
        float b = coeffs[1];
        float c = coeffs[0];
        
        float discriminant = b*b - 4*a*c;
        if (discriminant < 0)
            return 0;
        
        float sqrt_d = sqrt(discriminant);
        roots.push_back((-b - sqrt_d) / (2*a));
        roots.push_back((-b + sqrt_d) / (2*a));
        return 2;
    }
    
    static float cbrt(float x) {
        if (x >= 0)
            return pow(x, 1.0f/3.0f);
        else
            return -pow(-x, 1.0f/3.0f);
    }
};

class yz_rect : public hittable {
public:
    yz_rect() {}
    yz_rect(float y0, float y1, float z0, float z1, float k, shared_ptr<material> mat)
        : y0(y0), y1(y1), z0(z0), z1(z1), k(k), mp(mat) {}

    bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
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

    bool bounding_box(float time0, float time1, aabb& output_box) const override {
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

class xz_rect : public hittable {
public:
    xz_rect() {}
    xz_rect(float x0, float x1, float z0, float z1, float k, shared_ptr<material> mat)
        : x0(x0), x1(x1), z0(z0), z1(z1), k(k), mp(mat) {}

    bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
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

    bool bounding_box(float time0, float time1, aabb& output_box) const override {
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

class bvh_node : public hittable {
public:
    bvh_node() {}

    bvh_node(const hittable_list& list, float time0, float time1)
        : bvh_node(list.objects, 0, list.objects.size(), time0, time1) {}

    bvh_node(
        const vector<shared_ptr<hittable>>& src_objects,
        size_t start, size_t end, float time0, float time1
    ) {
        auto objects = src_objects;

        int axis = static_cast<int>(3 * random_double());
        auto comparator = (axis == 0) ? box_x_compare
                        : (axis == 1) ? box_y_compare
                                      : box_z_compare;

        size_t object_span = end - start;

        if (object_span == 1) {
            left = right = objects[start];
        } else if (object_span == 2) {
            if (comparator(objects[start], objects[start+1])) {
                left = objects[start];
                right = objects[start+1];
            } else {
                left = objects[start+1];
                right = objects[start];
            }
        } else {
            sort(objects.begin() + start, objects.begin() + end, comparator);

            auto mid = start + object_span/2;
            left = make_shared<bvh_node>(objects, start, mid, time0, time1);
            right = make_shared<bvh_node>(objects, mid, end, time0, time1);
        }

        aabb box_left, box_right;

        if (!left->bounding_box(time0, time1, box_left)
            || !right->bounding_box(time0, time1, box_right))
            cerr << "No bounding box in bvh_node constructor.\n";

        box = surrounding_box(box_left, box_right);
    }

    bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const override {
        if (!box.hit(r, t_min, t_max))
            return false;

        bool hit_left = left->hit(r, t_min, t_max, rec);
        bool hit_right = right->hit(r, t_min, hit_left ? rec.t : t_max, rec);

        return hit_left || hit_right;
    }

    bool bounding_box(float time0, float time1, aabb& output_box) const override {
        output_box = box;
        return true;
    }

private:
    shared_ptr<hittable> left;
    shared_ptr<hittable> right;
    aabb box;

    static bool box_compare(
        const shared_ptr<hittable>& a, const shared_ptr<hittable>& b, int axis
    ) {
        aabb box_a, box_b;

        if (!a->bounding_box(0,0, box_a) || !b->bounding_box(0,0, box_b))
            cerr << "No bounding box in bvh_node constructor.\n";

        return box_a.min().e[axis] < box_b.min().e[axis];
    }

    static bool box_x_compare (const shared_ptr<hittable>& a, const shared_ptr<hittable>& b) {
        return box_compare(a, b, 0);
    }

    static bool box_y_compare (const shared_ptr<hittable>& a, const shared_ptr<hittable>& b) {
        return box_compare(a, b, 1);
    }

    static bool box_z_compare (const shared_ptr<hittable>& a, const shared_ptr<hittable>& b) {
        return box_compare(a, b, 2);
    }
};




class checker_texture {
public:
    enum class plane_type { XY, YZ, XZ };
    
    checker_texture() {}
    checker_texture(color c1, color c2, plane_type type = plane_type::XY) 
        : even(c1), odd(c2), plane(type) {}

    color value(float u, float v, const point3& p) const {
        float sines;
        switch (plane) {
            case plane_type::XY:
                sines = sin(10*p.x()) * sin(10*p.y());
                break;
            case plane_type::YZ:
                sines = sin(10*p.y()) * sin(10*p.z());
                break;
            case plane_type::XZ:
            default:
                sines = sin(10*p.x()) * sin(10*p.z());
                break;
        }
        if (sines < 0)
            return odd;
        else
            return even;
    }

private:
    color even;
    color odd;
    plane_type plane;
};

class material {
public:
    virtual ~material() = default;
    virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const = 0;
    virtual color emitted(float u, float v, const point3& p) const {
        return color(0, 0, 0);
    }
};

class diffuse_light : public material {
public:
    diffuse_light(color c) : emit(c) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        return false;
    }

    color emitted(float u, float v, const point3& p) const override {
        return emit;
    }

private:
    color emit;
};

class lambertian : public material {
public:
    lambertian(const color& a) : albedo(a), use_checker(false) {}
    lambertian(shared_ptr<checker_texture> c) : checker(c), use_checker(true) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        vec3 scatter_direction = rec.normal + random_unit_vector();
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;
        scattered = ray(rec.p, scatter_direction);
        if (use_checker)
            attenuation = checker->value(rec.u, rec.v, rec.p);
        else
            attenuation = albedo;
        return true;
    }

private:
    color albedo;
    shared_ptr<checker_texture> checker;
    bool use_checker;
};

class metal : public material {
public:
    metal(const color& a, float f) : albedo(a), fuzz(f < 1 ? f : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere());
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }

private:
    color albedo;
    float fuzz;
};

class dielectric : public material {
public:
    dielectric(float index_of_refraction, color c = color(1,1,1)) : ir(index_of_refraction), albedo(c) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        attenuation = albedo;
        float refraction_ratio = rec.front_face ? (1.0/ir) : ir;

        vec3 unit_direction = unit_vector(r_in.direction());
        float cos_theta = fmin(dot(-unit_direction, rec.normal), 1.0);
        float sin_theta = sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, refraction_ratio);

        scattered = ray(rec.p, direction);
        return true;
    }

private:
    float ir;
    color albedo;

    static float reflectance(float cosine, float ref_idx) {
        float r0 = (1 - ref_idx) / (1 + ref_idx);
        r0 = r0 * r0;
        return r0 + (1 - r0) * pow((1 - cosine), 5);
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

color ray_color(const ray& r, const hittable& world, int depth) {
    hit_record rec;

    if (depth <= 0)
        return color(0, 0, 0);

    if (world.hit(r, 0.001, INF, rec)) {
        ray scattered;
        color attenuation;
        color emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
        if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            return emitted + attenuation * ray_color(scattered, world, depth-1);
        return emitted;
    }

    vec3 unit_direction = unit_vector(r.direction());
    float t = 0.5*(unit_direction.z() + 1.0);
    return (1.0-t)*color(1.0, 1.0, 1.0) + t*color(0.5, 0.7, 1.0);
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

shared_ptr<hittable> chess_scene() {
    hittable_list world;

    auto floor_checker = make_shared<checker_texture>(color(0.05, 0.05, 0.05), color(0.95, 0.95, 0.95), checker_texture::plane_type::XY);
    auto floor_material = make_shared<lambertian>(floor_checker);
    world.add(make_shared<xy_rect>(-5, 5, -5, 5, 0, floor_material));

    auto wall_checker_yz = make_shared<checker_texture>(color(0.05, 0.05, 0.05), color(0.95, 0.95, 0.95), checker_texture::plane_type::YZ);
    auto wall_material_yz = make_shared<lambertian>(wall_checker_yz);
    world.add(make_shared<yz_rect>(-5, 5, 0, 3, -5, wall_material_yz));
    world.add(make_shared<yz_rect>(-5, 5, 0, 3, 5, wall_material_yz));

    auto wall_checker_xz = make_shared<checker_texture>(color(0.05, 0.05, 0.05), color(0.95, 0.95, 0.95), checker_texture::plane_type::XZ);
    auto wall_material_xz = make_shared<lambertian>(wall_checker_xz);
    world.add(make_shared<xz_rect>(-5, 5, 0, 3, 5, wall_material_xz));

    auto glass_material = make_shared<dielectric>(1.5);
    auto metal_material = make_shared<metal>(color(0.8, 0.8, 0.8), 0.0);
    auto red_material = make_shared<lambertian>(color(0.7, 0.2, 0.1));
    auto blue_material = make_shared<lambertian>(color(0.1, 0.2, 0.7));
    auto yellow_material = make_shared<lambertian>(color(0.8, 0.6, 0.2));
    auto green_material = make_shared<lambertian>(color(0.1, 0.6, 0.2));
    auto purple_material = make_shared<lambertian>(color(0.6, 0.1, 0.6));

    for (int i = 0; i < 50; i++) {
        float x = random_double(-3, 3);
        float y = random_double(-2, 3);
        float r = random_double(0.05, 0.15);
        
        if (random_double() < 0.5) {
            world.add(make_shared<sphere>(point3(x, y, r), r, glass_material));
        } else {
            world.add(make_shared<sphere>(point3(x, y, r), r, metal_material));
        }
    }

    world.add(make_shared<sphere>(point3(-1.2, 2.5, 0.4), 0.4, glass_material));
    world.add(make_shared<sphere>(point3(-0.6, 3.0, 0.12), 0.12, glass_material));
    world.add(make_shared<sphere>(point3(0.2, 2.8, 0.28), 0.28, glass_material));
    world.add(make_shared<sphere>(point3(0.8, 3.2, 0.15), 0.15, glass_material));
    world.add(make_shared<sphere>(point3(1.5, 2.7, 0.35), 0.35, glass_material));
    world.add(make_shared<sphere>(point3(-1.5, 3.5, 0.18), 0.18, glass_material));
    world.add(make_shared<sphere>(point3(-0.8, 3.8, 0.45), 0.45, glass_material));
    world.add(make_shared<sphere>(point3(1.0, 3.6, 0.32), 0.32, glass_material));

    world.add(make_shared<sphere>(point3(-2.5, 2.0, 0.35), 0.35, metal_material));
    world.add(make_shared<sphere>(point3(2.5, 2.2, 0.3), 0.3, metal_material));

    world.add(make_shared<cylinder>(point3(-3.2, 3.5, 0.375), 0.4, 0.75, metal_material));

    world.add(make_shared<sphere>(point3(-4.0, 2.0, 0.3), 0.3, red_material));
    world.add(make_shared<sphere>(point3(4.0, 2.2, 0.3), 0.3, blue_material));
    world.add(make_shared<sphere>(point3(0, 1.5, 0.35), 0.35, metal_material));
    world.add(make_shared<sphere>(point3(-4.0, 3.5, 0.28), 0.28, yellow_material));
    world.add(make_shared<sphere>(point3(4.0, 3.8, 0.28), 0.28, green_material));
    world.add(make_shared<sphere>(point3(0, 4.2, 0.32), 0.32, purple_material));

    return make_shared<bvh_node>(world, 0, 1);
}

shared_ptr<hittable> load_scene_from_file(const string& filename) {
    hittable_list world;
    
    ifstream file(filename);
    if (!file) {
        cerr << "Error: Could not open scene file: " << filename << endl;
        return chess_scene();
    }
    
    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string type;
        iss >> type;
        
        if (type.empty() || type[0] == '#')
            continue;
        
        if (type == "sphere") {
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
    const float aspect_ratio = 16.0 / 9.0;
    const int image_width = 1920;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    int samples_per_pixel = 150;
    int max_depth = 50;
    string scene_file = "";
    
    if (argc > 1) {
        samples_per_pixel = stoi(argv[1]);
    }
    if (argc > 2) {
        max_depth = stoi(argv[2]);
    }
    if (argc > 3) {
        scene_file = argv[3];
    }
    
    cerr << "Using " << samples_per_pixel << " samples per pixel, max depth " << max_depth;
    if (!scene_file.empty()) {
        cerr << ", scene file: " << scene_file;
    }
    cerr << endl;

    cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    shared_ptr<hittable> world;
    if (!scene_file.empty()) {
        world = load_scene_from_file(scene_file);
    } else {
        world = chess_scene();
    }

    point3 lookfrom(0, -6, 2.5);
    point3 lookat(0, 3.8, 0.5);
    vec3 vup(0, 0, 1);
    float dist_to_focus = (lookfrom - lookat).length();
    float aperture = 0.03;

    camera cam(lookfrom, lookat, vup, 40, aspect_ratio, aperture, dist_to_focus);

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
                for (int s = 0; s < samples_per_pixel; ++s) {
                    float u = (i + random_double()) / (image_width-1);
                    float v = (j + random_double()) / (image_height-1);
                    ray r = cam.get_ray(u, v);
                    pixel_color += ray_color(r, *world, max_depth);
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
            write_color(cout, framebuffer[j * image_width + i], samples_per_pixel);
        }
    }

    cerr << "\nDone.\n";
}
