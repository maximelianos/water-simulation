// Internal includes
#include "textured_wall.h"

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

TexturedWall::TexturedWall(int obj_idx) : obj_idx(obj_idx) {
    glGenVertexArrays(1, &g_vertexArrayObject); GL_CHECK_ERRORS; // alloc!

    // Compute shaders:)
    std::unordered_map<GLenum, std::string> compute_shaders;

    compute_shaders[GL_COMPUTE_SHADER] = "bottom_all_tri.glsl";
	bottom_all_tri_program = new ShaderProgram(compute_shaders); GL_CHECK_ERRORS;

    compute_shaders[GL_COMPUTE_SHADER] = "bottom_separator.glsl";
	bottom_separator_program = new ShaderProgram(compute_shaders); GL_CHECK_ERRORS;

	compute_shaders[GL_COMPUTE_SHADER] = "bottom_light_tri.glsl";
	bottom_tri_program = new ShaderProgram(compute_shaders); GL_CHECK_ERRORS;

    compute_shaders[GL_COMPUTE_SHADER] = "bottom_light_vert.glsl";
	bottom_vert_program = new ShaderProgram(compute_shaders); GL_CHECK_ERRORS;

    compute_shaders[GL_COMPUTE_SHADER] = "bottom_fill.glsl";
	bottom_fill_program = new ShaderProgram(compute_shaders); GL_CHECK_ERRORS;

    compute_shaders[GL_COMPUTE_SHADER] = "texture_blend.glsl";
	blend_program = new ShaderProgram(compute_shaders); GL_CHECK_ERRORS;

    allocate_SSbo(bottom_tri, (N_water - 1) * (N_water - 1) * 2);
    allocate_SSbo(bottom_vert, N_water * N_water);
    allocate_SSbo(bottom_pos, N_water * N_water);
    allocate_SSbo(draw_pos, (N_water - 1) * (N_water - 1) * 2 * 3);
    allocate_SSbo(draw_col, (N_water - 1) * (N_water - 1) * 2 * 3);

    bottom_all_tri_N = glGetUniformLocation(bottom_all_tri_program->GetProgram(), "N_water"); GL_CHECK_ERRORS;
    bottom_all_tri_light = glGetUniformLocation(bottom_all_tri_program->GetProgram(), "light"); GL_CHECK_ERRORS;

    bottom_separator_N = glGetUniformLocation(bottom_separator_program->GetProgram(), "N_water"); GL_CHECK_ERRORS;

    bottom_tri_N = glGetUniformLocation(bottom_tri_program->GetProgram(), "N_water"); GL_CHECK_ERRORS;
    bottom_tri_light = glGetUniformLocation(bottom_tri_program->GetProgram(), "light"); GL_CHECK_ERRORS;

    bottom_vert_N = glGetUniformLocation(bottom_vert_program->GetProgram(), "N_water"); GL_CHECK_ERRORS;

    bottom_fill_N = glGetUniformLocation(bottom_fill_program->GetProgram(), "N_water"); GL_CHECK_ERRORS;

    init_renderer();
    init_wall_draw();
}

void TexturedWall::init_renderer() {
    std::unordered_map<GLenum, std::string> shaders;
    shaders[GL_VERTEX_SHADER]   = "render_vertex.glsl";
    shaders[GL_FRAGMENT_SHADER] = "render_fragment.glsl";
    render_program = new ShaderProgram(shaders); GL_CHECK_ERRORS;

    glGenFramebuffers(1, &framebuffer); GL_CHECK_ERRORS;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer); GL_CHECK_ERRORS;

    // The texture we're going to render to
    glGenTextures(1, &rendered_texture); GL_CHECK_ERRORS;
    glBindTexture(GL_TEXTURE_2D, rendered_texture); GL_CHECK_ERRORS;
    // Give an empty image to OpenGL ( the last "(void*)0" means "empty" )
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA32F, N_texture, N_texture, 0,GL_RGBA, GL_UNSIGNED_BYTE, nullptr); GL_CHECK_ERRORS;
    // Poor filtering, needed!
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); GL_CHECK_ERRORS;

    glGenRenderbuffers(1, &depthrenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, N_texture, N_texture);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer); GL_CHECK_ERRORS;
    // Set "rendered_texture" as our colour attachement #0
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rendered_texture, 0); GL_CHECK_ERRORS;
    // Set the list of draw buffers
    GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, DrawBuffers); GL_CHECK_ERRORS; // "1" is the size of DrawBuffers
    die(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE,
        "Framebuffer create failed");

    glGenVertexArrays(1, &quad_vertexArrayObject); GL_CHECK_ERRORS;
    glBindVertexArray(quad_vertexArrayObject); GL_CHECK_ERRORS;
}

