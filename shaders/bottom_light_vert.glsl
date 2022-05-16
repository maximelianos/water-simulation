#version 430 compatibility
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(std140, binding=4) buffer bottom_light_tri
{
    vec4 bottom_tri[ ];
};

layout(std140, binding=5) buffer bottom_light_vert
{
    vec4 bottom_vert[ ];
};

layout(local_size_x=128, local_size_y=1, local_size_z=1) in;
uniform uint N_water;

void main()
{
    uint gid = gl_GlobalInvocationID.x; // Index of water vertex
    uint i = gid / N_water;
    uint j = gid - i * N_water;

    // Average area of light in neighbour triangles
    float intensity_sum = 0;
    intensity_sum += bottom_tri[(i * (N_water - 1) + j) * 2 + 0].y; // i, j
    //intensity_sum += bottom_tri[((i - 1) * (N_water - 1) + j) * 2 + 0].y; // i-1, j
    //intensity_sum += bottom_tri[((i - 1) * (N_water - 1) + j) * 2 + 1].y; // i-1, j
    //intensity_sum += bottom_tri[(i * (N_water - 1) + j - 1) * 2 + 0].y; // i, j-1
    //intensity_sum += bottom_tri[(i * (N_water - 1) + j - 1) * 2 + 1].y; // i, j-1
    //intensity_sum += bottom_tri[((i - 1) * (N_water - 1) + j - 1) * 2 + 1].y * 2; // i-1, j-1
    bottom_vert[gid].y = intensity_sum / 1;
}
