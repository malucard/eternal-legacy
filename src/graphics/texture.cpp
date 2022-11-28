#include <graphics.hpp>
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb/stb_image.h>
#include <png.h>
#ifdef PLATFORM_PSP
#define NO_WUFFS
#endif
#ifdef NO_WUFFS
//#include <fpng.h>
#else
#define WUFFS_IMPLEMENTATION
#include <wuffs.h>
#endif

TextureRef::TextureRef(): data(nullptr) {}

TextureRef::TextureRef(const TextureRef& o) {
	data = o.data;
	if(data) data->rc++;
}

void TextureRef::operator=(const TextureRef& o) {
	destroy();
	data = o.data;
	if(data) data->rc++;
}

TextureRef::~TextureRef() {
	destroy();
}

TextureRef::operator bool() {
	return data;
}

TextureData* TextureRef::operator->() {
	return data;
}

const TextureData* TextureRef::operator->() const {
	return data;
}

void TextureRef::destroy() {
	if(!data) return;
	if(data->rc == 1) {
		if(data->filename.size()) {
			log_info("deleting texture filename %s", data->filename.c_str());
		} else {
			log_info("deleting texture id %d", data->id);
		}
		destroy_impl();
		delete data;
	} else {
		data->rc--;
	}
	data = nullptr;
}

int TextureRef::width() const {
	return data? data->w: 0;
}

int TextureRef::height() const {
	return data? data->h: 0;
}

void TextureRef::upload_sub(void* data, int x, int y, int w, int h, int fmt) {
	if(!data || (fmt == this->data->format && this->data->pix_w == this->data->w && this->data->pix_h == this->data->h)) {
		upload_sub_impl(data, x, y, w, h);
	} else {
		ImageData imd {.buf = (u8*) data, .virtual_w = (u16) w, .virtual_h = (u16) h,
			.pix_w = (u16) w, .pix_h = (u16) h, .buf_w = (u16) w, .buf_h = (u16) h,
			.flags = 0, .fmt = (u16) fmt, .ownership = OWNERSHIP_KEEP};
		int new_w = w * this->data->pix_w / this->data->w;
		int new_h = h * this->data->pix_h / this->data->h;
		imd.resample(this->data->format, new_w, new_h, new_w, new_h);
		upload_sub_impl(imd.buf, x * this->data->pix_w / this->data->w, y * this->data->pix_h / this->data->h, new_w, new_h);
		imd.destroy();
	}
}

void store_image(u32* pix, int width, int height) {
	FILE *fp = fopen("screenshot.png", "wb");
	fclose(fp);
}

TextureRef g_load_texture(DataStream&& ds, int flags) {
	return g_load_texture(*&ds, flags);
}

#ifndef NO_WUFFS
struct WuffsCallbacks: wuffs_aux::DecodeImageCallbacks {
	bool opaque;

	wuffs_base__pixel_format SelectPixfmt(const wuffs_base__image_config& image_config) override {
		opaque = image_config.pixcfg.pixel_format().transparency() == WUFFS_BASE__PIXEL_ALPHA_TRANSPARENCY__OPAQUE;
		//return wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL);
		return wuffs_base__make_pixel_format(WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL);
	}

	DecodeImageCallbacks::AllocPixbufResult AllocPixbuf(
		const wuffs_base__image_config& image_config, bool allow_uninitialized_memory) override {
		uint32_t w = image_config.pixcfg.width();
		uint32_t h = image_config.pixcfg.height();
		if(w == 0 || h == 0) {
			return AllocPixbufResult("");
		}
		uint64_t len = image_config.pixcfg.pixbuf_len();
		if(len == 0 || len > SIZE_MAX) {
			return AllocPixbufResult(wuffs_aux::DecodeImage_UnsupportedPixelConfiguration);
		}
		void* ptr = malloc(len);
		if(!ptr) {
			return AllocPixbufResult(wuffs_aux::DecodeImage_OutOfMemory);
		}
		wuffs_base__pixel_buffer pixbuf;
		wuffs_base__status status = pixbuf.set_from_slice(&image_config.pixcfg,
			wuffs_base__make_slice_u8((uint8_t*) ptr, len));
		if(!status.is_ok()) {
			free(ptr);
			return AllocPixbufResult(status.message());
		}
		return AllocPixbufResult(wuffs_aux::MemOwner(ptr, &free), pixbuf);
	}
};

struct DataStreamWuffsInput: wuffs_aux::sync_io::Input {
	DataStreamWuffsInput(DataStream* ds): ds(ds) {}

