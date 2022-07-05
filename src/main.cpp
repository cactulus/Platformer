#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "platformer.h"

#include "log.cpp"
#include "gfx.cpp"

static glm::mat4 proj_mat, view_mat;

static PositionalLight *light;
static Shader *shader;
static Texture *texture;

bool checkOpenGLError() {
	bool found_error = false;
	int glErr = glGetError();
	while (glErr != GL_NO_ERROR) {
		std::cout << "glError: " << glErr << "\n";
		found_error = true;
		glErr = glGetError();
	}
	return found_error;
}

std::string read_file(const char *file_path) {
	std::string content, line = "";
	std::ifstream file_stream(file_path, std::ios::in);

	while (!file_stream.eof()) {
		std::getline(file_stream, line);
		content.append(line).append("\n");
	}
	file_stream.close();
	return content;
}

float to_radians(float degrees) {
	return (degrees * 2.0f * 3.14159f) / 360.0f;
}

PositionalLight *get_white_light(glm::vec3 pos) {
	return new PositionalLight(pos, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f), glm::vec4(1.0f));
}

void frame_buffer_resize(GLFWwindow *window, int nwidth, int nheight) {
	glViewport(0, 0, nwidth, nheight);
	proj_mat = glm::perspective(1.0472f, (float)nwidth / (float)nheight, 0.1f, 1000.0f);
}

GLFWwindow *create_window() {
	if (!glfwInit()) {
		die("Failed to initialize GLFW!");
	}
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	GLFWwindow *window = glfwCreateWindow(800, 600, "Platformer", 0, 0);

	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK) {
		die("Failed to initialize GLEW!");
	}

	/* vSync */
	glfwSwapInterval(1);

	glfwSetFramebufferSizeCallback(window, frame_buffer_resize);
	glfwShowWindow(window);

	return window;
}

void init() {
	shader = new Shader("resources/shader/vert.glsl", "resources/shader/frag.glsl");
	texture = new Texture("resources/textures/colors.png");
	light = get_white_light(glm::vec3(5.0f, 2.0f, 2.0f));
}

void render() {
	shader->use();
	shader->load_mat4("proj_matrix", proj_mat);

	view_mat = glm::lookAt(glm::vec3(0, 3, 3), glm::vec3(0), glm::vec3(0, 1, 0));
	shader->load_mat4("view_matrix", view_mat);

	light->install(shader);

	glActiveTexture(GL_TEXTURE0);
	texture->bind();
}

int main() {
	GLFWwindow *window = create_window();

	init();

	while (!glfwWindowShouldClose(window)) {
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	//	render();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}