#include <metal_stdlib>
using namespace metal;

// -----------------------------------------------------------------------------
// Random Number Generator (PCG Hash)
// -----------------------------------------------------------------------------
uint pcg_hash(uint seed) {
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float random_float(thread uint& state) {
    state = pcg_hash(state);
    return float(state) / 4294967296.0;
}

float3 random_in_unit_sphere(thread uint& state) {
    float3 p;
    do {
        p = 2.0 * float3(random_float(state), random_float(state), random_float(state)) - 1.0;
    } while (length_squared(p) >= 1.0);
    return p;
}

float3 random_unit_vector(thread uint& state) {
    return normalize(random_in_unit_sphere(state));
}

// -----------------------------------------------------------------------------
// Structures
// -----------------------------------------------------------------------------
struct Ray {
    float3 origin;
    float3 direction;
};

struct HitRecord {
    float t;
    float3 p;
    float3 normal;
    int mat_type; // 0: Lambertian, 1: Metal, 2: Dielectric
    float3 albedo;
    float fuzz;
    float ref_idx;
    bool front_face;
};

// -----------------------------------------------------------------------------
// Geometry Functions
// -----------------------------------------------------------------------------
bool hit_sphere(float3 center, float radius, Ray r, float t_min, float t_max, thread HitRecord& rec) {
    float3 oc = r.origin - center;
    float a = length_squared(r.direction);
    float half_b = dot(oc, r.direction);
    float c = length_squared(oc) - radius * radius;
    float discriminant = half_b * half_b - a * c;

    if (discriminant < 0) return false;
    float sqrtd = sqrt(discriminant);

    float root = (-half_b - sqrtd) / a;
    if (root < t_min || root > t_max) {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || root > t_max)
            return false;
    }

    rec.t = root;
    rec.p = r.origin + rec.t * r.direction;
    float3 outward_normal = (rec.p - center) / radius;
    
    rec.front_face = dot(r.direction, outward_normal) < 0;
    rec.normal = rec.front_face ? outward_normal : -outward_normal;
    
    return true;
}

bool hit_torus(float3 center, float R, float r_tube, Ray r, float t_min, float t_max, thread HitRecord& rec) {
    float3 ro = r.origin - center;
    float3 rd = r.direction;
    
    float t = t_min;
    for(int i=0; i<512; i++) {
        float3 p = ro + t * rd;
        
        float2 q = float2(length(p.xy) - R, p.z);
        float d = length(q) - r_tube;
        
        if(abs(d) < 0.0007) {
            if(t > t_max) return false;
            
            rec.t = t;
            rec.p = r.origin + t * r.direction;
            
            float len_xy = length(p.xy);
            float3 grad;
            if(len_xy < 0.0001) {
                grad = float3(0,0,1);
            } else {
                grad = float3(p.x * (1.0 - R/len_xy), p.y * (1.0 - R/len_xy), p.z);
            }
            rec.normal = normalize(grad);
            
            rec.front_face = dot(r.direction, rec.normal) < 0;
            if (!rec.front_face) rec.normal = -rec.normal;
            
            return true;
        }
        
        float step = clamp(abs(d), 0.0005, 0.3);
        t += step;
        if(t > t_max) return false;
    }
    
    return false;
}

bool hit_plane(float z, Ray r, float t_min, float t_max, thread HitRecord& rec) {
    // Plane: z = C. Normal: (0, 0, 1)
    // Ray: O + tD. Oz + t*Dz = C => t = (C - Oz) / Dz
    if (abs(r.direction.z) < 1e-6) return false;
    
    float t = (z - r.origin.z) / r.direction.z;
    if (t < t_min || t > t_max) return false;
    
    rec.t = t;
    rec.p = r.origin + t * r.direction;
    float3 outward_normal = float3(0, 0, 1);
    
    rec.front_face = dot(r.direction, outward_normal) < 0;
    rec.normal = rec.front_face ? outward_normal : -outward_normal;
    
    return true;
}

bool world_hit(Ray r, float t_min, float t_max, thread HitRecord& rec) {
    HitRecord temp_rec;
    bool hit_anything = false;
    float closest_so_far = t_max;

    // 1. Checkerboard Floor (z=0)
    if (hit_plane(0.0, r, t_min, closest_so_far, temp_rec)) {
        hit_anything = true;
        closest_so_far = temp_rec.t;
        rec = temp_rec;
        
        // Checkerboard Pattern
        float sines = sin(10.0 * rec.p.x) * sin(10.0 * rec.p.y);
        if (sines < 0) rec.albedo = float3(0.1, 0.1, 0.1); // Black
        else rec.albedo = float3(0.9, 0.9, 0.9); // White
        
        rec.mat_type = 0; // Lambertian
    }



    // 3. Wall (Checkerboard) - Vertical Wall behind sphere
    // Plane y = C. Normal (0, 1, 0)
    // Ray O+tD: Oy + tDy = C => t = (C - Oy) / Dy
    // Place wall at y = 5
    if (abs(r.direction.y) > 1e-6) {
        float t = (5.0 - r.origin.y) / r.direction.y;
        if (t > t_min && t < closest_so_far) {
            
            // Check z height to clip the wall
            float3 p_on_wall = r.origin + t * r.direction;
            if (p_on_wall.z < 2.0) { // Limit wall height to z < 2.0
                hit_anything = true;
                closest_so_far = t;
                
                rec.t = t;
                rec.p = p_on_wall;
                rec.normal = float3(0, -1, 0); // Points towards -y (towards camera at y=-6)
                
                rec.front_face = dot(r.direction, rec.normal) < 0;
                if (!rec.front_face) rec.normal = -rec.normal;
                
                // Vertical Checkerboard (XZ plane mapping)
                float sines = sin(10.0 * rec.p.x) * sin(10.0 * rec.p.z);
                if (sines < 0) rec.albedo = float3(0.1, 0.1, 0.1);
                else rec.albedo = float3(0.9, 0.9, 0.9);
                
                rec.mat_type = 0; // Lambertian
            }
        }
    }

    if (hit_torus(float3(0, 0, 0.4), 1.0, 0.4, r, t_min, closest_so_far, temp_rec)) {
        hit_anything = true;
        closest_so_far = temp_rec.t;
        rec = temp_rec;
        rec.mat_type = 2; // Dielectric
        rec.ref_idx = 1.5;
        rec.albedo = float3(1.0, 1.0, 1.0);
    }
    
    return hit_anything;
}

