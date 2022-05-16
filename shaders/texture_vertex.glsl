#version 430

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 vertex_uv_in;
out vec2 vertex_uv;

uniform mat4 MVP;

void main(void)
{
  gl_Position = MVP * vec4(vertex, 1);
  vertex_uv = vertex_uv_in;
}
