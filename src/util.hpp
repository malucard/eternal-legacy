#pragma once
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdint>

using namespace std::string_literals;

#ifdef PLATFORM_ANDROID
#include <android/log.h>
#define log_warn(x, ...) __android_log_print(ANDROID_LOG_WARN, "warning", x "\n", ##__VA_ARGS__)
#define log_err(x, ...) __android_log_print(ANDROID_LOG_ERROR, "error", x "\n", ##__VA_ARGS__)
#define log_info(x, ...) __android_log_print(ANDROID_LOG_INFO, "info", x "\n", ##__VA_ARGS__)
#elif defined(PLATFORM_PSP) || defined(PLATFORM_SWITCH)
void psp_push_logs(const char* line);
#define log_warn(x, ...) {char* __s = new char[256]; snprintf(__s, 256, "warning: " x, ##__VA_ARGS__); psp_push_logs(__s); delete[] __s;}
#define log_err(x, ...) {char* __s = new char[256]; snprintf(__s, 256, "error: " x, ##__VA_ARGS__); psp_push_logs(__s); delete[] __s;}
#define log_info(x, ...) {char* __s = new char[256]; snprintf(__s, 256, "info: " x, ##__VA_ARGS__); psp_push_logs(__s); delete[] __s;}
#else
#define log_warn(x, ...) printf("warning: " x "\n", ##__VA_ARGS__)
#define log_err(x, ...) printf("error: " x "\n", ##__VA_ARGS__)
#define log_info(x, ...) printf("info: " x "\n", ##__VA_ARGS__)
#endif

struct SemVer {
	int major = 1;
	int minor = 0;
	int patch = 0;
};

bool starts_with(const std::string& s, const std::string& o, int at = 0);
bool starts_with(const std::u32string& s, const std::u32string& o, int at = 0);
bool ends_with(const std::string& s, const std::string& o, int at = 0);

inline int sign(int x) {
	return x > 0? 1: x < 0? -1: 0;
}

template<typename T>
inline T clamp(T x, T min, T max) {
	return x > max? max: x < min? min: x;
}

inline int align_up(int v, int alignment) {
	int aligned_down = v & ~(alignment - 1);
	return v == aligned_down? v: (aligned_down + alignment);
}

int is_pot(int v);
int to_pot(int v);

std::u32string strcvt(std::string s);
std::string ustr_to_str(std::u32string s);
uint32_t sys_millis();
float sys_secs();

template<typename T> // type must have an "rc" field
struct Rc {
	T* data = nullptr;

	Rc(): data(nullptr) {}

	Rc(T* data): data(data) {
		if(data) data->rc++;
	}

	Rc(const Rc<T>& o) {
		data = o.data;
		if(data) data->rc++;
	}

	Rc(Rc<T>&& o) {
		data = o.data;
		o.data = nullptr;
	}

	static Rc<T> from_raw(T* data) {
		Rc rc;
		rc.data = data;
		return rc;
	}

	void operator=(const Rc<T>& o) {
		if(data) destroy();
		data = o.data;
		if(data) data->rc++;
	}

	void operator=(Rc<T>&& o) {
		if(data) destroy();
		data = o.data;
		o.data = nullptr;
	}

	operator bool() {
		return data;
	}

	T* operator->() {
		return data;
	}

	const T* operator->() const {
		return data;
	}

	void destroy() {
		if(data) {
			if(data->rc <= 1) {
				delete data;
			} else {
				data->rc--;
			}
			data = nullptr;
		}
	}

	~Rc() {
		destroy();
	}
};

#ifdef PLATFORM_PSP
void memcpy_vfpu(void* dst, const void* src, size_t size);
#ifdef memcpy
#undef memcpy
#endif
#define memcpy memcpy_vfpu
#endif
