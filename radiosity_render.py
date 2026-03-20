import math
import os
from PIL import Image

def vec_add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])

def vec_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])

def vec_mul(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)

def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]

def length(a):
    return math.sqrt(dot(a, a))

def normalize(a):
    l = length(a)
    if l == 0:
        return (0.0, 0.0, 0.0)
    return (a[0] / l, a[1] / l, a[2] / l)

def clamp01(x):
    return 0.0 if x < 0.0 else (1.0 if x > 1.0 else x)

def checker_color(x, y, size):
    s = int(math.floor(x / size) + math.floor(y / size))
    return (0.1, 0.1, 0.1) if (s % 2 == 0) else (0.9, 0.9, 0.9)

def build_patches():
    patches = []
    groups = {}

    checker_size = 1.0
    floor_nx = 64
    floor_ny = 64
    x0, x1 = -4.0, 4.0
    y0, y1 = -4.0, 4.0
    dx = (x1 - x0) / floor_nx
    dy = (y1 - y0) / floor_ny
    for iy in range(floor_ny):
        for ix in range(floor_nx):
            cx = x0 + (ix + 0.5) * dx
            cy = y0 + (iy + 0.5) * dy
            cz = 0.0
            col = checker_color(cx, cy, checker_size)
            patches.append({
                "c": (cx, cy, cz),
                "n": (0.0, 0.0, 1.0),
                "a": dx * dy,
                "rho": col,
                "e": (0.0, 0.0, 0.0)
            })
    groups["floor"] = (0, len(patches), floor_nx, floor_ny, x0, y0, dx, dy)

    back_nx = 64
    back_nz = 32
    x0, x1 = -4.0, 4.0
    z0, z1 = 0.0, 4.0
    dx = (x1 - x0) / back_nx
    dz = (z1 - z0) / back_nz
    start = len(patches)
    for iz in range(back_nz):
        for ix in range(back_nx):
            cx = x0 + (ix + 0.5) * dx
            cy = 4.0
            cz = z0 + (iz + 0.5) * dz
            col = checker_color(cx, cz, checker_size)
            patches.append({
                "c": (cx, cy, cz),
                "n": (0.0, -1.0, 0.0),
                "a": dx * dz,
                "rho": col,
                "e": (0.0, 0.0, 0.0)
            })
    groups["back"] = (start, len(patches), back_nx, back_nz, x0, z0, dx, dz)

    side_ny = 64
    side_nz = 32
    y0, y1 = -4.0, 4.0
    z0, z1 = 0.0, 4.0
    dy = (y1 - y0) / side_ny
    dz = (z1 - z0) / side_nz
    start = len(patches)
    for iz in range(side_nz):
        for iy in range(side_ny):
            cx = -4.0
            cy = y0 + (iy + 0.5) * dy
            cz = z0 + (iz + 0.5) * dz
            col = checker_color(cy, cz, checker_size)
            patches.append({
                "c": (cx, cy, cz),
                "n": (1.0, 0.0, 0.0),
                "a": dy * dz,
                "rho": col,
                "e": (0.0, 0.0, 0.0)
            })
    groups["left"] = (start, len(patches), side_ny, side_nz, y0, z0, dy, dz)

    start = len(patches)
    for iz in range(side_nz):
        for iy in range(side_ny):
            cx = 4.0
            cy = y0 + (iy + 0.5) * dy
            cz = z0 + (iz + 0.5) * dz
            col = checker_color(cy, cz, checker_size)
            patches.append({
                "c": (cx, cy, cz),
                "n": (-1.0, 0.0, 0.0),
                "a": dy * dz,
                "rho": col,
                "e": (0.0, 0.0, 0.0)
            })
    groups["right"] = (start, len(patches), side_ny, side_nz, y0, z0, dy, dz)

    light_nx = 16
    light_ny = 8
    x0, x1 = -1.5, 1.5
    y0, y1 = -1.0, 1.0
    dx = (x1 - x0) / light_nx
    dy = (y1 - y0) / light_ny
    start = len(patches)
    for iy in range(light_ny):
        for ix in range(light_nx):
            cx = x0 + (ix + 0.5) * dx
            cy = y0 + (iy + 0.5) * dy
            cz = 4.0
            patches.append({
                "c": (cx, cy, cz),
                "n": (0.0, 0.0, -1.0),
                "a": dx * dy,
                "rho": (0.0, 0.0, 0.0),
                "e": (8.0, 8.0, 8.0)
            })
    groups["light"] = (start, len(patches), light_nx, light_ny, x0, y0, dx, dy)

    sph_u = 96
    sph_v = 48
    r = 1.0
    cx, cy, cz = 0.0, 0.0, 1.0
    start = len(patches)
    for v in range(sph_v):
        for u in range(sph_u):
            theta0 = (u / sph_u) * 2.0 * math.pi
            theta1 = ((u + 1) / sph_u) * 2.0 * math.pi
            phi0 = (v / sph_v) * math.pi
            phi1 = ((v + 1) / sph_v) * math.pi
            theta = (theta0 + theta1) * 0.5
            phi = (phi0 + phi1) * 0.5
            nx = math.sin(phi) * math.cos(theta)
            ny = math.sin(phi) * math.sin(theta)
            nz = math.cos(phi)
            px = cx + r * nx
            py = cy + r * ny
            pz = cz + r * nz
            area = (2.0 * math.pi * r * r / sph_u) * (math.sin(phi1) - math.sin(phi0))
            patches.append({
                "c": (px, py, pz),
                "n": (nx, ny, nz),
                "a": abs(area),
                "rho": (0.7, 0.7, 0.75),
                "e": (0.0, 0.0, 0.0)
            })
    groups["sphere"] = (start, len(patches), sph_u, sph_v)

    cyl_theta = 48
    cyl_z = 24
    cyl_r = 0.4
    cyl_h = 1.4
    cyl_cx, cyl_cy, cyl_cz = 2.0, 0.0, cyl_h * 0.5
    start = len(patches)
    for iz in range(cyl_z):
        z0 = -cyl_h * 0.5 + (iz + 0.5) * (cyl_h / cyl_z)
        for it in range(cyl_theta):
            theta = (it + 0.5) / cyl_theta * 2.0 * math.pi
            nx = math.cos(theta)
            ny = math.sin(theta)
            px = cyl_cx + cyl_r * nx
            py = cyl_cy + cyl_r * ny
            pz = cyl_cz + z0
            area = (2.0 * math.pi * cyl_r / cyl_theta) * (cyl_h / cyl_z)
            patches.append({
                "c": (px, py, pz),
                "n": (nx, ny, 0.0),
                "a": area,
                "rho": (0.1, 0.8, 0.2),
                "e": (0.0, 0.0, 0.0)
            })
    side_end = len(patches)
    cap_n = 16
    cap_dr = cyl_r / cap_n
    cap_area = math.pi * cyl_r * cyl_r
    cap_patch_area = cap_area / (cap_n * cap_n)
    cap_start = len(patches)
    for iz in range(cap_n):
        r0 = (iz + 0.5) * cap_dr
        ring_count = max(6, int(cyl_theta * (r0 / cyl_r)))
        for it in range(ring_count):
            theta = (it + 0.5) / ring_count * 2.0 * math.pi
            px = cyl_cx + r0 * math.cos(theta)
            py = cyl_cy + r0 * math.sin(theta)
            pz = cyl_cz + cyl_h * 0.5
            patches.append({
                "c": (px, py, pz),
                "n": (0.0, 0.0, 1.0),
                "a": cap_patch_area,
                "rho": (0.1, 0.8, 0.2),
                "e": (0.0, 0.0, 0.0)
            })
    cap_top_end = len(patches)
    for iz in range(cap_n):
        r0 = (iz + 0.5) * cap_dr
        ring_count = max(6, int(cyl_theta * (r0 / cyl_r)))
        for it in range(ring_count):
            theta = (it + 0.5) / ring_count * 2.0 * math.pi
            px = cyl_cx + r0 * math.cos(theta)
            py = cyl_cy + r0 * math.sin(theta)
            pz = cyl_cz - cyl_h * 0.5
            patches.append({
                "c": (px, py, pz),
                "n": (0.0, 0.0, -1.0),
                "a": cap_patch_area,
                "rho": (0.1, 0.8, 0.2),
                "e": (0.0, 0.0, 0.0)
            })
    groups["cylinder"] = (start, side_end, cyl_theta, cyl_z, cyl_cx, cyl_cy, cyl_cz, cyl_r, cyl_h)
    groups["cylinder_top"] = (cap_start, cap_top_end, cyl_cx, cyl_cy, cyl_cz + cyl_h * 0.5, cyl_r)
    groups["cylinder_bottom"] = (cap_top_end, len(patches), cyl_cx, cyl_cy, cyl_cz - cyl_h * 0.5, cyl_r)

    return patches, groups

