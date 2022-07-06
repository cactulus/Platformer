#version 430 core

in vec3 frag_pos;
in vec2 uv_coord;
in vec3 normal;

out vec4 color;

uniform sampler2D color_palette;

struct PositionalLight {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec3 pos;
};

uniform PositionalLight light;
uniform vec4 block_color;

void main() {
	vec3 ray = normalize(light.pos - frag_pos);
	vec3 norm = normalize(normal);
	float diff = dot(norm, ray);

	vec4 diffuse = diff * light.diffuse;

	color = (light.ambient + diffuse) * texture(color_palette, uv_coord) * block_color;

	color = vec4(pow(color.xyz, vec3(0.4545)), 1.0);
}