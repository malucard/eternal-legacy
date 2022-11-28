#include "gl_gfx.hpp"
#include "gl_header.hpp"
#include <graphics.hpp>
#include <api.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ==================================== main loop

void api_frame();
void gl_frame() {
	glViewport(0, 0, physical_width, physical_height);
	glScissor(0, 0, physical_width, physical_height);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	g_reset_stacks();
	g_proj_reset();
	if(physical_width && physical_height) api_frame();
}

// ==================================== setup

unsigned default_framebuffer = 0;
bool upscaling_enabled = false;
unsigned TEXTURE_MAX_WIDTH, TEXTURE_MAX_HEIGHT;

static u32 square, fbo, fblumtex;
float square_data[] {
	0.f, 1.f, 0.f,
	0.f, 1.f,
	1.f, 0.f, 0.f,
	1.f, 0.f,
	0.f, 0.f, 0.f,
	0.f, 0.f,
	0.f, 1.f, 0.f,
	0.f, 1.f,
	1.f, 0.f, 0.f,
	1.f, 0.f,
	1.f, 1.f, 0.f,
	1.f, 1.f
};

void load_shaders();
void gl_init() {
	sys_gfx_vendor = (const char*) glGetString(GL_VENDOR);
	sys_gfx_version = (const char*) glGetString(GL_VERSION);
	sys_gfx_renderer = (const char*) glGetString(GL_RENDERER);
	load_shaders();
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*) &TEXTURE_MAX_WIDTH);
	TEXTURE_MAX_HEIGHT = TEXTURE_MAX_WIDTH;
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
#ifdef USE_GL2
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_MULTISAMPLE);
#endif
	glGenBuffers(1, &square);
	glBindBuffer(GL_ARRAY_BUFFER, square);
	glBufferData(GL_ARRAY_BUFFER, sizeof(square_data), square_data, GL_STATIC_DRAW);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	g_default_proj = glm::ortho(0.f, 1.f, 1.f, 0.f, -1000.f, 1000.f);
#ifndef USE_GLES2
#if defined(USE_GLES3) || defined(PLATFORM_SWITCH)
	if(true) {
#else
	if(GLEW_EXT_framebuffer_object) {
#endif
		upscaling_enabled = true;
		log_info("Anime4k upscaling enabled");
		glGenFramebuffersEXT(1, &fbo);
		glGenTextures(1, &fblumtex);
	}
#endif
}

#ifdef USE_GLES2 // GLES2 does not support swizzling so no BGR or 16-bit formats, also no RGBX
int fmt_to_gl(int format, int& internal_format, int& type) {
	type = GL_UNSIGNED_BYTE;
	switch(format) {
		case TEX_FORMAT_RGBA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			return internal_format = GL_RGBA;
		case TEX_FORMAT_RGB:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			return internal_format = GL_RGB;
		case TEX_FORMAT_ALPHA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			return internal_format = GL_ALPHA;
		case TEX_FORMAT_GRAY:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			return internal_format = GL_LUMINANCE;
		case TEX_FORMAT_GRAY_ALPHA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			return internal_format = GL_LUMINANCE_ALPHA;
	}
	platform_throw();
	return internal_format = GL_RGBA;
}
#elif defined(USE_GLES3)
int fmt_to_gl(int format, int& internal_format, int& type) {
	type = GL_UNSIGNED_BYTE;
	switch(format) {
		case TEX_FORMAT_BGRA:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		case TEX_FORMAT_RGBA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			return internal_format = GL_RGBA;
		case TEX_FORMAT_BGRX:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		case TEX_FORMAT_RGBX:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			return internal_format = GL_RGBA;
		case TEX_FORMAT_BGR:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		case TEX_FORMAT_RGB:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			return internal_format = GL_RGB;
		case TEX_FORMAT_4444:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ALPHA);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_GREEN);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
			type = GL_UNSIGNED_SHORT_4_4_4_4;
			internal_format = GL_RGBA4;
			return GL_RGBA;
			// GLES does not support 1_5_5_5 so no 5551 format
		case TEX_FORMAT_565:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
			type = GL_UNSIGNED_SHORT_5_6_5;
			internal_format = GL_RGB565;
			return GL_RGB;
#ifdef GL_LUMINANCE
		case TEX_FORMAT_ALPHA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			return internal_format = GL_ALPHA;
		case TEX_FORMAT_GRAY:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			return internal_format = GL_LUMINANCE;
		case TEX_FORMAT_GRAY_ALPHA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			return internal_format = GL_LUMINANCE_ALPHA;
