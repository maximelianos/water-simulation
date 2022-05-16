#version 430

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 vertex_uv_in;
layout(location = 2) in vec4 pipe_pos;
layout(location = 3) in vec4 pipe_refracted;
layout(location = 4) in vec4 pipe_reflected;

out vec3 cur_pos;
out vec3 refracted;
out vec3 reflected;

uniform mat4 MVP;

void main(void) {
    gl_Position = MVP * vertex;
    cur_pos = pipe_pos.xyz;
    refracted = pipe_refracted.xyz;
    reflected = pipe_reflected.xyz;
}
