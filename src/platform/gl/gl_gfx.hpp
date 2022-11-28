#pragma once

// renderer for OpenGL 2.0+, OpenGL ES 2.0 and OpenGL ES 3.0

struct Shader {
	int id, loc_mat, loc_mv_mat, loc_tex_size, loc_light_type, loc_light_pos, loc_light_color, loc_ambient_color;

	void load(const char* shader_name, const char* verstr, const char* src, int kind = -1);
	void use();
};

extern unsigned default_framebuffer;
extern Shader shader_bicubic, shader_bicubic_batched, shader_copy_batched, shader_alpha_batched, shader_lighted, shader_push, shader_calc_grad, shader_push_grad;
void gl_init();
void gl_frame();
