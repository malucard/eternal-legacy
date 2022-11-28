#include <audio.hpp>
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>
#include <soloud_speech.h>
#include <soloud_file.h>
#include <mpg123.h>
#include <vector>
#include <optional>

static SoLoud::Soloud soloud;
static std::vector<std::pair<SoLoud::AudioSource*, int>> sources;

// called often so that the audio source objects can be freed
static void validate_sources() {
	int i = 0;
	while(i < sources.size()) {
		if(soloud.isValidVoiceHandle(sources[i].second)) {
			i++;
		} else {
			delete sources[i].first;
			sources.erase(sources.begin() + i);
		}
	}
}

void a_init() {
	soloud.init();
	//mpg123_init();
}

int a_play_mem(void* buf, u32 size, int fmt, bool loop, float volume) {
	validate_sources();
	SoLoud::Wav* wav = new SoLoud::Wav();
	wav->loadMem((u8*) buf, size);
	wav->setLooping(loop);
	wav->setVolume(volume);
	int h = soloud.play(*wav);
	sources.push_back(std::make_pair(wav, h));
	return h;
}

int a_play_raw(void* buf, u32 size, int sample_rate, int channels, int bits, bool loop, float volume) {
	validate_sources();
	SoLoud::Wav* wav = new SoLoud::Wav();
	if(bits == 32) {
		u16* resample = new u16[size / 4];
		for(int i = 0; i < size / 4; i++) {
			u32 sample = ((u32*) buf)[i];
			resample[i] = sample >> 16;
		}
		wav->loadRawWave16((short*) resample, size / 2, sample_rate, channels);
		delete[] resample;
	} else if(bits == 16) {
		wav->loadRawWave16((short*) buf, size, sample_rate, channels);
	} else if(bits == 8) {
		wav->loadRawWave8((u8*) buf, size, sample_rate, channels);
	} else {
		log_err("unsupported %d bits per sample", bits);
	}
	wav->setLooping(loop);
	wav->setVolume(volume);
	int h = soloud.play(*wav);
	sources.push_back(std::make_pair(wav, h));
	return h;
}

#include <cmath>

#define U32(addr) (*(u32*) (addr))
#define U16(addr) (*(u16*) (addr))
#define U8(addr) (*(u8*) (addr))

u8* load_adx(u8* bytes, u32 size, u32* outsize) {
	u32 sig = U32(bytes);
	if((sig & 0xFFFF) != 0x80) {
		printf("%X\n", sig);
		log_err("invalid ADX file (bad signature)");
		platform_throw();
	}
	u16 header_size = bswap16(sig >> 16);
	if(header_size < 0x10 || header_size >= size) {
		log_err("invalid ADX file (bad header size)");
		platform_throw();
	}
	if(strncmp((char*) bytes + header_size - 2, "(c)CRI", 6)) {
		log_err("invalid ADX file (bad header end)");
		platform_throw();
	}
	u8 frame_size = U8(bytes + 5);
	u8 bytes_per_sample = U8(bytes + 6);
	u32 samples_per_frame = ((u32) frame_size - 2) * 8 / bytes_per_sample;
	u8 channels = U8(bytes + 7);
	u16 encoding = bswap16(U16(bytes + 15));
	if(encoding != 0x0400) {
		log_err("unsupported ADX encoding");
		platform_throw();
	}
	u8 samples_per_second = U32(bytes + 8);
	u32 sample_count = bswap32(U32(bytes + 12));
	int lowest_freq = bswap16(U16(bytes + 16));
	float sqrt2 = std::sqrt(2.0);
	float x = sqrt2 - std::cos(2.0 * M_PI * lowest_freq / samples_per_second);
	float y = sqrt2 - 1.0;
	float z = (x - std::sqrt((x + y) * (x - y))) / y;
	int m_prev_scale0 = std::floor(z * 8192);
	int m_prev_scale1 = std::floor(z * z * -4096);
	int prev_samples[4];
	u32 pos = 4 + header_size;
	u32 scale = (u32) U16(bytes + pos) + 1;
	pos += 2;
	u8* out = (u8*) malloc(sample_count * 4);
	u32 out_pos = 0;
	for(int i = 0; i < samples_per_frame; ++i) {
		u32 sample = U32(bytes + pos);
		u16 l = sample;
		l = (l & 7) - (l & 8);
		int adjust = (m_prev_scale0 * prev_samples[0] + m_prev_scale1 * prev_samples[1]) >> 12;
		l = l * scale + adjust;
		U16(out + out_pos++) = l;
		prev_samples[1] = prev_samples[0];
		prev_samples[0] = l;
		u16 r = sample >> 16;
		r = (r & 7) - (r & 8);
		adjust = (m_prev_scale0 * prev_samples[2] + m_prev_scale1 * prev_samples[3]) >> 12;
		r = r * scale + adjust;
		U16(out + out_pos++) = r;
		prev_samples[3] = prev_samples[0];
		prev_samples[2] = r;
		pos += 4;
	}
	*outsize = sample_count * 4;
	return out;
}

