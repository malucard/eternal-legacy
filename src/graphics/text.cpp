#include <graphics.hpp>
#include <texture_atlas.h>
#ifdef PLATFORM_PS4
#include <orbis/Sysmodule.h>
#include <proto-include.h>
#else
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H
#endif
#include <codecvt>
#include <locale>
#include <map>

static bool init_ft = false;
static FT_Library ft;
static FT_Face face;

void check_init_freetype() {
	if(!init_ft) {
#ifdef PLATFORM_PS4
		if(sceSysmoduleLoadModule(0x009A) < 0) {
			log_err("could not load FreeType module: %s", strerror(errno));
			platform_throw();
		}
#endif
		if(FT_Init_FreeType(&ft)) {
			log_err("could not init FreeType");
			platform_throw();
		}
		if(FT_New_Face(ft, FileDataStream::to_real_path("res/n7/rounded-mgenplus-1m-medium.ttf").c_str(), 0, &face)) {
			log_err("could not load font");
			platform_throw();
		}
		init_ft = true;
	}
}

struct CachedGlyph {
	FT_GlyphSlotRec glyph;
	int frames_unused = 0;
	u32 vtex_id;
	RectInt bounds;
};

static std::map<u32, CachedGlyph> glyph_cache;
static TextureRef glyph_atlas_tex;
static Atlas* glyph_atlas;
static u32 glyph_age_oldest = 1;

void glyph_cache_frame_end() {
	glyph_age_oldest = 1;
	for(auto it = glyph_cache.begin(); it != glyph_cache.end();) {
		if(it->second.frames_unused >= glyph_age_oldest) {
			glyph_age_oldest = it->second.frames_unused;
			//atlas_destroy_vtex(glyph_atlas, it->second.vtex_id);
			//it = glyph_cache.erase(it);
		} else {
			it->second.frames_unused++;
		}
		it++;
	}
}

void glyph_cache_get_rid(int min) {
	for(auto it = glyph_cache.begin(); it != glyph_cache.end(); it++) {
		if(it->second.frames_unused >= min) {
			atlas_destroy_vtex(glyph_atlas, it->second.vtex_id);
			it = glyph_cache.erase(it);
		}
	}
}

static u32 ft_size = 0;

void set_ft_size(float font_height) {
	static u32 ft_size;
	u32 new_ft_size = std::ceil(font_height);
	if(font_height != ft_size) {
		ft_size = new_ft_size;
		FT_Set_Pixel_Sizes(face, 0, new_ft_size);
	}
}

void set_ft_size(u32 new_ft_size) {
	if(new_ft_size != ft_size) {
		ft_size = new_ft_size;
		log_info("%u", ft_size);
		FT_Set_Pixel_Sizes(face, 0, new_ft_size);
	}
}

