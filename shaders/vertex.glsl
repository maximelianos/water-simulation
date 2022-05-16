#version 430

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 vertex_color;
out vec4 fragment_color;

uniform mat4 MVP;

void main(void)
{
  gl_Position = MVP * vertex;
  fragment_color = vertex_color;
}
