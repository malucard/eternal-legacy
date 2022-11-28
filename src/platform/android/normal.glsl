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
	uv = uv / tex_size + 0.5;
	vec2 iuv = floor( uv );
	vec2 fuv = fract( uv );

	float g0x = g0(fuv.x);
	float g1x = g1(fuv.x);
	float h0x = h0(fuv.x);
	float h1x = h1(fuv.x);
	float h0y = h0(fuv.y);
	float h1y = h1(fuv.y);

	vec2 p0 = (vec2(iuv.x + h0x, iuv.y + h0y) - 0.5) * tex_size;
	vec2 p1 = (vec2(iuv.x + h1x, iuv.y + h0y) - 0.5) * tex_size;
	vec2 p2 = (vec2(iuv.x + h0x, iuv.y + h1y) - 0.5) * tex_size;
	vec2 p3 = (vec2(iuv.x + h1x, iuv.y + h1y) - 0.5) * tex_size;
	
	return g0(fuv.y) * (g0x * texture2D(tex, vec2(clamp(p0.x, tex_off.x, tex_off.x + tex_scale.x - tex_size.x), clamp(p0.y, tex_off.y, tex_off.y + tex_scale.y - tex_size.y)))  +
						g1x * texture2D(tex, vec2(clamp(p1.x, tex_off.x, tex_off.x + tex_scale.x - tex_size.x), clamp(p1.y, tex_off.y, tex_off.y + tex_scale.y - tex_size.y)))) +
		   g1(fuv.y) * (g0x * texture2D(tex, vec2(clamp(p2.x, tex_off.x, tex_off.x + tex_scale.x - tex_size.x), clamp(p2.y, tex_off.y, tex_off.y + tex_scale.y - tex_size.y)))  +
						g1x * texture2D(tex, vec2(clamp(p3.x, tex_off.x, tex_off.x + tex_scale.x - tex_size.x), clamp(p3.y, tex_off.y, tex_off.y + tex_scale.y - tex_size.y))));
}

void main() {
	if(scissor.z > 0 && (int(gl_FragCoord.x) < scissor.x || int(gl_FragCoord.y) < scissor.y || int(gl_FragCoord.x) >= scissor.z || int(gl_FragCoord.y) >= scissor.w)) {
		cout = vec4(0.0, 0.0, 0.0, 1.0);
	} else {
		cout = bicubic(uv * tex_scale + tex_off) * color + color_off;
	}
}

#endif
