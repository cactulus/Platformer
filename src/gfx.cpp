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
		s->load_vec3("light.position", pos);
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