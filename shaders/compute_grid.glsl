#version 430 compatibility
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object: enable

// I use vec4 even for the height grid because
// vec4 and std140 are compatible. If the type is not
// vec4 then std would need to change (probably)
layout(std140, binding=4) buffer prev
{
    vec4 prev_map[ ];
};

layout(std140, binding=5) buffer cur
{
    vec4 cur_map[ ];
};

layout(std140, binding=6) buffer next
{
    vec4 next_map[ ];
};

layout(local_size_x=128, local_size_y=1, local_size_z=1) in;
uniform uint N;
uniform float W;

void main()
{
    uint gid = gl_GlobalInvocationID.x;
    uint i = gid / N;
    uint j = gid - i * N;

    float sum = 0;
    sum += cur_map[i * N + j + 1].y + cur_map[i * N + j - 1].y +
        cur_map[(i + 1) * N + j].y + cur_map[(i - 1) * N + j].y;

    next_map[i * N + j].y = (1 - W) * prev_map[i * N + j].y + W * sum / 4;
    if (i == 0 || i == N - 1 || j == 0 || j == N - 1) {
        next_map[i * N + j].y = 0;
    }
}
