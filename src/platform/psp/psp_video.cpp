#include <video.hpp>
#include <pspvideocodec.h>

struct VideoPlaybackData {
	~VideoPlaybackData() {
	}
};

VideoPlayback::~VideoPlayback() {
	delete (VideoPlaybackData*) data;
}

void jj() {
	sceVideocodecInit();
}