CachedGlyph& g_load_glyph(char32_t c, u32 height) {
	if(height >= 0x7FF) height = 0x7FF; // font size limit at 2047 because unicode is 21-bit and 11 bits remain in key for font size
	u32 id = (u32) height << 21 | (c & ((1 << 21) - 1));
	auto it = glyph_cache.find(id);
	if(it != glyph_cache.end()) {
		it->second.frames_unused = 0;
		return it->second;
	}
	set_ft_size(height);
	//if(FT_Load_Glyph(face, FT_Get_Char_Index(face, c), FT_LOAD_RENDER | FT_LOAD_COMPUTE_METRICS)) {
	if(FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_COMPUTE_METRICS)) {
		log_err("failed to load glyph for %lc at height %u", c, height);
		platform_throw();
	}
	if(!glyph_atlas_tex.data) {
		glyph_atlas_tex = g_upload_texture(nullptr, FONT_ATLAS_SIZE, FONT_ATLAS_SIZE, TEX_FORMAT_ALPHA, TEX_LINEAR, false);
		if(!atlas_create(&glyph_atlas, FONT_ATLAS_SIZE, 1)) {
			log_err("failed creating glyph texture atlas");
			platform_throw();
		}
		log_info("font atlas size %d", atlas_get_dimensions(glyph_atlas));
	}
	u32 vtex_id;
	if(!atlas_gen_texture(glyph_atlas, &vtex_id)) {
		log_err("atlas_gen_texture failed");
		platform_throw();
	}
	FT_Bitmap& bitmap = face->glyph->bitmap;
	for(int i = glyph_age_oldest;; i >>= 1) {
		if(atlas_allocate_vtex_space(glyph_atlas, vtex_id, bitmap.width, bitmap.rows)) {
			break;
		} else {
			glyph_cache_get_rid(i);
			if(i == 0) {
				log_err("atlas_allocate_vtex_space failed, too much text on screen?");
				platform_throw();
			}
		}
	}
	u16 bounds[4];
	atlas_get_vtex_xywh_coords(glyph_atlas, vtex_id, 1, bounds);
	glyph_atlas_tex.upload_sub(nullptr, bounds[0], bounds[1], bounds[2], bounds[3], TEX_FORMAT_ALPHA);
	atlas_get_vtex_xywh_coords(glyph_atlas, vtex_id, 0, bounds);
	glyph_atlas_tex.upload_sub(bitmap.buffer, bounds[0], bounds[1], bounds[2], bounds[3], TEX_FORMAT_ALPHA);
	if((bitmap.width != 0) && bounds[2] == 0) {
		platform_throw();
	}
	CachedGlyph cached {
		.glyph = *face->glyph,
		.frames_unused = 0,
		.vtex_id = vtex_id,
		.bounds = RectInt(bounds[0], bounds[1], bounds[2], bounds[3])
	};
	return glyph_cache.insert(std::make_pair(id, cached)).first->second;
}

CachedGlyph& g_load_glyph(char32_t c, float height) {
	return g_load_glyph(c, (u32) std::ceil(height));
}

std::u32string g_wrap_text(const std::u32string& text, float w, float h) {
	check_init_freetype();
	g_virtual_to_real(&w, &h);
	set_ft_size(h);
	std::u32string out;
	std::u32string word;
	float x = 0;
	float wordw = 0;
	for(char32_t c: text) {
		CachedGlyph& cached = g_load_glyph(c, h);
		if(c == U' ' || c == U'\n') {
			if(x + wordw > w) {
				out += U'\n';
				x = wordw;
			} else {
				x += wordw;
			}
			out += word;
			word.clear();
			wordw = 0;
			if(c == U'\n' || x + cached.glyph.advance.x * (1.f / (1 << 6)) > w) {
				out += U'\n';
				x = 0;
			} else {
				out += c;
				x += cached.glyph.advance.x * (1.f / (1 << 6));
			}
		} else {
			wordw += cached.glyph.advance.x * (1.f / (1 << 6));
			word += c;
		}
	}
	out += word;
	return out;
}

TextInfo g_calc_text(const std::u32string& text, float w, float h) {
	check_init_freetype();
	std::u32string wrapped = w == -1? text: g_wrap_text(text, w, h);
	float x = 0, y = 0;
	float font_height = h;
	g_virtual_to_real(nullptr, &font_height);
	set_ft_size(font_height);
	TextInfo info;
	FT_UInt prev_glyph_idx = 0;
	bool use_kerning = FT_HAS_KERNING(face);
	float width_mult = (float) g_width() / g_real_width();
	float width_mult_over_64 = width_mult * (1.f / (1 << 6));
	float height_mult = (float) g_height() / g_real_height();
	float height_mult_over_64 = height_mult * (1.f / (1 << 6));
	float one_over_64 = 1.f / (1 << 6);
	float mx = x;
	float line_height = h * (5.f / 4.f) - 1;
	for(char32_t c: wrapped) {
		if(c == '\n') {
			x = 0;
			y += line_height;
		} else {
			CachedGlyph& cached = g_load_glyph(c, font_height);
			if(w != -1 && x + cached.glyph.advance.x * width_mult_over_64 > w) {
				x = 0;
				y += line_height;
			} else if(use_kerning && prev_glyph_idx) {
				FT_UInt glyph_idx = FT_Get_Char_Index(face, c);
				if(glyph_idx) {
					FT_Vector delta;
					FT_Get_Kerning(face, prev_glyph_idx, glyph_idx, FT_KERNING_DEFAULT, &delta);
					x += (float) delta.x * one_over_64;
				}
			}
			x += (float) cached.glyph.advance.x * width_mult_over_64;
			if(x > mx) {
				mx = x;
			}
			info.end_x = x;
			info.end_y = y;
		}
	} 
	info.w = std::ceil(mx);
	info.h = y + line_height;
	return info;
}

