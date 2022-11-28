#define MAX_LIGHTS 4

varying vec3 world_pos;
varying vec3 normal;
varying vec2 uv;

#ifdef VERTEX

uniform mat4 o_mat;
uniform mat4 o_mv_mat;

attribute vec4 in_weights;
attribute vec4 in_bone_ids;
attribute vec2 in_uv;
attribute vec3 in_normal;
attribute vec3 in_pos;

void main() {
	gl_Position = o_mat * o_mv_mat * vec4(in_pos, 1);
	world_pos = (o_mv_mat * vec4(in_pos, 1)).xyz + vec3(in_weights.x * 0.000000001 + in_bone_ids.x * 0.000000001, 0, 0) * 0.00000000000001;
	uv = in_uv;
	normal = (o_mv_mat * vec4(in_normal, 0)).xyz;
}

#endif

#ifdef FRAGMENT

uniform sampler2D tex;
uniform vec3 ambient_color;

uniform int light_type[MAX_LIGHTS];
uniform vec3 light_pos[MAX_LIGHTS];
uniform vec3 light_color[MAX_LIGHTS];

void main() {
	vec3 tint = ambient_color;
	for(int i = 0; i < MAX_LIGHTS; i++) {
		if(light_type[i] == 1) {
			vec3 eye_dir = vec3(0, 0, 0) - world_pos;
			vec3 light_dir = light_pos[0] + eye_dir;
			float cosTheta = dot(normalize(normal), normalize(light_dir));
			float dist = distance(light_pos[0], world_pos);
			dist *= dist;
			tint += light_color[i] * cosTheta / dist;
		} else if(light_type[i] == 2) {
			vec3 light_dir = light_pos[0];
			float cosTheta = dot(normalize(normal), normalize(light_dir));
			tint += light_color[i] * cosTheta;
		}
	}
	vec4 hdr = texture2D(tex, uv) * vec4(tint, 1.0);
	//const float gamma = 1.0 / 2.2;
	//gl_FragColor = vec4(pow(hdr.rgb / (hdr.rgb + vec3(1.0)), vec3(gamma)), hdr.a);
	if(hdr.a < 0.6) {
		discard;
	}
	gl_FragColor = hdr;
}

#endif
