#version 430 compatibility
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(std140, binding=4) buffer cur
{
    vec4 cur_map[ ];
};

layout(std140, binding=5) buffer bottom_light_tri
{
    vec4 bottom_tri[ ];
};

layout(std140, binding=6) buffer bottom_light_pos // uv coors in format (x, y, z) = (u, _, v)
{
    vec4 bottom_pos[ ];
};

layout(local_size_x=128, local_size_y=1, local_size_z=1) in;
uniform uint N_water;
uniform vec3 light;

float clamp(float x, float min_value, float max_value) {
    return x < min_value ? min_value : (x > max_value ? max_value : x);
}

float length(vec3 v) {
    return sqrt(dot(v, v));
}

vec3 norm(vec3 v) {
    return v / length(v);
}

vec3 refract(vec3 vec_in, vec3 normal, float theta) {
    // normal must be of unit length!

    // Large angle test
    vec3 n_proj = vec3(normal.x, 0, normal.z);
    float slope;
    if (length(n_proj) == 0) {
        slope = 0;
    } else {
        slope = dot(normal, norm(n_proj));
    }

    float theta_sq = theta * theta;

    float dot_v_n = dot(vec_in, normal);
    if (dot_v_n < 0 && slope < 0.2) {
        float norm_h = abs(dot_v_n);

        vec3 u = vec_in + normal * norm_h;

        float cos_a = norm_h / length(vec_in);
        float sin_a_sq = 1 - cos_a * cos_a;

        // Refract through surface
        float sin_a = sqrt(sin_a_sq);
        vec3 refracted = -normal * sqrt(theta_sq - sin_a_sq) +
                u / length(u) * sin_a;

        return refracted;
    } else {
        return vec3(0, 0, 0);
    }
}

vec3 intersect(vec3 cur_point, vec3 refracted) {
    if (refracted.y != 0) {
        // Intersect with bottom, y = -1
        float t = (-1 - cur_point.y) / refracted.y;
        float x = cur_point.x + t * refracted.x;
        float z = cur_point.z + t * refracted.z;
        return vec3(x * 100, -1, z * 100);
    } else {
        return vec3(0, 0, 0);
    }
}

vec3 water_normal(uint i, uint j) {
    float h = cur_map[i * N_water + j].y;
    vec3 grad_x = vec3(0.1, cur_map[i * N_water + j + 1].y - h, 0.0);
    vec3 grad_z = vec3(0.0, cur_map[(i + 1) * N_water + j].y - h, 0.1);
    vec3 normal = cross(grad_z, grad_x);
    return norm(normal);
}

float area(vec3 p1, vec3 p2, vec3 p3) {
    if (p1.y == 0 || p2.y == 0 || p3.y == 0) {
        return 1000000;
    }
    vec3 a = p1 - p2;
    vec3 b = p3 - p2;
    return abs((a.x * b.z - a.z * b.x) / 2);
}

void main()
{
    uint gid = gl_GlobalInvocationID.x; // Index of square
    uint i = gid / (N_water - 1);
    uint j = gid - i * (N_water - 1);

    vec3 p1 = vec3( // i, j = 0, 0
        float(j) * 0.01,
        cur_map[i * N_water + j].y,
        float(i) * 0.01
    );
    vec3 p2 = vec3( // i, j = 1, 0
        float(j) * 0.01,
        cur_map[(i + 1) * N_water + j].y,
        float(i + 1) * 0.01
    );
    vec3 p3 = vec3( // i, j = 0, 1
        float(j + 1) * 0.01,
        cur_map[i * N_water + j + 1].y,
        float(i) * 0.01
    );
    vec3 p4 = vec3( // i, j = 1, 1
        float(j + 1) * 0.01,
        cur_map[(i + 1) * N_water + j + 1].y,
        float(i + 1) * 0.01
    );

    // Normal of water, +x +z components
    vec3 n1 = water_normal(i, j);
    vec3 n2 = water_normal(i + 1, j);
    vec3 n3 = water_normal(i, j + 1);
    vec3 n4 = water_normal(i + 1, j + 1);

    // Light source
    float theta = 1.3;

    vec3 r1 = refract(p1 - light, n1, theta);
    vec3 r2 = refract(p2 - light, n2, theta);
    vec3 r3 = refract(p3 - light, n3, theta);
    vec3 r4 = refract(p4 - light, n4, theta);

    vec3 p1_bottom = intersect(p1, r1);
    vec3 p2_bottom = intersect(p2, r2);
    vec3 p3_bottom = intersect(p3, r3);
    vec3 p4_bottom = intersect(p4, r4);

    float k = 1 / float(N_water);

    bottom_pos[i * N_water + j].xyz = p1_bottom * k;
    if (i == N_water - 2) {
        bottom_pos[(i + 1) * N_water + j].xyz = p2_bottom * k; // i, j
        if (j == N_water - 2) {
            bottom_pos[(i + 1) * N_water + j + 1].xyz = p4_bottom * k; // i, j
        }
    }
    if (j == N_water - 2) {
        bottom_pos[i * N_water + j + 1].xyz = p3_bottom * k; // i, j
    }

    bottom_tri[gid * 2 + 0].y = area(p1_bottom, p2_bottom, p3_bottom);
    bottom_tri[gid * 2 + 1].y = area(p3_bottom, p2_bottom, p4_bottom);
}
