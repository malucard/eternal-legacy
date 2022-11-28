#include "gl1_gfx.hpp"
#include "gl1_header.hpp"
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
	g_proj_reset();
	if(physical_width && physical_height) api_frame();
}

// ==================================== setup

bool upscaling_enabled = false;
unsigned texture_max_width, texture_max_height;
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

void gl_init() {
	sys_gfx_vendor = (const char*) glGetString(GL_VENDOR);
	sys_gfx_version = (const char*) glGetString(GL_VERSION);
	sys_gfx_renderer = (const char*) glGetString(GL_RENDERER);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*) &texture_max_width);
	texture_max_height = texture_max_width;
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	g_default_proj = glm::ortho(0.f, 1.f, 1.f, 0.f, -1000.f, 1000.f);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, glm::value_ptr(glm::vec4(1, 1, 1, 1)));
}

int fmt_to_gl(int format, int& internal_format, int& type) {
	type = GL_UNSIGNED_BYTE;
	switch(format) {
		case TEX_FORMAT_RGBA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			return internal_format = GL_RGBA;
		case TEX_FORMAT_BGRA:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			return internal_format = GL_BGRA;
		case TEX_FORMAT_RGBX:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			return internal_format = GL_RGB;
		case TEX_FORMAT_BGRX:
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			return internal_format = GL_BGR;
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
			internal_format = GL_RGB5;
			return GL_RGB;
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
	glGenTextures(1, (GLuint*) &id);
	glBindTexture(GL_TEXTURE_2D, id);
	int internal_format, type, glfmt = fmt_to_gl(imd.fmt, internal_format, type);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, imd.buf_w, imd.buf_h, 0, glfmt, type, imd.buf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (imd.flags & TEX_NEAREST)? GL_NEAREST: GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (imd.flags & TEX_NEAREST)? GL_NEAREST: GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	TextureRef tex;
	tex.data = new TextureData;
	tex->id = id;
	tex->rc = 1;
	tex->w = imd.virtual_w;
	tex->h = imd.virtual_h;
	tex->pix_w = imd.pix_w;
	tex->pix_h = imd.pix_h;
#ifdef TEXTURE_POWER_OF_TWO
	tex->buf_w = imd.buf_w;
	tex->buf_h = imd.buf_h;
#endif
	tex->format = imd.fmt;
	tex->filter = (imd.flags & TEX_NEAREST)? TEX_NEAREST: (imd.flags & TEX_LINEAR)? TEX_LINEAR: 0;
	return tex;
}

void TextureRef::destroy_impl() {
	if(data->id) {
		glDeleteTextures(1, (GLuint*) &data->id);
		data->id = 0;
	}
}

glm::vec4 TextureRef::get_pixel(int x, int y, int tw, int th) const {
#ifdef USE_GLES2
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

void g_ensure_upscale(TextureRef& tex, int dw, int dh) {}

// ==================================== 3D rendering

MeshRef g_upload_mesh(MeshLoadData& data) {
	MeshLiveData* live = new MeshLiveData {
		.rc = 0,
		.vbo = data.verts.data(),
		.ibo = data.indices.data(),
		.count = (u32) data.indices.size(),
		.local_transform = data.local_transform,
		.material = data.material
	};
	new(&data.verts) std::vector<Vertex3D>;
	new(&data.indices) std::vector<u16>;
	return MeshRef(live);
}

MeshLiveData::~MeshLiveData() {
	delete[] (Vertex3D*) vbo;
	delete[] (u16*) ibo;
}

void setup_lights() {
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, glm::value_ptr(glm::vec4(g_ambient_color, 1)));
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, glm::value_ptr(glm::vec4(1, 1, 1, 1)));
	int light_type[MAX_LIGHTS];
	glm::vec3 light_pos[MAX_LIGHTS];
	glm::vec3 light_color[MAX_LIGHTS];
	for(int i = 0; i < MAX_LIGHTS; i++) {
		GLenum id = GL_LIGHT0 + i;
		switch(lights[i].type) {
			case G_POINT_LIGHT:
				glEnable(id);
				glLightfv(id, GL_DIFFUSE, glm::value_ptr(glm::vec4(lights[i].color, 1)));
				glLightfv(id, GL_POSITION, glm::value_ptr(glm::vec4(lights[i].pos, 1)));
				break;
			case G_SPOT_LIGHT:
				glEnable(id);
				glLightfv(id, GL_DIFFUSE, glm::value_ptr(glm::vec4(lights[i].color, 1)));
				glLightfv(id, GL_POSITION, glm::value_ptr(glm::vec4(lights[i].pos, 1)));
				break;
			case G_DIRECTIONAL_LIGHT:
				glEnable(id);
				glLightfv(id, GL_DIFFUSE, glm::value_ptr(glm::vec4(lights[i].color, 1)));
				glLightfv(id, GL_POSITION, glm::value_ptr(glm::vec4(lights[i].pos, 0)));
				break;
			default:
				glDisable(id);
		}
		light_type[i] = lights[i].type;
		light_pos[i] = lights[i].pos;
		light_color[i] = lights[i].color;
	}
}

void g_draw_mesh(const MeshRef& mesh) {
	int sw = g_real_width(), sh = g_real_height();
	glScissor(g_real_scissor.x + real_screen_x, sh - g_real_scissor.y - g_real_scissor.h + real_screen_y, g_real_scissor.w, g_real_scissor.h);
	glViewport(real_screen_x, real_screen_y, sw, sh);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_LIGHTING);
	setup_lights();
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(g_proj_matrix));
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(g_cur_matrix));
#ifdef TEXTURE_POWER_OF_TWO
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef((float) mesh->material.tex->pix_w / mesh->material.tex->buf_w, (float) mesh->material.tex->pix_h / mesh->material.tex->buf_h, 1);
#endif
	glBindTexture(GL_TEXTURE_2D, mesh->material.tex->id);
	glEnable(GL_DEPTH_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex3D), (u8*) mesh->vbo + offsetof(Vertex3D, uv));
	glNormalPointer(GL_FLOAT, sizeof(Vertex3D), (u8*) mesh->vbo + offsetof(Vertex3D, normal));
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex3D), (u8*) mesh->vbo + offsetof(Vertex3D, pos));
	glDrawElements(GL_TRIANGLES, mesh->count, GL_UNSIGNED_SHORT, mesh->ibo);
	draw_calls++;
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_LIGHTING);
}

// ==================================== 2D rendering

void DrawBatch::draw() {
	int sw = g_real_width(), sh = g_real_height();
	glViewport(real_screen_x, real_screen_y, sw, sh);
	glScissor(g_real_scissor.x + real_screen_x, sh - g_real_scissor.y - g_real_scissor.h + real_screen_y, g_real_scissor.w, g_real_scissor.h);
	glBindTexture(GL_TEXTURE_2D, tex->id);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(g_proj_matrix));
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#ifdef TEXTURE_POWER_OF_TWO
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
#endif
	if(lights_on) {
		glEnable(GL_LIGHTING);
		setup_lights();
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), (u8*) vertices.data() + offsetof(Vertex, uv));
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), (u8*) vertices.data() + offsetof(Vertex, color));
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), (u8*) vertices.data() + offsetof(Vertex, pos));
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, indices.data());
	draw_calls++;
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if(lights_on) glDisable(GL_LIGHTING);
}
