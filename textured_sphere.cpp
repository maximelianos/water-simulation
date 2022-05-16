// Internal includes
#include "textured_sphere.h"

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

TexturedSphere::TexturedSphere(int obj_idx) : obj_idx(obj_idx) {
    glGenVertexArrays(1, &g_vertexArrayObject); GL_CHECK_ERRORS; // alloc!

    // Compute shaders:)
    std::unordered_map<GLenum, std::string> compute_shaders;

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

    bottom_vert_N = glGetUniformLocation(bottom_vert_program->GetProgram(), "N_water"); GL_CHECK_ERRORS;

    bottom_fill_N = glGetUniformLocation(bottom_fill_program->GetProgram(), "N_water"); GL_CHECK_ERRORS;

    init_renderer();
    init_wall_draw();
}

void TexturedSphere::init_renderer() {
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

glm::vec3 rotate_z(glm::vec3 v, float angle) {
    glm::vec3 rotation_axis = glm::vec3(0, 0, 1);
    glm::mat4 rotate_m = glm::rotate(angle, rotation_axis);
    return glm::mat3(rotate_m) * v;
}

void TexturedSphere::init_wall_draw() {
    // PROGRAM
    std::unordered_map<GLenum, std::string> shaders;
    shaders[GL_VERTEX_SHADER]   = "texture_vertex.glsl";
    shaders[GL_FRAGMENT_SHADER] = "texture_fragment.glsl";
    texture_program = new ShaderProgram(shaders); GL_CHECK_ERRORS;

    // POSITION
    printf("TRIs %d\n", n_verticies / 9);
    float tri_pos[n_verticies];
    float tri_uv[n_verticies / 3 * 2];

    int flow_pos = 0;
    int flow_uv = 0;
    float angle_v = PI / n_v;
    float angle_h = 2 * PI / n_h;
    float r = 0.3;
    for (int i_psi = -n_v / 2 + 1; i_psi <= n_v / 2; i_psi++) {
        for (int i_phi = 0; i_phi <= n_h - 1; i_phi++) {
            float phi, psi, r_h;

            int start_i = 0;
            int end_i = 6;
            if (i_psi == -n_v / 2 + 1) {
                start_i = 0;
                end_i = 3;
            } else if (i_psi == n_v / 2) {
                start_i = 3;
                end_i = 6;
            }
            int v[] = {0, -1, 0, 0, -1, -1};
            int h[] = {0, 0, 1, 1, 0, 1};
            for (int i = start_i; i < end_i; i++) {
                psi = angle_v * (i_psi + v[i]);
                phi = angle_h * (i_phi + h[i]);
                r_h = cos(psi);
                glm::vec3 pos(
                    cos(phi) * r_h * r,
                    sin(psi) * r,
                    sin(phi) * r_h * r
                );
                pos = rotate_z(pos, -PI / 4);
                tri_pos[flow_pos++] = s_pos.x + pos.x; // x
                tri_pos[flow_pos++] = s_pos.y + pos.y; // y
                tri_pos[flow_pos++] = s_pos.z + pos.z; // z
                // tri_pos[flow_pos++] = s_pos.x + cos(phi) * r_h * r; // x
                // tri_pos[flow_pos++] = s_pos.y + sin(psi) * r; // y
                // tri_pos[flow_pos++] = s_pos.z + sin(phi) * r_h * r; // z

                tri_uv[flow_uv++] = phi / (2 * PI); // u
                tri_uv[flow_uv++] = psi / PI + 0.5; // v
            }
        }
    }

    // Create the buffer object for verticies' position
    glGenBuffers(1, &wall_vertexBufferObject); GL_CHECK_ERRORS;
    glBindBuffer(GL_ARRAY_BUFFER, wall_vertexBufferObject); GL_CHECK_ERRORS;
    glBufferData(GL_ARRAY_BUFFER, n_verticies * sizeof(*tri_pos), (GLfloat *)tri_pos, GL_STATIC_DRAW); GL_CHECK_ERRORS;

    // Smth weird
    glGenVertexArrays(1, &wall_vertexArrayObject); GL_CHECK_ERRORS;
    glBindVertexArray(wall_vertexArrayObject); GL_CHECK_ERRORS;


    // TEXTURE IMAGE
    stone_texture.load("ball.bmp");
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
    // coors in tri_uv

    glGenBuffers(1, &g_texture_uv); GL_CHECK_ERRORS;
    glBindBuffer(GL_ARRAY_BUFFER, g_texture_uv); GL_CHECK_ERRORS;
    glBufferData(GL_ARRAY_BUFFER, n_verticies / 3 * 2 * sizeof(*tri_uv), tri_uv, GL_STATIC_DRAW); GL_CHECK_ERRORS;

    MatrixID = glGetUniformLocation(texture_program->GetProgram(), "MVP"); GL_CHECK_ERRORS;
    g_sampler = glGetUniformLocation(texture_program->GetProgram(), "myTextureSampler"); GL_CHECK_ERRORS;
}

void TexturedSphere::refract() {
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

void TexturedSphere::render() {
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

void TexturedSphere::wall_draw() {
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
    glUniform1i(g_sampler, 0);
    glDrawArrays(GL_TRIANGLES, 0, n_verticies / 3);       GL_CHECK_ERRORS;  // The last parameter of glDrawArrays is equal to VertexShader invocations

    texture_program->StopUseShader();
}
