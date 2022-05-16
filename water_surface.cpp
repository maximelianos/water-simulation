// Internal includes
#include "common.h"
#include "ShaderProgram.h"
#include "water_surface.h"
using namespace env_water;

// External dependencies
// Easy screen handling
#define GLFW_DLL
#include <GLFW/glfw3.h>

static void allocate_SSbo(GLuint &bufferSSbo, int elements) {
    glGenBuffers(1, &bufferSSbo); GL_CHECK_ERRORS;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bufferSSbo); GL_CHECK_ERRORS;
    glBufferData(GL_SHADER_STORAGE_BUFFER, elements * sizeof(struct pos), nullptr, GL_STATIC_DRAW);

    GLint buf_mask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

    struct pos *memory = (struct pos *)glMapBufferRange(
        GL_SHADER_STORAGE_BUFFER, 0, elements * sizeof(*memory), buf_mask
    );
    for (int i = 0; i < elements; i++) {
        memory[i].x = 0;
        memory[i].y = 0;
        memory[i].z = 0;
        memory[i].w = 1;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void gaussian(struct pos *arr, int N, int x, int y, float height, float width) {
    float EPS = 0.0000001;
    float r = sqrt(-width * log(EPS / height));
    for (int x1 = fmax(0, x-r); x1 <= fmin(N-1, x+r); x1++) {
        for (int y1 = fmax(0, y-r); y1 <= fmin(N-1, y+r); y1++) {
            int dx = x - x1;
            int dy = y - y1;
            float g = height * exp( -(dx * dx + dy * dy) / width );
            arr[y1 * N + x1].y += g;
        }
    }
}

WaterSurface::WaterSurface() {
    // All places where main or OpenGL memory is allocated I mark with alloc!
    glGenVertexArrays(1, &g_vertexArrayObject); GL_CHECK_ERRORS; // alloc!

    // Compute shaders!:)
    std::unordered_map<GLenum, std::string> shaders_compute;
	shaders_compute[GL_COMPUTE_SHADER] = "compute_grid.glsl";
	compute_program = new ShaderProgram(shaders_compute); GL_CHECK_ERRORS;

	shaders_compute[GL_COMPUTE_SHADER] = "fill_triangles.glsl";
	fill_program = new ShaderProgram(shaders_compute); GL_CHECK_ERRORS;

    shaders_compute[GL_COMPUTE_SHADER] = "wtexture_camera.glsl";
	camera_program = new ShaderProgram(shaders_compute); GL_CHECK_ERRORS;

    shaders_compute[GL_COMPUTE_SHADER] = "disturb_grid.glsl";
	disturb_program = new ShaderProgram(shaders_compute); GL_CHECK_ERRORS;

    std::unordered_map<GLenum, std::string> shaders;
    shaders[GL_VERTEX_SHADER]   = "wtexture_vertex.glsl";
    shaders[GL_FRAGMENT_SHADER] = "wtexture_fragment.glsl";
    draw_program = new ShaderProgram(shaders); GL_CHECK_ERRORS;

    shaders[GL_VERTEX_SHADER]   = "sv.glsl";
    shaders[GL_FRAGMENT_SHADER] = "sf.glsl";
    wireframe_program = new ShaderProgram(shaders); GL_CHECK_ERRORS;

    // Allocate Shader Storage Buffer Objects inside OpenGL context:
    // prev_map, cur_map, next_map
    allocate_SSbo(prevSSbo, N_water * N_water); // alloc!
    allocate_SSbo(curSSbo, N_water * N_water); // alloc!

    float h = 0.25;
    float sigma = 20;
    GLint buf_mask;
    struct pos *cur_map;

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, curSSbo); GL_CHECK_ERRORS;
    buf_mask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
    cur_map = (struct pos *)glMapBufferRange(
        GL_SHADER_STORAGE_BUFFER, 0, N_water * N_water * sizeof(*cur_map), buf_mask
    );
    gaussian(cur_map, N_water, N_water/2, N_water * 3/4, h, sigma);
    gaussian(cur_map, N_water, N_water/5, N_water/3, h, sigma);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, prevSSbo); GL_CHECK_ERRORS;
    buf_mask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
    cur_map = (struct pos *)glMapBufferRange(
        GL_SHADER_STORAGE_BUFFER, 0, N_water * N_water * sizeof(*cur_map), buf_mask
    );
    gaussian(cur_map, N_water, N_water/2, N_water * 3/4, h, sigma);
    gaussian(cur_map, N_water, N_water/5, N_water/3, h, sigma);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    allocate_SSbo(nextSSbo, N_water * N_water); // alloc!
    allocate_SSbo(tri_posSSbo, (N_water - 1) * (N_water - 1) * 6); // alloc!
    allocate_SSbo(tri_colorSSbo, (N_water - 1) * (N_water - 1) * 6); // alloc!

    allocate_SSbo(floor_uv, (N_water - 1) * (N_water - 1) * 6); // alloc!
    allocate_SSbo(pipe_pos, (N_water - 1) * (N_water - 1) * 6); // alloc!
    allocate_SSbo(pipe_refracted, (N_water - 1) * (N_water - 1) * 6); // alloc!
    allocate_SSbo(pipe_reflected, (N_water - 1) * (N_water - 1) * 6); // alloc!

    sky_texture.load("sky.bmp");

    glGenTextures(1, &g_sky); GL_CHECK_ERRORS;
    glBindTexture(GL_TEXTURE_2D, g_sky); GL_CHECK_ERRORS;
    // Transfer texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, N_texture,
        N_texture, 0, GL_RGB, GL_UNSIGNED_BYTE, sky_texture.image); GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GL_CHECK_ERRORS;
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// Input: float *prev_map, float *cur_map. Output: next_map
// At the end the arrays are rotated
void WaterSurface::simulation_step() {
    // prev, cur -> next
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, prevSSbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, curSSbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, nextSSbo);

    compute_program->StartUseShader();
    GLuint stage1_N_ID = glGetUniformLocation(compute_program->GetProgram(), "N"); GL_CHECK_ERRORS;
    GLuint stage1_W_ID = glGetUniformLocation(compute_program->GetProgram(), "W"); GL_CHECK_ERRORS;
    glUniform1ui(stage1_N_ID, N_water); GL_CHECK_ERRORS;
    glUniform1f(stage1_W_ID, W); GL_CHECK_ERRORS;
    glDispatchCompute(N_water * N_water / WORK_GROUP_SIZE, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    compute_program->StopUseShader();

    // cur -> prev, next -> cur
    GLuint tmpSSbo = prevSSbo;
    prevSSbo = curSSbo;
    curSSbo = nextSSbo;
    nextSSbo = tmpSSbo;

    // Copy coordinates to their positions (bad but easy approach)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, curSSbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, tri_posSSbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, tri_colorSSbo);

    fill_program->StartUseShader();
    GLuint stage2_N_ID = glGetUniformLocation(fill_program->GetProgram(), "N"); GL_CHECK_ERRORS;
    glUniform1ui(stage2_N_ID, N_water); GL_CHECK_ERRORS;
    glDispatchCompute((N_water - 1) * (N_water - 1) / WORK_GROUP_SIZE+1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    fill_program->StopUseShader();

    // Fill texture coors for camera point-of-view refraction
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, curSSbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, floor_uv);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, pipe_pos);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, pipe_refracted);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, pipe_reflected);

    camera_program->StartUseShader();
    GLuint camera_N = glGetUniformLocation(camera_program->GetProgram(), "N_water"); GL_CHECK_ERRORS;
    glUniform1ui(camera_N, N_water); GL_CHECK_ERRORS;
    GLuint camera_ID = glGetUniformLocation(camera_program->GetProgram(), "camera_pos"); GL_CHECK_ERRORS;
    glUniform3f(camera_ID, env.camera_pos.x, env.camera_pos.y, env.camera_pos.z);
    glDispatchCompute((N_water - 1) * (N_water - 1) / WORK_GROUP_SIZE+1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    camera_program->StopUseShader();
}

