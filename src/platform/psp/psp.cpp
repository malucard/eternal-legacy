#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <psputility_sysparam.h>
#include <vram.h>

extern "C" {
/* Define the module info section */
PSP_MODULE_INFO("eternal", 0, 1, 0);
/* Define the main thread's attribute value (optional) */
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1);
/* Define printf, just to make typing easier */
//#define printf	pspDebugScreenPrintf
}

#ifndef PLATFORM_PSP // for intellisense
#undef PLATFORM_PC
#define PLATFORM_PSP
#undef _platform_throw
#endif

#include <util.hpp>
#include <cstdlib>
#include <utility>
#include <vector>
#include <map>
#include <api.hpp>
#include <input.hpp>
#include <graphics.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "psp_defines.hpp"

void a_init();

static_assert((sizeof(Vertex) & 0b11) == 0);

static bool running = false;

int callback_thread(SceSize args, void* argp) {
	int cbid;
	cbid = sceKernelCreateCallback("Exit Callback", [](int arg1, int arg2, void*) {
		running = false;
		return 0;
	}, nullptr);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

#define BUF_WIDTH 512
#define SCR_WIDTH 480
#define SCR_HEIGHT 272
static unsigned int __attribute__((aligned(16))) list[262144];

#define LOGS_SIZE 2048
static char logs[LOGS_SIZE];

void psp_push_logs(const char* line) {
	size_t len = strlen(line);
	fprintf(stdout, "%s\n", line);
	// we avoid overwriting the last byte of the buffer because it's a trailing zero
	memmove(logs + len + 1, logs, LOGS_SIZE - len - 2);
	memcpy(logs, line, len);
	logs[len] = '\n';
	logs[LOGS_SIZE - 1] = 0;
}

static bool in_frame = false;
static bool jojo = false;
static void* fbp0;
static u32 __attribute__((aligned(16))) gray_clut[256];
static std::vector<void*> frame_bufs;

int main() {
	logs[0] = 0;
	a_init();
	int thread = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
	if(thread >= 0) {
		sceKernelStartThread(thread, 0, 0);
	}
	fbp0 = vrelptr(valloc(BUF_WIDTH * SCR_HEIGHT * 4)); // GU_PSM_8888
	void* fbp1 = vrelptr(valloc(BUF_WIDTH * SCR_HEIGHT * 4)); // GU_PSM_8888
	void* zbp = vrelptr(valloc(BUF_WIDTH * SCR_HEIGHT * 2)); // GU_PSM_4444
	sys_gfx_vendor = "Sony";
	sys_gfx_renderer = "Tachyon";
	sys_gfx_version = "Eternal PSP renderer";
	int swap_o_x;
	sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &swap_o_x);
	input_map_setup(!swap_o_x);
	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	sceGuDrawBuffer(GU_PSM_8888, fbp0, BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, fbp1, BUF_WIDTH);
	sceGuDepthBuffer(zbp, BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
	sceGuDepthRange(65535, 0);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuDisable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CCW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuDisable(GU_CULL_FACE);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);
	sceGuTexScale(1.f, 1.f);
	// to save memory, we support 8-bit grayscale by indexing
	for(int i = 0; i < 256; i++) {
		gray_clut[i] = i << 24 | 0x00FFFFFF;
	}
	if(HARDWARE_TEX_FORMATS & TEX_FORMAT_ALPHA4) {
		// to save memory, we support 4-bit grayscale by indexing
		for(int i = 0; i < 16; i++) {
			gray_clut[i] = i << 24 | i << 28 | 0x00FFFFFF;
		}
	}
	sceKernelDcacheWritebackRange(gray_clut, sizeof(gray_clut));
	sceGuClutMode(GU_PSM_8888, 0, 0xFF, 0);
	sceGuClutLoad((HARDWARE_TEX_FORMATS & TEX_FORMAT_ALPHA4)? 2: 32, gray_clut);
	physical_width = 480;
	physical_height = 272;
	update_screen_size();
#ifdef LOW_BANDWIDTH
	g_default_proj = glm::ortho(0.f, 0.14285714285714f, 0.14285714285714f, 0.f, -1000.f, 1000.f);
#else
	g_default_proj = glm::ortho(0.f, 1.f, 1.f, 0.f, -1000.f, 1000.f);
#endif
	api_init(0, nullptr);
	api_load();
	sceGuFinish();
	sceGuSync(0, 0);
	sceGuDisplay(GU_TRUE);
	SceCtrlData prev;
	running = true;
	while(running) {
		sceGuStart(GU_DIRECT, list);
		in_frame = true;
		sceGuClearColor(0xFF000000);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
		sceGumMatrixMode(GU_PROJECTION);
		sceGumLoadIdentity();
		sceGumMatrixMode(GU_VIEW);
		sceGumLoadIdentity();
		SceCtrlData ctrl;
		sceCtrlReadBufferPositive(&ctrl, 1);
#define CHECK_BTN_DOWN(btn) ((ctrl.Buttons & btn) && !(prev.Buttons & btn))
#define CHECK_BTN_UP(btn) (!(ctrl.Buttons & btn) && (prev.Buttons & btn))
#define CHECK_BTN(btn, input)\
	if(CHECK_BTN_DOWN(btn)) {input_map_submit(input, 1.f, false);}\
	else if(CHECK_BTN_UP(btn)) {input_map_submit(input, 0.f, false);}
		CHECK_BTN(PSP_CTRL_UP, CTRL_UP);
		CHECK_BTN(PSP_CTRL_DOWN, CTRL_DOWN);
		CHECK_BTN(PSP_CTRL_LEFT, CTRL_LEFT);
		CHECK_BTN(PSP_CTRL_RIGHT, CTRL_RIGHT);
		CHECK_BTN(PSP_CTRL_CROSS, CTRL_A);
		CHECK_BTN(PSP_CTRL_CIRCLE, CTRL_B);
		CHECK_BTN(PSP_CTRL_SQUARE, CTRL_X);
		CHECK_BTN(PSP_CTRL_TRIANGLE, CTRL_Y);
		CHECK_BTN(PSP_CTRL_LTRIGGER, CTRL_L);
		CHECK_BTN(PSP_CTRL_RTRIGGER, CTRL_R);
		CHECK_BTN(PSP_CTRL_SELECT, CTRL_SELECT);
		CHECK_BTN(PSP_CTRL_START, CTRL_START);
		input_map_submit(CTRL_STICK_R_RIGHT, ctrl.Lx, false);
		input_map_submit(CTRL_STICK_R_DOWN, ctrl.Ly, false);
		prev = ctrl;
		for(int i = 0; i < frame_bufs.size(); i++) {
			delete[] (Vertex*) (frame_bufs[i]);
		}
		frame_bufs.clear();
		g_proj_reset();
		api_frame();
		sceGuFinish();
		in_frame = false;
		sceGuSync(0, 0);
		if(!unlock_fps && !sceDisplayIsVblank()) {
			sceDisplayWaitVblankStart();
		}
		fbp0 = sceGuSwapBuffers();
	}
	sceGuTerm();
	sceKernelExitGame();
	return 0;
}

