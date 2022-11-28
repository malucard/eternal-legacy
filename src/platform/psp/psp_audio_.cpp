#include <audio.hpp>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <pspaudio.h>
#include <pspkernel.h>
#include <vector>
#include <optional>
#include <cmath>

#define ERRCK(cond, msg) if(cond) {log_err(msg); platform_throw(); return;}
#define SAMPLES_PER_CALL 1024

struct Stream {
	virtual ~Stream() {}
	virtual void update() = 0;
};

struct OggStream final: Stream {
	FileDataStream fds;
	ogg_sync_state sync_state;
	ogg_stream_state stream_state;
	ogg_page page;
	ogg_packet packet;
	vorbis_info vi;
	vorbis_comment vc;
	vorbis_dsp_state vd;
	vorbis_block vb;
	int channel;
	int convsize;
	bool eos = false;
	ogg_int16_t convbuffer[4096];

	OggStream(FileDataStream fds): fds(fds), eos(!fds.remaining()) {}
	void update() {
		if(sceAudioGetChannelRestLen(channel) != 0) {
			return;
		}
		while(!eos) {
		while(!eos) {
		  int result = ogg_sync_pageout(&sync_state, &page);
		  if(result == 0) break; /* need more data */
		  if(result < 0) { /* missing or corrupt data at this page position */
			log_err("Corrupt or missing data in bitstream; "
					"continuing...\n");
		  } else {
			ogg_stream_pagein(&stream_state,&page); /* can safely ignore errors at
										   this point */
			while(1){
			  result=ogg_stream_packetout(&stream_state,&packet);

			  if(result==0)break; /* need more data */
			  if(result<0){ /* missing or corrupt data at this page position */
				/* no reason to complain; already complained above */
			  }else{
				/* we have a packet.  Decode it */
				float **pcm;
				int samples;

				if(vorbis_synthesis(&vb,&packet)==0) /* test for success! */
				  vorbis_synthesis_blockin(&vd,&vb);
				/*
				**pcm is a multichannel float vector.  In stereo, for
				example, pcm[0] is left, and pcm[1] is right.  samples is
				the size of each channel.  Convert the float values
				(-1.<=range<=1.) to whatever PCM format and write it out */

				while((samples=vorbis_synthesis_pcmout(&vd,&pcm))>0){
				  int j;
				  int clipflag=0;
				  int bout=(samples<convsize?samples:convsize);

				  /* convert floats to 16 bit signed ints (host order) and
					 interleave */
				  for(int i=0;i<vi.channels;i++){
					ogg_int16_t *ptr=convbuffer+i;
					float  *mono=pcm[i];
					for(j=0;j<bout;j++){
#if 1
					  int val=std::floor(mono[j]*32767.f+.5f);
#else /* optional dither */
					  int val=mono[j]*32767.f+drand48()-0.5f;
#endif
					  /* might as well guard against clipping */
					  if(val>32767){
						val=32767;
						clipflag=1;
					  }
					  if(val<-32768){
						val=-32768;
						clipflag=1;
					  }
					  *ptr=val;
					  ptr+=vi.channels;
					}
				  }

				  if(clipflag)
					log_err("clipping in frame %ld\n",(long)(vd.sequence));
					sceAudioOutput(channel, PSP_AUDIO_VOLUME_MAX, convbuffer);
				  //fwrite(convbuffer,2*vi.channels,bout,stdout);

				  vorbis_synthesis_read(&vd,bout); /* tell libvorbis how
													  many samples we
													  actually consumed */
					return;
				}
			  }
			}
			if(ogg_page_eos(&page))eos=1;
		  }
		}
		if(!eos){
		  u32 bytes = std::min(fds.remaining(), (u32) 4096);
		  char* buffer = ogg_sync_buffer(&sync_state, 4096);
		  fds.read(buffer, bytes);
		  ogg_sync_wrote(&sync_state, bytes);
		  if(bytes == 0) eos=1;
		}
	  }
	}
};

static Stream* running_streams[PSP_AUDIO_CHANNEL_MAX];

static int audio_thread(SceSize args, void* argp) {
	int cbid;
	while(true) {
		for(int i = 0; i < PSP_AUDIO_CHANNEL_MAX; i++) {
			if(running_streams[i]) running_streams[i]->update();
		}
		sceKernelDelayThread(10000);
	}
	return 0;
}

void a_init() {
	//int audio_thread_id = sceKernelCreateThread("update_thread", audio_thread, 0x12, 0xFA0, 0, 0);
	//if(audio_thread_id >= 0) {
		//sceKernelStartThread(audio_thread_id, 0, 0);
	//}
}

void a_update() {
	for(int i = 0; i < PSP_AUDIO_CHANNEL_MAX; i++) {
		if(running_streams[i]) running_streams[i]->update();
	}
}

