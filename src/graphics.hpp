#pragma once
#include <fs.hpp>
#include <glm/glm.hpp>

// this is a bitfield to simplify the code in image_data.cpp
// platforms required to support at least one format with both color and alpha channels
enum {
	TEX_FORMAT_RGBA = 1,
	TEX_FORMAT_BGRA = 2,
	TEX_FORMAT_RGBX = 4,
	TEX_FORMAT_BGRX = 8,
	TEX_FORMAT_RGB = 16,
	TEX_FORMAT_BGR = 32,
	TEX_FORMAT_4444 = 64,
	TEX_FORMAT_5551 = 128,
	TEX_FORMAT_565 = 256,
	TEX_FORMAT_ALPHA = 512,
	TEX_FORMAT_ALPHA4 = 1024,
	TEX_FORMAT_GRAY = 2048,
	TEX_FORMAT_GRAY_ALPHA = 4096
};

#ifdef PLATFORM_PC
#include <platform/pc/pc_defines.hpp>
#elif defined(PLATFORM_ANDROID)
#include <platform/android/android_defines.hpp>
#elif defined(PLATFORM_PS4)
#include <platform/ps4/ps4_defines.hpp>
#elif defined(PLATFORM_PS3)
#include <platform/ps3/ps3_defines.hpp>
#elif defined(PLATFORM_PSP)
#include <platform/psp/psp_defines.hpp>
#elif defined(PLATFORM_WII_U)
#include <platform/wiiu/wiiu_defines.hpp>
#elif defined(PLATFORM_SWITCH)
#include <platform/switch/switch_defines.hpp>
#else
#include <platform/null/null_graphics.hpp>
#endif

#ifndef FONT_ATLAS_SIZE
#define FONT_ATLAS_SIZE LARGE_FONT_ATLAS_SIZE
#endif

extern bool unlock_fps;
extern float delta;

struct ImageData {
	u8* buf;
	// dimensions that the game code expects
	u16 virtual_w;
	u16 virtual_h;
	// dimensions of the actual image data
	u16 pix_w;
	u16 pix_h;
	// dimensions of the buffer (may be larger than pix because of power of two requirements)
	u16 buf_w;
	u16 buf_h;
	int flags;
	u16 fmt;
	u8 ownership;

	void resample(u16 dst_fmt, u16 dst_w, u16 dst_h, u16 dst_buf_w, u16 dst_buf_h);
	void optimize();
	u32 buf_size();
	void destroy();
	static int fmt_stride(u16 fmt);
	// how many bytes a string of num pixels would be in fmt
	static int fmt_len(u32 num, u16 fmt);
	static void optimize_size(u16 format, u16& buf_w, u16& buf_h);
};

struct TextureData: TextureDataExtraFields {
	u16 w;
	u16 h;
#ifdef TEXTURE_POWER_OF_TWO
	u16 buf_w;
	u16 buf_h;
#endif
	u16 pix_w;
	u16 pix_h;
	u32 rc = 0;
	u32 id;
	std::string filename;
};

struct TextureRef {
	TextureData* data = nullptr;

	TextureRef();
	TextureRef(const TextureRef& o);
	void operator=(const TextureRef& o);
	~TextureRef();
	operator bool();
	TextureData* operator->();
	const TextureData* operator->() const;
	// safely destroys this reference and resets it to nullptr
	void destroy();
	int width() const;
	int height() const;
	void upload_sub(void* data, int x, int y, int w, int h, int fmt);
	// in platform, may return all-0 if impossible
	// this function was once needed for a reason i don't even remember
	// absolutely should not be used, why is it still a thing
	glm::vec4 get_pixel(int x, int y, int tw, int th) const;
private:
	// in platform
	void destroy_impl();
	// in platform, must accept data = nullptr for zeroing out regions
	void upload_sub_impl(void* data, int x, int y, int w, int h);
};

TextureRef g_load_texture(DataStream&& ds, int flags = 0);
TextureRef g_load_texture(DataStream& ds, int flags = 0);

// ==================================== transformations