#define VRAM_BASE 0x04000000
#define UNCACHED_POINTER 0x40000000

void swizzle_fast(u8* dest, const u8* source, unsigned int width, unsigned int height) {
	i32 i,j;
	i32 rowblocks = (width / 16);
	i32 rowblocks_add = (rowblocks-1)*128;
	u32 block_address = 0;
	u32* src = (u32*) source;

	// (j-blocky*8)*16 + (i-blockx*16) + (blockx + blocky*rowblocks)*16*8 =
	// j*16 + (blocky*rowblocks-blocky)*16*8 + i + (blockx*8-blockx)*16 =   | blocky8 := j/8*8 = j&~0x7, blockx16 := i/16*16 = i&~0xf
	// j*16 + blocky8*(rowblocks-1)*16 + i + blockx16*8-blockx16 =   | k = 0..15, rowmul := (rowblocks-1)*16
	// j*16 + blocky8*rowmul + k + blockx16 + blockx16*8-blockx16 =

	for (j = 0; j < height; j++,block_address+=16)
	{
		u32 *block;
		if ((u32)dest>=VRAM_BASE+0x00200000)
			block = (u32*)&dest[block_address];
		else
			block = (u32*)((u32)&dest[block_address] | UNCACHED_POINTER);
		for (i = 0; i < rowblocks; i++)
		{
			*block++ = *src++;
			*block++ = *src++;
			*block++ = *src++;
			*block++ = *src++;
			block += 28;
		}

		if ((j&0x7)==0x7)
			block_address += rowblocks_add;
	}
}

int fmt_to_gu(int fmt) {
	switch(fmt) {
		case TEX_FORMAT_ALPHA: return GU_PSM_T8;
		case TEX_FORMAT_ALPHA4: return GU_PSM_T4;
		case TEX_FORMAT_4444: return GU_PSM_4444;
		case TEX_FORMAT_5551: return GU_PSM_5551;
		case TEX_FORMAT_565: return GU_PSM_5650;
		default: return GU_PSM_8888;
	}
}

