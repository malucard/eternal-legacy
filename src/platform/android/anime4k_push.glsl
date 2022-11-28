#version 300 es

#ifdef VERTEX

uniform vec3 o_pos;
uniform vec3 o_scale;

in vec3 in_pos;
in vec2 in_uv;

out vec2 uv;

void main() {
	gl_Position = vec4((in_pos * o_scale + o_pos) * vec3(2.0, -2.0, 2.0) + vec3(-1.0, 1.0, 0.0), 1);
	uv = in_uv;
}

#endif

#ifdef FRAGMENT

uniform vec2 tex_size;
uniform ivec4 scissor;
uniform sampler2D tex;
uniform float STRENGTH;

in vec2 uv;
out vec4 cout;

vec4 lerp(vec4 x, vec4 y, float t) {
	return x + (y - x) * t;
}

float lerp(float x, float y, float t) {
	return x + (y - x) * t;
}

float min3(float a, float b, float c) {
	return min(min(a, b), c);
}

float max3(float a, float b, float c) {
	return max(max(a, b), c);
}

float min4(float a, float b, float c, float d) {
	return min(min(min(a, b), c), d);
}

float max4(float a, float b, float c, float d) {
	return max(max(max(a, b), c), d);
}

float lum(vec4 col) {
	return (col.x + col.x + col.y + col.y + col.y + col.z) * col.w * (1.0 / 6.0);
}

vec4 cubic(float v) {
	vec4 n = vec4(1.0, 2.0, 3.0, 4.0) - v;
	vec4 s = n * n * n;
	float x = s.x;
	float y = s.y - 4.0 * s.x;
	float z = s.z - 4.0 * s.y + 6.0 * s.x;
	float w = 6.0 - x - y - z;
	return vec4(x, y, z, w) * (1.0 / 6.0);
}

void Largest(vec4 mc, float mclum, inout vec4 lightest, inout float lightestlum, vec4 a, vec4 b, vec4 c, float alum, float blum, float clum) {
	float abclum = lerp(mclum, (alum + blum + clum) * (1.0 / 3.0), STRENGTH);
	if(abclum > lightestlum) {
		lightest = lerp(mc, (a + b + c) * (1.0 / 3.0), STRENGTH);
		lightestlum = abclum;
	}
}

void main() {
	/*if(t.w == 1.0) {
		cout = t;
		return;
	}*/
	// [tl tc tr]
	// [ml mc mr]
	// [bl bc br]

	vec4 duv = tex_size.xyxy * vec4(1, 1, -1, 0);

	vec4 tl = texture2D(tex, uv - duv.xy);
	vec4 tc = texture2D(tex, uv - duv.wy);
	vec4 tr = texture2D(tex, uv - duv.zy);
	float tllum = lum(tl);
	float tclum = lum(tc);
	float trlum = lum(tr);

	vec4 ml = texture2D(tex, uv - duv.xw);
	vec4 mc = texture2D(tex, uv);
	vec4 mr = texture2D(tex, uv + duv.xw);
	float mllum = lum(ml);
	float mclum = lum(mc);
	float mrlum = lum(mr);

	vec4 bl = texture2D(tex, uv + duv.zy);
	vec4 bc = texture2D(tex, uv + duv.wy);
	vec4 br = texture2D(tex, uv + duv.xy);
	float bllum = lum(bl);
	float bclum = lum(bc);
	float brlum = lum(br);

	vec4 lightest = mc;
	float lightestlum = mclum;

	// Kernel 0 and 4
	if (min3(tllum, tclum, trlum) > max4(mclum, brlum, bclum, bllum))
		Largest(mc, mclum, lightest, lightestlum, tl, tc, tr, tllum, tclum, trlum);
	else if (min3(brlum, bclum, bllum) > max4(mclum, tllum, tclum, trlum))
		Largest(mc, mclum, lightest, lightestlum, br, bc, bl, brlum, bclum, bllum);

	// Kernel 1 and 5
	if (min3(mrlum, tclum, trlum) > max3(mclum, mllum, bclum))
		Largest(mc, mclum, lightest, lightestlum, mr, tc, tr, mrlum, tclum, trlum);
	else if (min3(bllum, mllum, bclum) > max3(mclum, mrlum, tclum))
		Largest(mc, mclum, lightest, lightestlum, bl, ml, bc, bllum, mllum, bclum);

	// Kernel 2 and 6
	if (min3(mrlum, brlum, trlum) > max4(mclum, mllum, tllum, bllum))
		Largest(mc, mclum, lightest, lightestlum, mr, br, tr, mrlum, brlum, trlum);
	else if (min3(mllum, tllum, bllum) > max4(mclum, mrlum, brlum, trlum))
		Largest(mc, mclum, lightest, lightestlum, ml, tl, bl, mllum, tllum, bllum);

	//Kernel 3 and 7
	if (min3(mrlum, brlum, bclum) > max3(mclum, mllum, tclum))
		Largest(mc, mclum, lightest, lightestlum, mr, br, bc, mrlum, brlum, bclum);
	else if (min3(tclum, mllum, tllum) > max3(mclum, mrlum, bclum))
		Largest(mc, mclum, lightest, lightestlum, tc, ml, tl, tclum, mllum, tllum);

	cout = lightest;
}

#endif