extern int draw_calls, polys;
extern int physical_width, physical_height;
void g_reset_stacks();
extern std::vector<glm::mat4> matrix_stack;
extern glm::mat4 g_cur_matrix;
extern glm::mat4 g_proj_matrix;
extern glm::mat4 g_default_proj;
void g_identity();
void g_translate(float x, float y, float z = 0.f);
void g_scale(float x, float y, float z = 1.f);
void g_rotate(float rot, glm::vec3 axis = {0, 0, 1});
void g_push();
void g_pop();

void g_proj_reset();
void g_proj_ortho(float l, float r, float b, float t, float near, float far);
void g_proj_perspective(float fov, float aspect, float near, float far);

template<typename T>
struct TRect {
	union {
		glm::tvec4<T> vec;
		struct {
			T x, y, w, h;
		};
	};
	TRect(): vec() {}
	TRect(T x, T y, T w, T h): vec(x, y, w, h) {}
	TRect(glm::tvec4<T> vec): vec(vec) {}
	template<typename U>
	TRect(TRect<U> o): vec(o.vec) {}
	TRect<T> cut_corners(T amount) {
		return vec + glm::tvec4<T>(amount, amount, -(amount + amount), -(amount + amount));
	}
	TRect<T> move(T by_x, T by_y) {
		return vec + glm::tvec4<T>(by_x, by_y, 0, 0);
	}
	TRect<T> grow(T by_w, T by_h) {
		return vec + glm::tvec4<T>(0, 0, by_w, by_h);
	}
	TRect<T> intersection(TRect<T> o) {
		T new_x = glm::max(x, o.x);
		T new_y = glm::max(y, o.y);
		return TRect<T>(new_x, new_y,
			glm::min(glm::max((T) (x + w - new_x), (T) 0), glm::max((T) (o.x + o.w - new_x), (T) 0)),
			glm::min(glm::max((T) (y + h - new_y), (T) 0), glm::max((T) (o.y + o.h - new_y), (T) 0)));
	}
	bool contains(T point_x, T point_y) {
		return point_x >= x && point_y >= y && point_x < x + w && point_y < y + h;
	}
	bool touches(TRect<T> o) {
		return o.x + o.w >= x && o.y + o.h >= y && o.x < x + w && o.y < y + h;
	}
	void to_relative(T& point_x, T& point_y) {
		point_x -= x;
		point_y -= y;
	}
	TRect<T> to_relative(T inner_x, T inner_y, T inner_w, T inner_h) {
		return TRect<T>(inner_x - x, inner_y - y, inner_w, inner_h);
	}
	TRect<T> to_relative(TRect<T> inner) {
		return TRect<T>(inner.x - x, inner.y - y, inner.w, inner.h);
	}
	void to_absolute(T& point_x, T& point_y) {
		point_x += x;
		point_y += y;
	}
	TRect<T> to_absolute(T inner_x, T inner_y, T inner_w, T inner_h) {
		return TRect<T>(x + inner_x, y + inner_y, inner_w, inner_h);
	}
	TRect<T> to_absolute(TRect<T> inner) {
		return TRect<T>(x + inner.x, y + inner.y, inner.w, inner.h);
	}
	TRect<T> get_corner(T inner_w, T inner_h, u8 corner) {
		TRect<T> inner {x, y, inner_w, inner_h};
		switch(corner) {
			case G_CENTER:
				inner.x += w / 2 - inner.w / 2;
				inner.y += h / 2 - inner.h / 2;
				break;
			case G_TOP_LEFT:
				break;
			case G_TOP_RIGHT:
				inner.x += w - inner.w;
				break;
			case G_BOTTOM_LEFT:
				inner.y += h - inner.h;
				break;
			case G_BOTTOM_RIGHT:
				inner.x += w - inner.w;
				inner.y += h - inner.h;
				break;
			case G_CENTER_TOP:
				inner.x += w / 2 - inner.w / 2;
				break;
			case G_CENTER_BOTTOM:
				inner.x += w / 2 - inner.w / 2;
				inner.y += h - inner.h;
				break;
			case G_CENTER_LEFT:
				inner.y += h / 2 - inner.h / 2;
				break;
			case G_CENTER_RIGHT:
				inner.x += w - inner.w;
				inner.y += h / 2 - inner.h / 2;
				break;
		}
		return inner;
	}
	TRect<T> get_corner_relative(T inner_w, T inner_h, u8 corner) {
		TRect<T> inner {0, 0, inner_w, inner_h};
		switch(corner) {
			case G_CENTER:
				inner.x += w / 2 - inner.w / 2;
				inner.y += h / 2 - inner.h / 2;
				break;
			case G_TOP_LEFT:
				break;
			case G_TOP_RIGHT:
				inner.x += w - inner.w;
				break;
			case G_BOTTOM_LEFT:
				inner.y += h - inner.h;
				break;
			case G_BOTTOM_RIGHT:
				inner.x += w - inner.w;
				inner.y += h - inner.h;
				break;
			case G_CENTER_TOP:
				inner.x += w / 2 - inner.w / 2;
				break;
			case G_CENTER_BOTTOM:
				inner.x += w / 2 - inner.w / 2;
				inner.y += h - inner.h;
				break;
			case G_CENTER_LEFT:
				inner.y += h / 2 - inner.h / 2;
				break;
			case G_CENTER_RIGHT:
				inner.x += w - inner.w;
				inner.y += h / 2 - inner.h / 2;
				break;
		}
		return inner;
	}
};