TextureRef _g_upload_texture(ImageData& imd) {
	TextureRef tex;
	tex.data = new TextureData;
	tex->swizzled = imd.fmt < TEX_FORMAT_ALPHA && !(imd.buf_w & 15) && !(imd.buf_h & 7);
	tex->ownership = OWNERSHIP_OWNED;
	void* nbuf = valloc(imd.buf_size());
	if(!imd.buf) {
		if(nbuf) {
			memset(nbuf, 0, imd.buf_size());
			tex->on_vram = true;
			log_info("tex created on vram, %u avail", vmemavail());
		} else {
			nbuf = calloc(1, imd.buf_size());
			tex->on_vram = false;
		}
	} else {
		if(nbuf) {
			tex->on_vram = true;
			log_info("tex created on vram x, %u avail", vmemavail());
		} else {
			nbuf = malloc(imd.buf_size());
			tex->on_vram = false;
		}
		if(tex->swizzled) {
			swizzle_fast((u8*) nbuf, imd.buf, ImageData::fmt_len(imd.buf_w, imd.fmt), imd.buf_h);
		} else {
			memcpy(nbuf, imd.buf, imd.buf_size());
		}
	}
	tex->id = (int) nbuf;
	if(!tex->on_vram) {
		sceKernelDcacheWritebackRange((void*) tex->id, imd.buf_size());
		sceGuTexFlush();
	}
	tex->rc = 1;
	tex->w = imd.virtual_w;
	tex->h = imd.virtual_h;
	tex->pot_x = 0;
	tex->pot_y = 0;
	tex->pix_w = imd.pix_w;
	tex->pix_h = imd.pix_h;
	tex->buf_w = imd.buf_w;
	tex->buf_h = imd.buf_h;
	tex->format = imd.fmt;
	tex->filter = (imd.flags & TEX_NEAREST)? TEX_NEAREST: (imd.flags & TEX_LINEAR)? TEX_LINEAR: 0;
	//log_info("new texture w:%u h:%u vram:%d swiz:%d", tex->buf_w, tex->buf_h, tex->on_vram, tex->swizzled);
	return tex;
}

void TextureRef::upload_sub_impl(void* stamp2, int x, int y, int w, int h) {
	u32 stamp_row_size = ImageData::fmt_len(w, this->data->format);
	u32 buf_row_size = ImageData::fmt_len(this->data->buf_w, this->data->format);
	u32 x_off = ImageData::fmt_len(x, this->data->format);
	u8* ptr = (u8*) this->data->id;
	if(stamp2) {
		if(this->data->swizzled) {
			u8 stamp[stamp_row_size * h];
			swizzle_fast(stamp, (u8*) stamp2, stamp_row_size, h);
			for(int cur_y = 0; cur_y < h; cur_y++) {
				memcpy(ptr + (y + cur_y) * buf_row_size + x_off, (u8*) stamp + cur_y * stamp_row_size, stamp_row_size);
			}
		} else {
			for(int cur_y = 0; cur_y < h; cur_y++) {
				memcpy(ptr + (y + cur_y) * buf_row_size + x_off, (u8*) stamp2 + cur_y * stamp_row_size, stamp_row_size);
			}
		}
	} else {
		for(int cur_y = y; cur_y < y + h; cur_y++) {
			memset(ptr + cur_y * buf_row_size + x_off, 0, stamp_row_size);
		}
	}
	sceKernelDcacheWritebackRange((void*) this->data->id, buf_row_size * data->buf_h);
}

void TextureRef::destroy_impl() {
	if(data->id && data->ownership == OWNERSHIP_OWNED) {
		if(data->on_vram) {
			vfree((void*) data->id);
		} else {
			free((void*) data->id);
		}
	}
	data->id = 0;
}

glm::vec4 TextureRef::get_pixel(int x, int y, int tw, int th) const {
	if(data) {
		u32 color = *(u32*) data->id + y * data->pix_h / data->h * data->buf_w + x * data->pix_w / data->w;
		return glm::vec4((color & 0xFF) / 255.f, (color >> 8 & 0xFF) / 255.f, (color >> 16 & 0xFF) / 255.f, (color >> 24) / 255.f);
	} else return {};
}

void g_ensure_upscale(TextureRef& tex, int dw, int dh) {}

