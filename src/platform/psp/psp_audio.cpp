#include <audio.hpp>
#include <sndfile.hh>
#include <vector>
#include <pspkernel.h>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include <pspthreadman.h>

#define UNCACHED_POINTER 0x40000000
#define BUFFER_SIZE 4096

static int psp_thread(SceSize args, void *argp);

static float master_volume = 1.f;

struct AudioSource {
	float volume = 1.f;
	int real_volume = PSP_AUDIO_VOLUME_MAX;
	int channel = -1;
	SceUID thread;
	bool paused = false;
	bool looping = false;

	virtual ~AudioSource() {
		while(sceKernelTerminateDeleteThread(thread) < 0) {}
		sceAudioChRelease(channel);
	}

	void begin(int rate, bool stereo) {
		real_volume = master_volume * volume * PSP_AUDIO_VOLUME_MAX + 0.5f;
		channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, BUFFER_SIZE, stereo? PSP_AUDIO_FORMAT_STEREO: PSP_AUDIO_FORMAT_MONO);
		if(channel >= 0) {
			thread = sceKernelCreateThread("soloud audio output", psp_thread, 0x12, 0x10000, 0, nullptr);
			if(thread >= 0) {
				void* data = this; // send a pointer to this to the thread
				sceKernelStartThread(thread, sizeof(this), &data);
			} else {
				sceAudioChRelease(channel);
				channel = -1;
			}
		}
	}
	// called by the thread, decoded = 0 if the stream is over
	virtual void* decode(int& decoded) {
		decoded = 0;
		return nullptr;
	}
};

static int psp_thread(SceSize args, void *argp) {
	AudioSource* data = *(AudioSource**) argp;
	int buf_id = 0;
	int last_decoded = BUFFER_SIZE;
	while(true) {
		int decoded;
		void* samples = data->decode(decoded);
		if(decoded) {
			if(decoded != last_decoded) {
				sceAudioSetChannelDataLen(data->channel, decoded);
			}
			sceAudioOutputBlocking(data->channel, data->real_volume, samples);
		} else {
			break;
		}
	}
	sceAudioChRelease(data->channel);
	data->channel = -1;
	return 0;
}

struct FileAudioSource final: AudioSource {
	DataStream* ds;
	SNDFILE* sndfile;
	SF_VIRTUAL_IO io;
	SF_INFO info;
	sf_count_t size;
	short buffer[BUFFER_SIZE];

	FileAudioSource(DataStream* ds, float volume): ds(ds) {
		this->volume = volume;
		ds->seek(0);
		size = ds->remaining();
		io.get_filelen = [](void* data) {
			FileAudioSource* me = (FileAudioSource*) data;
			return me->size;
		};
		io.read = [](void* buf, sf_count_t count, void* data) {
			DataStream* ds = ((FileAudioSource*) data)->ds;
			if(count > ds->remaining()) count = ds->remaining();
			ds->read(buf, count);
			return count;
		};
		io.seek = [](sf_count_t count, int whence, void* data) {
			FileAudioSource* me = (FileAudioSource*) data;
			switch(whence) {
				case SEEK_SET:
					me->ds->seek(count);
					break;
				case SEEK_CUR:
					me->ds->skip(count);
					break;
				case SEEK_END:
					me->ds->seek(me->size + count);
					break;
			}
			return (sf_count_t) me->ds->tell();
		};
		io.tell = [](void* data) {
			return (sf_count_t) ((FileAudioSource*) data)->ds->tell();
		};
		io.write = [](const void* buf, sf_count_t count, void* data) {
			return (sf_count_t) 0;
		};
		info.format = 0;
		if(sndfile = sf_open_virtual(&io, SFM_READ, &info, this)) {
			begin(info.samplerate, info.channels > 1);
		}
	}

	~FileAudioSource() override {
		sf_close(sndfile);
		delete ds;
	}

	void* decode(int& decoded) override {
		decoded = sf_read_short(sndfile, buffer, BUFFER_SIZE);
		while(decoded < BUFFER_SIZE) { // didn't fill the buffer. let's find out why to prevent stuttering
			int decoded2 = sf_read_short(sndfile, buffer + decoded, BUFFER_SIZE - decoded);
			if(decoded2) { // guess it just needed another call
				decoded += decoded2;
			} else if(looping) { // we're over, let's loop
				sf_seek(sndfile, 0, SEEK_SET);
				decoded += sf_read_short(sndfile, buffer + decoded, BUFFER_SIZE - decoded);
			} else break;
		}
		return buffer;
	}
};

struct SamplesAudioSource final: AudioSource {
	void* data;
	sf_count_t cursor = 0;
	sf_count_t size;

