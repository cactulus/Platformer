#define VB_VERT 0
#define VB_UV 1
#define VB_NORM 2
#define VB_IND 3

struct Shader {
	std::unordered_map<const char *, GLint> uniform_location_cache;
	GLuint program;

	Shader(const char *vert_path, const char *frag_path) {
		GLint linked;
		auto vert_shader_src = read_file(vert_path);
		auto frag_shader_src = read_file(frag_path);

		auto vert_shader = load_shader(vert_shader_src.c_str(), GL_VERTEX_SHADER, "Vertex Shader");
		auto frag_shader = load_shader(frag_shader_src.c_str(), GL_FRAGMENT_SHADER, "Fragment Shader");

		program = glCreateProgram();
		glAttachShader(program, vert_shader);
		glAttachShader(program, frag_shader);

		glLinkProgram(program);
		checkOpenGLError();
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if (linked != 1) {
			std::cout << "Linking failed\n";
			print_program_log(program);
		}
	}

	void use() {
		glUseProgram(program);
	}

	void load_float(const char *name, float value) {
		GLint loc = get_uniform_location(name);
		glUniform1f(loc, value);
	}

	void load_float4(const char *name, float *value) {
		GLint loc = get_uniform_location(name);
		glUniform4fv(loc, 1, value);
	}

	void load_vec2(const char *name, glm::vec2 vec) {
		GLint loc = get_uniform_location(name);
		glUniform2f(loc, vec.x, vec.y);
	}

	void load_vec3(const char *name, glm::vec3 vec) {
		GLint loc = get_uniform_location(name);
		glUniform3f(loc, vec.x, vec.y, vec.z);
	}
	
	void load_vec4(const char *name, glm::vec4 vec) {
		GLint loc = get_uniform_location(name);
		glUniform4f(loc, vec.x, vec.y, vec.z, vec.w);
	}
	
	void load_mat4(const char *name, glm::mat4 mat) {
		GLuint loc = get_uniform_location(name);
		glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
	}

	GLuint load_shader(const char *src, GLenum type, const char *type_name) {
		GLint compiled;

		auto shader = glCreateShader(type);
		glShaderSource(shader, 1, &src, 0);

		glCompileShader(shader);
		checkOpenGLError();
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (compiled != 1) {
			std::cout << type_name << " Compilation failed\n";
			print_shader_log(shader);
		}

		return shader;
	}

	GLint get_uniform_location(const char *name) {
		auto it = uniform_location_cache.find(name);
		if (it != uniform_location_cache.end())
			return it->second;

		GLint loc = glGetUniformLocation(program, name);
		uniform_location_cache.insert(std::make_pair(name, loc));
		return loc;
	}

	void print_shader_log(GLuint shader) {
		int len = 0, chWrittn = 0;
		char *log;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

		if (len <= 0)
			return;

		log = new char[len];
		glGetShaderInfoLog(shader, len, &chWrittn, log);
		std::cout << "Shader Info Log: " << log << "\n";
		delete[] log;
	}

	void print_program_log(GLuint prog) {
		int len = 0, chWrittn = 0;
		char *log;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);

		if (len <= 0)
			return;

		log = new char[len];
		glGetProgramInfoLog(prog, len, &chWrittn, log);
		std::cout << "Program Info Log: " << log << "\n";
		delete[] log;
	}
};

struct PositionalLight {
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;
	glm::vec3 pos;

	PositionalLight(glm::vec3 p, glm::vec4 a, glm::vec4 d, glm::vec4 s) {
		pos = p;
		ambient = a;
		diffuse = d;
		specular = s;
	}

	void install(Shader *s) {
		s->load_vec4("light.ambient", ambient);
		s->load_vec4("light.diffuse", diffuse);
		s->load_vec4("light.specular", specular);
		s->load_vec3("light.pos", pos);
	}
};

struct Texture {
	GLuint id;

	Texture(const char *path) {
		int w, h;

		stbi_set_flip_vertically_on_load(true);
		unsigned char *image = stbi_load(path, &w, &h, 0, STBI_rgb_alpha);

		if (!image) {
			std::cout << "Failed to load texture '" << path << "'!\n";
			std::exit(EXIT_FAILURE);
		}

		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);

		if (glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
			GLfloat anisoSetting = 0.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisoSetting);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisoSetting);
		}

		glBindTexture(GL_TEXTURE_2D, 0);

		stbi_image_free(image);
	}

	void bind() {
		glBindTexture(GL_TEXTURE_2D, id);
	}
};

