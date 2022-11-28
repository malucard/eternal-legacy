#include <audio.hpp>
#include <vector>
#include <optional>

void a_init() {}

int a_play(const char* path, int fmt, bool loop, float volume) {
	return 0;
}

int a_play_mem(void* buf, u32 size, int fmt, bool loop, float volume) {
	return 0;
}

int a_play_raw(void* buf, u32 size, int sample_rate, int channels, int bits, bool loop, float volume) {
	return 0;
}

int a_play_stream(AudioStreamFn fn, void* userdata, int sample_rate, int channels, bool loop, float volume) {
	return 0;
}

int a_speak(const char* text, float volume) {
	return 0;
}

void a_pause(int ch, bool paused) {}

void a_master_volume(float volume) {}

void a_volume(int ch, float volume) {}

bool a_playing(int ch) {
	return false;
}

void a_destroy(int ch) {}
