#ifndef MATERIAL_H
#define MATERIAL_H

#include "rt_utils.h"
#include "hittable.h"

class material {
public:
    virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const = 0;
    virtual color emitted(float u, float v, const point3& p) const {
        return color(0,0,0);
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

class lambertian : public material {
public:
    lambertian(const color& a) : albedo(a), use_checker(false) {}
    lambertian(shared_ptr<checker_texture> c) : checker(c), use_checker(true) {}

    virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        auto scatter_direction = rec.normal + random_unit_vector();
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;
        scattered = ray(rec.p, scatter_direction);
        
        if (use_checker)
            attenuation = checker->value(rec.u, rec.v, rec.p);
        else
            attenuation = albedo;
        return true;
    }

public:
    color albedo;
    shared_ptr<checker_texture> checker;
    bool use_checker;
};

class metal : public material {
public:
    metal(const color& a, float f) : albedo(a), fuzz(f < 1 ? f : 1) {}

    virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere());
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }

public:
    color albedo;
    float fuzz;
};

class dielectric : public material {
public:
    dielectric(float index_of_refraction, color c = color(1,1,1)) : ir(index_of_refraction), albedo(c) {}

    virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
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
        // Use Schlick's approximation for reflectance.
        auto r0 = (1-ref_idx) / (1+ref_idx);
        r0 = r0*r0;
        return r0 + (1-r0)*pow((1 - cosine),5);
    }
};

class diffuse_light : public material {
public:
    diffuse_light(color c) : emit(c) {}

    virtual bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered) const override {
        return false;
    }

    virtual color emitted(float u, float v, const point3& p) const override {
        return emit;
    }

public:
    color emit;
};

#endif
