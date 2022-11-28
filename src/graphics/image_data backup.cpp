#include <graphics.hpp>
#include <map>

#ifndef HARDWARE_TEX_FORMATS
#define HARDWARE_TEX_FORMATS (TEX_FORMAT_RGBA)
#endif

std::map<int, const char*> fmt_names {
	{TEX_FORMAT_RGBA, "TEX_FORMAT_RGBA"},
	{TEX_FORMAT_BGRA, "TEX_FORMAT_BGRA"},
	{TEX_FORMAT_RGBX, "TEX_FORMAT_RGBX"},
	{TEX_FORMAT_BGRX, "TEX_FORMAT_BGRX"},
	{TEX_FORMAT_RGB, "TEX_FORMAT_RGB"},
	{TEX_FORMAT_BGR, "TEX_FORMAT_BGR"},
	{TEX_FORMAT_4444, "TEX_FORMAT_4444"},
	{TEX_FORMAT_5551, "TEX_FORMAT_5551"},
	{TEX_FORMAT_565, "TEX_FORMAT_565"},
	{TEX_FORMAT_ALPHA, "TEX_FORMAT_ALPHA"},
	{TEX_FORMAT_GRAY, "TEX_FORMAT_GRAY"},
	{TEX_FORMAT_GRAY_ALPHA, "TEX_FORMAT_GRAY_ALPHA"}
};

int is_pot(int v) {
	return v == 1 || ((v & 1) == 0 && is_pot(v >> 1));
}

int to_pot(int v) {
	if(is_pot(v)) return v;
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}


int ImageData::fmt_len(u32 num, u16 fmt) {
	switch(fmt) {
		case TEX_FORMAT_RGBA: return num * 4;
		case TEX_FORMAT_BGRA: return num * 4;
		case TEX_FORMAT_RGBX: return num * 4;
		case TEX_FORMAT_BGRX: return num * 4;
		case TEX_FORMAT_RGB: return num * 3;
		case TEX_FORMAT_BGR: return num * 3;
		case TEX_FORMAT_4444: return num * 2;
		case TEX_FORMAT_5551: return num * 2;
		case TEX_FORMAT_565: return num * 2;
		case TEX_FORMAT_ALPHA: return num;
		case TEX_FORMAT_ALPHA4: return num / 2;
		case TEX_FORMAT_GRAY: return num;
		case TEX_FORMAT_GRAY_ALPHA: return num * 2;
		default: return 0;
	}
}

int ImageData::fmt_stride(u16 fmt) {
	switch(fmt) {
		case TEX_FORMAT_RGBA: return 4;
		case TEX_FORMAT_BGRA: return 4;
		case TEX_FORMAT_RGBX: return 4;
		case TEX_FORMAT_BGRX: return 4;
		case TEX_FORMAT_RGB: return 3;
		case TEX_FORMAT_BGR: return 3;
		case TEX_FORMAT_4444: return 2;
		case TEX_FORMAT_5551: return 2;
		case TEX_FORMAT_565: return 2;
		case TEX_FORMAT_ALPHA: return 1;
		case TEX_FORMAT_GRAY: return 1;
		case TEX_FORMAT_GRAY_ALPHA: return 2;
		default: return 0;
	}
}

inline u32 rgba_to_bgra(u32 rgba) {
#ifdef IS_BIG_ENDIAN
	return (rgba & 0x00FF00FF) | (rgba >> 16 & 0x0000FF00) | (rgba << 16 & 0xFF000000);
#else
	return (rgba & 0xFF00FF00) | (rgba >> 16 & 0x000000FF) | (rgba << 16 & 0x00FF0000);
#endif
}

inline u32 gray_spread(u32 gray) {
#ifdef IS_BIG_ENDIAN
	return gray << 8 | gray << 16 | gray << 24;
#else
	return gray | gray << 8 | gray << 16;
#endif
}

