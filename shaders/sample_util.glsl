// to-do: possibly move this to another constants file
const float PI = 3.14159265359f;
const float PI_2 = 1.57079632679f;
const float PI_4 = 0.78539816339f;
const float INV_PI = 0.31830988618f;

// Utility to sample different distributions
// given a random value where each component is uniformly sampled from [0, 1].
// Reference implementations from PBRTv3.

vec2 concentric_sample_disc(const vec2 uv) {
    // Map values to [-1, 1]
    const vec2 offset = 2.0f * uv - vec2(1.0f);
    if (offset.x == 0.0f && offset.y == 0.0f) {
        return vec2(0.0f);
    }

    float theta, r;
    if (abs(offset.x) > abs(offset.y)) {
        r = offset.x;
        theta = PI_4 * (offset.y / offset.x);
    } else {
        r = offset.y;
        theta = PI_2 - PI_4 * (offset.x / offset.y);
    }

    return r * vec2(cos(theta), sin(theta));
}

vec3 cosine_sample_hemisphere(const vec2 uv) {
    const vec2 disc = concentric_sample_disc(uv);
    float up = sqrt(max(0.0f, 1 - disc.x * disc.x - disc.y * disc.y));
    return normalize(vec3(disc.x, up, disc.y));
}
