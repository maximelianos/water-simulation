#version 430 compatibility
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(rgba32f, binding=5) uniform image2D stone_texture;
layout(rgba32f, binding=6) uniform image2D blend_texture;

layout(local_size_x=128, local_size_y=1, local_size_z=1) in;
uniform uint N_texture;

float clamp(float x, float min_value, float max_value) {
    return x < min_value ? min_value : (x > max_value ? max_value : x);
}

void main()
{
    uint gid = gl_GlobalInvocationID.x;
    uint i = gid / N_texture;
    uint j = gid - i * N_texture;

    ivec2 coors = ivec2(j, i);

    vec4 stone_color = imageLoad(stone_texture, coors).rgba;
    vec4 blend_color = imageLoad(blend_texture, coors).rgba;
    blend_color = vec4(
        clamp(blend_color.x, 0.05, 1),
        clamp(blend_color.y, 0.05, 1),
        clamp(blend_color.z, 0.05, 1),
        blend_color.w
    );
    imageStore(blend_texture, coors, stone_color * blend_color);
}
