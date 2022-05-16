#version 430

layout(location = 0) in vec4 vertex;
out vec4 fragment_color;

uniform mat4 MVP;

void main(void)
{
    gl_Position = MVP * vec4(vertex);
    float y = vertex.y * 1000;
    fragment_color = vec4(1 - exp(y), 1 - exp(-y), 1, 1);
}