// -----------------------------------------------------------------------------
// Material Scatter Functions
// -----------------------------------------------------------------------------
float reflectance(float cosine, float ref_idx) {
    // Schlick's approximation
    float r0 = (1.0 - ref_idx) / (1.0 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow((1.0 - cosine), 5.0);
}

bool scatter(Ray r_in, HitRecord rec, thread float3& attenuation, thread Ray& scattered, thread uint& state) {
    if (rec.mat_type == 0) { // Lambertian
        float3 scatter_direction = rec.normal + random_unit_vector(state);
        if (length_squared(scatter_direction) < 1e-8) scatter_direction = rec.normal;
        scattered.origin = rec.p;
        scattered.direction = scatter_direction;
        attenuation = rec.albedo;
        return true;
    } else if (rec.mat_type == 1) { // Metal
        float3 reflected = reflect(normalize(r_in.direction), rec.normal);
        scattered.origin = rec.p;
        scattered.direction = reflected + rec.fuzz * random_in_unit_sphere(state);
        attenuation = rec.albedo;
        return (dot(scattered.direction, rec.normal) > 0);
    } else if (rec.mat_type == 2) { // Dielectric
        attenuation = exp(-float3(0.08, 0.08, 0.12) * rec.t);
        float refraction_ratio = rec.front_face ? (1.0 / rec.ref_idx) : rec.ref_idx;
        
        float3 unit_direction = normalize(r_in.direction);
        float cos_theta = min(dot(-unit_direction, rec.normal), 1.0);
        float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
        
        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        float3 direction;
        
        if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_float(state)) {
            // Reflect
            direction = reflect(unit_direction, rec.normal);
        } else {
            // Refract
            direction = refract(unit_direction, rec.normal, refraction_ratio);
        }
        
        scattered.origin = rec.p;
        scattered.direction = direction;
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
// Ray Color Function
// -----------------------------------------------------------------------------
float3 ray_color(Ray r, thread uint& state) {
    Ray cur_ray = r;
    float3 cur_attenuation = float3(1.0, 1.0, 1.0);
    
    for (int depth = 0; depth < 50; depth++) {
        HitRecord rec;
        if (world_hit(cur_ray, 0.001, 1000.0, rec)) {
            Ray scattered;
            float3 attenuation;
            if (scatter(cur_ray, rec, attenuation, scattered, state)) {
                cur_attenuation *= attenuation;
                cur_ray = scattered;
            } else {
                return float3(0,0,0);
            }
        } else {
            // Background (Sky)
            float3 unit_direction = normalize(cur_ray.direction);
            float t = 0.5 * (unit_direction.y + 1.0);
            float3 bg = (1.0 - t) * float3(1.0, 1.0, 1.0) + t * float3(0.5, 0.7, 1.0);
            return cur_attenuation * bg;
        }
    }
    return float3(0,0,0);
}

// -----------------------------------------------------------------------------
// Kernel
// -----------------------------------------------------------------------------
kernel void render(texture2d<float, access::write> output [[texture(0)]],
                   uint2 gid [[thread_position_in_grid]],
                   uint2 dims [[threads_per_grid]]) {
    
    if (gid.x >= dims.x || gid.y >= dims.y) return;
    
    // Seed RNG based on pixel coordinates
    uint rng_state = gid.x + gid.y * dims.x + 12345;
    
    // Camera Setup (Look at origin from z=3, y=2)
    // Move camera back to see more context
    float3 lookfrom = float3(0, -6, 2); 
    float3 lookat = float3(0, 0, 1);
    float3 vup = float3(0, 0, 1);
    float vfov = 40.0;
    float aspect_ratio = float(dims.x) / float(dims.y);
    
    float theta = vfov * M_PI_F / 180.0;
    float h = tan(theta / 2.0);
    float viewport_height = 2.0 * h;
    float viewport_width = aspect_ratio * viewport_height;
    
    float3 w = normalize(lookfrom - lookat);
    float3 u_axis = normalize(cross(vup, w));
    float3 v_axis = cross(w, u_axis);
    
    float3 horizontal = viewport_width * u_axis;
    float3 vertical = viewport_height * v_axis;
    float3 lower_left_corner = lookfrom - horizontal / 2.0 - vertical / 2.0 - w;
    
    float3 pixel_color = float3(0, 0, 0);
    int samples_per_pixel = 200;
    
    for (int s = 0; s < samples_per_pixel; s++) {
        float u_jitter = (float(gid.x) + random_float(rng_state)) / float(dims.x - 1);
        float v_jitter = (float(dims.y - 1 - gid.y) + random_float(rng_state)) / float(dims.y - 1); // Flip Y
        
        Ray r;
        r.origin = lookfrom;
        r.direction = lower_left_corner + u_jitter * horizontal + v_jitter * vertical - lookfrom;
        
        pixel_color += ray_color(r, rng_state);
    }
    
    pixel_color /= float(samples_per_pixel);
    
    // Gamma Correction (Gamma 2.0)
    pixel_color = sqrt(pixel_color);
    
    output.write(float4(pixel_color, 1.0), gid);
}
