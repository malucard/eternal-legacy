#pragma once
#include <cstdint>
#include "util.hpp"

#define LARGE_FONT_ATLAS_SIZE 8192
enum {
	G_TOP_LEFT,
	G_TOP_RIGHT,
	G_BOTTOM_LEFT,
	G_BOTTOM_RIGHT,
	G_CENTER_LEFT,
	G_CENTER_RIGHT,
	G_CENTER_TOP,
	G_CENTER_BOTTOM,
	G_CENTER
};

enum {
	INPUT_OK = 1,
	INPUT_BACK = 2,
	INPUT_MENU = 4,
	INPUT_MENU_BACK = INPUT_MENU | INPUT_BACK,
	INPUT_SYS_MENU = 8,
	INPUT_MORE = 16,
	INPUT_HIDE = 32,
	INPUT_AUTO = 64,
	INPUT_SKIP = 128,
	INPUT_SKIP_INSTANT = 256,
	INPUT_UP = 512,
	INPUT_DOWN = 1024,
	INPUT_LEFT = 2048,
	INPUT_RIGHT = 4096,
	INPUT_DIRECTIONAL = INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT,
	INPUT_CLICK = 8192,
	INPUT_SCROLL_UP = 16384,
	INPUT_SCROLL_DOWN = 32768,
	INPUT_SCROLL_LEFT = 65536,
	INPUT_SCROLL_RIGHT = 131072,
	INPUT_QUICK_SAVE = 262144,
	INPUT_QUICK_LOAD = 524288,
	INPUT_REWIND = 1048576
};

enum {
	CTRL_UP, CTRL_DOWN, CTRL_LEFT, CTRL_RIGHT, // dpad
	CTRL_STICK_L_UP, CTRL_STICK_L_DOWN, CTRL_STICK_L_LEFT, CTRL_STICK_L_RIGHT,
	CTRL_STICK_R_UP, CTRL_STICK_R_DOWN, CTRL_STICK_R_LEFT, CTRL_STICK_R_RIGHT,
	CTRL_A, CTRL_B, CTRL_X, CTRL_Y,
	CTRL_SELECT, CTRL_START,
	CTRL_L, CTRL_R, // for systems with 2 shoulder buttons
	CTRL_L1, CTRL_R1, CTRL_L2, CTRL_R2, // for systems with 4 shoulder buttons
	CTRL_L3, CTRL_R3, // analog stick buttons
	CTRL_L4, CTRL_R4, CTRL_L5, CTRL_R5, // extra back buttons
	CTRL_COUNT
};

using u8 = uint8_t;
using i8 = int8_t;
using u16 = uint16_t;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;
using i64 = int64_t;
using usize = size_t;
using isize = ssize_t;
using uint = unsigned int;

constexpr usize ptrsize = sizeof(usize);

#define STRINGIFY(x) #x
#define unimplemented() {log_err("unimplemented at %s:%d", __FILE__, __LINE__); _platform_throw();}

inline u16 bswap16(u16 x) {
	return (x & 0xFF00) >> 8 | (x & 0xFF) << 8;
}

inline u32 bswap32(u32 x) {
	return (x & 0xFF000000) >> 24 | (x & 0xFF0000) >> 8 | (x & 0xFF00) << 16 | (x & 0xFF) << 24;
}

inline u16 le16(u16 x) {
#ifdef IS_BIG_ENDIAN
	return (x & 0xFF00) >> 8 | (x & 0xFF) << 8;
#else
	return x;
#endif
}

inline u32 le32(u32 x) {
#ifdef IS_BIG_ENDIAN
	return (x & 0xFF000000) >> 24 | (x & 0xFF0000) >> 8 | (x & 0xFF00) << 16 | (x & 0xFF) << 24;
#else
	return x;
#endif
}

#ifdef PLATFORM_NO_EXCEPTIONS
void _platform_throw();
#else
#define _platform_throw() throw;
#endif
#define platform_throw() {log_err("error thrown at %s:%d", __FILE__, __LINE__); _platform_throw();}

#define assert(x) {if(!(x)) {log_err("assert failed for '" #x "'"); platform_throw();}}

enum EtResult {
	RES_OK,
	RES_ERR,
	RES_NO
};
