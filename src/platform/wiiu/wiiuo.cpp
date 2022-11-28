#include <util.hpp>
#include <cstdlib>
#include <utility>
#include <vector>
#include <map>
#include <chrono>
#include <api.hpp>
#include <graphics.hpp>
#include <whb/log_console.h>
#include <whb/log.h>
#include <whb/proc.h>
#include <gx2/draw.h>
#include <gx2/context.h>
#include <gx2/surface.h>
#include <gx2/utils.h>
#include <gx2/display.h>
#include <gx2/swap.h>
#include <gx2/mem.h>
#include <gx2/clear.h>
#include <gx2/state.h>
#include <gx2/registers.h>
#include <coreinit/memheap.h>
#include <coreinit/memunitheap.h>
#include <coreinit/memblockheap.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memfrmheap.h>
#include "gx2gl/include/GL/gx2gl.h"
#include "gx2gl/include/GL/gl.h"
#include "gx2gl/include/GL/glut.h"

static int max_aspect_ratio_w = 0, max_aspect_ratio_h = 0; //static int max_aspect_ratio_w = 7, max_aspect_ratio_h = 3;
static int min_aspect_ratio_w = 0, min_aspect_ratio_h = 0; //static int min_aspect_ratio_w = 4, min_aspect_ratio_h = 3;

struct Shader {
	int id, loc_pos, loc_scale, loc_rot, loc_pivot, loc_scissor, loc_tex_off, loc_tex_scale, loc_tex_size, loc_color, loc_color_off;

	void load(const char* shader_name, const char* verstr, const char* src) {
		id = 1;
		// compile vert
		// compile frag
		// link shaders
		// init uniforms
	}

	void use() {
		// bind shader in the graphics API
		int sw = g_width(), sh = g_height();
		int x = (physical_width - sw) / 2, y = (physical_height - sh) / 2;
	}
};

Shader shader_normal, shader_copy_batched, shader_n7_calendar, shader_alpha_batched, shader_push, shader_calc_grad, shader_push_grad;
bool upscaling_enabled = false;

u32 square, fbo, fbtex, fblumtex;
float square_data[] {
	0.f, 1.f, 0.f,
	0.f, 1.f,
	1.f, 0.f, 0.f,
	1.f, 0.f,
	0.f, 0.f, 0.f,
	0.f, 0.f,
	0.f, 1.f, 0.f,
	0.f, 1.f,
	1.f, 0.f, 0.f,
	1.f, 0.f,
	1.f, 1.f, 0.f,
	1.f, 1.f
};

void a_init();

// used when going back from fullscreen to windowed
static int prev_x = 128, prev_y = 128, prev_w = 800, prev_h = 600;

void store_image(u32* pix, int width, int height);

static bool running = false;
static MEMHeapHandle heap_mem1, heap_fg, heap_mem2;

int main(int argc, char** argv) {
	WHBProcInit();
	WHBLogConsoleInit();
	while(WHBProcIsRunning() && running) {
		WHBLogPrintf("jojo\n");
		WHBLogConsoleDraw();
	}
	return 0;
}

TextureRef _g_upload_texture(void* data, int w, int h, int format, int flags, bool* keep_alloc) {
	u32 id = 1;
	int glfmt;
	switch(format) {
		case TEX_FORMAT_RGBA:
			break;
		case TEX_FORMAT_BGRA:
			break;
		case TEX_FORMAT_RGB:
			break;
		case TEX_FORMAT_BGR:
			break;
		case TEX_FORMAT_ALPHA:
			break;
		case TEX_FORMAT_GRAY:
			break;
		case TEX_FORMAT_GRAY_ALPHA:
			break;
	}
	TextureRef tex;
	tex.data = new TextureData();
	tex->id = id;
	tex->oid = id;
	tex->rc = 1;
	tex->w = w;
	tex->h = h;
	tex->upw = w;
	tex->uph = h;
	tex->upscale = true;
	tex->format = format;
	return tex;
}

void TextureRef::upload_sub_impl(void* data, int x, int y, int w, int h) {
}

void TextureRef::destroy() {
	if(data->filename) {
		log_info("deleting %s", data->filename.value().c_str());
	}
	if(data->rc == 1) {
		// delete it in the graphics API
		delete data;
	} else {
		data->rc--;
	}
	data = nullptr;
}

MeshRef g_upload_mesh(MeshLoadData& data) {
	MeshLiveData* live = new MeshLiveData {
		.rc = 0,
		.vbo = 0,
		.ibo = 0,
		.count = 0,
		.local_transform = data.local_transform,
		.material = data.material
	};
	return MeshRef(live);
}

MeshLiveData::~MeshLiveData() {
}

void g_draw_mesh(const MeshRef& mesh) {
}

void g_ensure_upscale(TextureRef& tex, int dw, int dh) {
}

void DrawBatch::draw() const {
}

void sys_exit() {
	running = false;
	exit(0);
}

void sys_show_logs() {
}
