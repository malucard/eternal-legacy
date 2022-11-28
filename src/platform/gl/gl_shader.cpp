#include "gl_gfx.hpp"
#include "gl_header.hpp"

#ifdef USE_GLES2
#define PRECISION_STR "precision mediump float;\n"
#else
#define PRECISION_STR ""
#endif

void Shader::load(const char* shader_name, const char* verstr, const char* src, int kind) {
	id = glCreateProgram();
	int vert = glCreateShader(GL_VERTEX_SHADER);
	const char* srcs[] = {verstr, PRECISION_STR "#define VERTEX\n", src};
	glShaderSource(vert, 3, srcs, 0);
	glCompileShader(vert);
	int res;
	glGetShaderiv(vert, GL_COMPILE_STATUS, &res);
	if(res != GL_TRUE) {
		glGetShaderiv(vert, GL_INFO_LOG_LENGTH, &res);
		char* buf = new char[++res];
		glGetShaderInfoLog(vert, res, 0, buf);
		log_err("vertex shader error in %s: %s", shader_name, buf);
		exit(1);
	}
	glAttachShader(id, vert);
	int frag = glCreateShader(GL_FRAGMENT_SHADER);
	srcs[1] = PRECISION_STR "#define FRAGMENT\n";
	glShaderSource(frag, 3, srcs, 0);
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &res);
	if(res != GL_TRUE) {
		glGetShaderiv(frag, GL_INFO_LOG_LENGTH, &res);
		char* buf = new char[++res];
		glGetShaderInfoLog(frag, res, 0, buf);
		log_err("fragment shader error in %s: %s", shader_name, buf);
		exit(1);
	}
	glAttachShader(id, frag);
	if(kind == 0) {
		glBindAttribLocation(id, 0, "in_pos");
		glBindAttribLocation(id, 1, "in_uv");
	} else if(kind == 1) {
		glBindAttribLocation(id, 0, "in_uv");
		glBindAttribLocation(id, 1, "in_color");
		glBindAttribLocation(id, 2, "in_color_off");
		glBindAttribLocation(id, 3, "in_pos");
	} else if(kind == 2) {
		glBindAttribLocation(id, 0, "in_weights");
		glBindAttribLocation(id, 1, "in_bone_ids");
		glBindAttribLocation(id, 2, "in_uv");
		glBindAttribLocation(id, 3, "in_normal");
		glBindAttribLocation(id, 4, "in_pos");
	}
	glLinkProgram(id);
	glGetProgramiv(id, GL_LINK_STATUS, &res);
	if(res != GL_TRUE) {
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &res);
		char* buf = new char[++res];
		glGetProgramInfoLog(id, res, 0, buf);
		log_err("linking error in %s: %s\n", shader_name, buf);
		exit(1);
	}
	glUseProgram(id);
	loc_mat = glGetUniformLocation(id, "o_mat");
	loc_mv_mat = glGetUniformLocation(id, "o_mv_mat");
	loc_tex_size = glGetUniformLocation(id, "tex_size");
	loc_light_type = glGetUniformLocation(id, "light_type");
	loc_light_pos = glGetUniformLocation(id, "light_pos");
	loc_light_color = glGetUniformLocation(id, "light_color");
	loc_ambient_color = glGetUniformLocation(id, "ambient_color");
	//glUniform1i(glGetUniformLocation(id, "tex"), 0);
}

void Shader::use() {
	glUseProgram(id);
}

Shader shader_bicubic, shader_bicubic_batched, shader_copy_batched, shader_alpha_batched, shader_lighted, shader_push, shader_calc_grad, shader_push_grad;
void load_shaders() {
#ifndef USE_GLES2
	shader_push.load("anime4k_push",
#include <anime4k_push.glsl.inc>
	, 0);
	shader_calc_grad.load("anime4k_calc_grad",
#include <anime4k_calc_grad.glsl.inc>
	, 0);
	shader_push_grad.load("anime4k_push_grad",
#include <anime4k_push_grad.glsl.inc>
	, 0);
#endif
	shader_bicubic.load("bicubic",
#include <bicubic.glsl.inc>
	, 0);
	shader_bicubic_batched.load("bicubic",
#include <bicubic_batched.glsl.inc>
	, 1);
	shader_copy_batched.load("copy",
#include <copy_batched.glsl.inc>
	, 1);
	shader_alpha_batched.load("alpha",
#include <alpha_batched.glsl.inc>
	, 1);
	shader_lighted.load("lighted",
#include <lighted.glsl.inc>
	, 2);
}