using RectFloat = TRect<float>;
using RectInt = TRect<i16>;
using Rect8 = TRect<i8>;

void g_push_scissor(RectFloat scissor);
void g_pop_scissor();
// are these names accurate?? i dont even know if g_virtual_scissor is actually in virtual space or why it works
extern RectFloat g_virtual_scissor;
extern RectInt g_real_scissor;

enum {
	G_BLEND_SRC = 0x813,
	G_BLEND_DST,
	G_BLEND_ADD,
	G_BLEND_MUL
};

extern glm::vec4 g_cur_color;
extern glm::vec4 g_cur_color_2; // for text gradients
extern int g_cur_color_2_blend;
extern glm::vec4 g_cur_color_off;
void g_push_color(float r, float g, float b, float a);
void g_push_color(glm::vec4 c);
void g_pop_color();
void g_set_color_2(float r, float g, float b, float a, int blend = G_BLEND_MUL);
void g_set_color_2(glm::vec4 c, int blend = G_BLEND_MUL);
glm::vec4 g_blend(glm::vec4 a, glm::vec4 b, int blend = G_BLEND_MUL);
void g_push_color_off(float r, float g, float b, float a);
void g_pop_color_off();

// ==================================== general rendering

struct HaloSettings {
	float halo_lightness = 0.f;
	float halo_alpha = 0.5f;
	float halo_distance = 2;
	float shadow_alpha = 0.5f;
	float shadow_distance = 2;
	i8 shadow_detail = 0; // how many to draw, set it to -1 for an automatic smooth detail level
	i8 halo_detail = 0; // how many to draw, set it to -1 for an automatic smooth detail level
	int calc_shadow_detail = 0;
	int calc_halo_detail = 0;

	static HaloSettings with_shadow();
	int get_shadow_detail();
	int get_halo_detail();
	void resolve();
};

