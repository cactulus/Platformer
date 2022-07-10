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
#include <glm/gtx/string_cast.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "platformer.h"

#include "log.cpp"
#include "gfx.cpp"
#include "model.cpp"

#define ID_CUBE 1
#define ID_CRATE 2

#define WATER_TEX_W 1280
#define WATER_TEX_H 720

struct Player {
	float x = 7.0;
	float y = 1.0;
	float z = 7.0;

	float last_x = x;
	float last_z = z;

	Texture texture;
	ComplexModel *model;
};

struct Block {
	float initial_x;
	float initial_y;
	float initial_z;

	float x;
	float y;
	float z;

	int kind;

	float target_y;
	glm::mat4 model_matrix;

	Block(float _x, float _y, float _z, int _kind) {
		initial_x = _x;
		initial_y = _y;
		initial_z = _z;
		x = _x;
		y = _y;
		z = _z;
		kind = _kind;

		target_y = _y;
		model_matrix = glm::translate(glm::mat4(1.0), glm::vec3(x, y, z));
	}

	bool colliding(Player *player);

	bool pushable() {
		return kind != ID_CUBE;
	}

	bool equals(int ox, int oy, int oz) const {
		return round(x) == ox && round(y) == oy && round(z) == oz;
	}

	bool equalsf(float ox, float oy, float oz) const {
		return x == ox && y == oy && z == oz;
	}
};

struct World {
	std::vector<Block> blocks;
	
	bool has_block(int x, int y, int z) {
		for (const Block &block : blocks) {
			if (block.equals(x, y, z)) {
				return true;
			}
		}
		return false;
	}
};

struct Water {
	GLuint frame_buffer;
	GLuint frame_buffer_texture;

	Texture water_texture;
	SimpleModel *model;
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
	std::unordered_map<int, Texture> texture_atlas;
	std::unordered_map<int, ComplexModel *> model_atlas;
};

const int world_size_x = 32;
const int world_size_z = 16;

const float water_y = 0.7;
const float water_vertices[] = {
   0, water_y, 1,
   1, water_y, 1,
   0, water_y, 0,

   0, water_y, 0,
   1, water_y, 1,
   1, water_y, 0
};

const float player_size = 0.5;
const float player_size_half = player_size / 2;
const float player_speed = 0.08;

const float block_size = 1.0;

static bool keys_down[512];

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

bool Block::colliding(Player *player) {
	if (player->y != y) {
		return false;
	}

	float block_x2 = x + block_size;
	float block_z2 = z + block_size;
	float px = player->x;
	float pz = player->z;
	float player_x2 = px + player_size;
	float player_z2 = pz + player_size;

	return x < player_x2 && block_x2 > player->x &&
		z < player_z2 && block_z2 > player->z;
}

bool block_collisions(World *world, Block *block) {
	for (int i = 0; i < world->blocks.size(); ++i) {
		Block *other = &world->blocks[i];

		if (other->y != block->y || block->equalsf(other->x, other->y, other->z)) {
			continue;
		}

		float block_x2 = block->x + block_size;
		float block_z2 = block->z + block_size;
		float ox = other->x;
		float oz = other->z;
		float other_x2 = ox + block_size;
		float other_z2 = oz + block_size;

		if (block->x < other_x2 && block_x2 > ox &&
			block->z < other_z2 && block_z2 > oz) {
			return true;
		}
	}

	return false;
}

