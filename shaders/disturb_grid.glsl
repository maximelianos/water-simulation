#version 430 compatibility
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(std140, binding=4) buffer prev
{
    vec4 prev_map[ ];
};

layout(std140, binding=5) buffer cur
{
    vec4 cur_map[ ];
};

layout(local_size_x=128, local_size_y=1, local_size_z=1) in;
uniform uint N_water;
uniform vec3 disturb_params; // (x, z, h)

float gaussian(float x1, float y1, float x2, float y2, float height, float width) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    return height * exp( -(dx * dx + dy * dy) / width );
}

void main()
{
    uint gid = gl_GlobalInvocationID.x;
    uint i = gid / N_water;
    uint j = gid - i * N_water;

    float fi = float(i);
    float fj = float(j);

    float dt_j = disturb_params.x * 100;
    float dt_i = disturb_params.y * 100;
    float dt_h = disturb_params.z;

    prev_map[i * N_water + j].y += gaussian(dt_j, dt_i, fj, fi, dt_h, 25);
    cur_map[i * N_water + j].y += gaussian(dt_j, dt_i, fj, fi, dt_h, 25);
}