u8* load_ahx(u8* bytes, u32 size, u32* outsize) {
	u32 sig = U32(bytes);
	if((sig & 0xFFFF) != 0x80) {
		printf("%X\n", sig);
		log_err("invalid AHX file (bad signature)");
		platform_throw();
	}
	u16 header_size = bswap16(sig >> 16);
	if(header_size < 0x10 || header_size >= size) {
		log_err("invalid AHX file (bad header size)");
		platform_throw();
	}
	if(strncmp((char*) bytes + header_size - 2, "(c)CRI", 6)) {
		log_err("invalid AHX file (bad header end)");
		platform_throw();
	}
	u8 frame_size = U8(bytes + 5);
	u8 bytes_per_sample = U8(bytes + 6);
	u32 samples_per_frame = ((u32) frame_size - 2) * 8 / bytes_per_sample;
	u8 channels = U8(bytes + 7);
	u16 encoding = U8(bytes + 4);
	if(encoding != 0x10 && encoding != 0x11) {
		log_err("unsupported AHX encoding");
		platform_throw();
	}
	u32 samples_per_second = bswap32(U32(bytes + 8));
	u32 sample_count = bswap32(U32(bytes + 0xC));
	u32 start_offset = bswap16(U16(bytes + 2)) + 4;
	int lowest_freq = bswap16(U16(bytes + 16));
	/*mpg123_handle* mpg = mpg123_new(nullptr, nullptr);
	mpg123_param(mpg, MPG123_REMOVE_FLAGS, MPG123_GAPLESS, 0.0);
	mpg123_open_feed(mpg);*/
	float sqrt2 = std::sqrt(2.0);
	float x = sqrt2 - std::cos(2.0 * M_PI * lowest_freq / samples_per_second);
	float y = sqrt2 - 1.0;
	float z = (x - std::sqrt((x + y) * (x - y))) / y;
	int m_prev_scale0 = std::floor(z * 8192);
	int m_prev_scale1 = std::floor(z * z * -4096);
	int prev_samples[4];
	u32 pos = 4 + header_size;
	u32 scale = (u32) U16(bytes + pos) + 1;
	pos += 2;
	u8* out = (u8*) malloc(sample_count * 4);
	u32 out_pos = 0;
	for(int i = 0; i < samples_per_frame; ++i) {
		u32 sample = U32(bytes + pos);
		u16 l = sample;
		l = (l & 7) - (l & 8);
		int adjust = (m_prev_scale0 * prev_samples[0] + m_prev_scale1 * prev_samples[1]) >> 12;
		l = l * scale + adjust;
		U16(out + out_pos++) = l;
		prev_samples[1] = prev_samples[0];
		prev_samples[0] = l;
		u16 r = sample >> 16;
		r = (r & 7) - (r & 8);
		adjust = (m_prev_scale0 * prev_samples[2] + m_prev_scale1 * prev_samples[3]) >> 12;
		r = r * scale + adjust;
		U16(out + out_pos++) = r;
		prev_samples[3] = prev_samples[0];
		prev_samples[2] = r;
		pos += 4;
	}
	*outsize = sample_count * 4;
	return out;
}

/*struct AhxSoundSourceInstance: SoLoud::AudioSourceInstance {
	unsigned int getAudio(float *aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize) override {
		
	}
};*/

