#version 430 core

in vec4 fragment_color;
layout(location = 0) out vec4 color;

void main()
{
  color = fragment_color;
}