void TexturedWall::init_wall_draw() {
    // PROGRAM
    std::unordered_map<GLenum, std::string> shaders;
    shaders[GL_VERTEX_SHADER]   = "texture_vertex.glsl";
    shaders[GL_FRAGMENT_SHADER] = "texture_fragment.glsl";
    texture_program = new ShaderProgram(shaders); GL_CHECK_ERRORS;

    // POSITION
    float d = -1;
    float e = 0.02;
    float n = (N_water - 1) * 0.01;
    float m = (N_water - 4) * 0.01;
    float triangle_pos0[] =
    {
        0, d, 0,
        0, d, n,
        n, d, 0,
        n, d, 0,
        0, d, n,
        n, d, n
    };
    float triangle_pos1[] =
    {
        0, d, 0,
        0, 0, 0,
        n, d, 0,
        n, d, 0,
        0, 0, 0,
        n, 0, 0
    };
    float triangle_pos2[] =
    {
        m, d, 0,
        m, 0, 0,
        m, d, n,
        m, d, n,
        m, 0, 0,
        m, 0, n
    };
    float triangle_pos3[] =
    {
        0, d, n,
        0, 0, n,
        n, d, n,
        n, d, n,
        0, 0, n,
        n, 0, n
    };
    float triangle_pos4[] =
    {
        0, d, 0,
        0, 0, 0,
        0, d, n,
        0, d, n,
        0, 0, 0,
        0, 0, n
    };
    float *triangle_pos_arr[] =
    {
        triangle_pos0,
        triangle_pos1,
        triangle_pos2,
        triangle_pos3,
        triangle_pos4
    };
    float *triangle_pos = triangle_pos_arr[obj_idx];

    // Create the buffer object for verticies' position
    glGenBuffers(1, &wall_vertexBufferObject); GL_CHECK_ERRORS;
    glBindBuffer(GL_ARRAY_BUFFER, wall_vertexBufferObject); GL_CHECK_ERRORS;
    glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(*triangle_pos), (GLfloat *)triangle_pos, GL_STATIC_DRAW); GL_CHECK_ERRORS;

    // Smth weird
    glGenVertexArrays(1, &wall_vertexArrayObject); GL_CHECK_ERRORS;
    glBindVertexArray(wall_vertexArrayObject); GL_CHECK_ERRORS;


    // TEXTURE IMAGE
    stone_texture.load("granite.bmp");
    //N_texture = stone_texture.n_cols;

    glGenTextures(1, &g_stone); GL_CHECK_ERRORS;
    glBindTexture(GL_TEXTURE_2D, g_stone); GL_CHECK_ERRORS;
    // Transfer texture from stone_texture to g_texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, N_texture,
        N_texture, 0, GL_RGB, GL_UNSIGNED_BYTE, stone_texture.image); GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GL_CHECK_ERRORS;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Resultant texture
    glGenTextures(1, &wall_texture);
    glBindTexture(GL_TEXTURE_2D, wall_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, N_texture, N_texture, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // TEXTURE COORDINATES
    const GLfloat tri_uv[] = {
        0, 0,
        0, 1,
        1, 0,
        1, 0,
        0, 1,
        1, 1
    };

    glGenBuffers(1, &g_texture_uv); GL_CHECK_ERRORS;
    glBindBuffer(GL_ARRAY_BUFFER, g_texture_uv); GL_CHECK_ERRORS;
    glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(*tri_uv), tri_uv, GL_STATIC_DRAW); GL_CHECK_ERRORS;

    MatrixID = glGetUniformLocation(texture_program->GetProgram(), "MVP"); GL_CHECK_ERRORS;
    g_sampler = glGetUniformLocation(texture_program->GetProgram(), "myTextureSampler"); GL_CHECK_ERRORS;
}

