#pragma once
#include <fs.hpp>

enum {
	A_FMT_UNKNOWN,
	A_FMT_OGG,
	A_FMT_WAV,
	A_FMT_ADX,
	A_FMT_AHX,
	A_FMT_S16
};

//int a_play(DataStream&& data, int fmt, bool loop);
// play file from filesystem
int a_play(const char* path, int fmt, bool loop, float volume = 1.f);
// play file from memory
int a_play_mem(void* buf, u32 size, int fmt, bool loop, float volume = 1.f);
// play raw sample data
int a_play_raw(void* buf, u32 size, int sample_rate, int channels, int bits, bool loop, float volume = 1.f);
typedef EtResult (*AudioStreamFn)(float* buf, unsigned int& samples_to_read, void* userdata);
// play raw sample data streamed from elsewhere
int a_play_stream(AudioStreamFn fn, void* userdata, int sample_rate, int channels, bool loop, float volume = 1.f);
int a_speak(const char* text, float volume);
void a_pause(int ch);
void a_master_volume(float volume);
void a_volume(int ch, float volume);
bool a_playing(int ch);
// -1 to destroy all
void a_destroy(int ch);
