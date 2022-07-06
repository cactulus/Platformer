#version 430 core

in vec2 uv_coords;
in vec4 clip_space;

out vec4 color;

uniform sampler2D water_texture;

void main() {
	vec2 ndc = (clip_space.xy / clip_space.w) / 2.0 + 0.5;
	vec4 water_tex = texture(water_texture, ndc);

	color = mix(water_tex, vec4(0.0, 0.3, 0.5, 1.0), 0.2);
}