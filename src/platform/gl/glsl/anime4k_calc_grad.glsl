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

float min3(vec4 a, vec4 b, vec4 c) {
	return min(min(a.a, b.a), c.a);
}

float max3(vec4 a, vec4 b, vec4 c) {
	return max(max(a.a, b.a), c.a);
}

float min4(vec4 a, vec4 b, vec4 c, vec4 d) {
	return min(min(min(a.a, b.a), c.a), d.a);
}

float max4(vec4 a, vec4 b, vec4 c, vec4 d) {
	return max(max(max(a.a, b.a), c.a), d.a);
}

float lum(vec4 col) {
	return (col.x + col.x + col.y + col.y + col.y + col.z) * col.w * (1.0 / 6.0);
}

uniform vec2 tex_size;
uniform sampler2D tex;

void main() {
	vec4 c0 = texture2D(tex, uv);
	/*if(lum(c0) == 1.0) {
		gl_FragData[0] = c0;
		gl_FragData[1] = vec4(1.0);
		return;
	}*/

	// [tl tc tr]
	// [ml	mr]
	// [bl bc br]

	vec4 duv = tex_size.xyxy * vec4(1, 1, -1, 0);

	float tl = lum(texture2D(tex, uv - duv.xy));
	float tc = lum(texture2D(tex, uv - duv.wy));
	float tr = lum(texture2D(tex, uv - duv.zy));

	float ml = lum(texture2D(tex, uv - duv.xw));
	float mr = lum(texture2D(tex, uv + duv.xw));

	float bl = lum(texture2D(tex, uv + duv.zy));
	float bc = lum(texture2D(tex, uv + duv.wy));
	float br = lum(texture2D(tex, uv + duv.xy));

	// Horizontal gradient
	// [-1  0  1]
	// [-2  0  2]
	// [-1  0  1]

	// Vertical gradient
	// [-1 -2 -1]
	// [ 0  0  0]
	// [ 1  2  1]

	float xgrad = tr + mr * 2.0 + br - (tl + ml * 2.0 + bl);
	float ygrad = bl + bc * 2.0 + br - (tl + tc * 2.0 + tr);

	// Computes the luminance's gradient and saves it in the unused alpha channel
	gl_FragData[0] = c0;
	float v = (xgrad * xgrad + ygrad * ygrad);
	gl_FragData[1] = vec4(vec3(v > 1.0? 0.0: 1.0 - clamp(sqrt(v), 0.0, 1.0)), 1.0);
}

#endif