void WaterSurface::disturb_surface() {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, prevSSbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, curSSbo);

    disturb_program->StartUseShader();
    GLuint N_ID = glGetUniformLocation(disturb_program->GetProgram(), "N_water"); GL_CHECK_ERRORS;
    glUniform1ui(N_ID, N_water); GL_CHECK_ERRORS;

    GLuint disturb_params_ID = glGetUniformLocation(disturb_program->GetProgram(), "disturb_params"); GL_CHECK_ERRORS;
    glUniform3f(disturb_params_ID, env.dt_x, env.dt_z, env.dt_h); GL_CHECK_ERRORS;

    glDispatchCompute(N_water * N_water / WORK_GROUP_SIZE, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    disturb_program->StopUseShader();
}

void WaterSurface::wireframe_draw() {
    wireframe_program->StartUseShader();
    glBindVertexArray(g_vertexArrayObject); GL_CHECK_ERRORS;

    // Define the position as an attribute
    GLuint vertexLocation = 0;
    glBindBuffer(GL_ARRAY_BUFFER, tri_posSSbo); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(vertexLocation); GL_CHECK_ERRORS;
    glVertexAttribPointer(
        vertexLocation,    // Location identical to layout in the shader
        4,// Components in the attribute (4 coordinates)
        GL_FLOAT,          // Type of components
        GL_FALSE,          // Normalization
        0,                 // Stride: byte size of structure
        0                  // Offset (in bytes) of attribute inside structure
    ); GL_CHECK_ERRORS;



    GLuint MatrixID = glGetUniformLocation(draw_program->GetProgram(), "MVP"); GL_CHECK_ERRORS;
    glm::mat4 mvp = MVP();
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]); GL_CHECK_ERRORS;

    glDrawArrays(GL_LINES, 0, (N_water - 1) * (N_water - 1) * 6);       GL_CHECK_ERRORS;  // The last parameter of glDrawArrays is equal to VertexShader invocations
    wireframe_program->StopUseShader();
}

