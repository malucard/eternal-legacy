varying vec2 uv;
varying vec4 color;
varying vec4 color_off;

#ifdef VERTEX

uniform vec2 tex_size;
uniform mat4 o_mat;

attribute vec2 in_uv;
attribute vec4 in_color;
attribute vec4 in_color_off;
attribute vec3 in_pos;

void main() {
	gl_Position = o_mat * vec4(in_pos, 1);
	uv = in_uv;
	color = in_color;
	color_off = in_color_off;
}

#endif

#ifdef FRAGMENT

uniform sampler2D tex;

void main() {
	gl_FragColor = texture2D(tex, uv) * color + color_off;
}

#endif
