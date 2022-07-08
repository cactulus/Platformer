#version 430 core

in vec2 uv_coords;
in vec4 clip_space;

out vec4 color;

uniform sampler2D world_texture;
uniform sampler2D water_texture;
uniform float move_factor;

const float wave_strength = 0.05;

void main() {
	vec2 ndc = (clip_space.xy / clip_space.w) / 2.0 + 0.5;

	vec2 water_uv = vec2(ndc.x, ndc.y);
	vec4 water_tex = texture(water_texture, water_uv);

	vec4 texture_color = vec4(0.2, 0.5, 0.7, 1.0);
    
    vec4 k = vec4(move_factor);
	k.xy = ndc * 15.0;
    float val1 = length(0.5-fract(k.xyw*=mat3(vec3(-2.0,-1.0,0.0), vec3(3.0,-1.0,1.0), vec3(1.0,-1.0,-1.0))*0.5));
    float val2 = length(0.5-fract(k.xyw*=mat3(vec3(-2.0,-1.0,0.0), vec3(3.0,-1.0,1.0), vec3(1.0,-1.0,-1.0))*0.2));
    float val3 = length(0.5-fract(k.xyw*=mat3(vec3(-2.0,-1.0,0.0), vec3(3.0,-1.0,1.0), vec3(1.0,-1.0,-1.0))*0.5));
    vec4 water_color = vec4 ( pow(min(min(val1,val2),val3), 6.0) * 2.5)+texture_color;
	
	vec4 world_tex = texture(world_texture, ndc);
	color = mix(water_color, water_tex, 0.2);
	color = mix(color, world_tex, 0.5);
}