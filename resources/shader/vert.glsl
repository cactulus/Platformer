#version 430 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 texture_coords;
layout (location = 2) in vec3 in_normal;

out vec3 frag_pos;
out vec2 uv_coord;
out vec3 normal;

uniform mat4 proj_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;

const vec4 plane = vec4(0.0, -1.0, 0.0, 0.7);

void main() {
	vec4 world_pos = model_matrix * vec4(pos, 1.0);

	gl_ClipDistance[0] = dot(world_pos, plane);

	gl_Position = proj_matrix * view_matrix * world_pos;

	frag_pos = pos;
	uv_coord = texture_coords;
	normal = mat3(transpose(inverse(model_matrix))) * in_normal; 
}