// returns a system endianness RGBA pixel, 0xAABBGGRR in little endian and 0xRRGGBBAA in big endian
inline u32 get_pix(void* pix, u16 fmt) {
	switch(fmt) {
		case TEX_FORMAT_RGBA: return *(u32*) pix;
		case TEX_FORMAT_BGRA: return rgba_to_bgra(*(u32*) pix);
		case TEX_FORMAT_RGBX:
		case TEX_FORMAT_RGB: return *(u32*) pix | le32(0xFF000000);
		case TEX_FORMAT_BGRX:
		case TEX_FORMAT_BGR: return rgba_to_bgra(*(u32*) pix) | le32(0xFF000000);
		case TEX_FORMAT_4444: {
			u16 x = *(u16*) pix;
			x = (x & 0x0000000F) | (x << 4 & 0x00000F00) | (x << 8 & 0x000F0000) | (x << 12 & 0x0F000000);
			return x | x << 4;
		}
		case TEX_FORMAT_565: { //0xF800 0x07C0 0x003F
			u16 x = *(u16*) pix;
			return (x << 4 & 0x000000F8) | (x << 8 & 0x0000F800) | (x << 12 & 0x00F80000) | 0xFF000000;
		}
#ifdef IS_BIG_ENDIAN
		case TEX_FORMAT_ALPHA: return (u32) *(u8*) pix | le32(0xFFFFFF00);
		case TEX_FORMAT_GRAY: {
			u32 gray = *(u8*) pix;
			return gray << 8 | gray << 16 | gray << 24 | le32(0xFF000000);
		}
		case TEX_FORMAT_GRAY_ALPHA: {
			u32 gray = *(u16*) pix;
			u32 alpha = gray >> 8;
			gray &= 0xFF;
			return gray << 8 | gray << 16 | gray << 24 | alpha;
		}
#else
		case TEX_FORMAT_ALPHA: return (u32) *(u8*) pix << 24 | le32(0x00FFFFFF);
		case TEX_FORMAT_GRAY: {
			u32 gray = *(u8*) pix;
			return gray | gray << 8 | gray << 16 | le32(0xFF000000);
		}
		case TEX_FORMAT_GRAY_ALPHA: {
			u32 gray = *(u16*) pix;
			u32 alpha = gray << 16 & 0xFF000000;
			gray &= 0xFF;
			return gray | gray << 8 | gray << 16 | alpha;
		}
#endif
	}
	return 0;
}

inline u32 get_pix_safe(void* pix, u16 fmt) {
	switch(fmt) {
#ifdef IS_BIG_ENDIAN
		case TEX_FORMAT_RGB: return (u32) *(u16*) ((u8*) pix + 1) | (u32) *(u8*) pix << 16 | le32(0xFF000000);
		case TEX_FORMAT_BGR: return rgba_to_bgra((u32) *(u16*) ((u8*) pix + 1) | (u32) *(u8*) pix << 16) | le32(0xFF000000);
#else
		case TEX_FORMAT_RGB: return (u32) *(u16*) pix | (u32) *((u8*) pix + 2) << 16 | le32(0xFF000000);
		case TEX_FORMAT_BGR: return rgba_to_bgra((u32) *(u16*) pix | (u32) *((u8*) pix + 2) << 16) | le32(0xFF000000);
#endif
	}
	return get_pix(pix, fmt);
}

// stores a system endianness RGBA pixel as the desired format
inline void set_pix(void* dst, u32 pix, u16 fmt) {
	switch(fmt) {
		case TEX_FORMAT_RGBA: *(u32*) dst = pix; break;
		case TEX_FORMAT_BGRA: *(u32*) dst = rgba_to_bgra(pix); break;
		case TEX_FORMAT_RGBX:
		case TEX_FORMAT_RGB: *(u32*) dst = pix; break;
		case TEX_FORMAT_BGRX:
		case TEX_FORMAT_BGR: *(u32*) dst = rgba_to_bgra(pix); break;
		case TEX_FORMAT_4444: {
			*(u16*) dst = (pix >> 4 & 0x000F) | (pix >> 8 & 0x00F0) | (pix >> 12 & 0x0F00) | (pix >> 16 & 0xF000);
			break;
		}
		case TEX_FORMAT_5551: {
			*(u16*) dst = (pix >> 3 & 0x001F) | (pix >> 6 & 0x03E0) | (pix >> 9 & 0x7C00) | (pix >> 16 & 0x8000);
			break;
		}
		case TEX_FORMAT_565: {
			*(u16*) dst = (pix >> 3 & 0x001F) | (pix >> 5 & 0x07E0) | (pix >> 8 & 0xF800);
			break;
		}
#ifdef IS_BIG_ENDIAN
		case TEX_FORMAT_ALPHA: *(u8*) dst = pix; break;
		case TEX_FORMAT_GRAY: *(u8*) dst = pix; break;
		case TEX_FORMAT_GRAY_ALPHA: *(u16*) dst = pix; break;
#else
		case TEX_FORMAT_ALPHA: *(u8*) dst = pix >> 24; break;
		case TEX_FORMAT_GRAY: *(u8*) dst = pix >> 8; break;
		case TEX_FORMAT_GRAY_ALPHA: *(u16*) dst = pix >> 16; break;
#endif
	}
}

