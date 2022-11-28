#include <graphics.hpp>
#include <libdivide.h>
#include <map>

#ifndef HARDWARE_TEX_FORMATS
#define HARDWARE_TEX_FORMATS (TEX_FORMAT_RGBA)
#endif

std::map<int, const char*> fmt_names {
	{TEX_FORMAT_RGBA, "RGBA"},
	{TEX_FORMAT_BGRA, "BGRA"},
	{TEX_FORMAT_RGBX, "RGBX"},
	{TEX_FORMAT_BGRX, "BGRX"},
	{TEX_FORMAT_RGB, "RGB"},
	{TEX_FORMAT_BGR, "BGR"},
	{TEX_FORMAT_4444, "4444"},
	{TEX_FORMAT_5551, "5551"},
	{TEX_FORMAT_565, "565"},
	{TEX_FORMAT_ALPHA, "ALPHA"},
	{TEX_FORMAT_ALPHA4, "ALPHA4"},
	{TEX_FORMAT_GRAY, "GRAY"},
	{TEX_FORMAT_GRAY_ALPHA, "GRAY_ALPHA"}
};

int is_pot(int v) {
	return v <= 1 || ((v & 1) == 0 && is_pot(v >> 1));
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
		case TEX_FORMAT_ALPHA4: return (num + 1) / 2;
		case TEX_FORMAT_GRAY: return num;
		case TEX_FORMAT_GRAY_ALPHA: return num * 2;
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
		case TEX_FORMAT_ALPHA4: return (u32) (*(u8*) pix << 24 & 0x0F000000) | *(u8*) pix << 28 | le32(0x00FFFFFF);
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

inline u32 get_pix_2(u8* pix, u16 fmt) {
	switch(fmt) {
		case TEX_FORMAT_RGBA:
		case TEX_FORMAT_BGRA:
		case TEX_FORMAT_RGBX:
		case TEX_FORMAT_BGRX: return get_pix(pix + 4, fmt);
		case TEX_FORMAT_RGB:
		case TEX_FORMAT_BGR: return get_pix(pix + 3, fmt);
		case TEX_FORMAT_4444: 
		case TEX_FORMAT_565:
		case TEX_FORMAT_5551:
		case TEX_FORMAT_GRAY_ALPHA: return get_pix(pix + 2, fmt);
		case TEX_FORMAT_ALPHA:
		case TEX_FORMAT_GRAY: return get_pix(pix + 1, fmt);
		case TEX_FORMAT_ALPHA4: return (u32) (*(u8*) pix << 20 & 0x0F000000) | (*(u8*) pix << 24 & 0xF0000000) | le32(0x00FFFFFF);
	}
	return 0;
}

inline u32 get_pix_safe_2(u8* pix, u16 fmt) {
	if(fmt == TEX_FORMAT_RGB | fmt == TEX_FORMAT_BGR) {
		return get_pix_safe(pix + 3, fmt);
	}
	return get_pix_2(pix, fmt);
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
		case TEX_FORMAT_ALPHA4: *(u8*) dst = pix >> 24; break;
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

inline void set_pix_double(u8* dst, u32 pix, u32 pix2, u16 fmt) {
	switch(fmt) {
		case TEX_FORMAT_RGBA: case TEX_FORMAT_BGRA:
		case TEX_FORMAT_RGBX: case TEX_FORMAT_BGRX:
			set_pix(dst, pix, fmt);
			set_pix(dst + 4, pix2, fmt);
			break;
		case TEX_FORMAT_RGB: case TEX_FORMAT_BGR:
			set_pix(dst, pix, fmt);
			set_pix(dst + 3, pix2, fmt);
			break;
		case TEX_FORMAT_4444: case TEX_FORMAT_565: case TEX_FORMAT_5551: case TEX_FORMAT_GRAY_ALPHA:
			set_pix(dst, pix, fmt);
			set_pix(dst + 2, pix2, fmt);
			break;
		case TEX_FORMAT_ALPHA: case TEX_FORMAT_GRAY:
			set_pix(dst, pix, fmt);
			set_pix(dst + 1, pix2, fmt);
			break;
		case TEX_FORMAT_ALPHA4:
			*(dst) = pix >> 28 | (pix2 >> 24 & 0xF0);
			break;
	}
}

inline void set_pix_double_safe(u8* dst, u32 pix, u32 pix2, u16 fmt) {
	if(fmt == TEX_FORMAT_RGB || fmt == TEX_FORMAT_BGR) {
		set_pix(dst, pix, fmt);
		set_pix_safe(dst + 3, pix2, fmt);
	} else {
		set_pix_double(dst, pix, pix2, fmt);
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
	//if(!fmt || !dst_fmt) platform_throw();
	//log_info("resampled %s %ux%u %ux%u -> %s %ux%u %ux%u", fmt_names[fmt], pix_w, pix_h, buf_w, buf_h, fmt_names[dst_fmt], dst_w, dst_h, dst_buf_w, dst_buf_h);
	int src_double_stride = fmt_len(2, fmt);
	int dst_double_stride = fmt_len(2, dst_fmt);
	int src_row_size = fmt_len(buf_w, fmt);
	int dst_row_size = fmt_len(dst_buf_w, dst_fmt);
	u32 src_size = src_row_size * buf_h;
	u32 dst_size = dst_row_size * dst_buf_h;
	u8* newbuf = buf;
	if(dst_double_stride > src_double_stride || dst_buf_w > buf_w || dst_size > src_size || ownership == OWNERSHIP_KEEP) {
		newbuf = (u8*) malloc(dst_size);
	}
	bool odd_width = dst_w & 1;
	if(dst_w == pix_w && dst_h == pix_h) {
		int dst_yoff = 0, src_yoff = 0;
		for(int dst_y = 0; dst_y < dst_h - 1; dst_y++) {
			int dst_xoff = 0, src_xoff = 0;
			for(; dst_xoff < dst_w * dst_double_stride / 2; dst_xoff += dst_double_stride, src_xoff += src_double_stride) {
				u8* dst_pix = newbuf + dst_yoff + dst_xoff;
				u8* src_pix = buf + src_yoff + src_xoff;
				set_pix_double(dst_pix, get_pix(src_pix, fmt), get_pix_2(src_pix, fmt), dst_fmt);
			}
			if(odd_width) {
				u8* dst_pix = newbuf + dst_yoff + dst_xoff;
				u8* src_pix = buf + src_yoff + src_xoff;
				set_pix(dst_pix, get_pix(src_pix, fmt), dst_fmt);
			}
			dst_yoff += dst_row_size;
			src_yoff += src_row_size;
		}
		int dst_xoff = 0, src_xoff = 0;
		for(; dst_xoff < dst_w * dst_double_stride / 2; dst_xoff += dst_double_stride, src_xoff += src_double_stride) {
			u8* dst_pix = newbuf + dst_yoff + dst_xoff;
			u8* src_pix = buf + src_yoff + src_xoff;
			set_pix_double_safe(dst_pix, get_pix_safe(src_pix, fmt), get_pix_safe_2(src_pix, fmt), dst_fmt);
		}
		if(odd_width) {
			u8* dst_pix = newbuf + dst_yoff + dst_xoff;
			u8* src_pix = buf + src_yoff + src_xoff;
			set_pix_safe(dst_pix, get_pix_safe(src_pix, fmt), dst_fmt);
		}
	} else if(dst_w == pix_w) {
		libdivide::divider<u32> div_dst_h(dst_h);
		int dst_yoff = 0, src_yoff = 0, dst_y = 0;
		while(dst_y < dst_h - 1) {
			int dst_xoff = 0, src_xoff = 0;
			for(; dst_xoff < dst_w * dst_double_stride / 2; dst_xoff += dst_double_stride, src_xoff += src_double_stride) {
				u8* dst_pix = newbuf + dst_yoff + dst_xoff;
				u8* src_pix = buf + src_yoff + src_xoff;
				set_pix_double(dst_pix, get_pix(src_pix, fmt), get_pix_2(src_pix, fmt), dst_fmt);
			}
			if(odd_width) {
				u8* dst_pix = newbuf + dst_yoff + dst_xoff;
				u8* src_pix = buf + src_yoff + src_xoff;
				set_pix(dst_pix, get_pix(src_pix, fmt), dst_fmt);
			}
			dst_yoff += dst_row_size;
			dst_y++;
			src_yoff = (u32) (dst_y * pix_h) / div_dst_h * src_row_size;
		}
		int dst_xoff = 0, src_xoff = 0;
		for(; dst_xoff < dst_w * dst_double_stride / 2; dst_xoff += dst_double_stride, src_xoff += src_double_stride) {
			u8* dst_pix = newbuf + dst_yoff + dst_xoff;
			u8* src_pix = buf + src_yoff + src_xoff;
			set_pix_double_safe(dst_pix, get_pix_safe(src_pix, fmt), get_pix_safe_2(src_pix, fmt), dst_fmt);
		}
		if(odd_width) {
			u8* dst_pix = newbuf + dst_yoff + dst_xoff;
			u8* src_pix = buf + src_yoff + src_xoff;
			set_pix_safe(dst_pix, get_pix_safe(src_pix, fmt), dst_fmt);
		}
	} else {
		libdivide::divider<u32> div_dst_w(dst_w);
		libdivide::divider<u32> div_dst_h(dst_h);
		int dst_yoff = 0, src_yoff = 0, dst_y = 0;
		while(dst_y < dst_h - 1) {
			int dst_xoff = 0, dst_x = 0;
			for(; dst_x < dst_w; dst_xoff += dst_double_stride, dst_x += 2) {
				u8* dst_pix = newbuf + dst_yoff + dst_xoff;
				u8* src_pix = buf + src_yoff + (u32) (dst_x * pix_w) / div_dst_w * src_double_stride / 2;
				set_pix_double(dst_pix, get_pix(src_pix, fmt), get_pix_2(src_pix, fmt), dst_fmt);
			}
			if(odd_width) {
				u8* dst_pix = newbuf + dst_yoff + dst_xoff;
				u8* src_pix = buf + src_yoff + (u32) (dst_x * pix_w) / div_dst_w * src_double_stride / 2;
				set_pix(dst_pix, get_pix(src_pix, fmt), dst_fmt);
			}
			dst_yoff += dst_row_size;
			dst_y++;
			src_yoff = (u32) (dst_y * pix_h) / div_dst_h * src_row_size;
		}
		int dst_xoff = 0, dst_x = 0;
		for(; dst_x < dst_w; dst_xoff += dst_double_stride, dst_x += 2) {
			u8* dst_pix = newbuf + dst_yoff + dst_xoff;
			u8* src_pix = buf + src_yoff + (u32) (dst_x * pix_w) / div_dst_w * src_double_stride / 2;
			set_pix_double_safe(dst_pix, get_pix_safe(src_pix, fmt), get_pix_safe_2(src_pix, fmt), dst_fmt);
		}
		if(odd_width) {
			u8* dst_pix = newbuf + dst_yoff + dst_xoff;
			u8* src_pix = buf + src_yoff + (u32) (dst_x * pix_w) / div_dst_w * src_double_stride / 2;
			set_pix_safe(dst_pix, get_pix_safe(src_pix, fmt), dst_fmt);
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
			memcpy(nbuf, buf, fmt_len(dst_buf_w, dst_fmt) * dst_h);
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
			int size = fmt_len(dst_buf_w * dst_buf_h, fmt);
			if(ownership == OWNERSHIP_OWNED) {
				buf = (u8*) realloc(buf, size);
			} else {
				u8* nbuf = (u8*) malloc(size);
				memcpy(nbuf, buf, fmt_len(dst_buf_w, fmt) * pix_h);
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
		case TEX_FORMAT_RGBX: ENTRY(RGB) ENTRY(BGR) ENTRY(BGRX) ENTRY(RGBA) ENTRY(BGRA) ENTRY(565) ENTRY_FAVOR(COLOR, 5551) ENTRY(4444) ENTRY(5551) {} break;
		case TEX_FORMAT_BGRX: ENTRY(BGR) ENTRY(RGB) ENTRY(RGBX) ENTRY(BGRA) ENTRY(RGBA) ENTRY(565) ENTRY_FAVOR(COLOR, 5551) ENTRY(4444) ENTRY(5551) {} break;
		case TEX_FORMAT_RGB: ENTRY(BGR) ENTRY(RGBX) ENTRY(BGRX) ENTRY(RGBA) ENTRY(BGRA) ENTRY(565) ENTRY_FAVOR(COLOR, 5551) ENTRY(4444) ENTRY(5551) {} break;
		case TEX_FORMAT_BGR: ENTRY(RGB) ENTRY(BGRX) ENTRY(RGBX) ENTRY(BGRA) ENTRY(RGBA) ENTRY(565) ENTRY_FAVOR(COLOR, 5551) ENTRY(4444) ENTRY(5551) {} break;
		case TEX_FORMAT_ALPHA: ENTRY(GRAY_ALPHA) ENTRY(RGBA) ENTRY(BGRA) ENTRY(ALPHA4) ENTRY(4444) ENTRY(5551) {} break;
		case TEX_FORMAT_GRAY: ENTRY(GRAY_ALPHA) ENTRY(RGBA) ENTRY(BGRA) ENTRY(565) ENTRY(5551) ENTRY(4444) {} break;
		case TEX_FORMAT_GRAY_ALPHA: ENTRY(RGBA) ENTRY(BGRA) ENTRY(4444) ENTRY(5551) {} break;
		case TEX_FORMAT_ALPHA4: ENTRY(ALPHA) ENTRY(GRAY_ALPHA) ENTRY(RGBA) ENTRY(BGRA) ENTRY(ALPHA4) ENTRY(4444) ENTRY(5551) {} break;
	}
}

u32 ImageData::buf_size() {
	return fmt_len(buf_w * buf_h, fmt);
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