int a_play(const char* path, int fmt, bool loop, float volume) {
	FileDataStream fds(path);
	ogg_sync_state sync_state;
	ogg_stream_state stream_state;
	ogg_page page;
	ogg_packet packet;
	vorbis_info vi;
	vorbis_comment vc;
	vorbis_dsp_state vd;
	vorbis_block vb;
	if(ogg_sync_init(&sync_state)) {
		log_err("error from ogg_sync_init");
		return -1;
	}
	while(true) {
		u32 to_read = std::min(fds.remaining(), (u32) 4096);
		if(!to_read) return -1;
		char* buffer = ogg_sync_buffer(&sync_state, to_read);
		if(!buffer) {
			log_err("error from ogg_sync_buffer");
			return -1;
		}
		fds.read(buffer, to_read);
		if(ogg_sync_wrote(&sync_state, 4096)) {
			log_err("error from ogg_sync_wrote");
			return -1;
		}
		if(ogg_sync_pageout(&sync_state, &page) != 1) {
			if(to_read < 4096) break;
			log_err("error in stream assumed to be ogg");
			return -1;
		}
		ogg_stream_init(&stream_state, ogg_page_serialno(&page));
		vorbis_info_init(&vi);
		vorbis_comment_init(&vc);
		if(ogg_stream_pagein(&stream_state, &page) < 0) {
			/* error; stream version mismatch perhaps */
			log_err("error reading first page of ogg data\n");
			return -1;
		}
		if(ogg_stream_packetout(&stream_state, &packet) != 1) {
	  		/* no page? must not be vorbis */
	  		log_err("error reading initial header packet.\n");
			return -1;
		}
		if(vorbis_synthesis_headerin(&vi, &vc, &packet) < 0) {
			/* error case; not a vorbis header */
			log_err("ogg does not contain vorbis audio data\n");
	  		return -1;
		}
		int i = 0;
		while(i < 2) {
			while(i < 2) {
				int result = ogg_sync_pageout(&sync_state, &page);
				if(result == 0) break; /* Need more data */
				/* Don't complain about missing or corrupt data yet. We'll
				   catch it at the packet output phase */
				if(result == 1) {
					ogg_stream_pagein(&stream_state, &page); /* we can ignore any errors here
												 as they'll also become apparent
												 at packetout */
					while(i < 2){
						result = ogg_stream_packetout(&stream_state, &packet);
						if(result == 0) break;
						if(result < 0) {
							/* Uh oh; data at some point was corrupted or missing!
								We can't tolerate that in a header.  Die. */
							log_err("corrupt secondary header in ogg\n");
							return -1;
						}
						result = vorbis_synthesis_headerin(&vi, &vc, &packet);
						if(result < 0) {
							log_err("corrupt secondary header in ogg\n");
							return -1;
						}
						i++;
					}
				}
			}
			to_read = fds.remaining();
			if(to_read > 4096) to_read = 4096;
			if(!to_read) return -1;
			/* no harm in not checking before adding more */
			buffer = ogg_sync_buffer(&sync_state, to_read);
			fds.read(buffer, to_read);
			if(to_read == 0 && i < 2) {
				log_err("End of file before finding all Vorbis headers!\n");
				return -1;
			}
			ogg_sync_wrote(&sync_state, to_read);
		}
		{
			char **ptr = vc.user_comments;
			while(*ptr) {
				log_err("%s\n", *ptr);
				++ptr;
			}
			log_err("\nBitstream is %d channel, %ldHz\n", vi.channels, vi.rate);
			log_err("Encoded by: %s\n\n", vc.vendor);
		}
		int convsize = 4096 / vi.channels;
		if(vorbis_synthesis_init(&vd, &vi) == 0) {
			vorbis_block_init(&vd, &vb);
			int ch = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, SAMPLES_PER_CALL, vi.channels == 1? PSP_AUDIO_FORMAT_MONO: PSP_AUDIO_FORMAT_STEREO);
			OggStream* stream = new OggStream(fds);
			stream->sync_state = sync_state;
			stream->stream_state = stream_state;
			stream->page = page;
			stream->packet = packet;
			stream->vi = vi;
			stream->vc = vc;
			stream->vd = vd;
			stream->vb = vb;
			stream->channel = ch;
			stream->convsize = convsize;
			running_streams[ch] = stream;
			return ch;
		}
	}
	platform_throw();
	return -1;
}

int a_play_mem(void* buf, u32 size, int fmt, bool loop, float volume) {
	return -1;
}

int a_play_raw(void* buf, u32 size, int sample_rate, int channels, int bits, bool loop, float volume) {
	//int ch = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, size, channels == 1? PSP_AUDIO_FORMAT_MONO: PSP_AUDIO_FORMAT_STEREO);
	//sceAudioInput(size, buf)
	return -1;
}

int a_play_stream(AudioStreamFn fn, void* userdata, int sample_rate, int channels, bool loop, float volume) {
	return -1;
}

int a_speak(const char* text, float volume) {
	return -1;
}

void a_pause(int ch, bool paused) {}

void a_master_volume(float volume) {}

void a_volume(int ch, float volume) {}

bool a_playing(int ch) {
	return running_streams[ch];
}

void a_destroy(int ch) {
	if(ch >= 0 && ch < PSP_AUDIO_CHANNEL_MAX && running_streams[ch]) {
		delete running_streams[ch];
		running_streams[ch] = nullptr;
		sceAudioChRelease(ch);
	}
}

void a_destroy_all() {
	for(int i = 0; i < PSP_AUDIO_CHANNEL_MAX; i++) {
		if(running_streams[i]) {
			delete running_streams[i];
			running_streams[i] = nullptr;
			sceAudioChRelease(i);
		}
	}
}