#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
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

#define ID_CUBE 1
#define ID_CRATE 2

#define WATER_TEX_W 1280
#define WATER_TEX_H 720

struct Block {
	float x;
	float y;
	float z;
	int kind;

	glm::mat4 model_matrix;
};

struct Player {
	float x = 0.0;
	float y = 1.0;
	float z = 0.0;

	Texture *texture;
	Model *model;
};

struct World {
	std::vector<Block> blocks;
};

struct Water {
	GLuint frame_buffer;
	GLuint frame_buffer_texture;

	Model *model;
	Shader *shader;
};

struct Camera {
	float x = 0.0;
	float z = 0.0;

	glm::mat4 view_matrix;
};

struct Platformer {
	PositionalLight *light;
	Shader *shader;
	Camera camera;
	World world;
	Player player;
	Water water;

	int width;
	int height;
	
	glm::mat4 proj_mat;
	std::unordered_map<int, Texture *> texture_atlas;
	std::unordered_map<int, Model *> model_atlas;
};

const int world_size_x = 10;
const int world_size_z = 10;
const int world_size_x_half = world_size_x / 2;
const int world_size_z_half = world_size_z / 2;

const float water_y = 0.5;
const float water_vertices[] = {
	-world_size_x_half, water_y, -world_size_z_half,
	-world_size_x_half, water_y, world_size_z + world_size_z_half,
	world_size_x + world_size_x_half, water_y, world_size_z + world_size_z_half,
	world_size_x + world_size_x_half, water_y, -world_size_z_half,
	-world_size_x_half, water_y, -world_size_z_half,
	world_size_x + world_size_x_half, water_y, world_size_z + world_size_z_half,
};

const float water_tex_coords[] = {
	0.0, 0.0,
	0.0, 1.0,
	1.0, 1.0,
	1.0, 0.0,
	0.0, 0.0,
	1.0, 1.0,
};

const float player_size = 0.5;
const float player_size_half = player_size / 2;

static bool keys_down[256];

static Platformer *global_platformer;

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

float lerp(float min, float max, float am) {
	return min + (max - min) * am;
}

PositionalLight *get_white_light(glm::vec3 pos) {
	return new PositionalLight(pos, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f), glm::vec4(1.0f));
}

void frame_buffer_resize(GLFWwindow *window, int nwidth, int nheight) {
	glViewport(0, 0, nwidth, nheight);
	global_platformer->proj_mat = glm::perspective(1.0472f, (float)nwidth / (float)nheight, 0.1f, 1000.0f);
	global_platformer->width = nwidth;
	global_platformer->height = nheight;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	keys_down[key] = action != GLFW_RELEASE;
}

void unbind_water_frame_buffer(Platformer *platformer) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, platformer->width, platformer->height);
}

void create_water_frame_buffer(Platformer *platformer, Water *water) {
	water->frame_buffer = create_frame_buffer();
	water->frame_buffer_texture = create_texture_attachment(WATER_TEX_W, WATER_TEX_H);
	unbind_water_frame_buffer(platformer);
}

void bind_water_frame_buffer(Platformer *platformer, Water *water) {
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, water->frame_buffer);
	glViewport(0, 0, WATER_TEX_W, WATER_TEX_H);
}

GLFWwindow *create_window() {
	if (!glfwInit()) {
		die("Failed to initialize GLFW!");
	}
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_SAMPLES, 4);

	GLFWwindow *window = glfwCreateWindow(800, 600, "Platformer", 0, 0);

	glfwMakeContextCurrent(window);

	// vsync
	// glfwSwapInterval(1);

	if (glewInit() != GLEW_OK) {
		die("Failed to initialize GLEW!");
	}

	glfwSetFramebufferSizeCallback(window, frame_buffer_resize);
	glfwSetKeyCallback(window, key_callback);

	glfwShowWindow(window);

	return window;
}

Block create_block(float x, float y, float z, int kind) {
	Block block;
	block.x = x;
	block.y = y;
	block.z = z;
	block.kind = kind;

	block.model_matrix = glm::identity<glm::mat4>();
	block.model_matrix = glm::translate(block.model_matrix, glm::vec3(block.x, block.y, block.z));

	return block;
}

