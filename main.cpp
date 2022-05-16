// Internal includes
#include "common.h" // Includes Graphics Library Math
#include "ShaderProgram.h"
#include "water_surface.h"
#include "textured_wall.h"

using env_water::N_water;
using env_water::W;

// Easy screen handling
#define GLFW_DLL
#include <GLFW/glfw3.h>



// ***********************************************************
// ********************** Global variables *******************
// ***********************************************************

int initGL()
{
	int res = 0;
	//грузим функции opengl через glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	std::cout << "Vendor: "   << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version: "  << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL: "     << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

	return 0;
}


// ***********************************************************
// ***************** Keyboard and mouse **********************
// ***********************************************************

// Detect keyboard presses
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    switch (key) {
        case GLFW_KEY_Q:
            if (action == GLFW_PRESS) {
                env.wireframe_water = !env.wireframe_water;
            }
            break;
        case GLFW_KEY_W:
            if (action == GLFW_PRESS) {
                env.w_pressed = true;
            } else if (action == GLFW_RELEASE) {
                env.w_pressed = false;
            }
            break;
        case GLFW_KEY_A:
            if (action == GLFW_PRESS) {
                env.a_pressed = true;
            } else if (action == GLFW_RELEASE) {
                env.a_pressed = false;
            }
            break;
        case GLFW_KEY_S:
            if (action == GLFW_PRESS) {
                env.s_pressed = true;
            } else if (action == GLFW_RELEASE) {
                env.s_pressed = false;
            }
            break;
        case GLFW_KEY_D:
            if (action == GLFW_PRESS) {
                env.d_pressed = true;
            } else if (action == GLFW_RELEASE) {
                env.d_pressed = false;
            }
            break;
        case GLFW_KEY_F1: // Static positions
            env.phi = 2.54; // Horizontal
            env.psi = 0.59; // Vertical
            env.camera_pos = glm::vec3(0.4, 0.57, 2.4);
            break;
        case GLFW_KEY_F2:
            env.phi = 3.21; // Horizontal
            env.psi = 0.19; // Vertical
            env.camera_pos = glm::vec3(1.57, -0.23, 2.1);
            break;
        case GLFW_KEY_F3:
            env.phi = -0.11; // Horizontal
            env.psi = 0.87; // Vertical
            env.camera_pos = glm::vec3(1.91, 0.73, -0.16);
            break;
        case GLFW_KEY_F4:
            env.phi = 1.0; // Horizontal
            env.psi = 0.612; // Vertical
            env.camera_pos = glm::vec3(0.795, 0.-0.03, 0.33);
            break;
    }
}

// Detect mouse movements
void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
    env.cursor_dir = glm::vec2(xpos, ypos) - env.cursor_pos;
    env.cursor_pos = glm::vec2(xpos, ypos);
    if (env.cursor_dir[0] < env.cursor_shock_thr && env.cursor_dir[1] < env.cursor_shock_thr) {
        env.phi -= env.cursor_dir[0] / env.screen_angle_unit;
        env.psi += env.cursor_dir[1] / env.screen_angle_unit;
        env.psi = clamp(env.psi, -M_PI / 2 + 0.01, M_PI / 2 - 0.01);
    }
    env.cursor_dir = glm::vec2(0, 0);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            glm::vec3 cur_point = env.camera_pos;
            float phi, psi, r_h, x, y, z, t, u, v;
            phi = env.phi;
            psi = -env.psi;
            r_h = cos(psi);
            x = sin(phi) * r_h;
            y = sin(psi);
            z = cos(phi) * r_h;
            glm::vec3 dir = glm::vec3(x, y, z);

            // Intersect with water surface, y = 0
            t = (0 - cur_point.y) / dir.y;
            x = cur_point.x + t * dir.x;
            z = cur_point.z + t * dir.z;

            env.dt_x = x;
            env.dt_z = z;
            env.time_pressed = get_wall_time();
            printf("CLICK x=%f z=%f phi=%f psi=%f\n\n", x, z, phi, psi);
        } else {
            double c = get_wall_time();
            env.dt_h = min(0.2, (c - env.time_pressed));
            printf("cur=%lf past=%lf dt_h=%f\n\n", c, env.time_pressed, env.dt_h);
            env.released = true;
        }
    }
}


// ***********************************************************
// ***** Camera movement, Model View Projection matrix *******
// ***********************************************************

