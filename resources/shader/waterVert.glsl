#version 430 core

layout (location = 0) in vec3 pos;

out vec2 uv_coords;
out vec4 clip_space;

uniform mat4 proj_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;

const float tiling = 6.0;

void main() {
	clip_space = proj_matrix * view_matrix * model_matrix * vec4(pos, 1.0);
	gl_Position = clip_space;
	uv_coords = vec2(pos.x / 2.0 + 0.5, pos.z / 2.0 + 0.5) * tiling;
}