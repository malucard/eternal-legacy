#pragma once

#define ROOT_DIR "/usb0/eternal/"
#define PLATFORM_UPSCALE
extern bool upscaling_enabled;

struct TextureDataExtraFields {
	u32 oid;
	u16 w;
	u16 h;
	u16 format;
	u8 filter;
	bool upscale;
	u16 upw;
	u16 uph;
};
