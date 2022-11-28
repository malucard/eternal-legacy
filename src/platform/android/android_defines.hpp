#pragma once
#include <optional>

extern char* ROOT_DIR;
#define ROOT_DIR_NOT_CONST
#define PLATFORM_UPSCALE
extern bool upscaling_enabled;
#define FONT_ATLAS_SIZE 8192
#define HARDWARE_TEX_FORMATS (TEX_FORMAT_RGBA | TEX_FORMAT_BGRA | TEX_FORMAT_RGB | TEX_FORMAT_BGR | TEX_FORMAT_RGBX | TEX_FORMAT_BGRX | TEX_FORMAT_ALPHA | TEX_FORMAT_GRAY | TEX_FORMAT_GRAY_ALPHA | TEX_FORMAT_4444 | TEX_FORMAT_5551 | TEX_FORMAT_565)
extern unsigned texture_max_width, texture_max_height;
#define TEXTURE_MAX_WIDTH texture_max_width
#define TEXTURE_MAX_HEIGHT texture_max_height
#define FONT_ATLAS_SIZE (texture_max_height > LARGE_FONT_ATLAS_SIZE? LARGE_FONT_ATLAS_SIZE: texture_max_height)

struct TextureDataExtraFields {
	u32 oid;
	u16 format;
	u8 filter;
	bool upscale;
	u16 upw;
	u16 uph;
	u16 pix_w;
	u16 pix_h;
};
