#version 430 compatibility
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(std140, binding=4) buffer bottom_light_pos // In: vertex position
{
    vec4 bottom_pos[ ];
};

layout(std140, binding=5) buffer bottom_light_tri // In: area
{
    vec4 bottom_tri[ ];
};

layout(std140, binding=6) buffer bottom_light_vert // In: colour
{
    vec4 bottom_vert[ ];
};

layout(std140, binding=7) buffer bottom_draw_pos // Out: pos
{
    vec4 draw_pos[ ];
};

layout(std140, binding=8) buffer bottom_draw_color // Out: colour
{
    vec4 draw_col[ ];
};

layout(local_size_x=128, local_size_y=1, local_size_z=1) in;
uniform uint N_water;

float clamp(float x, float min_value, float max_value) {
    return x < min_value ? min_value : (x > max_value ? max_value : x);
}

float intensity(float area) {
    float h = 2;
    float sigma = 3;
    return h * exp(-area * area / (sigma * sigma));
}

void main()
{
    uint gid = gl_GlobalInvocationID.x;
    uint i = gid / (N_water - 1);
    uint j = gid - i * (N_water - 1);

    vec3 p1 = bottom_pos[i * N_water + j].xyz; // i, j
    vec3 p2 = bottom_pos[(i + 1) * N_water + j].xyz; // i+1, j
    vec3 p3 = bottom_pos[i * N_water + j + 1].xyz; // i, j+1
    vec3 p4 = bottom_pos[(i + 1) * N_water + j + 1].xyz; // i+1, j+1

    float area0 = bottom_tri[(i * (N_water - 1) + j) * 2 + 0].y;
    float area1 = bottom_tri[(i * (N_water - 1) + j) * 2 + 1].y;

    uint idx = gid * 6;
    // Triangle 0
    float depth = -float(i * N_water + j) / (N_water * N_water);
    if (p1.y == 0 || p2.y == 0 || p3.y == 0 || area0 > 10) {
        draw_pos[idx + 0].xyz = vec3(0, depth, 0);
        draw_pos[idx + 1].xyz = vec3(0, depth, 0);
        draw_pos[idx + 2].xyz = vec3(0, depth, 0);
    } else {
        draw_pos[idx + 0].xyz = vec3(p1.x, depth, p1.z);
        draw_pos[idx + 1].xyz = vec3(p2.x, depth, p2.z);
        draw_pos[idx + 2].xyz = vec3(p3.x, depth, p3.z);
    }
    draw_col[idx + 0].y = intensity(bottom_vert[i * N_water + j].y);
    draw_col[idx + 1].y = intensity(bottom_vert[(i + 1) * N_water + j].y);
    draw_col[idx + 2].y = intensity(bottom_vert[i * N_water + (j + 1)].y);

    // Triangle 1
    depth = -(float(i) * N_water + j + 0.5) / (N_water * N_water);
    if (p3.y == 0 || p2.y == 0 || p4.y == 0 || area1 > 10) {
        draw_pos[idx + 3].xyz = vec3(0, depth, 0);
        draw_pos[idx + 4].xyz = vec3(0, depth, 0);
        draw_pos[idx + 5].xyz = vec3(0, depth, 0);
    } else {
        draw_pos[idx + 3].xyz = vec3(p3.x, depth, p3.z);
        draw_pos[idx + 4].xyz = vec3(p2.x, depth, p2.z);
        draw_pos[idx + 5].xyz = vec3(p4.x, depth, p4.z);
    }
    draw_col[idx + 3].y = intensity(bottom_vert[i * N_water + j + 1].y);
    draw_col[idx + 4].y = intensity(bottom_vert[(i + 1) * N_water + j].y);
    draw_col[idx + 5].y = intensity(bottom_vert[(i + 1) * N_water + (j + 1)].y);
}