inline void set_pix_safe(void* dst, u32 pix, u16 fmt) {
	switch(fmt) {
#ifdef IS_BIG_ENDIAN
		case TEX_FORMAT_BGR:
			pix = rgba_to_bgra(pix);
		case TEX_FORMAT_RGB:
			*(u16*) ((u8*) dst + 1) = pix;
			*(u8*) = pix >> 16;
			break;
#else
		case TEX_FORMAT_BGR:
			pix = rgba_to_bgra(pix);
		case TEX_FORMAT_RGB:
			*(u16*) dst = pix;
			*((u8*) dst + 2) = pix >> 16;
			break;
#endif
		default: set_pix(dst, pix, fmt);
	}
}

void ImageData::resample(u16 dst_fmt, u16 dst_w, u16 dst_h, u16 dst_buf_w, u16 dst_buf_h) {
	if(!buf) {
		pix_w = dst_w;
		pix_h = dst_h;
		buf_w = dst_buf_w;
		buf_h = dst_buf_h;
		fmt = dst_fmt;
		return;
	}
	//log_info("resampled %s %ux%u -> %s %ux%u", fmt_names[fmt], fmt_names[dst_fmt], pix_w, pix_h, dst_w, dst_h);
	int src_stride = fmt_stride(fmt);
	int dst_stride = fmt_stride(dst_fmt);
	u32 src_size = buf_w * buf_h * src_stride;
	u32 dst_size = dst_buf_w * dst_buf_h * dst_stride;
	u8* newbuf = buf;
	if(dst_stride > src_stride || dst_buf_w > buf_w || dst_size > src_size || ownership == OWNERSHIP_KEEP) {
		newbuf = (u8*) malloc(dst_size);
	}
	if(dst_w == pix_w && dst_h == pix_h) {
		int dst_yoff = 0, src_yoff = 0;
		for(int dst_y = 0; dst_y < dst_h - 1; dst_y++) {
			for(int dst_xoff = 0, src_xoff = 0; dst_xoff < dst_w * dst_stride; dst_xoff += dst_stride, src_xoff += src_stride) {
				set_pix(newbuf + dst_yoff + dst_xoff, get_pix(buf + src_yoff + src_xoff, fmt), dst_fmt);
			}
			dst_yoff += dst_buf_w * dst_stride;
			src_yoff += buf_w * src_stride;
		}
		for(int dst_xoff = 0, src_xoff = 0; dst_xoff < dst_w * dst_stride; dst_xoff += dst_stride, src_xoff += src_stride) {
			set_pix_safe(newbuf + dst_yoff + dst_xoff, get_pix_safe(buf + src_yoff + src_xoff, fmt), dst_fmt);
		}
	} else if(dst_w == pix_w) {
		int dst_yoff = 0, src_yoff = 0, dst_y = 0;
		while(dst_y < dst_h - 1) {
			for(int dst_xoff = 0, src_xoff = 0; dst_xoff < dst_w * dst_stride; dst_xoff += dst_stride, src_xoff += src_stride) {
				set_pix(newbuf + dst_yoff + dst_xoff, get_pix(buf + src_yoff + src_xoff, fmt), dst_fmt);
			}
			dst_yoff += dst_buf_w * dst_stride;
			dst_y++;
			src_yoff = dst_y * pix_h / dst_h * (buf_w * src_stride);
		}
		for(int dst_xoff = 0, src_xoff = 0; dst_xoff < dst_w * dst_stride; dst_xoff += dst_stride, src_xoff += src_stride) {
			set_pix_safe(newbuf + dst_yoff + dst_xoff, get_pix_safe(buf + src_yoff + src_xoff, fmt), dst_fmt);
		}
	} else {
		int dst_yoff = 0, src_yoff = 0, dst_y = 0;
		while(dst_y < dst_h - 1) {
			for(int dst_xoff = 0, dst_x = 0; dst_x < dst_w; dst_xoff += dst_stride, dst_x++) {
				set_pix(newbuf + dst_yoff + dst_xoff, get_pix(buf + src_yoff + dst_x * pix_w / dst_w * src_stride, fmt), dst_fmt);
			}
			dst_yoff += dst_buf_w * dst_stride;
			dst_y++;
			src_yoff = dst_y * pix_h / dst_h * (buf_w * src_stride);
		}
		for(int dst_xoff = 0, dst_x = 0; dst_x < dst_w; dst_xoff += dst_stride, dst_x++) {
			set_pix_safe(newbuf + dst_yoff + dst_xoff, get_pix_safe(buf + src_yoff + dst_x * pix_w / dst_w * src_stride, fmt), dst_fmt);
		}
	}
	if(buf != newbuf) {
		if(ownership == OWNERSHIP_OWNED) free(buf);
		buf = newbuf;
		ownership = OWNERSHIP_OWNED;
	} else if(dst_size < src_size) {
		if(ownership == OWNERSHIP_OWNED) {
			buf = (u8*) realloc(buf, dst_size);
		} else {
			u8* nbuf = (u8*) malloc(dst_size);
			memcpy(nbuf, buf, dst_buf_w * dst_h * dst_stride);
			buf = nbuf;
			ownership = OWNERSHIP_OWNED;
		}
	}
	pix_w = dst_w;
	pix_h = dst_h;
	buf_w = dst_buf_w;
	buf_h = dst_buf_h;
	fmt = dst_fmt;
}