void g_calc(float* x, float* y, float* w, float* h, float tw, float th, int corner, int origin);
void g_calc_point(int* x, int* y, int origin);
void g_calc_corner(i16& obj_x, i16& obj_y, u16 obj_w, u16 obj_h, u16 bound_w, u16 bound_h, u8 corner);
void g_draw(const TextureRef& tex, float x, float y, int corner = G_TOP_LEFT, int origin = G_TOP_LEFT);
void g_draw_crop(const TextureRef& tex, float x, float y, int tx, int ty, int tw, int th, int corner = G_TOP_LEFT, int origin = G_TOP_LEFT);
void g_draw_quad(const TextureRef& tex, float x, float y, float w, float h, int corner = G_TOP_LEFT, int origin = G_TOP_LEFT);
void g_draw_quad_crop(const TextureRef& tex, float x, float y, float w, float h, float tx, float ty, float tw, float th, int corner = G_TOP_LEFT, int origin = G_TOP_LEFT);
void g_draw_quad_crop_halo(const TextureRef& tex, float x, float y, float w, float h, float tx, float ty, float tw, float th, HaloSettings halo, int corner = G_TOP_LEFT, int origin = G_TOP_LEFT);
void g_draw_quad_crop_3d(const TextureRef& tex, float x, float y, float z, float w, float h, float tx, float ty, float tw, float th, int corner = G_TOP_LEFT);
TextureRef& g_white_tex();
TextureRef& g_grad_tex();
TextureRef& g_grad_corner_tex();

extern int width, height, real_width, real_height, real_screen_x, real_screen_y;
void g_set_virtual_size(int w, int h);
inline int g_width() {return width;}
inline int g_height() {return height;}
inline int g_real_width() {return real_width;}
inline int g_real_height() {return real_height;}
void g_virtual_to_real(float* x, float* y);
void g_real_to_virtual(float* x, float* y);

enum {
	TEX_NEAREST = 1, // implies no upscale
	TEX_LINEAR = 2, // implies no upscale
	TEX_NO_UPSCALE = 4, // prohibit upscaling this texture
	TEX_DYNAMIC = 8, // hint that it will be updated often
	TEX_FAVOR_COLOR = 16 // favor color bit depth over alpha (not default because 1-bit alpha is often noticeable)
};

enum {
	OWNERSHIP_KEEP, // caller owns and function should not free or modify it
	OWNERSHIP_OWNED, // function owns and should free it when no longer needed
	OWNERSHIP_MUTABLE, // function can modify it, but not free it
	OWNERSHIP_PERMANENT // function can modify it, but not free it, and can rely on it not being freed
};

TextureRef g_upload_texture2(void* data, int virtual_w, int virtual_h, int pix_w, int pix_h, int buf_w, int buf_h, int format, int flags, u8 ownership = OWNERSHIP_KEEP);
TextureRef g_upload_texture(void* data, int w, int h, int format, int flags, u8 ownership = OWNERSHIP_KEEP);
// to be implemented by the platform
TextureRef _g_upload_texture(ImageData& imd);
struct UpscaleRequest {
	u16 w;
	u16 h;
};
UpscaleRequest g_check_upscale(TextureRef& tex, float w, float h, float tw, float th);
void g_ensure_upscale(TextureRef& tex, int w, int h);
int g_real_width();
int g_real_height();
void g_set_max_aspect(int w, int h);
void g_set_min_aspect(int w, int h);
void update_screen_size();
void sys_show_logs();
void sys_exit();

struct PackedBVec4 {
	u8 x, y, z, w;
	inline PackedBVec4() {}
	inline PackedBVec4(glm::u8vec4 v): x(v.x), y(v.y), z(v.z), w(v.w) {}
	inline PackedBVec4(glm::vec4 v): x(v.x), y(v.y), z(v.z), w(v.w) {}
	operator glm::u8vec4() {return {x, y, z, w};}
};

struct Vertex {
#ifdef LOW_BANDWIDTH
	using vec4_t = glm::i16vec4;
	using vec3_t = glm::i16vec3;
	using vec2_t = glm::i16vec2;
	glm::u16vec2 uv;
	PackedBVec4 color;
	vec3_t pos;
	u8 padding[2];
#else
	using vec4_t = glm::vec4;
	using vec3_t = glm::vec3;
	using vec2_t = glm::vec2;
	glm::vec2 uv;
	PackedBVec4 color;
#ifndef NO_COLOR_OFF_ATTRIB
	glm::vec4 color_off;
#endif
	vec3_t pos;
#endif
	void set(const vec3_t& pos, const glm::vec2& uv, const glm::u8vec4& color);
};

