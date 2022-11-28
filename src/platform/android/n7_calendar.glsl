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

uniform float effect_param;
uniform vec2 tex_off;
uniform vec2 tex_scale;
uniform vec2 tex_size;
uniform ivec4 scissor;
uniform vec4 color;
uniform vec4 color_off;
uniform sampler2D tex;

in vec2 uv;
out vec4 cout;

#define SIZE 0.15

bool side(vec2 a, vec2 b, vec2 c) {
	return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) > 0.0;
}

float line_dist(vec2 pt1, vec2 pt2, vec2 testPt) {
  vec2 lineDir = pt2 - pt1;
  vec2 perpDir = vec2(lineDir.y, -lineDir.x);
  vec2 dirToPt1 = pt1 - testPt;
  return abs(dot(normalize(perpDir), dirToPt1));
}

vec2 lerp(vec2 x, vec2 y, float t) {
	return x + (y - x) * t;
}

void main() {
	if(scissor.z > 0 && (int(gl_FragCoord.x) < scissor.x || int(gl_FragCoord.y) < scissor.y || int(gl_FragCoord.x) >= scissor.z || int(gl_FragCoord.y) >= scissor.w)) {
		cout = vec4(0.0, 0.0, 0.0, 1.0);
	} else {
		vec2 iuv = vec2(uv.x, 1.0 - uv.y * tex_size.x / tex_size.y);
		vec2 t1 = vec2(effect_param, 1.0);
		vec2 t2 = vec2(1.0, effect_param);
		vec2 b1 = vec2(effect_param - SIZE, 1.0);
		vec2 b2 = vec2(1.0, effect_param - SIZE);
		if(side(t1, t2, iuv)) {
			cout = vec4(0.0, 0.0, 0.0, 1.0);
		} else if(side(b1, b2, iuv)) {
			if(iuv.x < effect_param || iuv.y < effect_param) {
				float t = 1.0 - clamp(line_dist(t1, t2, iuv) / min(SIZE, 1.0 - effect_param) + 0.25, 0.0, 1.0);
				t = 1.0 - t * t;
				cout = texture2D(tex, uv) * vec4(t, t, t, 1.0) * color + color_off;
			} else {
				float t = line_dist(b1, b2, iuv) / SIZE / 2.0 + 0.5;
				cout = vec4(t, t, t, 1.0);
			}
		} else {
			cout = texture2D(tex, uv) * color + color_off;
		}
	}
}

#endif
