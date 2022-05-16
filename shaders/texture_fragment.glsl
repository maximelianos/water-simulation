#version 430 core

in vec2 vertex_uv;
uniform sampler2D myTextureSampler;
uniform sampler2D myTextureSampler2;

out vec4 color;

float clamp(float x, float min_value, float max_value) {
    return x < min_value ? min_value : (x > max_value ? max_value : x);
}

void main()
{
    vec4 col = texture(myTextureSampler, vertex_uv);
    color = col;
    /*color = vec4(
        clamp(col.x, 0.05, 1),
        clamp(col.y, 0.05, 1),
        clamp(col.z, 0.05, 1),
        col.w
    );*/ // * texture(myTextureSampler2, vertex_uv);
}