void TexturedWall::refract_all() {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, curSSbo); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bottom_tri); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bottom_pos); GL_CHECK_ERRORS;
    bottom_all_tri_program->StartUseShader();
    glUniform1ui(bottom_all_tri_N, N_water); GL_CHECK_ERRORS;
    glUniform3f(bottom_all_tri_light, -3, 2, -1); GL_CHECK_ERRORS;
    glDispatchCompute((N_water - 1) * (N_water - 1) / WORK_GROUP_SIZE + 1, 1, 1); GL_CHECK_ERRORS;
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); GL_CHECK_ERRORS;
    bottom_all_tri_program->StopUseShader();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bottom_tri); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bottom_pos); GL_CHECK_ERRORS;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, floor0->bottom_tri); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, floor0->bottom_pos); GL_CHECK_ERRORS;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, wall1->bottom_tri); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, wall1->bottom_pos); GL_CHECK_ERRORS;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, wall2->bottom_tri); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, wall2->bottom_pos); GL_CHECK_ERRORS;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, wall3->bottom_tri); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, wall3->bottom_pos); GL_CHECK_ERRORS;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, wall4->bottom_tri); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, wall4->bottom_pos); GL_CHECK_ERRORS;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, ball5->bottom_tri); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, ball5->bottom_pos); GL_CHECK_ERRORS;
    bottom_separator_program->StartUseShader();
    glUniform1ui(bottom_separator_N, N_water); GL_CHECK_ERRORS;
    glDispatchCompute((N_water - 1) * (N_water - 1) / WORK_GROUP_SIZE + 1, 1, 1); GL_CHECK_ERRORS;
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); GL_CHECK_ERRORS;
    bottom_separator_program->StopUseShader();
}

void TexturedWall::refract() {
    // Compute area of bottom triangles
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, curSSbo); GL_CHECK_ERRORS;
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bottom_tri); GL_CHECK_ERRORS;
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bottom_pos); GL_CHECK_ERRORS;
    // bottom_tri_program->StartUseShader();
    // glUniform1ui(bottom_tri_N, N_water); GL_CHECK_ERRORS;
    // glUniform3f(bottom_tri_light, -3, 2, -1); GL_CHECK_ERRORS;
    // glDispatchCompute((N_water - 1) * (N_water - 1) / WORK_GROUP_SIZE + 1, 1, 1); GL_CHECK_ERRORS;
    // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); GL_CHECK_ERRORS;
    // bottom_tri_program->StopUseShader();

    // Average the area on verticies
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bottom_tri); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bottom_vert); GL_CHECK_ERRORS;
    bottom_vert_program->StartUseShader();
    glUniform1ui(bottom_vert_N, N_water); GL_CHECK_ERRORS;
    glDispatchCompute(N_water * N_water / WORK_GROUP_SIZE, 1, 1); GL_CHECK_ERRORS;
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); GL_CHECK_ERRORS;
    bottom_vert_program->StopUseShader();

    // Fill triangles on the bottom - pos and color
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bottom_pos); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bottom_tri); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bottom_vert); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, draw_pos); GL_CHECK_ERRORS;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, draw_col); GL_CHECK_ERRORS;
    bottom_fill_program->StartUseShader();
    glUniform1ui(bottom_fill_N, N_water); GL_CHECK_ERRORS;
    glDispatchCompute((N_water - 1) * (N_water - 1) / WORK_GROUP_SIZE, 1, 1); GL_CHECK_ERRORS;
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); GL_CHECK_ERRORS;
    bottom_fill_program->StopUseShader();
}

