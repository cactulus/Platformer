bool checkOpenGLError();
std::string read_file(const char *file_path);
float to_radians(float degrees);

struct SimpleModel {
	GLuint vao;
	GLuint vbo;

	unsigned int vertices_count;

	SimpleModel(float *vertices, int num_vertices);

	void render();
};

struct ComplexModel {
	GLuint vao;
	GLuint vbos[4]; /* vertices, uv coords, normals */

	unsigned int indices_count;

	ComplexModel(float *vertices, int num_vertices, float *tex_coords, int num_tex_coords, float *normals, int num_normals, int *indices, int num_indices);

	void render();
};

struct Platformer;
struct World;
void reset_world(World *world);
void load_world(Platformer *platformer, const char *file_name);