TextInfo g_calc_text(const std::string& text, float w, float h) {
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
	std::u32string input = converter.from_bytes(text);
	return g_calc_text(input, w, h);
}

// from glm::radians implementation
constexpr float full_turn = 360.f * 0.01745329251994329576923690768489f;

TextInfo g_draw_text(const std::u32string& text, float x, float y, float w, float virtual_font_height, HaloSettings halo, int corner, int origin) {
	check_init_freetype();
	std::u32string wrapped = w == -1? text: g_wrap_text(text, w, virtual_font_height);
	TextInfo initial_info = g_calc_text(wrapped, -1, virtual_font_height);
	if(w == -1) w = initial_info.w;
	float th = initial_info.h;
	g_calc(&x, &y, &w, &th, w, th, corner, origin);
	float font_height = virtual_font_height;
	g_virtual_to_real(nullptr, &font_height);
	set_ft_size(font_height);
	float x1 = x;
	float y1 = y;
	float max_x = x;
	TextInfo info;
	FT_UInt prev_glyph_idx = 0;
	bool use_kerning = FT_HAS_KERNING(face);
	float width_mult = (float) g_width() / g_real_width();
	float width_mult_over_64 = width_mult * (1.f / (1 << 6));
	float height_mult = (float) g_height() / g_real_height();
	float height_mult_over_64 = height_mult * (1.f / (1 << 6));
	float one_over_64 = 1.f / (1 << 6);
	float line_height = virtual_font_height * (5.f / 4.f) - 1;
	for(char32_t c: wrapped) {
		if(c == '\n') {
			x = x1;
			y += line_height;
		} else {
			CachedGlyph& cached = g_load_glyph(c, font_height);
			if(w != -1 && x - x1 + cached.glyph.advance.x * width_mult_over_64 > w) {
				x = x1;
				y += line_height;
			} else if(use_kerning && prev_glyph_idx) {
				FT_UInt glyph_idx = FT_Get_Char_Index(face, c);
				if(glyph_idx) {
					FT_Vector delta;
					FT_Get_Kerning(face, prev_glyph_idx, glyph_idx, FT_KERNING_DEFAULT, &delta);
					x += (float) delta.x * one_over_64;
				}
			}
			float virtual_glyph_h = (float) cached.glyph.bitmap.rows * height_mult;
			float glyph_x = x + (float) cached.glyph.bitmap_left * width_mult;
			float glyph_y = y + ((float) face->size->metrics.ascender * one_over_64 - cached.glyph.bitmap_top) * height_mult;
			g_draw_quad_crop_halo(glyph_atlas_tex, glyph_x, glyph_y, -1, virtual_glyph_h, cached.bounds.x, cached.bounds.y, cached.bounds.w, cached.bounds.h, halo, G_TOP_LEFT, G_TOP_LEFT);
			x += (float) cached.glyph.advance.x * width_mult_over_64;
			if(x > max_x) {
				max_x = x;
			}
			info.end_x = x;
			info.end_y = y;
		}
	} 
	info.w = std::ceil(max_x - x1);
	info.h = y + line_height - y1;
	return info;
}

TextInfo g_draw_text(const std::string& text, float x, float y, float w, float h, HaloSettings halo, int corner, int origin) {
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
	std::u32string input = converter.from_bytes(text);
	return g_draw_text(input, x, y, w, h, halo, corner, origin);
}

float g_text_line_height(float h) {
	return h * (5.f / 4.f) - 1;
}