void ImageData::optimize_size(u16 format, u16& buf_w, u16& buf_h) {
	if(!(format & HARDWARE_TEX_FORMATS)) return; // if it's going to be resampled anyway then just keep the buffer small
#ifdef TEXTURE_POWER_OF_TWO // increase buffer to reduce chance of needing to copy or resample the image
#ifdef TEXTURE_MAX_WIDTH // texture over max size will be resampled
	if(buf_w < TEXTURE_MAX_WIDTH) buf_w = to_pot(buf_w);
	if(buf_h < TEXTURE_MAX_HEIGHT) buf_h = to_pot(buf_h);
#else
	buf_w = to_pot(buf_w);
	buf_h = to_pot(buf_h);
#endif
#endif
}

void ImageData::optimize() {
	u16 dst_w = pix_w;
	u16 dst_h = pix_h;
	u16 dst_buf_w = buf_w;
	u16 dst_buf_h = buf_h;
#ifdef TEXTURE_MAX_WIDTH
	if(dst_w > TEXTURE_MAX_WIDTH) dst_w = TEXTURE_MAX_WIDTH;
	if(dst_buf_w > TEXTURE_MAX_WIDTH) dst_buf_w = TEXTURE_MAX_WIDTH;
#endif
#ifdef TEXTURE_MAX_HEIGHT
	if(dst_h > TEXTURE_MAX_HEIGHT) dst_h = TEXTURE_MAX_HEIGHT;
	if(dst_buf_h > TEXTURE_MAX_HEIGHT) dst_buf_h = TEXTURE_MAX_HEIGHT;
#endif
#ifdef TEXTURE_POWER_OF_TWO
	dst_buf_w = to_pot(dst_buf_w);
	dst_buf_h = to_pot(dst_buf_h);
#endif
	if(fmt & HARDWARE_TEX_FORMATS) {
		if(dst_w != pix_w || dst_h != pix_h || dst_buf_w != buf_w || dst_buf_h != buf_h) {
			resample(fmt, dst_w, dst_h, dst_buf_w, dst_buf_h);
		} else if(dst_buf_h != buf_h) {
			int stride = fmt_stride(fmt);
			if(ownership == OWNERSHIP_OWNED) {
				buf = (u8*) realloc(buf, dst_buf_w * dst_buf_h * stride);
			} else {
				u8* nbuf = (u8*) malloc(dst_buf_w * dst_buf_h * stride);
				memcpy(nbuf, buf, dst_buf_w * pix_h * stride);
				buf = nbuf;
				ownership = OWNERSHIP_OWNED;
			}
		}
		return;
	}
	// pick a supported format to convert to
	// conditions are lists from most to least preferable (using a #define to make this more concise)
	// they are all constant expressions and optimized away
#define ENTRY(dst_fmt) \
	if(TEX_FORMAT_ ## dst_fmt & HARDWARE_TEX_FORMATS) { \
		resample(TEX_FORMAT_ ## dst_fmt, dst_w, dst_h, dst_buf_w, dst_buf_h); \
	} else
#define ENTRY_FAVOR(favor, dst_fmt) \
	if((TEX_FORMAT_ ## dst_fmt & HARDWARE_TEX_FORMATS) && (flags & TEX_FAVOR_ ## favor)) { \
		resample(TEX_FORMAT_ ## dst_fmt, dst_w, dst_h, dst_buf_w, dst_buf_h); \
	} else
	switch(fmt) {
		case TEX_FORMAT_RGBA: ENTRY(BGRA) ENTRY_FAVOR(COLOR, 5551) ENTRY(4444) ENTRY(5551) {} break;
		case TEX_FORMAT_BGRA: ENTRY(RGBA) ENTRY_FAVOR(COLOR, 5551) ENTRY(4444) ENTRY(5551) {} break;
		case TEX_FORMAT_RGBX: ENTRY(RGB) ENTRY(BGR) ENTRY(BGRX) ENTRY(RGBA) ENTRY(BGRA) ENTRY(565) ENTRY_FAVOR(COLOR, 5551) ENTRY(4444) {} break;
		case TEX_FORMAT_BGRX: ENTRY(BGR) ENTRY(RGB) ENTRY(RGBX) ENTRY(BGRA) ENTRY(RGBA) ENTRY(565) ENTRY_FAVOR(COLOR, 5551) ENTRY(4444) {} break;
		case TEX_FORMAT_RGB: ENTRY(BGR) ENTRY(RGBX) ENTRY(BGRX) ENTRY(RGBA) ENTRY(BGRA) ENTRY(565) ENTRY_FAVOR(COLOR, 5551) ENTRY(4444) {} break;
		case TEX_FORMAT_BGR: ENTRY(RGB) ENTRY(BGRX) ENTRY(RGBX) ENTRY(BGRA) ENTRY(RGBA) ENTRY(565) ENTRY_FAVOR(COLOR, 5551) ENTRY(4444) {} break;
		case TEX_FORMAT_ALPHA: ENTRY(GRAY_ALPHA) ENTRY(RGBA) ENTRY(BGRA) ENTRY(4444) {} break;
		case TEX_FORMAT_GRAY: ENTRY(GRAY_ALPHA) ENTRY(RGBA) ENTRY(BGRA) ENTRY(565) {} break;
		case TEX_FORMAT_GRAY_ALPHA: ENTRY(RGBA) ENTRY(BGRA) ENTRY(4444) ENTRY(5551) {} break;
	}
}

u32 ImageData::buf_size() {
	return fmt_stride(fmt) * buf_w * buf_h;
}

void ImageData::destroy() {
	if(ownership == OWNERSHIP_OWNED && buf) free(buf);
	buf = nullptr;
}

TextureRef g_upload_texture2(void* data, int virtual_w, int virtual_h, int pix_w, int pix_h, int buf_w, int buf_h, int format, int flags, u8 ownership) {
	ImageData imd {.buf = (u8*) data, .virtual_w = (u16) virtual_w, .virtual_h = (u16) virtual_h,
		.pix_w = (u16) pix_w, .pix_h = (u16) pix_h, .buf_w = (u16) buf_w, .buf_h = (u16) buf_h,
		.flags = flags, .fmt = (u16) format, .ownership = ownership};
	imd.optimize();
	TextureRef tex = _g_upload_texture(imd);
	imd.destroy();
	return std::move(tex);
}

TextureRef g_upload_texture(void* data, int w, int h, int format, int flags, u8 ownership) {
	ImageData imd {.buf = (u8*) data,
		.virtual_w = (u16) w, .virtual_h = (u16) h, .pix_w = (u16) w, .pix_h = (u16) h, .buf_w = (u16) w, .buf_h = (u16) h,
		.flags = flags, .fmt = (u16) format, .ownership = ownership};
	imd.optimize();
	TextureRef tex = _g_upload_texture(imd);
	imd.destroy();
	return std::move(tex);
}
