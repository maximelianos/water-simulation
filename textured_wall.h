#ifndef TEXTUREDWALL_H
#define TEXTUREDWALL_H

#include "common.h"
#include "ShaderProgram.h"
#include "textured_sphere.h"
using namespace env_water;

class TexturedWall {
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
    ShaderProgram *bottom_all_tri_program;
    GLuint bottom_all_tri_N = 0;
    GLuint bottom_all_tri_light = 0;
    ShaderProgram *bottom_separator_program;
    GLuint bottom_separator_N = 0;

    ShaderProgram *bottom_tri_program;
    GLuint bottom_tri_N = 0;
    GLuint bottom_tri_light = 0;
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



    TexturedWall(int);
    void init_renderer();
    void init_wall_draw();

    void refract_all();
    void refract();
    void render();
    void wall_draw();
};

#endif