	std::string CopyIn(wuffs_aux::IOBuffer* dst) override {
    	dst->compact();
		u32 n = dst->writer_length();
		n = std::min(n, ds->remaining());
		ds->read(dst->writer_pointer(), n);
    	dst->meta.wi += n;
    	dst->meta.closed = !ds->remaining();
		return "";
	}

private:
	DataStream* ds;
	DataStreamWuffsInput(const DataStreamWuffsInput&) = delete;
	DataStreamWuffsInput& operator=(const DataStreamWuffsInput&) = delete;
};
#endif

u8* load_cps(u8* data, int size, int* width, int* height, int* fmt);

TextureRef g_load_texture(DataStream& ds, int flags) {
	char sig[4];
	ds.read(sig, 4);
#ifndef NO_WUFFS
	if(!memcmp(sig, "\x89PNG", 4)) {
		ds.seek(0);
		WuffsCallbacks wcb;
		DataStreamWuffsInput dsinput(&ds);
		wuffs_aux::DecodeImageResult img = wuffs_aux::DecodeImage(wcb, dsinput);
		if(img.error_message.size()) {
			log_err("Wuffs error: %s", img.error_message.c_str());
			platform_throw();
		}
		img.pixbuf_mem_owner.release();
		wuffs_base__table_u8 plane = img.pixbuf.plane(0);
		int fmt = wcb.opaque? TEX_FORMAT_RGBX: TEX_FORMAT_RGBA;
		return g_upload_texture(plane.ptr, img.pixbuf.pixcfg.width(), img.pixbuf.pixcfg.height(), fmt, flags, OWNERSHIP_OWNED);
	}
#else
	if(!memcmp(sig, "\x89PNG", 4)) {
		ds.read(nullptr, 4);
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		png_infop png_info = png_create_info_struct(png_ptr);
		png_set_read_fn(png_ptr, &ds, [](png_structp png_ptr, png_bytep out, png_size_t size) {
			DataStream* ds = (DataStream*) png_get_io_ptr(png_ptr);
			ds->read(out, size);
		});
		png_set_sig_bytes(png_ptr, 8);
		png_read_info(png_ptr, png_info);
		png_uint_32 w = 0, h = 0;
		int fmt, bitdep;
		png_get_IHDR(png_ptr, png_info, &w, &h, &bitdep, &fmt, nullptr, nullptr, nullptr);
		u32 row_size = png_get_rowbytes(png_ptr, png_info);
		u16 buf_w = w, buf_h = h;
		u32 off = 0;
		switch(fmt) {
			case PNG_COLOR_TYPE_RGB: {
				if(TEX_FORMAT_RGB & HARDWARE_TEX_FORMATS) {
					ImageData::optimize_size(TEX_FORMAT_RGB, buf_w, buf_h);
					u8* pixels = (u8*) malloc(buf_w * buf_h * 3);
					for(int y = 0; y < h; y++) {
						png_read_row(png_ptr, pixels + off, nullptr);
						off += buf_w * 3;
					}
					return g_upload_texture2(pixels, w, h, w, h, buf_w, buf_h, TEX_FORMAT_RGB, flags, OWNERSHIP_OWNED);
				} else {
					png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
					ImageData::optimize_size(TEX_FORMAT_RGBA, buf_w, buf_h);
					u8* pixels = (u8*) malloc(buf_w * buf_h * 4);
					for(int y = 0; y < h; y++) {
						png_read_row(png_ptr, pixels + off, nullptr);
						off += buf_w * 4;
					}
					png_destroy_read_struct(&png_ptr, &png_info, nullptr);
					return g_upload_texture2(pixels, w, h, w, h, buf_w, buf_h, TEX_FORMAT_RGBA, flags, OWNERSHIP_OWNED);
				}
			}
			case PNG_COLOR_TYPE_RGBA: {
				ImageData::optimize_size(TEX_FORMAT_RGBA, buf_w, buf_h);
				u8* pixels = (u8*) malloc(buf_w * buf_h * 4);
				for(int y = 0; y < h; y++) {
					png_read_row(png_ptr, pixels + off, nullptr);
					off += buf_w * 4;
				}
				png_destroy_read_struct(&png_ptr, &png_info, nullptr);
				return g_upload_texture2(pixels, w, h, w, h, buf_w, buf_h, TEX_FORMAT_RGBA, flags, OWNERSHIP_OWNED);
			}
			case PNG_COLOR_TYPE_GRAY: {
				ImageData::optimize_size(TEX_FORMAT_GRAY, buf_w, buf_h);
				u8* pixels = (u8*) malloc(buf_w * buf_h);
				if(bitdep < 8) {
					png_set_expand_gray_1_2_4_to_8(png_ptr);
				}
				for(int y = 0; y < h; y++) {
					png_read_row(png_ptr, pixels + off, nullptr);
					off += buf_w;
				}
				png_destroy_read_struct(&png_ptr, &png_info, nullptr);
				return g_upload_texture2(pixels, w, h, w, h, buf_w, buf_h, TEX_FORMAT_GRAY, flags, OWNERSHIP_OWNED);
			}
			case PNG_COLOR_TYPE_GRAY_ALPHA: {
				ImageData::optimize_size(TEX_FORMAT_GRAY_ALPHA, buf_w, buf_h);
				u8* pixels = (u8*) malloc(buf_w * buf_h * 2);
				for(int y = 0; y < h; y++) {
					png_read_row(png_ptr, pixels + off, nullptr);
					off += buf_w * 2;
				}
				png_destroy_read_struct(&png_ptr, &png_info, nullptr);
				return g_upload_texture2(pixels, w, h, w, h, buf_w, buf_h, TEX_FORMAT_GRAY_ALPHA, flags, OWNERSHIP_OWNED);
			}
			default:
				png_destroy_read_struct(&png_ptr, &png_info, nullptr);
				unimplemented();
				break;
		}
	}
#endif
	/*else if(!memcmp(sig, "\xFF\xD8\xFF", 3) || !memcmp(sig, "GIF", 3)) {
		ds.seek(0);
		stbi_io_callbacks cbs {
			.read = [](void* user, char* out, int size) {
				if(size > ((DataStream*) user)->remaining()) {
					size = ((DataStream*) user)->remaining();
				}
				((DataStream*) user)->read(out, size);
				return size;
			},
			.skip = [](void* user, int size) {
				if(size < 0) {
					((DataStream*) user)->seek(((DataStream*) user)->tell() - size);
				} else {
					if(size > ((DataStream*) user)->remaining()) {
						size = ((DataStream*) user)->remaining();
					}
					((DataStream*) user)->skip(size);
				}
			},
			.eof = [](void* user) -> int {
				return !((DataStream*) user)->remaining();
			}
		};
		int w, h, channels;
		u8* pixels = stbi_load_from_callbacks(&cbs, (void*) &ds, &w, &h, &channels, 0);
		if(!pixels) {
			log_err("error loading image");
			platform_throw();
		}
		TextureRef tex = g_upload_texture(pixels, w, h,
			channels == 4? TEX_FORMAT_RGBA:
			channels == 3? TEX_FORMAT_RGB:
			channels == 2? TEX_FORMAT_GRAY_ALPHA:
			TEX_FORMAT_GRAY, flags, true);
		return tex;
	} */else if(!memcmp(sig, "ETP\0", 4)) {
		unimplemented();
	} else if(!memcmp(sig, "CPS\0", 4)) {
		ds.seek(0);
		u32 sz = ds.remaining();
		u8* buf = new u8[sz];
		ds.read(buf, sz);
		int w, h, fmt;
		u8* pixels = load_cps(buf, sz, &w, &h, &fmt);
		TextureRef tex = g_upload_texture(pixels, w, h, fmt, flags, true);
		return tex;
	} else {
		log_err("unknown image format signature %.4s", sig);
		platform_throw();
	}
	return TextureRef();
}

static u8 white_pix[] {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
static TextureRef white_tex;
TextureRef& g_white_tex() {
	if(!white_tex.data) {
		white_tex = g_upload_texture(white_pix, 2, 1, TEX_FORMAT_RGBA, TEX_NEAREST, OWNERSHIP_KEEP);
	}
	return white_tex;
}

static u8 grad_pix[] {255, 255, 255, 255, 255, 255, 255, 0};
static TextureRef grad_tex;
TextureRef& g_grad_tex() {
	if(!grad_tex.data) {
		grad_tex = g_upload_texture(grad_pix, 2, 1, TEX_FORMAT_RGBA, TEX_LINEAR, OWNERSHIP_KEEP);
	}
	return grad_tex;
}

static u8 grad_edge_pix[] {255, 255, 255, 255, 255, 255, 255, 0, 255, 255, 255, 0, 255, 255, 255, 0};
static TextureRef grad_edge_tex;
TextureRef& g_grad_corner_tex() {
	if(!grad_edge_tex.data) {
		grad_edge_tex = g_upload_texture(grad_edge_pix, 2, 2, TEX_FORMAT_RGBA, TEX_LINEAR, OWNERSHIP_KEEP);
	}
	return grad_edge_tex;
}
