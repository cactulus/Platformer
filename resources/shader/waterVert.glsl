#version 430 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex_coords;

out vec2 uv_coords;
out vec4 clip_space;

uniform mat4 proj_matrix;
uniform mat4 view_matrix;

void main() {
	clip_space = proj_matrix * view_matrix * vec4(pos, 1.0);
	gl_Position = clip_space;
	uv_coords = tex_coords;
}