//#include <SDL2/SDL_audio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

void a_init() {
	SDL_Init(SDL_INIT_AUDIO);
	Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3 | MIX_INIT_FLAC | MIX_INIT_OPUS | MIX_INIT_MID | MIX_INIT_MOD);
	if(Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 4096)) {
		log_err("could not init audio, error in Mix_OpenAudio: %s\n", Mix_GetError());
		platform_throw();
	}
}

int a_play_mem(void* buf, u32 size, int fmt, bool loop, float volume) {
	validate_sources();
	Mix_Chunk* wav = Mix_LoadWAV_RW(SDL_RWFromConstMem(buf, size), true);
	int h = Mix_PlayChannel(-1, wav, loop? -1: 0);
	Mix_VolumeChunk(wav, Mix_linear volume)
	wav->setVolume(volume);
	int h = soloud.play(*wav);
	sources.push_back(std::make_pair(wav, h));
	return h;
}