def compute_form_factors(patches):
    n = len(patches)
    F = [[0.0] * n for _ in range(n)]
    for i in range(n):
        ci = patches[i]["c"]
        ni = patches[i]["n"]
        for j in range(n):
            if i == j:
                continue
            cj = patches[j]["c"]
            nj = patches[j]["n"]
            rij = vec_sub(cj, ci)
            r2 = dot(rij, rij)
            if r2 == 0:
                continue
            r = math.sqrt(r2)
            cos_i = dot(ni, rij) / r
            cos_j = -dot(nj, rij) / r
            if cos_i <= 0.0 or cos_j <= 0.0:
                continue
            F[i][j] = (cos_i * cos_j * patches[j]["a"]) / (math.pi * r2)
        row_sum = sum(F[i])
        if row_sum > 1.0:
            inv = 1.0 / row_sum
            for j in range(n):
                F[i][j] *= inv
    return F

def solve_radiosity(patches, F, iterations=45):
    n = len(patches)
    B = [patch["e"] for patch in patches]
    for _ in range(iterations):
        for i in range(n):
            acc = (0.0, 0.0, 0.0)
            for j in range(n):
                if F[i][j] == 0.0:
                    continue
                bj = B[j]
                f = F[i][j]
                acc = (acc[0] + bj[0] * f, acc[1] + bj[1] * f, acc[2] + bj[2] * f)
            rho = patches[i]["rho"]
            ei = patches[i]["e"]
            B[i] = (ei[0] + rho[0] * acc[0], ei[1] + rho[1] * acc[1], ei[2] + rho[2] * acc[2])
    return B

