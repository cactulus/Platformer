#define VB_VERT 0
#define VB_UV 1
#define VB_NORM 2
#define VB_IND 3

SimpleModel::SimpleModel(float *vertices, int num_vertices) {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	vertices_count = num_vertices / 3;

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices, &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);
}

void SimpleModel::render() {
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, vertices_count);
}

ComplexModel::ComplexModel(float *vertices, int num_vertices, float *tex_coords, int num_tex_coords, float *normals, int num_normals, int *indices, int num_indices) {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	indices_count = num_indices;

	glGenBuffers(4, vbos);

	glBindBuffer(GL_ARRAY_BUFFER, vbos[VB_VERT]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices, &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbos[VB_UV]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_tex_coords, &tex_coords[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, 0, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbos[VB_NORM]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_normals, &normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, 0, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[VB_IND]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * num_indices, &indices[0], GL_STATIC_DRAW);
}

void ComplexModel::render() {
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, 0);
}

ComplexModel *load_obj_file(const char *file) {
	Assimp::Importer importer;

	const aiScene *scene = importer.ReadFile(file, aiProcess_GenSmoothNormals | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);
	
	if (scene->mNumMeshes != 1) {
		die("Illegal number of meshes in object file!");
	}

	aiMesh *mesh = scene->mMeshes[0];

	int num_vertices = mesh->mNumVertices * 3;
	float *vertices = new float[num_vertices];

	int num_tex_coords = mesh->mNumVertices * 2;
	float *tex_coords = new float[num_tex_coords];

	int num_normals = mesh->mNumVertices * 3;
	float *normals = new float[num_normals];

	int num_indices = mesh->mNumFaces * 3;
	int *indices = new int[num_indices];

	aiVector3D zero_vector(0.0f);
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		aiVector3D pos = mesh->mVertices[i];
		aiVector3D tex_coords = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : zero_vector;
		aiVector3D normal = mesh->mNormals[i];

		vertices[i * 3] = pos.x;
		vertices[i * 3 + 1] = pos.y;
		vertices[i * 3 + 2] = pos.z;

		tex_coords[i * 2] = tex_coords.x;
		tex_coords[i * 2 + 1] = tex_coords.y;

		normals[i * 3] = normal.x;
		normals[i * 3 + 1] = normal.y;
		normals[i * 3 + 2] = normal.z;
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace face = mesh->mFaces[i];
		indices[i * 3] = face.mIndices[0];	
		indices[i * 3 + 1] = face.mIndices[1];	
		indices[i * 3 + 2] = face.mIndices[2];	
	}

	return new ComplexModel(vertices, num_vertices, tex_coords, num_tex_coords, normals, num_normals, indices, num_indices);
}
/*
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
				} else if (counter == 1) {
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
}*/
