// uses the general OpenGL backend

#ifndef PLATFORM_SWITCH
#define PLATFORM_SWITCH // for IDE
#endif

#include <switch.h>
#include <unistd.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <api.hpp>
#include <input.hpp>
#include <graphics.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <switch/audio/driver.h>
#include "../gl/gl_gfx.hpp"
#include "../gl/gl_header.hpp"

static EGLDisplay egl_display;
static EGLContext egl_context;
static EGLSurface egl_surface;

static bool initEgl(NWindow* win) {
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
		EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR,
		EGL_CONTEXT_MAJOR_VERSION_KHR, 2,
		EGL_CONTEXT_MINOR_VERSION_KHR, 1,
		EGL_NONE
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

static bool running = false;

void a_init();

int main(int argc, char* argv[]) {
	if(!initEgl(nwindowGetDefault()))
		return EXIT_FAILURE;
	gladLoadGL();
	a_init();
	gl_init();
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	PadState pad;
	padInitializeDefault(&pad);
	HidTouchScreenState touch;
	HidTouchScreenState prev_touch;
	hidInitializeTouchScreen();
	nwindowGetDimensions(nwindowGetDefault(), (u32*) &physical_width, (u32*) &physical_height);
	glViewport(0, 0, physical_width, physical_height);
	glScissor(0, 0, physical_width, physical_height);
	running = true;
	api_init(argc, argv);
	api_load();
	std::map<u32, HidTouchState> touches;
	while(appletMainLoop() && running) {
		u32 new_width = physical_width, new_height = physical_height;
		nwindowGetDimensions(nwindowGetDefault(), (u32*) &new_width, (u32*) &new_height);
		if(new_width != physical_width || new_height != physical_height) {
			api_resize(new_width, new_height);
		}
		padUpdate(&pad);
		u32 btns_down = padGetButtonsDown(&pad), btns_up = padGetButtonsUp(&pad);
#define BTNCK(btn, inp) if(btns_down & (btn)) input_map_submit(inp, 1.f, false); else if(btns_up & (btn)) input_map_submit(inp, 0.f, false);
		BTNCK(HidNpadButton_A | HidNpadButton_Palma, CTRL_A);
		BTNCK(HidNpadButton_B, CTRL_B);
		BTNCK(HidNpadButton_Y, CTRL_X);
		BTNCK(HidNpadButton_X, CTRL_Y);
		BTNCK(HidNpadButton_L, CTRL_L1);
		BTNCK(HidNpadButton_R, CTRL_R1);
		BTNCK(HidNpadButton_ZL, CTRL_L2);
		BTNCK(HidNpadButton_ZR, CTRL_R2);
		BTNCK(HidNpadButton_Plus, CTRL_START);
		BTNCK(HidNpadButton_Minus, CTRL_SELECT);
		BTNCK(HidNpadButton_Up, CTRL_UP);
		BTNCK(HidNpadButton_Down, CTRL_DOWN);
		BTNCK(HidNpadButton_Left, CTRL_LEFT);
		BTNCK(HidNpadButton_Right, CTRL_RIGHT);
		BTNCK(HidNpadButton_StickLUp, CTRL_STICK_L_UP);
		BTNCK(HidNpadButton_StickLDown, CTRL_STICK_L_DOWN);
		BTNCK(HidNpadButton_StickLLeft, CTRL_STICK_L_LEFT);
		BTNCK(HidNpadButton_StickLRight, CTRL_STICK_L_RIGHT);
		BTNCK(HidNpadButton_StickRUp | HidNpadButton_LagonCUp, CTRL_STICK_R_UP);
		BTNCK(HidNpadButton_StickRDown | HidNpadButton_LagonCDown, CTRL_STICK_R_DOWN);
		BTNCK(HidNpadButton_StickRLeft | HidNpadButton_LagonCLeft, CTRL_STICK_R_LEFT);
		BTNCK(HidNpadButton_StickRRight | HidNpadButton_LagonCRight, CTRL_STICK_R_RIGHT);
		BTNCK(HidNpadButton_AnySL, CTRL_L);
		BTNCK(HidNpadButton_AnySR, CTRL_R);
		hidGetTouchScreenStates(&touch, 1);
		if(touch.count && !prev_touch.count) {
			api_cursor(touch.touches[0].x, touch.touches[0].y, false);
			api_input(INPUT_OK | INPUT_CLICK, true, false);
		} else if(touch.count) {
			api_cursor(touch.touches[0].x, touch.touches[0].y, true);
		} else if(!touch.count && prev_touch.count) {
			api_input(INPUT_OK | INPUT_CLICK, false, false);
		}
		prev_touch = touch;
		gl_frame();
		eglSwapInterval(egl_display, unlock_fps? 0: 1);
		eglSwapBuffers(egl_display, egl_surface);
	}
	deinitEgl();
	return EXIT_SUCCESS;
}

#define LOGS_SIZE 1024
static char logs[LOGS_SIZE];

void sys_show_logs() {
	PadState pad;
	padInitializeDefault(&pad);
	g_reset_stacks();
	while(appletMainLoop()) {
		padUpdate(&pad);
		u32 btns_down = padGetButtonsDown(&pad), btns_up = padGetButtonsUp(&pad);
		if(btns_down & (HidNpadButton_A | HidNpadButton_B | HidNpadButton_Plus | HidNpadButton_Minus)) return;
		nwindowGetDimensions(nwindowGetDefault(), (u32*) &physical_width, (u32*) &physical_height);
		glViewport(0, 0, physical_width, physical_height);
		glScissor(0, 0, physical_width, physical_height);
		glClearColor(0.f, 0.f, 0.5f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		g_draw_text(logs, 0, 0, -1, 16);
		g_flush();
		eglSwapInterval(egl_display, unlock_fps? 0: 1);
		eglSwapBuffers(egl_display, egl_surface);
	}
}

void sys_exit() {
	running = false;
	exit(0);
}

void psp_push_logs(const char* line) {
	size_t len = strlen(line);
	fprintf(stdout, "%s\n", line);
	// we avoid overwriting the last byte of the buffer because it's a trailing zero
	memmove(logs + len + 1, logs, LOGS_SIZE - len - 2);
	memcpy(logs, line, len);
	logs[len] = '\n';
}

#ifdef _platform_throw
#undef _platform_throw // for IDE
#endif

void _platform_throw() {
	g_reset_stacks();
	while(appletMainLoop()) {
		nwindowGetDimensions(nwindowGetDefault(), (u32*) &physical_width, (u32*) &physical_height);
		glViewport(0, 0, physical_width, physical_height);
		glScissor(0, 0, physical_width, physical_height);
		glClearColor(0.f, 0.f, 0.5f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		g_draw_text(logs, 0, 0, -1, 16);
		g_flush();
		eglSwapInterval(egl_display, unlock_fps? 0: 1);
		eglSwapBuffers(egl_display, egl_surface);
	}
	sys_exit();
}