void WaterSurface::draw() {
    draw_program->StartUseShader();
    glBindVertexArray(g_vertexArrayObject); GL_CHECK_ERRORS;

    // Define the position as an attribute
    GLuint vertexLocation = 0;
    glBindBuffer(GL_ARRAY_BUFFER, tri_posSSbo); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(vertexLocation); GL_CHECK_ERRORS;
    glVertexAttribPointer(
        vertexLocation,    // Location identical to layout in the shader
        4,// Components in the attribute (4 coordinates)
        GL_FLOAT,          // Type of components
        GL_FALSE,          // Normalization
        0,                 // Stride: byte size of structure
        0                  // Offset (in bytes) of attribute inside structure
    ); GL_CHECK_ERRORS;

    GLuint uvLocation = 1;
    glBindBuffer(GL_ARRAY_BUFFER, floor_uv); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(uvLocation); GL_CHECK_ERRORS;
    glVertexAttribPointer(
        uvLocation,     // Location identical to layout in the shader
        4,              // Components in the attribute (3 coordinates)
        GL_FLOAT,       // Type of components
        GL_FALSE,       // Normalization
        0,              // Stride: byte size of structure
        0               // Offset (in bytes) of attribute inside structure
    );
    GL_CHECK_ERRORS;

    GLuint pipePosLocation = 2;
    glBindBuffer(GL_ARRAY_BUFFER, pipe_pos); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(pipePosLocation); GL_CHECK_ERRORS;
    glVertexAttribPointer(
        pipePosLocation,     // Location identical to layout in the shader
        4,              // Components in the attribute (3 coordinates)
        GL_FLOAT,       // Type of components
        GL_FALSE,       // Normalization
        0,              // Stride: byte size of structure
        0               // Offset (in bytes) of attribute inside structure
    );
    GL_CHECK_ERRORS;

    GLuint pipeRefractedLocation = 3;
    glBindBuffer(GL_ARRAY_BUFFER, pipe_refracted); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(pipeRefractedLocation); GL_CHECK_ERRORS;
    glVertexAttribPointer(
        pipeRefractedLocation,     // Location identical to layout in the shader
        4,              // Components in the attribute (3 coordinates)
        GL_FLOAT,       // Type of components
        GL_FALSE,       // Normalization
        0,              // Stride: byte size of structure
        0               // Offset (in bytes) of attribute inside structure
    );
    GL_CHECK_ERRORS;

    GLuint pipeReflectedLocation = 4;
    glBindBuffer(GL_ARRAY_BUFFER, pipe_reflected); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(pipeReflectedLocation); GL_CHECK_ERRORS;
    glVertexAttribPointer(
        pipeReflectedLocation,     // Location identical to layout in the shader
        4,              // Components in the attribute (3 coordinates)
        GL_FLOAT,       // Type of components
        GL_FALSE,       // Normalization
        0,              // Stride: byte size of structure
        0               // Offset (in bytes) of attribute inside structure
    );
    GL_CHECK_ERRORS;

    GLuint MatrixID = glGetUniformLocation(draw_program->GetProgram(), "MVP"); GL_CHECK_ERRORS;
    glm::mat4 mvp = MVP();
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]); GL_CHECK_ERRORS;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floor0->rendered_texture);
    GLuint g_sampler = glGetUniformLocation(draw_program->GetProgram(), "myTextureSampler"); GL_CHECK_ERRORS;
    glUniform1i(g_sampler, 0);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, wall1->rendered_texture);
    GLuint g_sampler1 = glGetUniformLocation(draw_program->GetProgram(), "myTextureSampler1"); GL_CHECK_ERRORS;
    glUniform1i(g_sampler1, 1);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, wall2->rendered_texture);
    GLuint g_sampler2 = glGetUniformLocation(draw_program->GetProgram(), "myTextureSampler2"); GL_CHECK_ERRORS;
    glUniform1i(g_sampler2, 2);

    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_2D, wall3->rendered_texture);
    GLuint g_sampler3 = glGetUniformLocation(draw_program->GetProgram(), "myTextureSampler3"); GL_CHECK_ERRORS;
    glUniform1i(g_sampler3, 3);

    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_2D, wall4->rendered_texture);
    GLuint g_sampler4 = glGetUniformLocation(draw_program->GetProgram(), "myTextureSampler4"); GL_CHECK_ERRORS;
    glUniform1i(g_sampler4, 4);

    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_2D, ball5->rendered_texture);
    GLuint g_sampler5 = glGetUniformLocation(draw_program->GetProgram(), "myTextureSampler5"); GL_CHECK_ERRORS;
    glUniform1i(g_sampler5, 5);

    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_2D, g_sky);
    GLuint g_sampler6 = glGetUniformLocation(draw_program->GetProgram(), "myTextureSampler6"); GL_CHECK_ERRORS;
    glUniform1i(g_sampler6, 6);

    GLuint draw_N = glGetUniformLocation(draw_program->GetProgram(), "N_water"); GL_CHECK_ERRORS;
    glUniform1ui(draw_N, N_water);

    glDrawArrays(GL_TRIANGLES, 0, (N_water - 1) * (N_water - 1) * 6);       GL_CHECK_ERRORS;  // The last parameter of glDrawArrays is equal to VertexShader invocations
    draw_program->StopUseShader();
}

void WaterSurface::free() {
    glDeleteVertexArrays(1, &g_vertexArrayObject);
    glDeleteBuffers(1,      &prevSSbo);
    glDeleteBuffers(1,      &curSSbo);
    glDeleteBuffers(1,      &nextSSbo);
    glDeleteBuffers(1,      &tri_posSSbo);
    glDeleteBuffers(1,      &tri_colorSSbo);
}