	SamplesAudioSource(void* data, sf_count_t size, int sample_rate, int channels, float volume): data(data), size(size) {
		this->volume = volume;
		begin(sample_rate, channels);
	}

	~SamplesAudioSource() override {
		free(data);
	}

	void* decode(int& decoded) override {
		decoded = size - cursor;
		if(decoded > BUFFER_SIZE * 2) decoded = BUFFER_SIZE * 2;
		cursor += decoded;
		return (u8*) data + cursor;
	}
};

//static SoLoud::Soloud soloud;
static AudioSource* sources[PSP_AUDIO_CHANNEL_MAX];

// called often so that the audio source objects can be freed
static void validate_sources() {
	int i = 0;
	while(i < PSP_AUDIO_CHANNEL_MAX) {
		if(!sources[i] || sources[i]->channel >= 0) {
			i++;
		} else {
			delete sources[i];
			sources[i] = nullptr;
			//sources.erase(sources.begin() + i);
		}
	}
}

void a_init() {
	//int port = sceAudioSRCChReserve(4096, 44100, 2);
	//if(port < 0) {
	//	log_err("error in sceAudioSRCChReserve");
	//	platform_throw();
	//}
}

int insert(AudioSource* src) {
	validate_sources();
	if(src->channel >= 0) {
		if(sources[src->channel]) {
			log_err("impossible");
			platform_throw();
		}
		sources[src->channel] = src;
		return src->channel;
	} else {
		delete src;
		return -2;
	}
}

int a_play_mem(void* buf, u32 size, int fmt, bool loop, float volume) {
	validate_sources();
	return -2;
}

int a_play_raw(void* buf, u32 size, int sample_rate, int channels, int bits, bool loop, float volume) {
	if(bits == 32) {
		u16* resample = (u16*) malloc(size / 2);
		for(int i = 0; i < size / 4; i++) {
			resample[i] = ((u32*) buf)[i] >> 16;
		}
		free(buf);
		return insert(new SamplesAudioSource((short*) resample, size * 2, sample_rate, channels, volume));
	} else if(bits == 16) {
		return insert(new SamplesAudioSource((short*) buf, size, sample_rate, channels, volume));
	} else if(bits == 8) {
		u16* resample = (u16*) malloc(size * 2);
		for(int i = 0; i < size; i++) {
			u16 sample = ((u8*) buf)[i];
			sample |= sample << 8;
			resample[i] = sample;
		}
		free(buf);
		return insert(new SamplesAudioSource((short*) resample, size * 2, sample_rate, channels, volume));
	} else {
		log_err("unsupported %d bits per sample", bits);
	}
	return -2;
}

#include <math.h>
#define M_PI		3.14159265358979323846	/* pi */

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
	return insert(new FileAudioSource(new FileDataStream(path), volume));
}

int a_speak(const char* text, float volume) {
	return -2;
}

void a_pause(int ch, bool paused) {
	if(sources[ch]) {
		if(sources[ch]->paused != paused) {
			sources[ch]->paused = paused;
			if(paused) {
				if(sceKernelSuspendThread(sources[ch]->thread) < 0) {
					log_err("impossible2");
					platform_throw();
				}
			} else {
				if(sceKernelResumeThread(sources[ch]->thread) < 0) {
					log_err("impossible3");
					platform_throw();
				}
			}
		}
	}
}

void a_master_volume(float volume) {
	*(float*) (UNCACHED_POINTER | (uintptr_t) &master_volume) = volume;
	for(int i = 0; i < PSP_AUDIO_CHANNEL_MAX; i++) {
		sources[i]->real_volume = clamp((int) (master_volume * sources[i]->volume * PSP_AUDIO_VOLUME_MAX + 0.5f), 0, PSP_AUDIO_VOLUME_MAX);
	}
}

void a_volume(int ch, float volume) {
	if(sources[ch]) {
		sources[ch]->volume = volume;
		sources[ch]->real_volume = clamp((int) (master_volume * sources[ch]->volume * PSP_AUDIO_VOLUME_MAX + 0.5f), 0, PSP_AUDIO_VOLUME_MAX);
	}
}

bool a_playing(int ch) {
	validate_sources();
	return sources[ch] && sources[ch]->channel >= 0;
}

void a_destroy(int ch) {
	validate_sources();
	if(ch == -1) {
		for(int i = 0; i < PSP_AUDIO_CHANNEL_MAX; i++) {
			if(sources[i]) {
				delete sources[i];
				sources[i] = nullptr;
			}
		}
	} else if(ch >= 0) {
		if(sources[ch]) {
			delete sources[ch];
			sources[ch] = nullptr;
		}
	}
}

int a_play_stream(AudioStreamFn fn, void* userdata, int sample_rate, int channels, bool loop, float volume) {
	return -2;
}