void TexturedWall::render() {
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render to our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer); GL_CHECK_ERRORS;
    glViewport(0, 0, N_texture, N_texture);  GL_CHECK_ERRORS;

    // Clear the screen
    glClearColor(0.0, 0.0, 0.0, 0.0f); GL_CHECK_ERRORS;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); GL_CHECK_ERRORS;



    render_program->StartUseShader();
    glBindVertexArray(quad_vertexArrayObject); GL_CHECK_ERRORS;

    GLuint vertexLocation = 0;
    glBindBuffer(GL_ARRAY_BUFFER, draw_pos); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(vertexLocation); GL_CHECK_ERRORS;
    glVertexAttribPointer(
        vertexLocation, // Location identical to layout in the shader
        4,              // Components in the attribute (3 coordinates)
        GL_FLOAT,       // Type of components
        GL_FALSE,       // Normalization
        0,              // Stride: byte size of structure
        0               // Offset (in bytes) of attribute inside structure
    ); GL_CHECK_ERRORS;

    GLuint colorLocation = 1;
    glBindBuffer(GL_ARRAY_BUFFER, draw_col); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(colorLocation); GL_CHECK_ERRORS;
    glVertexAttribPointer(
        colorLocation, // Location identical to layout in the shader
        4,              // Components in the attribute (3 coordinates)
        GL_FLOAT,       // Type of components
        GL_FALSE,       // Normalization
        0,              // Stride: byte size of structure
        0               // Offset (in bytes) of attribute inside structure
    ); GL_CHECK_ERRORS;

    // Draw the triangles !
	glDrawArrays(GL_TRIANGLES, 0, (N_water - 1) * (N_water - 1) * 2 * 3);       GL_CHECK_ERRORS;  // The last parameter of glDrawArrays is equal to VertexShader invocations

    render_program->StopUseShader();



    glBindFramebuffer(GL_FRAMEBUFFER, 0); GL_CHECK_ERRORS;
    glDisable(GL_BLEND);
}

void TexturedWall::wall_draw() {
    blend_program->StartUseShader();
    glBindImageTexture(5, g_stone, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(6, rendered_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    GLuint N_texture_ID = glGetUniformLocation(blend_program->GetProgram(), "N_texture"); GL_CHECK_ERRORS;
    glUniform1ui(N_texture_ID, N_texture); GL_CHECK_ERRORS;

    glDispatchCompute(N_texture * N_texture / WORK_GROUP_SIZE, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    blend_program->StopUseShader();




    texture_program->StartUseShader();
    glBindVertexArray(wall_vertexArrayObject); GL_CHECK_ERRORS;

    GLuint vertexLocation = 0;
    glBindBuffer(GL_ARRAY_BUFFER, wall_vertexBufferObject); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(vertexLocation); GL_CHECK_ERRORS;
    glVertexAttribPointer(
        vertexLocation, // Location identical to layout in the shader
        3,              // Components in the attribute (3 coordinates)
        GL_FLOAT,       // Type of components
        GL_FALSE,       // Normalization
        0,              // Stride: byte size of structure
        0               // Offset (in bytes) of attribute inside structure
    ); GL_CHECK_ERRORS;


    GLuint uvLocation = 1;
    glBindBuffer(GL_ARRAY_BUFFER, g_texture_uv); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(uvLocation); GL_CHECK_ERRORS;
    glVertexAttribPointer(
        uvLocation,     // Location identical to layout in the shader
        2,              // Components in the attribute (3 coordinates)
        GL_FLOAT,       // Type of components
        GL_FALSE,       // Normalization
        0,              // Stride: byte size of structure
        0               // Offset (in bytes) of attribute inside structure
    );
    GL_CHECK_ERRORS;

    glm::mat4 mvp = MVP();
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]); GL_CHECK_ERRORS;


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rendered_texture);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, g_stone);

    glUniform1i(g_sampler, 0);

    GLuint g_sampler2 = glGetUniformLocation(texture_program->GetProgram(), "myTextureSampler2"); GL_CHECK_ERRORS;
    glUniform1i(g_sampler2, 1);

    glDrawArrays(GL_TRIANGLES, 0, 6);       GL_CHECK_ERRORS;  // The last parameter of glDrawArrays is equal to VertexShader invocations

    texture_program->StopUseShader();
}