int a_play(const char* path, int fmt, bool loop, float volume) {
	validate_sources();
	if(fmt == A_FMT_UNKNOWN) {
		const char* ext = path + strlen(path) - 4;
		if(!strcmp(ext, ".adx")) {
			fmt = A_FMT_ADX;
		} else if(!strcmp(ext, ".ahx")) {
			fmt = A_FMT_AHX;
		} else if(!strcmp(ext, ".wav")) {
			fmt = A_FMT_WAV;
		}
		fmt = A_FMT_OGG;
	}
	//soloud.play()
	//if(fmt == A_FMT_ADX) {
		//data = calloc(1, sizeof(mpeg_codec_data));
	//}
	SoLoud::WavStream* wav = new SoLoud::WavStream();
	std::string real_path = FileDataStream::to_real_path(path);
	log_info("a_play %s %d", real_path.c_str(), FileDataStream::exists(real_path.c_str(), true));
	wav->load(real_path.c_str());
	//wav->load(path);
	wav->setLooping(loop);
	wav->setVolume(volume);
	int h = soloud.play(*wav);
	//log_info("h %d %llu", h, wav);
	sources.push_back(std::make_pair(wav, h));
	return h;
}

int a_speak(const char* text, float volume) {
	SoLoud::Speech* sp = new SoLoud::Speech();
	sp->setText(text);
	sp->setVolume(volume);
	int h = soloud.play(*sp);
	sources.push_back(std::make_pair(sp, h));
	return h;
}

void a_pause(int ch, bool paused) {
	validate_sources();
	soloud.setPause(ch, paused);
}

void a_master_volume(float volume) {
	soloud.setGlobalVolume(volume);
}

void a_volume(int ch, float volume) {
	validate_sources();
	soloud.setVolume(ch, volume);
}

bool a_playing(int ch) {
	return soloud.isValidVoiceHandle(ch);
}

void a_destroy(int ch) {
	if(ch == -1) {
		while(sources.size()) {
			int i = sources.size() - 1;
			soloud.stop(sources[i].second);
			delete sources[i].first;
			sources.erase(sources.begin() + i);
		}
		validate_sources();
	} else {
		soloud.stop(ch);
		validate_sources();
	}
}

struct StreamSourceInstance final: SoLoud::AudioSourceInstance {
	AudioStreamFn fn;
	void* userdata;

	StreamSourceInstance(AudioStreamFn fn, void* userdata): fn(fn), userdata(userdata) {}
	~StreamSourceInstance() override {

	}

	unsigned int getAudio(float *aBuffer, unsigned int aSamplesToRead, unsigned int aBufferSize) override {
		EtResult res = fn(aBuffer, aSamplesToRead, userdata);
		u32 count = res == RES_OK? aSamplesToRead: (fn = nullptr, 0);
		if(count < aSamplesToRead) {
			memset(aBuffer + count, 0, aSamplesToRead - count);
		}
		return count;
	}

	bool hasEnded() override {
		return fn == nullptr;
	}

	SoLoud::result rewind() override {return SoLoud::SO_NO_ERROR;}
	float getInfo(unsigned int aInfoKey) override {return 0.f;}
};

struct StreamSource final: SoLoud::AudioSource {
	AudioStreamFn fn;
	void* userdata;

	StreamSource(AudioStreamFn fn, void* userdata, int sample_rate, int channels, bool loop, float volume):
		fn(fn), userdata(userdata) {
		mBaseSamplerate = sample_rate;
		mChannels = channels;
		mVolume = volume;
	}

	~StreamSource() override {

	}

	SoLoud::AudioSourceInstance* createInstance() override {
		StreamSourceInstance* inst = new StreamSourceInstance(fn, userdata);
		inst->mBaseSamplerate = mBaseSamplerate;
		inst->mChannels = mChannels;
		inst->mSetVolume = mVolume;
		return inst;
	}
};

int a_play_stream(AudioStreamFn fn, void* userdata, int sample_rate, int channels, bool loop, float volume) {
	StreamSource* s = new StreamSource(fn, userdata, sample_rate, channels, loop, volume);
	int h = soloud.play(*s);
	sources.push_back(std::make_pair(s, h));
	return h;
}
