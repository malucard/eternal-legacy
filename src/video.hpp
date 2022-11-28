#pragma once
#include <string>

// all functions in platform
// platforms use ffmpeg when possible; otherwise they use theora and vorbis to support just ogv
struct VideoPlayback {
	void* data;

	VideoPlayback(const char* path);
	~VideoPlayback();
	void get_dimensions(int* w, int* h);
	void decode_video();
	void draw(float x, float y, float w, float h);
	
	void decode_audio();
};