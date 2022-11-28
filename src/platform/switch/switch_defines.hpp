#pragma once
#include <optional>

#define ROOT_DIR "sdmc:/eternal/"
#define PLATFORM_UPSCALE
extern bool upscaling_enabled;
#define HARDWARE_TEX_FORMATS (TEX_FORMAT_RGBA | TEX_FORMAT_BGRA | TEX_FORMAT_RGB | TEX_FORMAT_BGR | TEX_FORMAT_RGBX | TEX_FORMAT_BGRX | TEX_FORMAT_ALPHA | TEX_FORMAT_GRAY | TEX_FORMAT_GRAY_ALPHA)

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