def intersect_scene(ray_o, ray_d):
    hits = []
    if abs(ray_d[2]) > 1e-6:
        t = -ray_o[2] / ray_d[2]
        if t > 0.001:
            p = vec_add(ray_o, vec_mul(ray_d, t))
            if -4.0 <= p[0] <= 4.0 and -4.0 <= p[1] <= 4.0:
                hits.append(("floor", t, p))
    if abs(ray_d[1]) > 1e-6:
        t = (4.0 - ray_o[1]) / ray_d[1]
        if t > 0.001:
            p = vec_add(ray_o, vec_mul(ray_d, t))
            if -4.0 <= p[0] <= 4.0 and 0.0 <= p[2] <= 4.0:
                hits.append(("back", t, p))
    if abs(ray_d[0]) > 1e-6:
        t = (-4.0 - ray_o[0]) / ray_d[0]
        if t > 0.001:
            p = vec_add(ray_o, vec_mul(ray_d, t))
            if -4.0 <= p[1] <= 4.0 and 0.0 <= p[2] <= 4.0:
                hits.append(("left", t, p))
        t = (4.0 - ray_o[0]) / ray_d[0]
        if t > 0.001:
            p = vec_add(ray_o, vec_mul(ray_d, t))
            if -4.0 <= p[1] <= 4.0 and 0.0 <= p[2] <= 4.0:
                hits.append(("right", t, p))
    oc = vec_sub(ray_o, (0.0, 0.0, 1.0))
    a = dot(ray_d, ray_d)
    b = 2.0 * dot(oc, ray_d)
    c = dot(oc, oc) - 1.0
    disc = b * b - 4.0 * a * c
    if disc > 0:
        t = (-b - math.sqrt(disc)) / (2.0 * a)
        if t > 0.001:
            p = vec_add(ray_o, vec_mul(ray_d, t))
            hits.append(("sphere", t, p))
    cx, cy, cz = 2.0, 0.0, 0.7
    r = 0.4
    h = 1.4
    ro = vec_sub(ray_o, (cx, cy, cz))
    a = ray_d[0] * ray_d[0] + ray_d[1] * ray_d[1]
    b = 2.0 * (ro[0] * ray_d[0] + ro[1] * ray_d[1])
    c = ro[0] * ro[0] + ro[1] * ro[1] - r * r
    disc = b * b - 4.0 * a * c
    if disc > 0 and a > 1e-6:
        t = (-b - math.sqrt(disc)) / (2.0 * a)
        if t > 0.001:
            p = vec_add(ray_o, vec_mul(ray_d, t))
            if cz - h * 0.5 <= p[2] <= cz + h * 0.5:
                hits.append(("cylinder", t, p))
    if abs(ray_d[2]) > 1e-6:
        t = (cz + h * 0.5 - ray_o[2]) / ray_d[2]
        if t > 0.001:
            p = vec_add(ray_o, vec_mul(ray_d, t))
            if (p[0] - cx) ** 2 + (p[1] - cy) ** 2 <= r * r:
                hits.append(("cylinder_top", t, p))
        t = (cz - h * 0.5 - ray_o[2]) / ray_d[2]
        if t > 0.001:
            p = vec_add(ray_o, vec_mul(ray_d, t))
            if (p[0] - cx) ** 2 + (p[1] - cy) ** 2 <= r * r:
                hits.append(("cylinder_bottom", t, p))
    if not hits:
        return None
    hits.sort(key=lambda x: x[1])
    return hits[0]

