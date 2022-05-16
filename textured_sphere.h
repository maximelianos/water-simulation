#ifndef TEXTUREDSPHERE_H
#define TEXTUREDSPHERE_H

#include "common.h"
#include "ShaderProgram.h"
using namespace env_water;

class TexturedSphere {
public:
    // Refracting the rays
    GLuint g_vertexArrayObject = 0;

    GLuint bottom_refract = 0; // Area of triangles on the bottom
    GLuint bottom_tri = 0; // Area of triangles on the bottom
    GLuint bottom_vert = 0; // Average of 6 triangles on the bottom
    GLuint bottom_pos = 0; // Position of verticies on the bottom
    GLuint draw_pos = 0; // Buffer ready to be drawn - positions
    GLuint draw_col = 0; // Colour buffer

    //ShaderProgram *refract_program;
    //GLuint refract_N = 0;
    //GLuint refract_light = 0;
    ShaderProgram *bottom_vert_program;
    GLuint bottom_vert_N = 0;
    ShaderProgram *bottom_fill_program;
    GLuint bottom_fill_N = 0;



    // Render triangles
    int N_texture = 512;
    ShaderProgram *render_program;
    GLuint framebuffer = 0;
    GLuint rendered_texture = 0; // Initialised with glGenTextures
    GLuint depthrenderbuffer = 0;
    GLuint quad_vertexArrayObject = 0;

    ShaderProgram *blend_program;



    // Render the wall
    int n_h = 32;
    int n_v = 32;
    int n_verticies = (n_h * 2 * (n_v / 2 - 1) + n_h) * 2 * 3 * 3;
    // (sectors * 2 triangles per sphere quad * bands in hemisphere,
    // not counting the last one + triangles in last band) *
    // * 2 hemispheres * verticies per triangle * coors per vertex
    glm::vec3 s_pos = glm::vec3(1.5, -0.7, 0.5);

    int obj_idx;
    ShaderProgram *texture_program;
    //ShaderProgram *copy_program;

    GLuint wall_vertexArrayObject = 0;
    GLuint wall_vertexBufferObject = 0;

    Image stone_texture;
    GLuint g_stone = 0;
    GLuint wall_texture = 0; // Resultant texture
    GLuint g_texture_uv = 0;

    GLuint g_sampler = 0;
    GLuint MatrixID = 0;



    TexturedSphere(int obj_idx);
    void init_renderer();
    void init_wall_draw();

    void refract_all();
    void refract();
    void render();
    void wall_draw();
};

#endif