struct Model {
	GLuint vao;
	GLuint vbos[3]; /* vertices, uv coords, normals */

	unsigned int vertices_count;

	Model(float *vertices, int num_vertices, float *tex_coords, int num_tex_coords, float *normals, int num_normals) {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		vertices_count = num_vertices;

		glGenBuffers(3, vbos);

		glBindBuffer(GL_ARRAY_BUFFER, vbos[VB_VERT]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices, &vertices[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);

		if (num_tex_coords > 0) {
			glBindBuffer(GL_ARRAY_BUFFER, vbos[VB_UV]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_tex_coords, &tex_coords[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, 0, 0, 0);
		}

		if (num_normals > 0) {
			glBindBuffer(GL_ARRAY_BUFFER, vbos[VB_NORM]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_normals, &normals[0], GL_STATIC_DRAW);
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, 0, 0, 0);
		}
	}

	void render() {
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, vertices_count / 3);
	}
};


Model *load_obj_file(const char *file_name) {
	std::vector<glm::fvec3> vertices;
	std::vector<glm::fvec2> tex_coords;
	std::vector<glm::fvec3> normals;

	std::vector<int> face_vertices;
	std::vector<int> face_tex_coords;
	std::vector<int> face_normals;

	std::stringstream ss;
	std::ifstream in_file(file_name);
	std::string line = "";
	std::string prefix = "";
	glm::vec3 temp_vec3;
	glm::vec2 temp_vec2;
	GLint temp_glint = 0;

	if (!in_file.is_open()) {
		die("Failed to open model!");
	}

	while (std::getline(in_file, line)) {
		ss.clear();
		ss.str(line);
		ss >> prefix;

		if (prefix == "v") {
			ss >> temp_vec3.x >> temp_vec3.y >> temp_vec3.z;
			vertices.push_back(temp_vec3);
		} else if (prefix == "vt") {
			ss >> temp_vec2.x >> temp_vec2.y;
			tex_coords.push_back(temp_vec2);
		} else if (prefix == "vn") {
			ss >> temp_vec3.x >> temp_vec3.y >> temp_vec3.z;
			normals.push_back(temp_vec3);
		} else if (prefix == "f") {
			int counter = 0;
			while (ss >> temp_glint) {
				if (counter == 0) {
					int index = temp_glint - 1;
					face_vertices.push_back(index);
				}  else if (counter == 1) {
					int tex_index = temp_glint - 1;
					face_tex_coords.push_back(tex_index);
				} else if (counter == 2) {
					int normal_index = temp_glint - 1;
					face_normals.push_back(normal_index);
				}

				if (ss.peek() == '/') {
					++counter;
					ss.ignore(1, '/');
				} else if (ss.peek() == ' ') {
					++counter;
					ss.ignore(1, ' ');
				}

				if (counter > 2)
					counter = 0;
			}
		}
	}

	int vertices_count = face_vertices.size() * 3;
	float *vertices_array = new float[vertices_count];

	int tex_coord_count = face_tex_coords.size() * 2;
	float *tex_coords_array = new float[tex_coord_count];

	int normals_count = face_normals.size() * 3;
	float *normals_array = new float[normals_count];

	for (int i = 0; i < face_vertices.size(); ++i) {
		glm::vec3 pos = vertices[face_vertices[i]];
		vertices_array[i * 3] = pos.x;
		vertices_array[i * 3 + 1] = pos.y;
		vertices_array[i * 3 + 2] = pos.z;
	}
	
	for (int i = 0; i < face_tex_coords.size(); ++i) {
		glm::vec2 pos = tex_coords[face_tex_coords[i]];
		tex_coords_array[i * 2] = pos.x;
		tex_coords_array[i * 2 + 1] = pos.y;
	}

	for (int i = 0; i < face_normals.size(); ++i) {
		glm::vec3 pos = normals[face_normals[i]];
		normals_array[i * 3] = pos.x;
		normals_array[i * 3 + 1] = pos.y;
		normals_array[i * 3 + 2] = pos.z;
	}

	return new Model(vertices_array, vertices_count,
					 tex_coords_array, tex_coord_count,
				     normals_array, normals_count);
}

GLuint create_frame_buffer() {
	GLuint fb;
	glGenFramebuffers(1, &fb);
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	return fb;
}

GLuint create_texture_attachment(int width, int height) {
	GLuint texture;
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

	return texture;
}