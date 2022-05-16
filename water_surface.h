#ifndef WATERSURFACE_H
#define WATERSURFACE_H

#include "common.h"
#include "ShaderProgram.h"

#include "textured_wall.h"
#include "textured_sphere.h"

class WaterSurface {
public:
    GLuint g_vertexArrayObject = 0;

    GLuint prevSSbo = 0; // Shader Storage Buffer Object
    // GLuint curSSbo = 0; in namespace
    GLuint nextSSbo = 0;

    GLuint tri_posSSbo = 0;
    GLuint tri_colorSSbo = 0;
    GLuint pipe_pos = 0;
    GLuint pipe_refracted = 0;
    GLuint pipe_reflected = 0;
    GLuint floor_uv = 0;

    ShaderProgram *compute_program;
    ShaderProgram *fill_program;
    ShaderProgram *camera_program;
    ShaderProgram *draw_program;

    ShaderProgram *wireframe_program;
    ShaderProgram *disturb_program;

    Image sky_texture;
    int N_texture = 512;
    GLuint g_sky = 0;

    WaterSurface();
    void simulation_step();
    void wireframe_draw();
    void disturb_surface();
    void draw();
    void free();
};

#endif
