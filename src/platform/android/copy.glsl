#version 300 es

uniform vec3 o_scale;

#ifdef VERTEX

uniform vec3 o_pos;

in vec3 in_pos;
in vec2 in_uv;

out vec2 uv;

void main() {
	gl_Position = vec4((in_pos * o_scale + o_pos) * vec3(2.0, -2.0, 2.0) + vec3(-1.0, 1.0, 0.0), 1);
	uv = in_uv;
}

#endif

#ifdef FRAGMENT

uniform vec2 tex_off;
uniform vec2 tex_scale;
uniform vec2 tex_size;
uniform ivec4 scissor;
uniform vec4 color;
uniform vec4 color_off;
uniform sampler2D tex;

in vec2 uv;
out vec4 cout;

void main() {
	if(scissor.z > 0 && (int(gl_FragCoord.x) < scissor.x || int(gl_FragCoord.y) < scissor.y || int(gl_FragCoord.x) >= scissor.z || int(gl_FragCoord.y) >= scissor.w)) {
		cout = vec4(0.0, 0.0, 0.0, 1.0);
	} else {
		cout = texture2D(tex, uv * tex_scale + tex_off) * color + color_off;
	}
}

#endif
