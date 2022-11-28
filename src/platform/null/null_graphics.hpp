#pragma once

#define ROOT_DIR ""

struct TextureDataExtraFields {
	u32 oid;
	u16 w;
	u16 h;
	u16 format;
	bool upscale;
	u16 upw;
	u16 uph;
};