#else
		case TEX_FORMAT_ALPHA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
			return internal_format = GL_RED;
		case TEX_FORMAT_GRAY:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
			return internal_format = GL_RED;
		case TEX_FORMAT_GRAY_ALPHA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_GREEN);
			return internal_format = GL_RG;
#endif
	}
	platform_throw();
	return internal_format = GL_RGBA;
}
#else
int fmt_to_gl(int format, int& internal_format, int& type) {
	type = GL_UNSIGNED_BYTE;
	switch(format) {
		case TEX_FORMAT_RGBA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			return internal_format = GL_RGBA;
		case TEX_FORMAT_BGRA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			return internal_format = GL_BGRA;
		case TEX_FORMAT_BGR:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			return internal_format = GL_BGR;
		case TEX_FORMAT_RGB:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			return internal_format = GL_RGB;
		case TEX_FORMAT_4444:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
			internal_format = GL_RGBA4;
			return GL_RGBA;
		case TEX_FORMAT_5551:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
			internal_format = GL_RGB5_A1;
			return GL_RGBA;
		case TEX_FORMAT_565:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			type = GL_UNSIGNED_SHORT_5_6_5_REV;
			internal_format = GL_RGB565;
			return GL_RGB;
		case TEX_FORMAT_ALPHA4:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			type = GL_UNSIGNED_BYTE;
			internal_format = GL_ALPHA4;
			return GL_ALPHA;
		case TEX_FORMAT_ALPHA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			return internal_format = GL_ALPHA;
		case TEX_FORMAT_GRAY:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			return internal_format = GL_LUMINANCE;
		case TEX_FORMAT_GRAY_ALPHA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			return internal_format = GL_LUMINANCE_ALPHA;
	}
	platform_throw();
	return internal_format = GL_RGBA;
}
#endif

void TextureRef::upload_sub_impl(void* data, int x, int y, int w, int h) {
	glBindTexture(GL_TEXTURE_2D, this->data->id);
	if(data) {
		int internal_format, type, glfmt = fmt_to_gl(this->data->format, internal_format, type);
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, glfmt, type, data);
	} else {
		data = calloc(1, w * h * 4);
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
		free(data);
	}
}

TextureRef _g_upload_texture(ImageData& imd) {
	u32 id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	int internal_format, type, glfmt = fmt_to_gl(imd.fmt, internal_format, type);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, imd.buf_w, imd.buf_h, 0, glfmt, type, imd.buf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (imd.flags & TEX_NEAREST)? GL_NEAREST: GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (imd.flags & TEX_NEAREST)? GL_NEAREST: GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	TextureRef tex;
	tex.data = new TextureData;
	tex->id = id;
	tex->oid = id;
	tex->rc = 1;
	tex->w = imd.pix_w;
	tex->h = imd.pix_h;
	tex->pix_w = imd.pix_w;
	tex->pix_h = imd.pix_h;
	tex->upw = imd.pix_w;
	tex->uph = imd.pix_h;
	tex->upscale = !(imd.flags & (TEX_NEAREST | TEX_LINEAR | TEX_NO_UPSCALE | TEX_DYNAMIC)) && imd.fmt < TEX_FORMAT_ALPHA;
	tex->format = imd.fmt;
	tex->filter = (imd.flags & TEX_NEAREST)? TEX_NEAREST: (imd.flags & TEX_LINEAR)? TEX_LINEAR: 0;
	return tex;
}

void TextureRef::destroy_impl() {
	if(data->id != data->oid) {
		glDeleteTextures(1, &data->oid);
	}
	glDeleteTextures(1, &data->id);
}

glm::vec4 TextureRef::get_pixel(int x, int y, int tw, int th) const {
#if defined(USE_GLES2) || defined(USE_GLES3)
	return glm::vec4();
#else
	u32* pixels = new u32[data->w * data->h];
	glBindTexture(GL_TEXTURE_2D, data->id);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	u32 pix = pixels[y * data->h / th * data->w + x * data->w / tw];
	delete[] pixels;
	return glm::vec4(
		(pix & 0xFF) / 255.f,
		(pix >> 8 & 0xFF) / 255.f,
		(pix >> 16 & 0xFF) / 255.f,
		(pix >> 24 & 0xFF) / 255.f
	);
#endif
}