def patch_index(groups, name, p):
    if name == "floor":
        start, _, nx, ny, x0, y0, dx, dy = groups["floor"]
        ix = int((p[0] - x0) / dx)
        iy = int((p[1] - y0) / dy)
        ix = max(0, min(nx - 1, ix))
        iy = max(0, min(ny - 1, iy))
        return start + iy * nx + ix
    if name == "back":
        start, _, nx, nz, x0, z0, dx, dz = groups["back"]
        ix = int((p[0] - x0) / dx)
        iz = int((p[2] - z0) / dz)
        ix = max(0, min(nx - 1, ix))
        iz = max(0, min(nz - 1, iz))
        return start + iz * nx + ix
    if name == "left":
        start, _, ny, nz, y0, z0, dy, dz = groups["left"]
        iy = int((p[1] - y0) / dy)
        iz = int((p[2] - z0) / dz)
        iy = max(0, min(ny - 1, iy))
        iz = max(0, min(nz - 1, iz))
        return start + iz * ny + iy
    if name == "right":
        start, _, ny, nz, y0, z0, dy, dz = groups["right"]
        iy = int((p[1] - y0) / dy)
        iz = int((p[2] - z0) / dz)
        iy = max(0, min(ny - 1, iy))
        iz = max(0, min(nz - 1, iz))
        return start + iz * ny + iy
    if name == "sphere":
        start, _, nu, nv = groups["sphere"]
        v = normalize(vec_sub(p, (0.0, 0.0, 1.0)))
        theta = math.atan2(v[1], v[0])
        if theta < 0:
            theta += 2.0 * math.pi
        phi = math.acos(max(-1.0, min(1.0, v[2])))
        iu = int((theta / (2.0 * math.pi)) * nu)
        iv = int((phi / math.pi) * nv)
        iu = max(0, min(nu - 1, iu))
        iv = max(0, min(nv - 1, iv))
        return start + iv * nu + iu
    if name == "cylinder":
        start, _, nt, nz, cx, cy, cz, r, h = groups["cylinder"]
        v = vec_sub(p, (cx, cy, cz))
        theta = math.atan2(v[1], v[0])
        if theta < 0:
            theta += 2.0 * math.pi
        it = int((theta / (2.0 * math.pi)) * nt)
        iz = int(((v[2] + h * 0.5) / h) * nz)
        it = max(0, min(nt - 1, it))
        iz = max(0, min(nz - 1, iz))
        return start + iz * nt + it
    if name == "cylinder_top":
        start, end, cx, cy, cz, r = groups["cylinder_top"]
        v = vec_sub(p, (cx, cy, cz))
        theta = math.atan2(v[1], v[0])
        if theta < 0:
            theta += 2.0 * math.pi
        ring = int((length((v[0], v[1], 0.0)) / r) * 16)
        ring = max(0, min(15, ring))
        ring_count = max(6, int(48 * ((ring + 0.5) / 16)))
        it = int((theta / (2.0 * math.pi)) * ring_count)
        it = max(0, min(ring_count - 1, it))
        idx = start
        for k in range(ring):
            idx += max(6, int(48 * ((k + 0.5) / 16)))
        return min(end - 1, idx + it)
    if name == "cylinder_bottom":
        start, end, cx, cy, cz, r = groups["cylinder_bottom"]
        v = vec_sub(p, (cx, cy, cz))
        theta = math.atan2(v[1], v[0])
        if theta < 0:
            theta += 2.0 * math.pi
        ring = int((length((v[0], v[1], 0.0)) / r) * 16)
        ring = max(0, min(15, ring))
        ring_count = max(6, int(48 * ((ring + 0.5) / 16)))
        it = int((theta / (2.0 * math.pi)) * ring_count)
        it = max(0, min(ring_count - 1, it))
        idx = start
        for k in range(ring):
            idx += max(6, int(48 * ((k + 0.5) / 16)))
        return min(end - 1, idx + it)
    return None

