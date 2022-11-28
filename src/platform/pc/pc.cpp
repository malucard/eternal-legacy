

extern "C" {
#include <sys/cdefs.h>
#if defined(__aarch64__)
#define _JBLEN 32
#elif defined(__arm__)
#define _JBLEN 64
#elif defined(__i386__)
#define _JBLEN 10
#elif defined(__x86_64__)
#define _JBLEN 11
#endif
typedef long sigjmp_buf[_JBLEN + 1];
typedef long jmp_buf[_JBLEN];
//int setjmp(jmp_buf __env) __returns_twice;
int setjmp(jmp_buf __env);
int _setjmp(jmp_buf __env) {
	return setjmp(__env);
}
}

// backend for Linux and Windows
// uses the general OpenGL backend

#include <util.hpp>
#include <cstdlib>
#include <utility>
#include <vector>
#include <map>
#include <api.hpp>
#include <graphics.hpp>
#include <glm/gtc/type_ptr.hpp>
#ifdef USE_GL1
#include "../gl1/gl1_gfx.hpp"
#include "../gl1/gl1_header.hpp"
#else
#include "../gl/gl_gfx.hpp"
#include "../gl/gl_header.hpp"
#endif
#include <GLFW/glfw3.h>
#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <shlobj.h>
#endif
#include <eui/eui.hpp>
#include <mutex>

GLFWwindow* win = nullptr;

void a_init();
void store_image(u32* pix, int width, int height);

// used when going back from fullscreen to windowed
static int prev_x = 128, prev_y = 128, prev_w = 800, prev_h = 600;
static bool running = false;
const char* ROOT_DIR = "";
std::mutex sync_lock;

int main(int argc, char* argv[]) {
	a_init();
	if(!glfwInit()) {
		log_err("could not init GLFW\n");
		return 1;
	}
#ifdef PLATFORM_LINUX
	char* home_parent = getenv("XDG_DATA_HOME");
	std::string home = home_parent? home_parent: "~/.local/share";
#elif defined(PLATFORM_WINDOWS)
	char* home_parent = getenv("APPDATA");
	std::string home;
	if(home_parent) {
		home = home_parent;
	} else {
		char appdata[256];
		if(!SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, appdata))) {
			log_err("could not find appdata");
			exit(1);
		}
		home = appdata;
	}
#endif
	home += "/eternal/";
	ROOT_DIR = home.c_str();
	api_init(argc, argv);
#ifdef USE_GLES2
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#elif defined(USE_GLES3)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_SAMPLES, 4);
#elif defined(USE_GL2)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
#elif defined(USE_GL1)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
#endif
	glfwSwapInterval(1);
	physical_width = 1280;
	physical_height = 720;
	update_screen_size();
	win = glfwCreateWindow(1280, 720, "Eternal", nullptr, nullptr);
	printf("using api: %s %u.%u\n",
		glfwGetWindowAttrib(win, GLFW_CLIENT_API) == GLFW_OPENGL_ES_API? "opengl es": "opengl",
		glfwGetWindowAttrib(win, GLFW_CONTEXT_VERSION_MAJOR), glfwGetWindowAttrib(win, GLFW_CONTEXT_VERSION_MINOR));
	glfwMakeContextCurrent(win);
#ifdef USE_GL2
	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK) {
		log_err("could not init GLEW, driver issue?\n");
		return 1;
	}
	if(!__GLEW_VERSION_2_0) {
		log_err("Eternal requires OpenGL 2.0, but it is not supported on this device\n");
		return 1;
	}