void g_ensure_upscale(TextureRef& tex, int dw, int dh) {
#if defined(USE_GLES3) || defined(USE_GL2)
	if(!tex->upscale || !upscaling_enabled) return;
	if(dw == -1) dw = ((i32) dh * tex->w + tex->h - 1) / tex->h;
	if(dh == -1) dh = ((i32) dw * tex->h + tex->w - 1) / tex->w;
	if(!dw || !dh) return;
	GLuint fbtex;
	if(tex->upw < (u16) dw && tex->uph < (u16) dh) {
		if(dw <= tex->w && dh <= tex->h) {	
			if(tex->oid != tex->id) glDeleteTextures(1, &tex->id);
			tex->id = tex->oid;
			tex->upw = dw;
			tex->uph = dh;
			return;
		}
		//glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
		glDisable(GL_SCISSOR_TEST);
		glBindFramebufferEXT(GL_FRAMEBUFFER, fbo);
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &fbtex);
		glBindTexture(GL_TEXTURE_2D, fbtex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dw, dh, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbtex, 0);
		glBindTexture(GL_TEXTURE_2D, fblumtex);
#ifdef USE_GLES2
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, dw, dh, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, dw, dh, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
#endif
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fblumtex, 0);
		GLenum db[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
		glDrawBuffers(2, db);
		int status;
		if((status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
			log_err("error %d with framebuffer; disabling upscaling", status);
			upscaling_enabled = false;
			glDeleteTextures(1, &fbtex);
			glViewport(0, 0, physical_width, physical_height);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBindFramebufferEXT(GL_FRAMEBUFFER, default_framebuffer);
			if(tex->oid != tex->id) glDeleteTextures(1, &tex->id);
			tex->id = tex->oid;
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glEnable(GL_SCISSOR_TEST);
			return;
		}
		glBindTexture(GL_TEXTURE_2D, tex->oid);
		glViewport(0, 0, dw, dh);
		//glClear(GL_COLOR_BUFFER_BIT);
		shader_bicubic.use();
		glUniformMatrix4fv(shader_bicubic.loc_mat, 1, 0, glm::value_ptr(glm::translate(glm::scale(glm::identity<glm::mat4>(), {1, -1, 1}), {0, -1, 0})));
		glUniform2f(shader_bicubic.loc_tex_size, (float) 1.f / tex->pix_w, (float) 1.f / tex->pix_h);
		glBindBuffer(GL_ARRAY_BUFFER, square);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(float) * 5, nullptr);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(float) * 5, (void*) (sizeof(float) * 3));
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		draw_calls++;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fbtex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, fblumtex);
		constexpr int iterations = 2;
		constexpr int div = 1;
		for(int i = 0; i < iterations; i++) {
			shader_push.use();
			glUniform1f(glGetUniformLocation(shader_push.id, "strength"), clamp((float) dh / tex->h / 5, 0.f, 1.f) / div);
			glUniform2f(shader_push.loc_tex_size, (float) 1.f / dw, (float) 1.f / dh);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			shader_calc_grad.use();
			glUniform2f(shader_calc_grad.loc_tex_size, (float) 1.f / dw, (float) 1.f / dh);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			shader_push_grad.use();
			glUniform1f(glGetUniformLocation(shader_push_grad.id, "strength"), clamp((float) dh / tex->h / 2, 0.f, 1.f) / div);
			glUniform1i(glGetUniformLocation(shader_push_grad.id, "lumtex"), 1);
			glUniform2f(shader_push_grad.loc_tex_size, (float) 1.f / dw, (float) 1.f / dh);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			draw_calls += 3;
		}
		glViewport(0, 0, physical_width, physical_height);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindFramebufferEXT(GL_FRAMEBUFFER, default_framebuffer);
		if(tex->oid != tex->id) glDeleteTextures(1, &tex->id);
		tex->id = fbtex;
		log_info("upscaled %d %d -> %d %d, %d", tex->w, tex->h, dw, dh, tex->upscale);
		tex->upw = dw;
		tex->uph = dh;
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glEnable(GL_SCISSOR_TEST);
	}
#endif
}

// ==================================== 3D rendering

