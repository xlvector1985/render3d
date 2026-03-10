#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uColor;
uniform vec3 uCameraPos;
uniform vec3 uLightPos;
uniform int uMaterialType;
uniform int uCheckerMode;
uniform float uAlpha;
uniform float uMetalness;
uniform float uRoughness;
uniform float uCheckerScale;

out vec4 FragColor;

vec3 checkerColor(vec3 p, int mode, float scale) {
    float a = 0.0;
    if (mode == 1) {
        a = floor(p.x * scale) + floor(p.y * scale);
    } else if (mode == 2) {
        a = floor(p.x * scale) + floor(p.z * scale);
    } else if (mode == 3) {
        a = floor(p.y * scale) + floor(p.z * scale);
    }
    float c = mod(a, 2.0);
    return mix(vec3(0.1), vec3(0.9), c);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(uLightPos - vWorldPos);
    vec3 H = normalize(V + L);

    float diff = max(dot(N, L), 0.0);
    float specPow = mix(8.0, 128.0, 1.0 - uRoughness);
    float spec = pow(max(dot(N, H), 0.0), specPow);

    vec3 baseColor = uColor;
    if (uCheckerMode != 0) {
        baseColor = checkerColor(vWorldPos, uCheckerMode, uCheckerScale);
    }

    vec3 color = vec3(0.0);
    if (uMaterialType == 0) {
        color = baseColor * (0.15 + diff) + spec * vec3(0.2);
    } else if (uMaterialType == 1) {
        color = baseColor * spec * 1.5 + vec3(0.02);
    } else if (uMaterialType == 2) {
        float fresnel = pow(1.0 - max(dot(N, V), 0.0), 5.0);
        vec3 glassTint = mix(baseColor * 0.05, vec3(1.0), fresnel);
        color = glassTint * (0.2 + diff * 0.2) + spec * vec3(1.0);
    }

    FragColor = vec4(color, uAlpha);
}