static void try_promote_vram(TextureRef& tex) {
	if(!tex->on_vram) {
		u32 size = ImageData::fmt_len(tex->buf_w, tex->format) * tex->buf_h;
		void* vmem = valloc(size);
		if(vmem) {
			memcpy(vmem, (void*) tex->id, size);
			if(tex->ownership == OWNERSHIP_OWNED) {
				free((void*) tex->id);
			} else {
				tex->ownership = OWNERSHIP_OWNED;
			}
			tex->id = (u32) vmem;
			tex->on_vram = true;
		}
	}
}

extern glm::mat4 tmp_mat;

void DrawBatch::draw() {
	try_promote_vram(tex);
	int sw = g_real_width(), sh = g_real_height();
	sceGuViewport(real_screen_x, real_screen_y, sw, sh);
	sceGuScissor(g_real_scissor.x + real_screen_x, g_real_scissor.y + real_screen_y, g_real_scissor.w, g_real_scissor.h);
	sceGuTexMode(fmt_to_gu(tex->format), 0, 0, tex->swizzled);
	sceGuTexImage(0, tex->buf_w, tex->buf_h, tex->buf_w, (void*) tex->id);
	sceGuTexFilter(tex->filter == TEX_NEAREST? GU_NEAREST: GU_LINEAR, tex->filter == TEX_NEAREST? GU_NEAREST: GU_LINEAR);
	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadMatrix((ScePspFMatrix4*) glm::value_ptr(g_proj_matrix));
	sceKernelDcacheWritebackRange(vertices.data(), vertices.size() * sizeof(Vertex));
#ifndef NO_INDEXED_RENDER
	sceKernelDcacheWritebackRange(indices.data(), indices.size() * sizeof(u16));
#ifdef LOW_BANDWIDTH
	sceGumDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_INDEX_16BIT | GU_TRANSFORM_3D, indices.size(), indices.data(), vertices.data());
#else
	sceGumDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_INDEX_16BIT | GU_TRANSFORM_3D, indices.size(), indices.data(), vertices.data());
#endif
	polys += indices.size() / 2;
	frame_bufs.push_back((void*) indices.data());
	new(&indices) std::vector<u16, BatchAllocator<u16>>;
#else
#ifdef LOW_BANDWIDTH
	sceGumDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_INDEX_16BIT | GU_TRANSFORM_3D, vertices.size(), 0, vertices.data());
#else
	sceGumDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_INDEX_16BIT | GU_TRANSFORM_3D, vertices.size(), 0, vertices.data());
#endif
	polys += vertices.size() / 2;
#endif
	frame_bufs.push_back((void*) vertices.data());
	new(&vertices) std::vector<Vertex, BatchAllocator<Vertex>>;
	draw_calls++;
}

MeshRef g_upload_mesh(MeshLoadData& data) {
	// we simply take the pointer and force the vectors to leak their memory to avoid an unnecessary copy
	MeshLiveData* live = new MeshLiveData {
		.rc = 0,
		.vbo = data.verts.data(),
		.ibo = data.indices.data(),
		.count = (u32) data.indices.size(),
		.local_transform = data.local_transform,
		.material = data.material
	};
	sceKernelDcacheWritebackRange(data.verts.data(), data.verts.size() * sizeof(Vertex3D));
	sceKernelDcacheWritebackRange(data.indices.data(), data.indices.size() * sizeof(u16));
	new(&data.verts) std::vector<Vertex3D>;
	new(&data.indices) std::vector<u16>;
	return MeshRef(live);
}

MeshLiveData::~MeshLiveData() {
	delete[] (Vertex3D*) vbo;
	delete[] (u16*) ibo;
}