void init(Platformer *platformer, GLFWwindow *window) {
	platformer->shader = new Shader("resources/shader/vert.glsl", "resources/shader/frag.glsl");
	platformer->light = get_white_light(glm::vec3(3, 8, 3));

	Texture *color_palette = new Texture("resources/textures/colors.png");
	platformer->texture_atlas[ID_CUBE] = new Texture("resources/textures/cube.png");
	platformer->texture_atlas[ID_CRATE] = color_palette;

	platformer->model_atlas[ID_CUBE] = load_obj_file("resources/models/cube.obj");
	platformer->model_atlas[ID_CRATE] = load_obj_file("resources/models/crate.obj");

	platformer->player.model = load_obj_file("resources/models/player.obj");
	platformer->player.texture = color_palette;

	platformer->water.shader = new Shader("resources/shader/waterVert.glsl", "resources/shader/waterFrag.glsl");
	platformer->water.model = new Model((float *)&water_vertices[0], 18, (float *)&water_tex_coords[0], 12, 0, 0);
	create_water_frame_buffer(platformer, &platformer->water);

	int fwidth, fheight;
	glfwGetFramebufferSize(window, &fwidth, &fheight);
	glViewport(0, 0, fwidth, fheight);
	platformer->width = fwidth;
	platformer->height = fheight;
	platformer->proj_mat = glm::perspective(1.0472f, (float)fwidth / (float)fheight, 0.1f, 1000.0f);

	glClearColor(0.1, 0.6, 1.0, 1.0);
	
	platformer->shader->use();
	platformer->light->install(platformer->shader);

	glActiveTexture(GL_TEXTURE0);

	glEnable(GL_MULTISAMPLE);

	for (int z = 0; z < world_size_z; ++z) {
		for (int x = 0; x < world_size_x; ++x) {
			Block block = create_block(x, 0, z, ID_CUBE);
			platformer->world.blocks.push_back(block);
		}
	}

	platformer->world.blocks.push_back(create_block(8, 1, 8, ID_CRATE));
	platformer->world.blocks.push_back(create_block(4, 1, 5, ID_CRATE));
	platformer->world.blocks.push_back(create_block(7, 1, 2, ID_CRATE));
}

void update(Platformer *platformer) {
	Player *player = &platformer->player;
	Camera *camera = &platformer->camera;

	if (keys_down[GLFW_KEY_D]) {
		player->x += 0.15;
	}
	
	if (keys_down[GLFW_KEY_A]) {
		player->x -= 0.15;
	}

	if (keys_down[GLFW_KEY_S]) {
		player->z += 0.15;
	}

	if (keys_down[GLFW_KEY_W]) {
		player->z -= 0.15;
	}

	if (player->x <= player_size_half) {
		player->x = player_size_half;
	}

	if (player->z <= player_size_half) {
		player->z = player_size_half;
	}

	if (player->x >= world_size_x - player_size_half) {
		player->x = world_size_x - player_size_half;
	}

	if (player->z >= world_size_z - player_size) {
		player->z = world_size_z - player_size;
	}
		
	camera->x = lerp(camera->x, player->x, 0.1);
	camera->z = lerp(camera->z, player->z / 2, 0.1);

	camera->view_matrix = glm::identity<glm::mat4>();
	camera->view_matrix = glm::rotate(camera->view_matrix, 3.14f / 4.0f, glm::vec3(1.0, 0.0, 0.0));
	camera->view_matrix = glm::translate(camera->view_matrix, glm::vec3(-platformer->camera.x, -8, -10 - platformer->camera.z));
}

void render_world(Platformer *platformer) {
	Shader *shader = platformer->shader;
	World *world = &platformer->world;

	shader->use();
	shader->load_mat4("proj_matrix", platformer->proj_mat);
	shader->load_mat4("view_matrix", platformer->camera.view_matrix);

	shader->load_vec4("block_color", glm::vec4(0.5, 0.3, 0.0, 1.0));
	for (int i = 0; i < world->blocks.size(); ++i) {
		Block block = world->blocks[i];
		platformer->texture_atlas[block.kind]->bind();

		shader->load_mat4("model_matrix", block.model_matrix);

		platformer->model_atlas[block.kind]->render();
	}
}

void render_player(Platformer *platformer) {
	Player *player = &platformer->player;
	Shader *shader = platformer->shader;

	glm::mat4 model_matrix(1);
	model_matrix = glm::translate(model_matrix, glm::vec3(player->x - player_size_half, player->y, player->z - player_size));
	shader->load_mat4("model_matrix", model_matrix);

	shader->load_vec4("block_color", glm::vec4(1.0));
	player->texture->bind();
	player->model->render();
}

void render_water(Platformer *platformer) {
	Water *water = &platformer->water;
	Shader *waterShader = water->shader;

	waterShader->use();
	waterShader->load_mat4("proj_matrix", platformer->proj_mat);
	waterShader->load_mat4("view_matrix", platformer->camera.view_matrix);

	glBindTexture(GL_TEXTURE_2D, water->frame_buffer_texture);

	water->model->render();
}

void render(Platformer *platformer) {
	Water *water = &platformer->water;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glFrontFace(GL_CW);

	glEnable(GL_CLIP_DISTANCE0);
	bind_water_frame_buffer(platformer, water);
	glClear(GL_COLOR_BUFFER_BIT);
	render_world(platformer);
	unbind_water_frame_buffer(platformer);
	glDisable(GL_CLIP_DISTANCE0);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	render_world(platformer);
	render_player(platformer);
	render_water(platformer);
}

int main() {
	GLFWwindow *window = create_window();
	Platformer platformer;

	global_platformer = &platformer;

	init(&platformer, window);

	int frames = 0;
	double last_fps = glfwGetTime();


	double last = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		double current = glfwGetTime();
		double delta = current - last;

		if (delta >= 1.0 / 60.0) {
			update(&platformer);

			last = current;
		}

		frames++;
		if (current - last_fps >= 1.0) {
			std::cout << frames << " FPS\n";
			frames = 0;
			last_fps = current;
		}

		render(&platformer);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}