def render(B, groups, out_path, width=640, height=360):
    camera = (0.0, -7.0, 2.0)
    target = (0.0, 0.0, 1.0)
    up = (0.0, 0.0, 1.0)
    forward = normalize(vec_sub(target, camera))
    right = normalize((forward[1] * up[2] - forward[2] * up[1],
                       forward[2] * up[0] - forward[0] * up[2],
                       forward[0] * up[1] - forward[1] * up[0]))
    true_up = normalize((right[1] * forward[2] - right[2] * forward[1],
                         right[2] * forward[0] - right[0] * forward[2],
                         right[0] * forward[1] - right[1] * forward[0]))
    fov = 35.0 * math.pi / 180.0
    aspect = width / height
    scale = math.tan(fov * 0.5)

    img = Image.new("RGB", (width, height))
    pixels = img.load()
    for y in range(height):
        for x in range(width):
            ndc_x = (2.0 * (x + 0.5) / width - 1.0) * aspect * scale
            ndc_y = (1.0 - 2.0 * (y + 0.5) / height) * scale
            dir = normalize(vec_add(forward, vec_add(vec_mul(right, ndc_x), vec_mul(true_up, ndc_y))))
            hit = intersect_scene(camera, dir)
            if hit is None:
                col = (0.7, 0.8, 0.95)
            else:
                name, _, p = hit
                idx = patch_index(groups, name, p)
                if idx is None:
                    col = (0.0, 0.0, 0.0)
                else:
                    col = B[idx]
            col = (math.sqrt(clamp01(col[0])), math.sqrt(clamp01(col[1])), math.sqrt(clamp01(col[2])))
            pixels[x, y] = (int(col[0] * 255.0), int(col[1] * 255.0), int(col[2] * 255.0))
    img.save(out_path)

def main():
    patches, groups = build_patches()
    F = compute_form_factors(patches)
    B = solve_radiosity(patches, F, iterations=45)
    out_dir = os.path.join("examples", "output")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "radiosity_room_red_sphere.png")
    render(B, groups, out_path, 1280, 720)

if __name__ == "__main__":
    main()