void init(Platformer *platformer, GLFWwindow *window) {
	platformer->shader = new Shader("resources/shader/vert.glsl", "resources/shader/frag.glsl");
	platformer->light = get_white_light(glm::vec3(world_size_x / 2, 12, world_size_z / 2));

	Texture color_palette = load_texture("resources/textures/colors.png");
	platformer->texture_atlas[ID_CUBE] = load_texture("resources/textures/cube.png");
	platformer->texture_atlas[ID_CRATE] = color_palette;

	platformer->model_atlas[ID_CUBE] = load_obj_file("resources/models/cube.obj");
	platformer->model_atlas[ID_CRATE] = load_obj_file("resources/models/crate.obj");

	platformer->player.model = load_obj_file("resources/models/player.obj");
	platformer->player.texture = color_palette;

	Shader *water_shader = new Shader("resources/shader/waterVert.glsl", "resources/shader/waterFrag.glsl");
	water_shader->use();
	water_shader->load_int("world_texture", 0);
	water_shader->load_int("water_texture", 1);
	water_shader->load_int("dudv_map", 2);
	water_shader->load_mat4("model_matrix", glm::scale(glm::mat4(1.0), glm::vec3(world_size_x, 1.0, world_size_z)));
	platformer->water.model = new SimpleModel((float *)&water_vertices[0], 18);
	platformer->water.shader = water_shader;
	platformer->water.water_texture = load_texture("resources/textures/water_texture.png");
	create_water_frame_buffer(platformer, &platformer->water);

	int fwidth, fheight;
	glfwGetFramebufferSize(window, &fwidth, &fheight);
	glViewport(0, 0, fwidth, fheight);
	platformer->width = fwidth;
	platformer->height = fheight;
	platformer->proj_mat = glm::perspective(1.0472f, (float)fwidth / (float)fheight, 0.1f, 1000.0f);
	std::cout << glm::to_string(platformer->proj_mat) << std::endl;

	glClearColor(0.53, 0.81, 0.92, 1.0);
	
	platformer->shader->use();
	platformer->shader->load_int("color_palette", 0);
	platformer->light->install(platformer->shader);

	glEnable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_DEPTH_TEST);

	load_world(platformer, "resources/worlds/world1.txt");
}

void load_world(Platformer *platformer, const char *file_name) {
	World *world = &platformer->world;
	Player *player = &platformer->player;

	world->blocks.clear();
	std::ifstream in_file(file_name);
	std::string line = "";

	int z = 0;
	while (std::getline(in_file, line)) {
		for (int x = 0; x < line.length(); ++x) {
			char c = line[x];

			switch (c) {
			case 'C':
				world->blocks.push_back(Block(x, 0, z, ID_CUBE));
				world->blocks.push_back(Block(x, 1, z, ID_CRATE));
				break;
			case 'X':
				player->x = x;
				player->z = z;
				world->blocks.push_back(Block(x, 0, z, ID_CUBE));
				break;
			case 'G':
				world->blocks.push_back(Block(x, 0, z, ID_CUBE));
				break;
			case 'Q':
				world->blocks.push_back(Block(x, 0, z, ID_CUBE));
				world->blocks.push_back(Block(x, 1, z, ID_CUBE));
				break;
			case '0':
				break;
			}
		}
		z++;
	}
}

void reset_world(World *world) {
	for (int i = 0; i < world->blocks.size(); ++i) {
		Block *block = &world->blocks[i];
		block->x = block->initial_x;
		block->y = block->initial_y;
		block->z = block->initial_z;
		block->target_y = block->initial_y;
	}
}

void move(Player *player, World *world) {
	if (keys_down[GLFW_KEY_D]) {
		float new_x = player->x + player_speed;
		if (world->has_block(new_x + player_size - player_size_half, player->y - 1, player->z + player_size_half)) {
			player->x = new_x;
		}
	}

	if (keys_down[GLFW_KEY_A]) {
		float new_x = player->x - player_speed;
		if (world->has_block(new_x + player_size_half, player->y - 1, player->z + player_size_half)) {
			player->x = new_x;
		}
	}

	if (keys_down[GLFW_KEY_S]) {
		float new_z = player->z + player_speed;
		if (world->has_block(player->x + player_size_half, player->y - 1, new_z + player_size - player_size_half)) {
			player->z = new_z;
		}
	}

	if (keys_down[GLFW_KEY_W]) {
		float new_z = player->z - player_speed;
		if (world->has_block(player->x + player_size_half, player->y - 1, new_z + player_size_half)) {
			player->z = new_z;
		}
	}

	for (int i = 0; i < world->blocks.size(); ++i) {
		Block *block = &world->blocks[i];
		bool colliding = block->colliding(player);

		if (colliding) {
			if (block->pushable()) {
				float x_move = (player->x - player->last_x) / 2.0;
				float z_move = (player->z - player->last_z) / 2.0;

				bool side_ways = abs(x_move) > abs(z_move);

				bool blocking = false;

				if (side_ways) {
					block->x += x_move;

					blocking = block_collisions(world, block);
					player->z = player->last_z;
					if (blocking) {
						block->x -= x_move;
						player->x = player->last_x;
					} else {
						player->x = player->last_x + x_move;
					}
				} else {
					block->z += z_move;

					blocking = block_collisions(world, block);
					player->x = player->last_x;

					if (blocking) {
						block->z -= z_move;
						player->z = player->last_z;
					} else {
						player->z = player->last_z + z_move;
					}
				}

				if (block->target_y > 0 && !blocking) {
					int below = block->target_y - 1;
					bool try_below = false;

					if (side_ways) {
						float frac_x = block->x - (long)block->x;

						if (!world->has_block(block->x, below, block->z) && frac_x <= 0.05) {
							try_below = true;
						}

						if (!world->has_block(block->x + 1, below, block->z) && frac_x >= 0.95) {
							try_below = true;
						}
					} else {
						float frac_z = block->z - (long)block->z;

						if (!world->has_block(block->x, below, block->z) && frac_z <= 0.05) {
							try_below = true;
						}

						if (!world->has_block(block->x, below, block->z + 1) && frac_z >= 0.95) {
							try_below = true;
						}
					}

					if (try_below) {
						float rounded_x = round(block->x);
						float rounded_z = round(block->z);
						if (!world->has_block(rounded_x, below, rounded_z)) {
							block->x = rounded_x;
							block->z = rounded_z;
							block->target_y = below;
						}
					}
				}
			} else {
				player->x = player->last_x;
				player->z = player->last_z;
			}
		}
	}

	player->last_x = player->x;
	player->last_z = player->z;
}

