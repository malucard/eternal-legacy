#version 300 es
precision highp float;

#ifdef VERTEX

uniform vec2 tex_size;
uniform mat4 o_mat;

in vec2 in_uv;
in vec4 in_color;
in vec4 in_color_off;
in vec3 in_pos;
out vec2 uv;
out vec4 color;
out vec4 color_off;

void main() {
	gl_Position = o_mat * vec4(in_pos, 1);
	uv = in_uv;
	color = in_color;
	color_off = in_color_off;
}

#endif

#ifdef FRAGMENT

uniform sampler2D tex;

in vec2 uv;
in vec4 color;
in vec4 color_off;
out vec4 frag_color;

void main() {
	frag_color = texture2D(tex, uv) * color + color_off;
}

#endif
