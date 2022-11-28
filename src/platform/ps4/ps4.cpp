#ifndef PLATFORM_PS4
#define PLATFORM_PS4 // for IDE
#undef _platform_throw
#endif
#include "ps4_defines.hpp"
#include <graphics.hpp>
#include "../gl/gl_header.hpp"
#include <orbis/Pigletv2VSH.h>
#include <orbis/libkernel.h>
#include <orbis/SystemService.h>
#include <orbis/Sysmodule.h>

static EGLDisplay egl_display;
static EGLContext egl_context;
static EGLSurface egl_surface;

static bool initEgl(void* win) {
	// Connect to the EGL default display
	egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if(!egl_display) {
		log_err("Could not connect to display! error: %d", eglGetError());
		goto _fail0;
	}

	// Initialize the EGL display connection
	eglInitialize(egl_display, nullptr, nullptr);

	// Select OpenGL (Core) as the desired graphics API
	if(eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
		log_err("Could not set API! error: %d", eglGetError());
		goto _fail1;
	}

	// Get an appropriate EGL framebuffer configuration
	EGLConfig config;
	int numConfigs;
	static const int framebuffer_attrib_list[] {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};
	eglChooseConfig(egl_display, framebuffer_attrib_list, &config, 1, &numConfigs);
	if(numConfigs == 0) {
		log_err("error getting egl config: %d", eglGetError());
		goto _fail1;
	}
	egl_surface = eglCreateWindowSurface(egl_display, config, (EGLNativeWindowType) win, nullptr);
	if(!egl_surface) {
		log_err("error creating egl surface: %d", eglGetError());
		goto _fail1;
	}
	static const int ctx_attrib_list[] {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE,
	};
	egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, ctx_attrib_list);
	if(!egl_context) {
		log_err("error creating egl context: %d", eglGetError());
		goto _fail2;
	}

	// Connect the context to the surface
	eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
	return true;

_fail2:
	eglDestroySurface(egl_display, egl_surface);
	egl_surface = nullptr;
_fail1:
	eglTerminate(egl_display);
	egl_display = nullptr;
_fail0:
	return false;
}

static void deinitEgl() {
	if(egl_display) {
		eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if(egl_context) {
			eglDestroyContext(egl_display, egl_context);
			egl_context = nullptr;
		}
		if(egl_surface) {
			eglDestroySurface(egl_display, egl_surface);
			egl_surface = nullptr;
		}
		eglTerminate(egl_display);
		egl_display = nullptr;
	}
}

int main() {
	std::string prefix = std::string("/") + sceKernelGetFsSandboxRandomWord() + "/common/lib/";
	std::string piglet_path = prefix + "libScePigletv2VSH.sprx";
	int result;
	int piglet = sceKernelLoadStartModule(piglet_path.c_str(), 0, nullptr, 0, nullptr, &result);
	if(piglet < ORBIS_OK) {
		log_err("error %x loading piglet module", piglet);
		return 1;
	}
	physical_width = 1920;
	physical_height = 1080;
	OrbisPglConfig pgl_config{};
	memset(&pgl_config, 0, sizeof(pgl_config));
	pgl_config.size = sizeof(pgl_config);
	pgl_config.flags = ORBIS_PGL_FLAGS_USE_COMPOSITE_EXT | ORBIS_PGL_FLAGS_USE_FLEXIBLE_MEMORY | 0x60;
	pgl_config.processOrder = 1;
	pgl_config.systemSharedMemorySize = 250 * 1024 * 1024;
	pgl_config.videoSharedMemorySize = 512 * 1024 * 1024; 
	pgl_config.maxMappedFlexibleMemory = 170 * 1024 * 1024;
	pgl_config.drawCommandBufferSize = 1024 * 1024;
	pgl_config.lcueResourceBufferSize = 1024 * 1024;
	pgl_config.dbgPosCmd_0x40 = physical_width;
	pgl_config.dbgPosCmd_0x44 = physical_height;
	pgl_config.dbgPosCmd_0x48 = 0;
	pgl_config.dbgPosCmd_0x4C = 0;
	pgl_config.unk_0x5C = 2;
	OrbisPglWindow render_window {
		.uID = 0,
		.uWidth = (u32) physical_width, 
		.uHeight = (u32) physical_height
	};
	sceSysmoduleLoadModuleInternal(0x80000010u);
	sceSystemServiceHideSplashScreen();
	initEgl(&render_window);
}

extern "C" int __clock_gettime(clockid_t clk, struct timespec *ts)
{
	return 0;
}

void _platform_throw() {

}