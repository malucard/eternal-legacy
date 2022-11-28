#include "graphics.hpp"
#include <cstring>
#include <cstdlib>
#include <string>
#include <iostream>

namespace n7 {

enum {
	INDEXED,
	BGR,
	BGRA
};

#define U8(x) (*(u8*) (x))
#define U16LE(x) le16(*(u16*) (x))
#define U32LE(x) le32(*(u32*) (x))
#define U32(x) (*(u32*) (x))

void unpack_lnd(u8* buf, u8* out, u32 size, u32 unpacked_size) {
	u32 src = 0;
	u32 dst = 0;
	while(dst < unpacked_size && src < size) {
		u32 ctl = U8(buf + src);
		src++;
		//if(ctl == 0xFF) break;
		if(ctl & 0x80) {
			if(ctl & 0x40) {
				u32 count = (ctl & 0x1F) + 2;
				if(ctl & 0x20) {
					count += (u32) U8(buf + src) << 5;
					src++;
				}
				if(unpacked_size - dst < count) count = unpacked_size - dst;
				u8 v = U8(buf + src);
				src++;
				//for(int i = 0; i < count; i++) out[dst++] = v;
				memset(out + dst, v, count);
				dst += count;
			} else {
				int count = ((ctl >> 2) & 0xF) + 2;
				int offset = ((ctl & 3) << 8) + U8(buf + src) + 1;
				src++;
				if(unpacked_size - dst < count) count = unpacked_size - dst;
				memcpy(out + dst, out + dst - offset, count);
				dst += count;
			}
		} else if(ctl & 0x40) {
			int length = (ctl & 0x3F) + 2 < unpacked_size - dst? (ctl & 0x3F) + 2: unpacked_size - dst;
			int count = U8(buf + src);
			src++;
			memcpy(out + dst, buf + src, length);
			src += length;
			dst += length;
			count = count * length < unpacked_size - dst? count * length: unpacked_size - dst;
			u32 s = dst - length;
			while(count) {
				int preceding = dst - s < count? dst - s: count;
				memcpy(out + dst, out + s, preceding);
				dst += preceding;
				count -= preceding;
			}
		} else {
			u32 count = (ctl & 0x1F) + 1;
			if(ctl & 0x20) {
				count += (u32) U8(buf + src) << 5;
				src++;
			}
			if(unpacked_size - dst < count) count = unpacked_size - dst;
			memcpy(out + dst, buf + src, count);
			src += count;
			dst += count;
		}
	}
}

static u8* prepare_cps(u8* buf, u32 size, u32* unpacked_size_o) {
	u32 key_off = U32LE(buf + size - 4) - 0x7534682;
	u32 key = U32LE(buf + key_off) + key_off + 0x3786425;
	u32 packed_size = U32LE(buf + 4);
	u16 compression = U16LE(buf + 10);
	u32 unpacked_size = U32LE(buf + 12);
	*unpacked_size_o = unpacked_size;
	for(u32 pos = 0x10; pos < size; pos += 4) {
		if(pos == size - 4) {
			U32(buf + pos) = 0;
			break;
		}
		u32 data = U32LE(buf + pos);
		if(pos != key_off && key_off != 0) {
			data -= key + size;
		}
		U32(buf + pos) = le32(data);
		key = 1103515245 * key + 39686;
	}
	u8* out = new u8[unpacked_size];
	if(compression & 1) {
		unpack_lnd(buf + 0x14, out, size - 0x14, unpacked_size);
	} else if(compression & 2) {
		log_err("lnd16 compression unsupported");
		platform_throw();
	} else {
		memcpy(out, buf + 4, unpacked_size);
	}
	delete[] buf;
	return out;
}

u8* load_cps(u8* data, int size, int* width, int* height, bool no_alpha) {
	u32 unpacked_size;
	data = prepare_cps(data, size, &unpacked_size);
	u32 sig = U32(data);
	u16 version = U16LE(data + 4);
	if(version != 101 && version != 102) {
		log_err("unsupported prt version");
		platform_throw();
	}
	u16 w = U16LE(data + 0xC);
	u16 h = U16LE(data + 0xE);
	u16 bpp = U16LE(data + 6);
	version = version;
	u16 palette_off = U16LE(data + 8);
	u16 data_off = U16LE(data + 0xA);
	bool alpha = U32LE(data + 0x10);
	u32 offx = 0, offy = 0;
	if(version == 102) {
		offx = U32LE(data + 0x14);
		offy = U32LE(data + 0x18);
	}
#ifdef LOW_RES
	u32 fw = w / 2, fh = h / 2;
#else
	u32 fw = w, fh = h;
#endif
	u32* out = new u32[fw * fh];
	u32 stride = (w * (bpp / 8) + 3) & ~3;
	int fmt;
	*width = fw;
	*height = fh;
	if(bpp == 8) {
		u32 j = 0;
#ifdef LOW_RES
		for(u32 y = 0; y < h; y += 2) {
			for(u32 x = 0; x < w; x += 2, j++) {
#else
		for(u32 y = 0; y < h; y++) {
			for(u32 x = 0; x < w; x++, j++) {
#endif
				out[j] = ((u32*) (data + palette_off))[U8(data + data_off + (h - y - 1) * w + x)] | 0xFF000000;
				out[j] = (out[j] & 0xFF000000) >> 0 | (out[j] & 0xFF0000) >> 16 | (out[j] & 0xFF00) << 0 | (out[j] & 0xFF) << 16;
			}
		}
	} else if(bpp == 24) {
		u8* alpha = data + data_off + stride * h;
		u32 j = 0;
		for(u32 yi = data_off + stride * h - stride; yi >= data_off && j < fw * fh; yi -= stride) {
			for(u32 i = 0; i < w * 3; i += 3, j++) {
				//u32 c = U32LE(data + yi + i) & 0xFFFFFF;
#ifdef LOW_RES
				u8 a = ((u32) U8(alpha) + U8(alpha + 1) + U8(alpha + w) + U8(alpha + w + 1)) / 4;
				u8 r = ((u32) U8(data + yi + i) + U8(data + yi + i + 3) + U8(data + yi - stride + i) + U8(data + yi - stride + i + 3)) / 4;
				u8 g = ((u32) U8(data + yi + i + 1) + U8(data + yi + i + 4) + U8(data + yi - stride + i + 1) + U8(data + yi - stride + i + 4)) / 4;
				u8 b = ((u32) U8(data + yi + i + 2) + U8(data + yi + i + 5) + U8(data + yi - stride + i + 2) + U8(data + yi - stride + i + 5)) / 4;
//#ifdef PLATFORM_PSP
				out[j] = (u32) U8(alpha++) << 24 | (u32) b | (u32) g << 8 | (u32) r << 16;
//#else
//				out[j] = (u32) U8(alpha++) << 24 | (u32) b << 16 | (u32) g << 8 | (u32) r;
//#endif
				alpha++;
				i += 3;
			}
			yi -= stride;
			alpha += w;
#else
				u8 r = U8(data + yi + i + 2);
				u8 g = U8(data + yi + i + 1);
				u8 b = U8(data + yi + i);
				out[j] = (u32) (no_alpha? 0xFF000000: U8(alpha++) << 24) | (u32) b << 16 | (u32) g << 8 | (u32) r;
			}
#endif
		}
	} else if(bpp == 32) {
		log_err("BGRA unsupported");
		platform_throw();
	}
	delete[] data;
	return (u8*) out;
}

}
