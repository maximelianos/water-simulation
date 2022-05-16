#include "common.h"

namespace env_water {
    GLuint curSSbo = 0;
    WaterSurface *water_surface = nullptr;
    TexturedWall *common = nullptr;
    TexturedWall *floor0 = nullptr;
    TexturedWall *wall1 = nullptr;
    TexturedWall *wall2 = nullptr;
    TexturedWall *wall3 = nullptr;
    TexturedWall *wall4 = nullptr;
    TexturedSphere *ball5 = nullptr;

    GLuint floor_uv = 0;
}

var_env env;

void die(const int condition, const char *msg, void (* const exit_function)()) {
    // The exit_function default was already declared in common.h,
    // it will not compile if declared here again

	if (condition) {
		if (exit_function != nullptr) {
			exit_function();
		}
		printf("Exiting: %s\n", msg);
		exit(0);
	}
}

double get_wall_time() {
    struct timeval time;
    die(gettimeofday(&time, NULL), "Couldn't call gettimeofday");
    return (double)time.tv_sec + (double)time.tv_usec * 0.000001;
}

// ***********************************************************
// ********************** Image loading **********************
// ***********************************************************

Image::Image(int rows, int cols) {
		image = new unsigned char[rows * cols * COLOR_DEPTH];
		n_rows = rows;
		n_cols = cols;
	}

	void Image::load(const char *filename) {
		bitmap_image img(filename);
		n_rows = img.height();
		n_cols = img.width();
		image = new unsigned char[n_rows * n_cols * COLOR_DEPTH];
		for (int y = 0; y < n_rows; ++y) {
			for (int x = 0; x < n_cols; ++x) {
				rgb_t colour;

				img.get_pixel(x, y, colour);
				int idx = y * (n_cols * n_colors) + x * n_colors;
				image[idx] = colour.red;
				image[idx + 1] = colour.green;
				image[idx + 2] = colour.blue;
			}
		}
	}

	unsigned char& Image::operator()(int row, int col, int color) {
		return image[row * (n_cols * n_colors) + col * n_colors + color];
	}

	void Image::merge(Image &buf, int n_start, int n_end) {
		for (int row = n_start; row < n_end; row++) {
			for (int col = 0; col < n_cols; col++) {
				int idx = row * (n_cols * n_colors) + col * n_colors;
				image[idx] = buf.image[idx];
				image[idx + 1] = buf.image[idx + 1];
				image[idx + 2] = buf.image[idx + 2];
			}
		}
	}

	void Image::set(int row, int col, glm::vec3 color) {
		int idx = row * (n_cols * n_colors) + col * n_colors;
		image[idx] = int(color[0]);
		image[idx + 1] = int(color[1]);
		image[idx + 2] = int(color[2]);
	}

	void Image::set(int idx, glm::vec3 color) {
		image[idx] = color[0];
		image[idx + 1] = color[1];
		image[idx + 2] = color[2];
	}

	glm::vec3 Image::get(int row, int col) {
		int idx = row * (n_cols * n_colors) + col * n_colors;
		return glm::vec3(image[idx], image[idx + 1], image[idx + 2]);
	}

	void Image::blur() {
		float kernel[25] = {};
		float sigma = 1;
		for (int y = -2; y < 3; y++) {
			for (int x = -2; x < 3; x++) {
				float g = exp(-(x * x + y * y) / (2 * sigma * sigma));
				g = g / (2 * M_PI * sigma * sigma);
				kernel[(y + 2) * 5 + (x + 2)] = g;
			}
		}
		unsigned char *buf = new unsigned char[n_rows * n_cols * COLOR_DEPTH];
		for (int i = 2; i < n_rows - 2; i++) {
			for (int j = 2; j < n_cols - 2; j++) {
				for (int c = 0; c < 3; c++) {
					float tmp = 0;
					for (int y = -2; y < 3; y++) {
						for (int x = -2; x < 3; x++) {
							tmp += kernel[(y + 2) * 5 + (x + 2)] * image[(i + y) * (n_cols * n_colors) + (j + x) * n_colors + c];
						}
					}
					buf[i * (n_cols * n_colors) + j * n_colors + c] = tmp;
				}
			}
		}
		delete image;
		image = buf;
	}

	void Image::save(const std::string &path) {
		blur();

		bitmap_image bitmapbuf(n_cols, n_rows);
		bitmapbuf.clear();
		int idx = 0;
		for (int y = 0; y < n_rows; y++) {
			for (int x = 0; x < n_cols; x++) {
				rgb_t c;
				c.red = image[idx];
				c.green = image[idx + 1];
				c.blue = image[idx + 2];
				bitmapbuf.set_pixel(x, y, c);

				idx += n_colors;
			}
		}
		bitmapbuf.save_image(path);
    }

glm::mat4 MVP() {
    // Projection matrix : 90 Field of View, WIDTH:HEIGHT ratio, clipping planes
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), (float) WIDTH / (float) HEIGHT, 0.001f, 100.0f);

    glm::mat4 view = glm::lookAt(
        env.camera_pos, // Camera position in World Space
        env.camera_pos + env.forward_step, // Looks at the origin
        glm::vec3(0, 1, 0)  // Camera rotation, "upward direction" (set to 0,-1,0 to look upside-down)
    );

    glm::mat4 model = glm::mat4(1.0f);
    return projection * view * model;
}

float clamp(float x, float min_value, float max_value) {
    return x < min_value ? min_value : (x > max_value ? max_value : x);
}