#endif
	gl_init();
	api_load();
	glfwSetWindowCloseCallback(win, [](auto _) {
		sync_lock.lock();
		api_close();
		glfwDestroyWindow(win);
		exit(0);
		sync_lock.unlock();
	});
	glfwSetWindowSizeCallback(win, [](auto _, auto w, auto h) {
		sync_lock.lock();
		api_resize(w, h);
		sync_lock.unlock();
	});
	static Rc<EuiLayout> lay = Rc(new EuiLayout);
	glfwSetCursorPosCallback(win, [](auto win, auto x, auto y) {
		sync_lock.lock();
		glfwGetCursorPos(win, &x, &y);
		api_cursor(x, y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
		sync_lock.unlock();
	});
	glfwSetKeyCallback(win, [](auto _, auto key, auto scancode, auto action, auto mods) {
		sync_lock.lock();
		switch(key) {
			case GLFW_KEY_LEFT: case GLFW_KEY_A:
				api_input(INPUT_LEFT, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_KEY_RIGHT: case GLFW_KEY_D:
				api_input(INPUT_RIGHT, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_KEY_UP: case GLFW_KEY_W:
				api_input(INPUT_UP, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_KEY_DOWN: case GLFW_KEY_S:
				api_input(INPUT_DOWN, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_KEY_B:
				api_input(INPUT_MORE, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_KEY_ENTER: case GLFW_KEY_SPACE:
				api_input(INPUT_OK, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_KEY_ESCAPE:
				api_input(INPUT_MENU_BACK, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_KEY_BACKSPACE:
				api_input(INPUT_BACK, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_KEY_LEFT_CONTROL:
				api_input(INPUT_SKIP, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_KEY_K:
				api_input(INPUT_SKIP_INSTANT, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_KEY_DELETE:
				api_input(INPUT_SYS_MENU, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_KEY_F1:
				api_input(INPUT_HIDE, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_KEY_F2:
#if defined(USE_GL2) || defined(USE_GLES3)
				if(action == GLFW_PRESS
#ifdef USE_GL2
				&& GLEW_EXT_framebuffer_object
#endif
					) {
					glGenFramebuffersEXT(1, &default_framebuffer);
					glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, default_framebuffer);
					GLuint scfbtex;
					glGenTextures(1, &scfbtex);
					glBindTexture(GL_TEXTURE_2D, scfbtex);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, physical_width, physical_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, scfbtex, 0);
					GLenum db[] = {GL_COLOR_ATTACHMENT0_EXT};
					glDrawBuffers(1, db);
					int status;
					if((status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)) != GL_FRAMEBUFFER_COMPLETE_EXT) {
						log_err("error %d with framebuffer; disabling upscaling", status);
						upscaling_enabled = false;
					}
					glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, default_framebuffer);
					glScissor(0, 0, physical_width, physical_height);
					glClearColor(0.f, 0.f, 0.f, 1.f);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					shader_bicubic.use();
					g_proj_reset();
					api_frame();
					g_reset_stacks();
					u32* pixels = new u32[physical_width * physical_height];
					glBindTexture(GL_TEXTURE_2D, scfbtex);
					glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
					glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
					glDeleteFramebuffersEXT(1, &default_framebuffer);
					glDeleteTextures(1, &scfbtex);
					default_framebuffer = 0;
					store_image(pixels, physical_width, physical_height);
					delete[] pixels;
				}
				break;
#endif
			case GLFW_KEY_F11:
				if(action == GLFW_PRESS) {
					if(glfwGetWindowMonitor(win)) {
						glfwSetWindowMonitor(win, nullptr, prev_x, prev_y, prev_w, prev_h, GLFW_DONT_CARE);
					} else {
						glfwGetWindowPos(win, &prev_x, &prev_y);
						glfwGetWindowSize(win, &prev_w, &prev_h);
						GLFWmonitor* monitor = glfwGetPrimaryMonitor();
						const GLFWvidmode* mode = glfwGetVideoMode(monitor);
						glfwSetWindowMonitor(win, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
					}
				}
				break;
			// resolution shortcuts for debugging
			case GLFW_KEY_F10: glfwSetWindowSize(win, 2880, 2160); break;
			case GLFW_KEY_F9: glfwSetWindowSize(win, 1440, 1080); break;
			case GLFW_KEY_F8: glfwSetWindowSize(win, 1280, 720); break;
			case GLFW_KEY_F7: glfwSetWindowSize(win, 800, 600); break;
			case GLFW_KEY_F6: glfwSetWindowSize(win, 640, 480); break;
			case GLFW_KEY_F5: glfwSetWindowSize(win, 700, 300); break;
			case GLFW_KEY_F4: glfwSetWindowSize(win, 480, 272); break;
			case GLFW_KEY_F3: glfwSetWindowSize(win, 400, 240); break;
		}
		sync_lock.unlock();
	});
	glfwSetScrollCallback(win, [](auto _, double x, double y) {
		sync_lock.lock();
		if(x < 0.f) {
			for(; x < 0.f; x++) {
				api_input(INPUT_SCROLL_LEFT, 1.f, false);
			}
		} else {
			for(; x > 0.f; x--) {
				api_input(INPUT_SCROLL_RIGHT, 1.f, false);
			}
		}
		if(y < 0.f) {
			for(; y < 0.f; y++) {
				api_input(INPUT_SCROLL_DOWN, 1.f, false);
			}
		} else {
			for(; y > 0.f; y--) {
				api_input(INPUT_SCROLL_UP, 1.f, false);
			}
		}
		sync_lock.unlock();
	});
	glfwSetMouseButtonCallback(win, [](auto _, auto button, auto action, auto mods) {
		sync_lock.lock();
		switch(button) {
			case GLFW_MOUSE_BUTTON_LEFT:
				api_input(INPUT_OK | INPUT_CLICK, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				api_input(INPUT_MENU, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
			case GLFW_MOUSE_BUTTON_MIDDLE:
				api_input(INPUT_MORE, action != GLFW_RELEASE, action == GLFW_REPEAT);
				break;
		}
		sync_lock.unlock();
	});
#ifdef USE_GL2
	//glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // uncomment to make breakpoints inside the callback work
	if(GLEW_ARB_debug_output) {
		glDebugMessageCallbackARB([](GLenum source, GLenum type, GLuint id, GLenum severity,
			GLsizei length, const GLchar *message, const void *userParam) {
			const char* _source;
			const char* _type;
			const char* _severity;
			switch(source) {
				case GL_DEBUG_SOURCE_API: _source = "api"; break;
				case GL_DEBUG_SOURCE_WINDOW_SYSTEM: _source = "window system"; break;
				case GL_DEBUG_SOURCE_SHADER_COMPILER: _source = "shader compiler"; break;
				case GL_DEBUG_SOURCE_THIRD_PARTY: _source = "third party"; break;
				case GL_DEBUG_SOURCE_APPLICATION: _source = "application"; break;
				case GL_DEBUG_SOURCE_OTHER: _source = "other"; break;
				default: _source = "unknown"; break;
			}
			switch(type) {
				case GL_DEBUG_TYPE_ERROR:_type = "error";break;
				case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:_type = "deprecated behavior";break;
				case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: _type = "undefined behavior"; break;
				case GL_DEBUG_TYPE_PORTABILITY: _type = "portability issue"; break;
				case GL_DEBUG_TYPE_PERFORMANCE: _type = "performance issue"; break;
				case GL_DEBUG_TYPE_OTHER: _type = "other issue"; break;
				case GL_DEBUG_TYPE_MARKER: _type = "marker"; break;
				default: _type = "unknown issue"; break;
			}
			switch(severity) {
				case GL_DEBUG_SEVERITY_HIGH: _severity = "severe"; break;	
				case GL_DEBUG_SEVERITY_MEDIUM: _severity = "mild"; break;	
				case GL_DEBUG_SEVERITY_LOW: _severity = "minor"; break;
				case GL_DEBUG_SEVERITY_NOTIFICATION: return; _severity = "notification"; break;
				default: _severity = "unknown severity"; break;
			}
			log_warn("GL %s %s from %s: 0x%X %s", _severity, _type, _source, id, message);
		}, nullptr);
	}
#endif
	running = true;
	//lay->add_child(EuiElementRef(new EuiLabel("System Menu")));
	//lay->add_child(EuiElementRef(new EuiButton("Return", [](EuiButton& b) {log_info("ojojo2");})));
	//lay->add_child(EuiElementRef(new EuiButton("Mods", [](EuiButton& b) {log_info("ojojo2");})));
	//lay->add_child(EuiElementRef(new EuiLabel("Running Launcher", "game_name")));
	//lay->add_child(EuiElementRef(new EuiLabel("61 fps", "fps_counter")));
	//lay->add_child(EuiElementRef(new EuiSeparator));
	//lay->add_child(EuiElementRef(new EuiToggle("VSync", [](EuiToggle& t, bool val) {log_info("ojojo2");}, "", !unlock_fps)));
	//lay->add_child(EuiElementRef(new EuiToggle("Test KO1", [](EuiToggle& t, bool val) {log_info("ojojo2");})));
	//lay->add_child(EuiElementRef(new EuiButton("Packages", [](EuiButton& b) {log_info("ojojo2");})));
	//lay->add_child(EuiElementRef(new EuiButton("Show logs", [](EuiButton& b) {log_info("ojojo2");})));
	//lay->add_child(EuiElementRef(new EuiButton("Exit to launcher", [](EuiButton& b) {log_info("ojojo2");})));
	//lay->add_child(EuiElementRef(new EuiButton("Exit", [](EuiButton& b) {log_info("ojojo2");})));
	//lay->get_child<EuiLabel>("fps_counter")->text = "69 fps";
	while(running) {
		glfwPollEvents();
		sync_lock.lock();
		gl_frame();
		//lay->draw(200, 0, 420.f * g_height() / 600, g_height());
		g_reset_stacks();
		g_flush();
		glfwSwapInterval(unlock_fps? 0: 1);
		glfwSwapBuffers(win);
		sync_lock.unlock();
	}
	return 0;
}

void sys_show_logs() {
}

void sys_exit() {
	running = false;
	exit(0);
}

u32 sys_millis() {
	return glfwGetTime() * 1000;
}

float sys_secs() {
	return glfwGetTime();
}