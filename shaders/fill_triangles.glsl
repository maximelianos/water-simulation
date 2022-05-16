#version 430 compatibility
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(std140, binding=4) buffer cur
{
    vec4 cur_map[ ];
};

layout(std140, binding=5) buffer tri_pos
{
    vec4 triangle_pos[ ];
};

layout(std140, binding=6) buffer tri_color
{
    vec4 triangle_color[ ];
};

layout(local_size_x=128, local_size_y=1, local_size_z=1) in;
uniform uint N;

float clamp(float x, float min_value, float max_value) {
    return x < min_value ? min_value : (x > max_value ? max_value : x);
}

void main()
{
    uint gid = gl_GlobalInvocationID.x;
    uint i = gid / (N - 1);
    uint j = gid - i * (N - 1);

    uint idx = gid * 6;

    // Iterate over 6 verticies of 2 triangles inside a square
    uint idx2 = 0;
    uint di, dj;

    uint di_list[] = {0, 1, 0, 0, 1, 1};
    uint dj_list[] = {0, 0, 1, 1, 0, 1};
    for (uint h = 0; h < 6; h++) {
        di = di_list[h];
        dj = dj_list[h];
        // Prototype:
        // triangle_pos[idx + 0] = j;
        // triangle_pos[idx + 1] = cur_map[i * N + j];
        // triangle_pos[idx + 2] = i;

        triangle_pos[idx + h] = vec4(
            float(j + dj) * 0.01,
            cur_map[(i + di) * N + j + dj].y,
            float(i + di) * 0.01,
            1.0
        );

        triangle_color[idx + h] = vec4(
            clamp(0.1 + cur_map[(i + di) * N + j + dj].y - 1, 0, 1),
            clamp(0.1 + sin(cur_map[(i + di) * N + j + dj].y*1000), 0, 1),
            //clamp(0.5 + exp(-cur_map[(i + di) * N + j + dj].y) - 1, 0, 1),
            clamp(exp(cur_map[(i + di) * N + j + dj].y), 0, 1),
            0.5
        );
    }
}
