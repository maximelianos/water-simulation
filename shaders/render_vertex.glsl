#version 430

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 vertex_color;
out vec4 fragment_color;

void main(void)
{
  gl_Position = vec4(vertex.x * 2 - 1, vertex.z * 2 - 1, vertex.y, 1);
  //gl_Position = vec4(vertex, 1);
  //fragment_color = vertex_color;
  fragment_color = vec4(vertex_color.y, vertex_color.y, vertex_color.y, 0.25);
}