MeshRef g_upload_mesh(MeshLoadData& data) {
	GLuint bufs[2];
	glGenBuffers(2, bufs);
	MeshLiveData* live = new MeshLiveData {
		.rc = 0,
		.vbo = (void*) (uintptr_t) bufs[0],
		.ibo = (void*) (uintptr_t) bufs[1],
		.count = (u32) data.indices.size(),
		.local_transform = data.local_transform,
		.material = data.material
	};
	glBindBuffer(GL_ARRAY_BUFFER, bufs[0]);
	glBufferData(GL_ARRAY_BUFFER, data.verts.size() * sizeof(Vertex3D), data.verts.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * 2, data.indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return MeshRef(live);
}

MeshLiveData::~MeshLiveData() {
	GLuint bufs[] {(GLuint) (uintptr_t) vbo, (GLuint) (uintptr_t) ibo};
	glDeleteBuffers(2, bufs);
}

void g_draw_mesh(const MeshRef& mesh) {
	i16 sw = g_real_width(), sh = g_real_height();
	glViewport(real_screen_x, real_screen_y, sw, sh);
	glScissor(g_real_scissor.x + real_screen_x, sh - g_real_scissor.y - g_real_scissor.h + real_screen_y, g_real_scissor.w, g_real_scissor.h);
	glEnable(GL_DEPTH_TEST);
	shader_lighted.use();
	glBindBuffer(GL_ARRAY_BUFFER, (GLuint) (uintptr_t) mesh->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint) (uintptr_t) mesh->ibo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, false, sizeof(Vertex3D), (void*) offsetof(Vertex3D, weights));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, false, sizeof(Vertex3D), (void*) offsetof(Vertex3D, bone_ids));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex3D), (void*) offsetof(Vertex3D, uv));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(Vertex3D), (void*) offsetof(Vertex3D, normal));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex3D), (void*) offsetof(Vertex3D, pos));
	glUniformMatrix4fv(shader_lighted.loc_mat, 1, 0, glm::value_ptr(g_proj_matrix));
	glUniformMatrix4fv(shader_lighted.loc_mv_mat, 1, 0, glm::value_ptr(g_cur_matrix));
	if(shader_lighted.loc_tex_size != -1) glUniform2f(shader_lighted.loc_tex_size, mesh->material.tex->w, mesh->material.tex->h);
	glUniform3f(shader_lighted.loc_ambient_color, g_ambient_color.x, g_ambient_color.y, g_ambient_color.z);
	int light_type[MAX_LIGHTS];
	glm::vec3 light_pos[MAX_LIGHTS];
	glm::vec3 light_color[MAX_LIGHTS];
	for(int i = 0; i < MAX_LIGHTS; i++) {
		light_type[i] = lights[i].type;
		light_pos[i] = lights[i].pos;
		light_color[i] = lights[i].color;
	}
	glUniform1iv(shader_lighted.loc_light_type, MAX_LIGHTS, light_type);
	glUniform3fv(shader_lighted.loc_light_pos, MAX_LIGHTS, (float*) light_pos);
	glUniform3fv(shader_lighted.loc_light_color, MAX_LIGHTS, (float*) light_color);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mesh->material.tex->id);
	//glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, indices.data());
	glDrawElements(GL_TRIANGLES, mesh->count, GL_UNSIGNED_SHORT, nullptr);
	polys += mesh->count / 3;
	draw_calls++;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisable(GL_DEPTH_TEST);
}

// ==================================== 2D rendering

uint batch_buf = 0;
uint batch_idx_buf = 0;

void DrawBatch::draw() {
	if(!batch_buf) {
		glGenBuffers(1, &batch_buf);
		glGenBuffers(1, &batch_idx_buf);
	}
	if(upscale_w) {
		g_ensure_upscale((TextureRef&) tex, upscale_w, upscale_h);
	}
	int sw = g_real_width(), sh = g_real_height();
	glViewport(real_screen_x, real_screen_y, sw, sh);
	glScissor(g_real_scissor.x + real_screen_x, sh - g_real_scissor.y - g_real_scissor.h + real_screen_y, g_real_scissor.w, g_real_scissor.h);
	glBindBuffer(GL_ARRAY_BUFFER, batch_buf);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch_idx_buf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(u16), indices.data(), GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, true, sizeof(Vertex), (void*) offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, sizeof(Vertex), (void*) offsetof(Vertex, color));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(Vertex), (void*) offsetof(Vertex, color_off));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(Vertex), (void*) offsetof(Vertex, pos));
	if(tex->format == TEX_FORMAT_ALPHA || tex->format == TEX_FORMAT_ALPHA4) {
		shader_alpha_batched.use();
		glUniformMatrix4fv(shader_alpha_batched.loc_mat, 1, 0, glm::value_ptr(g_proj_matrix));
		glUniform2f(shader_alpha_batched.loc_tex_size, tex->w, tex->h);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex->id);
	} else {
		Shader& shader = (upscale_w || tex->filter) && upscaling_enabled? shader_copy_batched: shader_bicubic_batched;
		shader.use();
		glUniformMatrix4fv(shader.loc_mat, 1, 0, glm::value_ptr(g_proj_matrix));
		if(shader.loc_tex_size != -1) glUniform2f(shader.loc_tex_size, (float) 1.f / tex->upw, (float) 1.f / tex->uph);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, upscale_w? tex->id: tex->oid);
	}
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, nullptr);
	polys += indices.size() / 3;
	draw_calls++;
}