void update(Platformer *platformer) {
	Player *player = &platformer->player;
	Camera *camera = &platformer->camera;
	Shader *shader = platformer->shader;
	World *world = &platformer->world;
		
	move(player, world);

	camera->x = lerp(camera->x, player->x, 0.1);
	camera->z = lerp(camera->z, player->z / 2, 0.1);

	camera->view_matrix = glm::lookAt(glm::vec3(camera->x, 8, camera->z + 10), glm::vec3(camera->x, 0, camera->z), glm::vec3(0, 1, 0));

	//camera->view_matrix = glm::translate(camera->view_matrix, glm::vec3(-platformer->camera.x, -8, -10 - platformer->camera.z));

	shader->use();
	shader->load_mat4("proj_matrix", platformer->proj_mat);
	shader->load_mat4("view_matrix", platformer->camera.view_matrix);

	if (keys_down[GLFW_KEY_BACKSPACE]) {
		reset_world(world);
	}
}

void render_world(Platformer *platformer) {
	Shader *shader = platformer->shader;
	World *world = &platformer->world;

	shader->use();

	shader->load_vec4("block_color", glm::vec4(0.5, 0.3, 0.0, 1.0));
	for (int i = 0; i < world->blocks.size(); ++i) {
		Block *block = &world->blocks[i];

		if (block->target_y < block->y) {
			block->y -= 0.005;			
		}

		bind_texture(platformer->texture_atlas[block->kind]);

		block->model_matrix = glm::translate(glm::mat4(1.0), glm::vec3(block->x, block->y, block->z));
		shader->load_mat4("model_matrix", block->model_matrix);

		platformer->model_atlas[block->kind]->render();
	}
}

void render_player(Platformer *platformer) {
	Player *player = &platformer->player;
	Shader *shader = platformer->shader;

	glm::mat4 model_matrix(1);
	model_matrix = glm::translate(model_matrix, glm::vec3(player->x, player->y, player->z));
	shader->load_mat4("model_matrix", model_matrix);

	shader->load_vec4("block_color", glm::vec4(1.0));
	bind_texture(player->texture);
	player->model->render();
}

void render_water(Platformer *platformer) {
	Water *water = &platformer->water;
	Shader *water_shader = water->shader;

	water_shader->use();
	water_shader->load_mat4("proj_matrix", platformer->proj_mat);
	water_shader->load_mat4("view_matrix", platformer->camera.view_matrix);
	water_shader->load_float("move_factor", glfwGetTime());

	glActiveTexture(GL_TEXTURE0);
	bind_texture(water->frame_buffer_texture);
	glActiveTexture(GL_TEXTURE1);
	bind_texture(water->water_texture);

	water->model->render();
}

void render(Platformer *platformer) {
	Water *water = &platformer->water;

	glCullFace(GL_BACK);
	glActiveTexture(GL_TEXTURE0);

	glEnable(GL_CLIP_DISTANCE0);
	bind_water_frame_buffer(platformer, water);
	glClear(GL_COLOR_BUFFER_BIT);
	render_world(platformer);
	unbind_water_frame_buffer(platformer);
	glDisable(GL_CLIP_DISTANCE0);

	glEnable(GL_CULL_FACE);
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