#ifndef CUSTOM_BATCH_ALLOC
template<typename T>
using BatchAllocator = std::allocator<T>;
#endif
struct DrawBatch {
	std::vector<Vertex, BatchAllocator<Vertex>> vertices;
	std::vector<u16, BatchAllocator<u16>> indices;
	TextureRef tex;
	u16 vert_count = 0;
	u16 index_count = 0;
	u16 upscale_w = 0;
	u16 upscale_h = 0;
	u32 rc = 0;
	void draw();
};
using DrawBatchRef = Rc<DrawBatch>;
DrawBatchRef g_new_batch();
void g_push_batch(DrawBatchRef batch);
void g_pop_batch();
void g_flush(DrawBatchRef batch);
void g_flush();

// ==================================== 3D rendering

#define MAX_LIGHTS 4

enum {
	G_POINT_LIGHT = 1,
	G_DIRECTIONAL_LIGHT,
	G_SPOT_LIGHT
};

struct Light {
	u8 type = 0;
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec3 spot_direction;
};

extern Light lights[MAX_LIGHTS];
extern bool lights_on;
extern glm::vec3 g_ambient_color;

void g_light(int i, int type, glm::vec3 pos, glm::vec3 color);
void g_light_ambient(glm::vec3 ambient);
void g_erase_lights();

struct Material {
	float metallic;
	float roughness;
	float specular;
	TextureRef tex;
};

struct Vertex3D {
	glm::vec4 weights;
	glm::u8vec4 bone_ids;
	glm::vec2 uv;
	glm::vec3 normal;
	glm::vec3 pos;
};

constexpr int j = sizeof(Vertex3D);

struct MeshLoadData {
	std::vector<Vertex3D> verts;
	std::vector<u16> indices;
	glm::mat4 local_transform;
	Material material;
};

struct MeshLiveData {
	u32 rc = 0;
	void* vbo;
	void* ibo;
	u32 count;
	glm::mat4 local_transform;
	Material material;
	~MeshLiveData();
};
using MeshRef = Rc<MeshLiveData>;

struct SceneData {
	u32 rc = 0;
	std::vector<MeshRef> meshes;
	std::vector<Light> lights;
	std::vector<Rc<SceneData>> children;
	glm::mat4 transform;
};
using SceneRef = Rc<SceneData>;

MeshRef g_upload_mesh(MeshLoadData& data);
SceneRef g_load_scene(DataStream* ds);
void g_draw_mesh(const MeshRef& mesh);
void g_draw_scene(const SceneRef& scene);

// ==================================== text

struct TextInfo {
	float w;
	float h;
	float end_x;
	float end_y;
};

std::u32string g_wrap_text(const std::u32string& text, float w, float h);
TextInfo g_calc_text(const std::u32string& text, float w, float h);
TextInfo g_calc_text(const std::string& text, float w, float h);
void glyph_cache_frame_end();
TextInfo g_draw_text(const std::u32string& text, float x, float y, float w, float h, HaloSettings halo = {}, int corner = G_TOP_LEFT, int origin = G_TOP_LEFT);
TextInfo g_draw_text(const std::string& text, float x, float y, float w, float h, HaloSettings halo = {}, int corner = G_TOP_LEFT, int origin = G_TOP_LEFT);
float g_text_line_height(float h);

int scale(int to_scale, int sh, int ogh = 600);
float _720p(float v);
inline float operator""_720p(unsigned long long v) {return _720p(v);}
float _600p(float v);
inline float operator""_600p(unsigned long long v) {return _600p(v);}
float _480p(float v);
inline float operator""_480p(unsigned long long v) {return _480p(v);}

template<typename T>
inline T lerp(T x, T y, float t) {
	return x + (y - x) * t;
}

template<typename T>
inline T lerpclamp(T x, T y, float t) {
	return glm::clamp(x + (y - x) * t, glm::min(x, y), glm::max(x, y));
}

// T must be >= 0
template<typename T>
inline T creepclamp(T x, T y, T amount) {
	if(y > x) {
		return glm::min(x + amount, y);
	} else {
		return glm::max(x - amount, y);
	}
}
