varying vec2 uv;

#ifdef VERTEX

uniform vec3 o_pos;
uniform vec3 o_scale;
uniform vec2 o_pivot;
uniform float o_rot;

attribute vec3 in_pos;
attribute vec2 in_uv;

void main() {
	gl_Position = vec4((vec3(o_pivot.x + (in_pos.x - o_pivot.x) * cos(o_rot) - (in_pos.y - o_pivot.y) * sin(o_rot), o_pivot.y + (in_pos.x - o_pivot.x) * sin(o_rot) + (in_pos.y - o_pivot.y) * cos(o_rot), in_pos.z) * o_scale + o_pos) * vec3(2.0, -2.0, 2.0) + vec3(-1.0, 1.0, 0.0), 1);
	uv = in_uv;
}

#endif

#ifdef FRAGMENT

uniform float effect_param;
uniform vec2 tex_off;
uniform vec2 tex_scale;
uniform vec2 tex_size;
uniform vec4 color;
uniform vec4 color_off;
uniform sampler2D tex;

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

void main() {
	vec2 iuv = vec2(uv.x, 1.0 - uv.y * tex_size.x / tex_size.y);
	vec2 t1 = vec2(effect_param, 1.0);
	vec2 t2 = vec2(1.0, effect_param);
	vec2 b1 = vec2(effect_param - SIZE, 1.0);
	vec2 b2 = vec2(1.0, effect_param - SIZE);
	if(side(t1, t2, iuv)) {
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	} else if(side(b1, b2, iuv)) {
		if(iuv.x < effect_param || iuv.y < effect_param) {
			float t = 1.0 - clamp(line_dist(t1, t2, iuv) / min(SIZE, 1.0 - effect_param) + 0.25, 0.0, 1.0);
			t = 1.0 - t * t;
			gl_FragColor = texture2D(tex, uv) * vec4(t, t, t, 1.0) * color + color_off;
		} else {
			float t = line_dist(b1, b2, iuv) / SIZE / 2.0 + 0.5;
			gl_FragColor = vec4(t, t, t, 1.0);
		}
	} else {
		gl_FragColor = texture2D(tex, uv) * color + color_off;
	}
}

#endif
