varying vec2 uv;

#ifdef VERTEX

attribute vec3 in_pos;
attribute vec2 in_uv;

void main() {
	gl_Position = vec4(in_pos, 1) * vec4(2, 2, 1, 1) + vec4(-1, -1, 0, 0);
	uv = in_uv;
}

#endif

#ifdef FRAGMENT

vec4 lerp(vec4 x, vec4 y, float t) {
	return x + (y - x) * t;
}

float lum(vec4 col) {
	return (col.x + col.x + col.y + col.y + col.y + col.z) * col.w * (1.0 / 6.0);
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

uniform vec2 tex_size;
uniform sampler2D tex;
uniform sampler2D lumtex;
uniform float strength;

vec4 average(vec4 mc, vec4 a, vec4 b, vec4 c) {
	return lerp(mc, (a + b + c) * (1.0 / 3.0), strength);
}

void main() {
	vec4 mc = texture2D(tex, uv);
	float mcg = texture2D(lumtex, uv).r;
	//gl_FragColor = texture2D(lumtex, uv); //vec4(vec3(mcg), mc.a); //vec4(vec3(lum(mc)), mc.a);
	//return;
	/*if(mcg == 1.0) {  
		gl_FragColor = mc;
		return;
	}*/
	// [tl tc tr]
	// [ml mc mr]
	// [bl bc br]

	vec4 duv = tex_size.xyxy * vec4(1, 1, -1, 0);

	vec4 tl = texture2D(tex, uv - duv.xy);
	float tlg = texture2D(lumtex, uv - duv.xy).r;
	vec4 tc = texture2D(tex, uv - duv.wy);
	float tcg = texture2D(lumtex, uv - duv.wy).r;
	vec4 tr = texture2D(tex, uv - duv.zy);
	float trg = texture2D(lumtex, uv - duv.zy).r;

	vec4 ml = texture2D(tex, uv - duv.xw);
	float mlg = texture2D(lumtex, uv - duv.xw).r;
	vec4 mr = texture2D(tex, uv + duv.xw);
	float mrg = texture2D(lumtex, uv + duv.xw).r;

	vec4 bl = texture2D(tex, uv + duv.zy);
	float blg = texture2D(lumtex, uv + duv.zy).r;
	vec4 bc = texture2D(tex, uv + duv.wy);
	float bcg = texture2D(lumtex, uv + duv.wy).r;
	vec4 br = texture2D(tex, uv + duv.xy);
	float brg = texture2D(lumtex, uv + duv.xy).r;

	// Kernel 0 and 4
	if (min3(tlg, tcg, trg) > max4(mcg, brg, bcg, blg)) gl_FragColor = average(mc, tl, tc, tr);
	else if (min3(brg, bcg, blg) > max4(mcg, tlg, tcg, trg)) gl_FragColor = average(mc, br, bc, bl);

	// Kernel 1 and 5
	else if (min3(mrg, tcg, trg) > max3(mcg, mlg, bcg	)) gl_FragColor = average(mc, mr, tc, tr);
	else if (min3(blg, mlg, bcg) > max3(mcg, mrg, tcg	)) gl_FragColor = average(mc, bl, ml, bc);

	// Kernel 2 and 6
	else if (min3(mrg, brg, trg) > max4(mcg, mlg, tlg, blg)) gl_FragColor = average(mc, mr, br, tr);
	else if (min3(mlg, tlg, blg) > max4(mcg, mrg, brg, trg)) gl_FragColor = average(mc, ml, tl, bl);

	// Kernel 3 and 7
	else if (min3(mrg, brg, bcg) > max3(mcg, mlg, tcg	)) gl_FragColor = average(mc, mr, br, bc);
	else if (min3(tcg, mlg, tlg) > max3(mcg, mrg, bcg	)) gl_FragColor = average(mc, tc, ml, tl);

	else gl_FragColor = mc;
}

#endif