void g_draw_mesh(const MeshRef& mesh) {
	TextureRef tex = mesh->material.tex;
	try_promote_vram(tex);
	int sw = g_real_width(), sh = g_real_height();
	sceGuViewport(real_screen_x, real_screen_y, sw, sh);
	sceGuScissor(g_real_scissor.x + real_screen_x, g_real_scissor.y + real_screen_y, g_real_scissor.w, g_real_scissor.h);
	sceGuEnable(GU_DEPTH_TEST);
	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadMatrix((ScePspFMatrix4*) glm::value_ptr(g_proj_matrix));
	sceGumMatrixMode(GU_MODEL);
	sceGumLoadMatrix((ScePspFMatrix4*) glm::value_ptr(g_cur_matrix));
	glm::ivec3 amb = glm::clamp(g_ambient_color, glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)) * 255.f;
	sceGuAmbientColor(GU_RGBA(amb.x, amb.y, amb.z, 0xFF));
	sceGuAmbient(GU_RGBA(amb.x, amb.y, amb.z, 0xFF));
	sceGuEnable(GU_LIGHTING);
	sceGuLightMode(GU_SINGLE_COLOR);
	for(int i = 0; i < MAX_LIGHTS; i++) {
		if(lights[i].type) {
			sceGuEnable(GU_LIGHT0 + i);
			sceGuLight(i, lights[i].type == 1? GU_POINTLIGHT: lights[i].type == 2? GU_DIRECTIONAL: GU_SPOTLIGHT,
				GU_DIFFUSE_AND_SPECULAR, (ScePspFVector3*) glm::value_ptr(lights[i].pos));
			glm::ivec3 diff = glm::normalize(lights[i].color) * 255.f;
			sceGuLightColor(i, GU_DIFFUSE, (GU_RGBA(diff.x, diff.y, diff.z, 0xFF)));
			sceGuLightColor(i, GU_SPECULAR, (GU_RGBA(0xFF, 0xFF, 0xFF, 0xFF)));
			sceGuLightAtt(i, 0.f, glm::length(lights[i].color), 0.f);
		} else {
			sceGuDisable(GU_LIGHT0 + i);
		}
	}
	sceGuSpecular(12.0f);
	sceGuTexMode(fmt_to_gu(tex->format), 0, 0, tex->swizzled);
	sceGuTexImage(0, tex->buf_w, tex->buf_h, tex->buf_w, (void*) tex->id);
	//sceGuTexEnvColor(0xffffffff);
	//sceGuColor(0xffffffff);
	//sceGuColorMaterial(GU_AMBIENT_AND_DIFFUSE);
	//sceGuModelColor(0, 0xffffffff, 0xffffffff, 0);
	//sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA); // NOTE: this enables reads of the alpha-component from the texture, otherwise blend/test won't work
	sceGuTexFilter(tex->filter == TEX_NEAREST? GU_NEAREST: GU_LINEAR, tex->filter == TEX_NEAREST? GU_NEAREST: GU_LINEAR);
	//sceGuTexWrap(GU_CLAMP, GU_CLAMP);
	ScePspFMatrix4 identity;
	gumLoadIdentity(&identity);
	for(int i = 0; i < 8; i++) {
		sceGuBoneMatrix(i, &identity);
		sceGuMorphWeight(i, 1.f);
	}
	sceGumDrawArray(GU_TRIANGLES, GU_WEIGHT_32BITF | GU_WEIGHTS(5)
		| GU_TEXTURE_32BITF
		| GU_NORMAL_32BITF
		| GU_VERTEX_32BITF | GU_TRANSFORM_3D
		| GU_INDEX_16BIT, mesh->count, (void*) mesh->ibo, (void*) mesh->vbo);
	polys += mesh->count / 3;
	draw_calls++;
	sceGuDisable(GU_LIGHTING);
	sceGuDisable(GU_DEPTH_TEST);
	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
}

void sys_show_logs() {
	if(in_frame) sceGuFinish();
	int y = 0;
	while(true) {
		SceCtrlData ctrl;
		sceCtrlReadBufferPositive(&ctrl, 1);
		if(ctrl.Buttons & (PSP_CTRL_HOME | PSP_CTRL_CIRCLE | PSP_CTRL_CROSS)) {
			sceGuStart(GU_DIRECT, list);
			return;
		} else if(ctrl.Buttons & PSP_CTRL_UP) {
			y += 7;
		} else if(ctrl.Buttons & PSP_CTRL_DOWN) {
			y -= 7;
		}
		for(int i = 0; i < frame_bufs.size(); i += 2) {
			delete[] (Vertex*) (frame_bufs[i]);
			delete[] (u16*) (frame_bufs[i + 1]);
		}
		frame_bufs.clear();
		sceGuStart(GU_DIRECT, list);
		sceGuClearColor(0xFF000000);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
		g_set_virtual_size(-1, -1);
		g_proj_reset();
		g_reset_stacks();
		g_draw_text(logs, 0, y, 480, 14);
		g_flush();
		sceGuFinish();
		sceGuSync(0, 0);
		sceDisplayWaitVblankStart();
		fbp0 = sceGuSwapBuffers();
	}
}

void _platform_throw() {
	log_err("exception thrown, stopping app and printing (higher = latest)");
	sys_show_logs();
	sceKernelExitGame();
}

void sys_exit() {
	running = false;
	log_err("exit requested");
	sceKernelExitGame();
}

static auto init_time = sceKernelGetSystemTimeLow();

u32 sys_millis() {
	return (sceKernelGetSystemTimeLow() - init_time) / 1000;
}

float sys_secs() {
	return (sceKernelGetSystemTimeLow() - init_time) / 1000 / 1000.f;
}