void frame_key_update() {
    glm::vec3 rotation_axis = glm::vec3(0, 1, 0);
    glm::mat4 rotate_h = glm::rotate(env.phi, rotation_axis); // Horizontal rotation

    rotation_axis = glm::vec3(1, 0, 0);
    glm::mat4 rotate_v1 = glm::rotate(env.psi, rotation_axis); // Vertical rotation

    rotation_axis = glm::vec3(0, 0, 1);
    glm::mat4 rotate_v2 = glm::rotate(env.psi, rotation_axis); // Vertical rotation

    // WARNING: can we please use mat3 instead of cut-down mat4 here?
    env.forward_step = glm::mat3(rotate_h * rotate_v1) * env.origin_forward_step;
    env.right_step = glm::mat3(rotate_h) * env.origin_right_step;

    // Move camera with W, A, S, D
    if (env.w_pressed) {
        env.camera_pos += env.forward_step;
    }
    if (env.a_pressed) {
        env.camera_pos -= env.right_step; // Minus determined by trial-and-error
    }
    if (env.s_pressed) {
        env.camera_pos -= env.forward_step;
    }
    if (env.d_pressed) {
        env.camera_pos += env.right_step;
    }
}

int main(int argc, char** argv)
{
    // GLFW and openGL initialisation
    die(!glfwInit(), "!glfwInit()");

    glfwWindowHint(GLFW_SAMPLES, 4); // Antialiasing
	//запрашиваем контекст opengl версии 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL basic sample", nullptr, nullptr);
    //GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "My Title", glfwGetPrimaryMonitor(), nullptr);
    die(window == nullptr, "Failed to create GLFW window", glfwTerminate);

	glfwMakeContextCurrent(window);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    die(initGL() != 0, "initGl() != 0");

    //Reset any OpenGL errors which could be present for some reason
	GLenum gl_error = glGetError();
	while (gl_error != GL_NO_ERROR) {
		gl_error = glGetError();
    }

	//создание шейдерной программы из двух файлов с исходниками шейдеров
	//используется класс-обертка ShaderProgram
	std::unordered_map<GLenum, std::string> shaders;
	shaders[GL_VERTEX_SHADER]   = "vertex.glsl";
	shaders[GL_FRAGMENT_SHADER] = "fragment.glsl";
	ShaderProgram program(shaders); GL_CHECK_ERRORS;

    //glfwSwapInterval(0); // force 60 frames per second

    //Создаем и загружаем геометрию поверхности
    GLuint g_vertexBufferObject = 0;
    GLuint g_vertexArrayObject = 0;
    GLuint color_buffer = 0;

    // VERY IMPORTANT
    water_surface = new WaterSurface();
    //TexturedWall floor(FLOOR0);
    common = new TexturedWall(FLOOR0);
    floor0 = new TexturedWall(FLOOR0);
    wall1 = new TexturedWall(WALL1);
    wall2 = new TexturedWall(WALL2);
    wall3 = new TexturedWall(WALL3);
    wall4 = new TexturedWall(WALL4);
    ball5 = new TexturedSphere(BALL5);

    if (0) {
        float trianglePos[] =
        {
          -1.0f, -1.0f, 0.0f,
          1.0f,  -1.0f, 0.0f,
          0.0f,  +1.0f, 0.0f
        };

        // Create the buffer object for verticies' position
        glGenBuffers(1, &g_vertexBufferObject); GL_CHECK_ERRORS;
        glBindBuffer(GL_ARRAY_BUFFER, g_vertexBufferObject); GL_CHECK_ERRORS;
        glBufferData(GL_ARRAY_BUFFER, 3 * 3 * sizeof(GLfloat), (GLfloat *)trianglePos, GL_STATIC_DRAW); GL_CHECK_ERRORS;

        // Smth weird
        glGenVertexArrays(1, &g_vertexArrayObject); GL_CHECK_ERRORS;
        glBindVertexArray(g_vertexArrayObject); GL_CHECK_ERRORS;

        // Define the position as an attribute
        GLuint vertexLocation = 0;
        glBindBuffer(GL_ARRAY_BUFFER, g_vertexBufferObject); GL_CHECK_ERRORS;
        glEnableVertexAttribArray(vertexLocation); GL_CHECK_ERRORS;
        glVertexAttribPointer(
            vertexLocation, // Location identical to layout in the shader
            3,              // Components in the attribute (3 coordinates)
            GL_FLOAT,       // Type of components
            GL_FALSE,       // Normalization
            0,              // Stride: byte size of structure
            0               // Offset (in bytes) of attribute inside structure
        );
        GL_CHECK_ERRORS;


        // The same process for the color attribute of verticies
        float colors[] =
        {
            0.5f, .0f, 0.5f,
            0.0f, .0f, 0.4f,
            0.1f, 1.0f, 1.0f
        };
        for (int i = 0; i < 3; i++) {
            colors[i * 3 + 0] = clamp(0.1 + trianglePos[i * 3 + 1], 0, 1);
            colors[i * 3 + 1] = 0.2;
            colors[i * 3 + 2] = 0.3;
        }

        GLuint color_buffer = 0;
        glGenBuffers(1, &color_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, color_buffer); GL_CHECK_ERRORS;
        glBufferData(GL_ARRAY_BUFFER, 3 * 3 * sizeof(GLfloat), (GLfloat *)colors, GL_STATIC_DRAW); GL_CHECK_ERRORS;

        GLuint colorLocation = 1;
        glBindBuffer(GL_ARRAY_BUFFER, color_buffer); GL_CHECK_ERRORS;
        glEnableVertexAttribArray(colorLocation); GL_CHECK_ERRORS;
        glVertexAttribPointer(
            colorLocation, // Location identical to layout in the shader
            3,              // Components in the attribute (3 coordinates)
            GL_FLOAT,       // Type of components
            GL_FALSE,       // Normalization
            0,              // Stride: byte size of structure
            0               // Offset (in bytes) of attribute inside structure
        );
        GL_CHECK_ERRORS;



        glBindVertexArray(0);
    }

    // Locate MVP matrix inside the vertex shader
    // Only during the initialisation
    GLuint MatrixID = glGetUniformLocation(program.GetProgram(), "MVP"); GL_CHECK_ERRORS;

    {
        glGenBuffers(1, &g_vertexBufferObject); GL_CHECK_ERRORS;
        glGenVertexArrays(1, &g_vertexArrayObject); GL_CHECK_ERRORS;
        glGenBuffers(1, &color_buffer);
    }

	//цикл обработки сообщений и отрисовки сцены каждый кадр
    double last_refresh = get_wall_time();
    const double frame_interval = 1.0 / 30.0;

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //glDisable(GL_CULL_FACE);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //texture_sq.g_texture = floor.rendered_texture;

    int simulation_wait = 0;

    int frames = 0;
    float render_time = 0;
	while (!glfwWindowShouldClose(window))
	{
        last_refresh = get_wall_time();

        // Prepare to draw...
        for (int i = 0; i < 30; i++) {
		    glfwPollEvents();
        }
        frame_key_update();

        // Update height map
        simulation_wait++;
        if (simulation_wait == 1) {
            simulation_wait = 0;
            water_surface->simulation_step();
        }
        if (env.released) {
            water_surface->disturb_surface();
            env.released = false;
        }

        // очистка и заполнение экрана цветом
        glViewport  (0, 0, WIDTH, HEIGHT); GL_CHECK_ERRORS;
        glClearColor(env.bkgcol[0], env.bkgcol[1], env.bkgcol[2], 0.0f); GL_CHECK_ERRORS;
        glClear     (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); GL_CHECK_ERRORS;



        // Draw!
        program.StartUseShader();                           GL_CHECK_ERRORS;
        // Transfer MVP for each object on the scene
        glm::mat4 mvp = MVP();
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]); GL_CHECK_ERRORS;
        program.StopUseShader();

        floor0->wall_draw();
        wall1->wall_draw();
        wall2->wall_draw();
        wall3->wall_draw();
        wall4->wall_draw();
        ball5->wall_draw();
        if (env.wireframe_water) {
            water_surface->wireframe_draw();
        } else {
            water_surface->draw();
        }

        common->refract_all();
        floor0->refract();
        floor0->render();
        wall1->refract();
        wall1->render();
        wall2->refract();
        wall2->render();
        wall3->refract();
        wall3->render();
        wall4->refract();
        wall4->render();
        ball5->refract();
        ball5->render();



        // Sleeping (constant frame rate), output
        frames++;
        double cur_time = get_wall_time();
        render_time += cur_time - last_refresh;
        //printf("Total render time %lf\n", cur_time - last_refresh);

        double cur_interval = cur_time - env.output_time;
        if (cur_interval > env.output_interval) {
            env.output_time = cur_time;
            printf("TIME %lf CAMERA POS %lf %lf %lf\n", cur_time, env.camera_pos[0], env.camera_pos[1], env.camera_pos[2]);
            printf("PHI = %lf PSI = %lf\n", env.phi, env.psi);
            printf("FRAME INTERVAL %lf\n", frame_interval);
            //printf("RENDER TIME %lf\n", cur_time - last_refresh);
            printf("RENDER TIME %lf\n", render_time / frames);
            render_time = 0;
            frames = 0;
        }

        cur_interval = cur_time - last_refresh;
        //printf("Sleep for %lf\n", (frame_interval - cur_interval));
        if (cur_interval < frame_interval) { // Sleep until beginning of next frame
            usleep((frame_interval - cur_interval) * 1000000);
        } else {
            printf("Negative sleep: %lf\n", frame_interval - cur_interval);
        }

        glfwSwapBuffers(window);
	}

	//очищаем vboи vao перед закрытием программ
	glDeleteVertexArrays(1, &g_vertexArrayObject);
    glDeleteBuffers(1,      &g_vertexBufferObject);
    water_surface->free();

	glfwTerminate();

	return 0;
}
