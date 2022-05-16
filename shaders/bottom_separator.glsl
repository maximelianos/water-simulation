#version 430 compatibility
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(std140, binding=1) buffer bottom_light_tri { vec4 bottom_tri[ ]; }; // Input
layout(std140, binding=2) buffer bottom_light_pos { vec4 bottom_pos[ ]; };

// Output
layout(std140, binding=3) buffer bottom_light_tri0 { vec4 bottom_tri0[ ]; }; // floor 0
layout(std140, binding=4) buffer bottom_light_pos0 { vec4 bottom_pos0[ ]; };

layout(std140, binding=5) buffer bottom_light_tri1 { vec4 bottom_tri1[ ]; }; // wall 1
layout(std140, binding=6) buffer bottom_light_pos1 { vec4 bottom_pos1[ ]; };

layout(std140, binding=7) buffer bottom_light_tri2 { vec4 bottom_tri2[ ]; }; // wall 2
layout(std140, binding=8) buffer bottom_light_pos2 { vec4 bottom_pos2[ ]; };

layout(std140, binding=9) buffer bottom_light_tri3 { vec4 bottom_tri3[ ]; }; // wall 3
layout(std140, binding=10) buffer bottom_light_pos3 { vec4 bottom_pos3[ ]; };

layout(std140, binding=11) buffer bottom_light_tri4 { vec4 bottom_tri4[ ]; }; // wall 4
layout(std140, binding=12) buffer bottom_light_pos4 { vec4 bottom_pos4[ ]; };

layout(std140, binding=13) buffer bottom_light_tri5 { vec4 bottom_tri5[ ]; }; // ball 5
layout(std140, binding=14) buffer bottom_light_pos5 { vec4 bottom_pos5[ ]; };

layout(local_size_x=128, local_size_y=1, local_size_z=1) in;
uniform uint N_water;

void separate_point(vec3 p, uint obj_idx, uint i, uint j) {
    // 1. u, v coors
    // 2. copy to array
    float u, v;
    if (obj_idx == 0) {
        u = p.x;
        v = p.z;
        bottom_pos0[i * N_water + j].xyz = vec3(u, -1, v);
    } else if (obj_idx == 1) {
        u = p.x;
        v = p.y;
        bottom_pos1[i * N_water + j].xyz = vec3(u, -1, v);
    } else if (obj_idx == 2) {
        u = p.z;
        v = p.y;
        bottom_pos2[i * N_water + j].xyz = vec3(u, -1, v);
    } else if (obj_idx == 3) {
        u = p.x;
        v = p.y;
        bottom_pos3[i * N_water + j].xyz = vec3(u, -1, v);
    } else if (obj_idx == 4) {
        u = p.z;
        v = p.y;
        bottom_pos4[i * N_water + j].xyz = vec3(u, -1, v);
    } else if (obj_idx == 5) {
        u = p.x;
        v = p.z;
        bottom_pos5[i * N_water + j].xyz = vec3(u, -1, v);
    }
}

void separate_area(float area, uint i1, uint i2, uint i3, uint arr_idx) {
    bottom_tri0[arr_idx].y = 1000000;
    bottom_tri1[arr_idx].y = 1000000;
    bottom_tri2[arr_idx].y = 1000000;
    bottom_tri3[arr_idx].y = 1000000;
    bottom_tri4[arr_idx].y = 1000000;
    bottom_tri5[arr_idx].y = 1000000;
    if (i1 == i2 && i1 == i3) {
        if (i1 == 0) {
            bottom_tri0[arr_idx].y = area;
        } else if (i1 == 1) {
            bottom_tri1[arr_idx].y = area;
        } else if (i1 == 2) {
            bottom_tri2[arr_idx].y = area;
        } else if (i1 == 3) {
            bottom_tri3[arr_idx].y = area;
        } else if (i1 == 4) {
            bottom_tri4[arr_idx].y = area;
        } else if (i1 == 5) {
            bottom_tri5[arr_idx].y = area * 0.4;
        }
    }

}

void main()
{
    uint gid = gl_GlobalInvocationID.x; // Index of square
    uint i = gid / (N_water - 1);
    uint j = gid - i * (N_water - 1);

    vec4 p1 = bottom_pos[i * N_water + j]; // vec4(x, y, z, obj_idx)
    vec4 p2 = bottom_pos[(i + 1) * N_water + j];
    vec4 p3 = bottom_pos[i * N_water + j + 1];
    vec4 p4 = bottom_pos[(i + 1) * N_water + j + 1];

    separate_point(p1.xyz, uint(p1.w), i, j);
    if (i == N_water - 2) {
        separate_point(p2.xyz, uint(p2.w), i + 1, j); // i + 1, j
        if (j == N_water - 2) {
            separate_point(p4.xyz, uint(p4.w), i + 1, j + 1); // i + 1, j + 1
        }
    }
    if (j == N_water - 2) {
        separate_point(p3.xyz, uint(p3.w), i, j + 1); // i, j + 1
    }

    float area0 = bottom_tri[gid * 2 + 0].y;
    float area1 = bottom_tri[gid * 2 + 1].y;
    separate_area(area0, uint(p1.w), uint(p2.w), uint(p3.w), gid * 2 + 0);
    separate_area(area1, uint(p3.w), uint(p2.w), uint(p4.w), gid * 2 + 1);
}
