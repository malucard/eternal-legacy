#version 300 es
precision mediump float;

#ifdef VERTEX

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

uniform vec2 tex_size;
uniform sampler2D tex;

in vec2 uv;
in vec4 color;
in vec4 color_off;
out vec4 frag_color;

float w0(float a) {
	return (1.0/6.0)*(a*(a*(-a + 3.0) - 3.0) + 1.0);
}

float w1(float a) {
	return (1.0/6.0)*(a*a*(3.0*a - 6.0) + 4.0);
}

float w2(float a) {
	return (1.0/6.0)*(a*(a*(-3.0*a + 3.0) + 3.0) + 1.0);
}

float w3(float a) {
	return (1.0/6.0)*(a*a*a);
}

// g0 and g1 are the two amplitude functions
float g0(float a) {
	return w0(a) + w1(a);
}

float g1(float a) {
	return w2(a) + w3(a);
}

// h0 and h1 are the two offset functions
float h0(float a) {
	return -1.0 + w1(a) / (w0(a) + w1(a));
}

float h1(float a) {
	return 1.0 + w3(a) / (w2(a) + w3(a));
}

vec4 bicubic(vec2 uv) {
	uv = uv*tex_size + 0.5;
	vec2 iuv = floor( uv );
	vec2 fuv = fract( uv );

	float g0x = g0(fuv.x);
	float g1x = g1(fuv.x);
	float h0x = h0(fuv.x);
	float h1x = h1(fuv.x);
	float h0y = h0(fuv.y);
	float h1y = h1(fuv.y);

	vec2 p0 = (vec2(iuv.x + h0x, iuv.y + h0y) - 0.5) / tex_size;
	vec2 p1 = (vec2(iuv.x + h1x, iuv.y + h0y) - 0.5) / tex_size;
	vec2 p2 = (vec2(iuv.x + h0x, iuv.y + h1y) - 0.5) / tex_size;
	vec2 p3 = (vec2(iuv.x + h1x, iuv.y + h1y) - 0.5) / tex_size;
	
	return g0(fuv.y) * (g0x * texture2D(tex, p0)  +
						g1x * texture2D(tex, p1)) +
		   g1(fuv.y) * (g0x * texture2D(tex, p2)  +
						g1x * texture2D(tex, p3));
}

vec3 lerp(vec3 x, vec3 y, float t) {
	return x + (y - x) * t;
}

void main() {
	//gl_FragColor = vec4(lerp(color.xyz, color_2.xyz, uv.y), texture2D(tex, uv).w * color.w) + color_off;
	frag_color = vec4(color.xyz, texture2D(tex, uv).w * color.w) + color_off;
}

#endif
