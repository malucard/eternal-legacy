#include <util.hpp>
#include <cstdlib>
#include <utility>
#include <vector>
#include <map>
#include <chrono>
#include <jni.h>
#include <graphics.hpp>
#include <api.hpp>
#include "../gl/gl_gfx.hpp"
#include "../gl/gl_header.hpp"

static int max_aspect_ratio_w = 7, max_aspect_ratio_h = 3;
static int min_aspect_ratio_w = 4, min_aspect_ratio_h = 3;

void a_init();

char* ROOT_DIR;
static u32 last_fps_check;

extern "C" JNIEXPORT void JNICALL Java_com_malucart_eternalvn_Renderer_init(JNIEnv* env, jobject jthis, jstring rootdir) {
	const char* rootdirc = env->GetStringUTFChars(rootdir, nullptr);
	u32 rootdirlen = strlen(rootdirc);
	ROOT_DIR = (char*) malloc(rootdirlen + sizeof("/eternal/"));
	memcpy(ROOT_DIR, rootdirc, rootdirlen);
	memcpy(ROOT_DIR + rootdirlen, "/eternal/", sizeof("/eternal/"));
	env->ReleaseStringUTFChars(rootdir, rootdirc);
	log_info("%s", ROOT_DIR);
	a_init();
	gl_init();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	api_init(0, nullptr);
	api_load();
	/*while(true) {
		//glfwPollEvents();
		gl_frame();
		g_reset_stacks();
		g_flush();
		//glfwSwapBuffers(win);
	}*/
}

extern "C" JNIEXPORT void JNICALL Java_com_malucart_eternalvn_Renderer_frame(JNIEnv* jnienv, jobject jnithis) {
	gl_frame();
	g_reset_stacks();
	g_flush();
}

extern "C" JNIEXPORT void JNICALL Java_com_malucart_eternalvn_Renderer_resize(JNIEnv* jnienv, jobject jnithis, jint w, jint h) {
	physical_width = w;
	physical_height = h;
	glViewport(0, 0, w, h);
	//glBindTexture(GL_TEXTURE_2D, fbtex);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

void sys_show_logs() {}
void sys_exit() {}