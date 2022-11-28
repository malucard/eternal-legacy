#include <video.hpp>
#include <graphics.hpp>
#include <audio.hpp>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
#include <queue>

#undef av_err2str
char* av_err2str(int errnum) {
	static char str[AV_ERROR_MAX_STRING_SIZE];
	memset(str, 0, sizeof(str));
	return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

#define AUDIO_BUF_SIZE 819200
#define AUDIO_REFILL_THRESH 4096
#define PACKET_QUEUE_COUNT 100

enum {
	STREAM_NONE,
	STREAM_VIDEO,
	STREAM_AUDIO
};

struct PacketEntry {
	AVPacket* packet;
	u8 stream = STREAM_NONE;
};

struct PacketQueue {
	PacketEntry entries[PACKET_QUEUE_COUNT];
	int earliest = -1;
	int earliest_video = -1;
	int earliest_audio = -1;
	bool full = false;

	int alloc() {
		if(earliest == -1) earliest = 0;
		int chosen = -1;
		for(int i = earliest; i < PACKET_QUEUE_COUNT + earliest; i++) {
			int queue_idx = i >= PACKET_QUEUE_COUNT? i - PACKET_QUEUE_COUNT: i;
			if(entries[queue_idx].stream == STREAM_NONE) {
				if(chosen == -1) chosen = queue_idx;
			} else {
				chosen = -1;
			}
		}
		if(chosen == -1) full = true;
		return chosen;
	}

	void free(int id) {
		full = false;
		av_packet_unref(entries[id].packet);
		entries[id].stream = STREAM_NONE;
		if(earliest_audio == id) {
			for(int i = earliest_audio + 1; i < PACKET_QUEUE_COUNT; i++) {
				if(entries[i].stream == STREAM_AUDIO) {
					earliest_audio = i;
					return;
				}
			}
			for(int i = 0; i < earliest_audio; i++) {
				if(entries[i].stream == STREAM_AUDIO) {
					earliest_audio = i;
					return;
				}
			}
			earliest_audio = -1;
		}
		if(earliest_video == id) {
			for(int i = earliest_video + 1; i < PACKET_QUEUE_COUNT; i++) {
				if(entries[i].stream == STREAM_VIDEO) {
					earliest_video = i;
					return;
				}
			}
			for(int i = 0; i < earliest_video; i++) {
				if(entries[i].stream == STREAM_VIDEO) {
					earliest_video = i;
					return;
				}
			}
			earliest_video = -1;
		}
		if(earliest == id) {
			for(int i = earliest + 1; i < PACKET_QUEUE_COUNT; i++) {
				if(entries[i].stream != STREAM_NONE) {
					earliest = i;
					return;
				}
			}
			for(int i = 0; i < earliest; i++) {
				if(entries[i].stream != STREAM_NONE) {
					earliest = i;
					return;
				}
			}
			earliest = -1;
		}
	}
};

struct VideoPlaybackData {
	float playback_start;
	AVFormatContext* fmt_ctx;
	PacketQueue packet_queue;

	int video_stream_idx;
	AVCodecContext* codec_ctx;
	SwsContext* sws_ctx = nullptr;
	AVFrame* frame;
	float time_base;
	u8* pic_rgba[4];
	int pic_rgba_linesizes[4];
	TextureRef texture;
	bool video_frame_changed = false;
	bool frame_awaiting = false;
	float frame_awaiting_time = 0.0;

	int audio_stream_idx;
	AVCodecContext* audio_codec_ctx;
	SwrContext* swr_ctx = nullptr;
	AVFrame* audio_frame;
	float* audio_buf;
	u32 audio_buf_count;
	bool audio_frame_awaiting = false;
	float audio_time_base;
	int audio_id;

	~VideoPlaybackData() {
		if(!fmt_ctx) return;
		avformat_close_input(&fmt_ctx);
		for(int i = 0; i < PACKET_QUEUE_COUNT; i++) {
			av_packet_free(&packet_queue.entries[i].packet);
		}
		av_frame_free(&frame);
		av_frame_free(&audio_frame);
		avcodec_free_context(&codec_ctx);
		if(sws_ctx) {
			sws_freeContext(sws_ctx);
			av_freep(&pic_rgba[0]);
		}
		if(audio_stream_idx != -1) {
			free(audio_buf);
			avcodec_free_context(&audio_codec_ctx);
			a_destroy(audio_id);
		}
		fmt_ctx = nullptr;
	}
};

VideoPlayback::~VideoPlayback() {
	delete (VideoPlaybackData*) data;
}

#define ERRCK(cond, msg) if(cond) {log_err(msg); platform_throw(); return;}

VideoPlayback::VideoPlayback(const char* opath): data(nullptr) {
	std::string path = FileDataStream::to_real_path(opath);
	AVFormatContext* fmt_ctx = avformat_alloc_context();
	if(avformat_open_input(&fmt_ctx, path.c_str(), nullptr, nullptr)) {
		log_err("could not open video file");
		platform_throw();
		return;
	}
	log_info("format %s, duration %lld us, bit_rate %lld", fmt_ctx->iformat->name, fmt_ctx->duration, fmt_ctx->bit_rate);
	log_info("finding stream info from format");
	ERRCK(avformat_find_stream_info(fmt_ctx, NULL) < 0, "could not get stream info");
	const AVCodec* codec = nullptr;
	AVCodecParameters* codec_params = nullptr;
	int video_stream_idx = -1;
	const AVCodec* audio_codec = nullptr;
	AVCodecParameters* audio_codec_params = nullptr;
	int audio_stream_idx = -1;
	for(int i = 0; i < fmt_ctx->nb_streams; i++) {
		AVCodecParameters* stream_codec_params = fmt_ctx->streams[i]->codecpar;
		log_info("AVStream->time_base before open coded %d/%d", fmt_ctx->streams[i]->time_base.num, fmt_ctx->streams[i]->time_base.den);
		log_info("AVStream->r_frame_rate before open coded %d/%d", fmt_ctx->streams[i]->r_frame_rate.num, fmt_ctx->streams[i]->r_frame_rate.den);
		log_info("AVStream->start_time %" PRId64, fmt_ctx->streams[i]->start_time);
		log_info("AVStream->duration %" PRId64, fmt_ctx->streams[i]->duration);
		log_info("finding the proper decoder (CODEC)");
		const AVCodec* stream_codec = avcodec_find_decoder(stream_codec_params->codec_id);
		if(!stream_codec) {
			log_err("unsupported codec, skipping");
			platform_throw();
			continue; //return;
		}
		if(stream_codec_params->codec_type == AVMEDIA_TYPE_VIDEO) {
	  		if(video_stream_idx == -1) {
	  			video_stream_idx = i;
	  			codec = stream_codec;
	  			codec_params = stream_codec_params;
	  		}
	  		log_info("Video Codec: resolution %d x %d", stream_codec_params->width, stream_codec_params->height);
		} else if(stream_codec_params->codec_type == AVMEDIA_TYPE_AUDIO) {
	  		if(audio_stream_idx == -1) {
	  			audio_stream_idx = i;
	  			audio_codec = stream_codec;
	  			audio_codec_params = stream_codec_params;
	  		}
			log_info("Audio Codec: %d channels, sample rate %d", stream_codec_params->channels, stream_codec_params->sample_rate);
			log_info("\tCodec %s ID %d bit_rate %lld", stream_codec->name, stream_codec->id, stream_codec_params->bit_rate);
		}
	}
	AVCodecContext* codec_ctx = nullptr;
	if(video_stream_idx != -1) {
		codec_ctx = avcodec_alloc_context3(codec);
		ERRCK(!codec_ctx, "video avcodec_alloc_context3 failed");
		ERRCK(avcodec_parameters_to_context(codec_ctx, codec_params) < 0, "video avcodec_parameters_to_context failed");
		ERRCK(avcodec_open2(codec_ctx, codec, NULL) < 0, "video avcodec_open2 failed");
	}
	AVCodecContext* audio_codec_ctx = nullptr;
	if(audio_stream_idx != -1) {
		audio_codec_ctx = avcodec_alloc_context3(audio_codec);
		ERRCK(!audio_codec_ctx, "audio avcodec_alloc_context3 failed");
		ERRCK(avcodec_parameters_to_context(audio_codec_ctx, audio_codec_params) < 0, "audio avcodec_parameters_to_context failed");
		ERRCK(avcodec_open2(audio_codec_ctx, audio_codec, NULL) < 0, "audio avcodec_open2 failed");
	}
	AVFrame* frame = av_frame_alloc();
	ERRCK(!frame, "failed to allocate memory for video AVFrame");
	AVFrame* audio_frame = av_frame_alloc();
	ERRCK(!audio_frame, "failed to allocate memory for audio AVFrame");
	VideoPlaybackData* data = new VideoPlaybackData {
		.playback_start = sys_secs() + fmt_ctx->start_time / (float) AV_TIME_BASE,
		.fmt_ctx = fmt_ctx,

		.video_stream_idx = video_stream_idx,
		.codec_ctx = codec_ctx,
		.frame = frame,
		.time_base = (float) av_q2d(av_stream_get_codec_timebase(fmt_ctx->streams[video_stream_idx])),

		.audio_stream_idx = audio_stream_idx,
		.audio_codec_ctx = audio_codec_ctx,
		.audio_frame = audio_frame,
		.audio_time_base = (float) av_q2d(av_stream_get_codec_timebase(fmt_ctx->streams[audio_stream_idx]))
	};
	for(int i = 0; i < PACKET_QUEUE_COUNT; i++) {
		data->packet_queue.entries[i].packet = av_packet_alloc();
		ERRCK(!data->packet_queue.entries[i].packet, "failed to allocate memory for AVPacket");
	}
	AVPacket* packet = av_packet_alloc();
	ERRCK(!packet, "failed to allocate memory for AVPacket");
	if(audio_stream_idx != -1) {
		data->audio_buf = (float*) malloc(AUDIO_BUF_SIZE);
		data->audio_id = a_play_stream([](float* buf, unsigned int& samples_to_read, void* userdata) {
			u32 osamples_to_read = samples_to_read;
			VideoPlaybackData* data = (VideoPlaybackData*) userdata;
			u32 oaudio_buf_count = data->audio_buf_count;
			samples_to_read = glm::min((unsigned int) data->audio_buf_count, samples_to_read);
			if(samples_to_read < data->audio_buf_count) {
				data->audio_buf_count -= samples_to_read;
				memcpy(buf, data->audio_buf, samples_to_read * 4);
				memmove(data->audio_buf, data->audio_buf + samples_to_read, data->audio_buf_count * 4);
			} else {
				data->audio_buf_count = 0;
				memcpy(buf, data->audio_buf, samples_to_read * 4);
			}
			//log_info("read %d\tof %u\twith %u", samples_to_read, osamples_to_read, oaudio_buf_count);
			return RES_OK;
		}, data, data->audio_codec_ctx->sample_rate, 1, false);
		if(!audio_codec_ctx->channel_layout) {
			audio_codec_ctx->channel_layout = av_get_default_channel_layout(audio_codec_ctx->channels);
		}
		data->swr_ctx = swr_alloc_set_opts(data->swr_ctx, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_FLT, audio_codec_ctx->sample_rate,
			audio_codec_ctx->channel_layout, audio_codec_ctx->sample_fmt, audio_codec_ctx->sample_rate, 0, nullptr);
		ERRCK(!data->swr_ctx, "error allocating swr_ctx");
		ERRCK(swr_init(data->swr_ctx) < 0, "error initializing swr_ctx");
	}
	this->data = (void*) data;
}

static int decode_packet(VideoPlaybackData* data, int queue_idx) {
	AVPacket* packet = data->packet_queue.entries[queue_idx].packet;
	int response = avcodec_send_packet(data->codec_ctx, packet);
	if(response < 0) {
		log_info("error sending packet to decoder: %s", av_err2str(response));
		return response;
	}
	while(response >= 0) {
		response = avcodec_receive_frame(data->codec_ctx, data->frame);
		if(response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
			break;
		} else if(response < 0) {
			log_info("error receiving frame from decoder: %s", av_err2str(response));
			return response;
		}
		return response;
	}
	return -1;
}

void VideoPlayback::decode_video() {
	int response = 0;
	VideoPlaybackData* data = (VideoPlaybackData*) this->data;
	if(!data) return;
	// step: process frames that are waiting for their time
	if(data->audio_frame_awaiting) {
		u32 count = std::min(AUDIO_BUF_SIZE / 4 - data->audio_buf_count, (u32) data->audio_frame->nb_samples);
		if(count >= data->audio_frame->nb_samples) {
			data->audio_frame_awaiting = false;
			u8* buf_ptr = (u8*) (data->audio_buf + data->audio_buf_count);
			swr_convert(data->swr_ctx, &buf_ptr, count, (const u8**) data->audio_frame->data, count);
			data->audio_buf_count += count;
			while(response >= 0) {
				response = avcodec_receive_frame(data->audio_codec_ctx, data->audio_frame);
				if(response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
					break;
				} else if(response < 0) {
					log_err("error receiving frame from audio decoder: %s", av_err2str(response));
					continue;
				}
				u32 count = std::min(AUDIO_BUF_SIZE / 4 - data->audio_buf_count, (u32) data->audio_frame->nb_samples);
				if(count < data->audio_frame->nb_samples) {
					data->audio_frame_awaiting = true;
					break;
				}
				u8* buf_ptr = (u8*) (data->audio_buf + data->audio_buf_count);
				swr_convert(data->swr_ctx, &buf_ptr, count, (const u8**) data->audio_frame->data, count);
				data->audio_buf_count += count;
			}
		}
	}
	if(data->frame_awaiting && data->frame_awaiting_time <= sys_secs()) {
		int w = data->frame->width, h = data->frame->height;
		if(!data->sws_ctx) {
			data->sws_ctx = sws_getContext(w, h, (AVPixelFormat) data->frame->format,
				w, h, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
			av_image_alloc(data->pic_rgba, data->pic_rgba_linesizes, data->frame->width, data->frame->height, AV_PIX_FMT_RGBA, 1);
		}
		if(data->sws_ctx) {
			sws_scale(data->sws_ctx, data->frame->data, data->frame->linesize, 0, h, data->pic_rgba, data->pic_rgba_linesizes);
			data->video_frame_changed = false;
	  		if(data->texture.data && data->texture.width() == data->frame->width && data->texture.height() == data->frame->height) {
				data->texture.upload_sub(data->pic_rgba[0], 0, 0, data->frame->width, data->frame->height, TEX_FORMAT_RGBA);
			} else {
				data->texture = g_upload_texture(data->pic_rgba[0], data->frame->width, data->frame->height, TEX_FORMAT_RGBA, TEX_DYNAMIC, false);
			}
		} else {
			log_err("error from sws_getContext");
			platform_throw();
		}
		data->frame_awaiting = false;
	}
	// step: put packets in queue
	while(true) {
		int queue_idx = data->packet_queue.alloc();
		if(queue_idx == -1) break;
		AVPacket* packet = data->packet_queue.entries[queue_idx].packet;
		if(av_read_frame(data->fmt_ctx, packet) >= 0) {
			if(packet->stream_index == data->video_stream_idx) {
				data->packet_queue.entries[queue_idx].stream = STREAM_VIDEO;
				if(data->packet_queue.earliest_video == -1) {
					data->packet_queue.earliest_video = queue_idx;
				}
				if(data->packet_queue.earliest == -1) {
					data->packet_queue.earliest = queue_idx;
				}
			} else if(packet->stream_index == data->audio_stream_idx) {
				data->packet_queue.entries[queue_idx].stream = STREAM_AUDIO;
				if(data->packet_queue.earliest_audio == -1) {
					data->packet_queue.earliest_audio = queue_idx;
				}
				if(data->packet_queue.earliest == -1) {
					data->packet_queue.earliest = queue_idx;
				}
			}
		} else break;
	}
	// step: decode queued packets
	if(data->packet_queue.earliest == -1) return;
	bool stop_video = data->frame_awaiting, stop_audio = data->audio_frame_awaiting;
	for(int i = data->packet_queue.earliest; i < PACKET_QUEUE_COUNT + data->packet_queue.earliest; i++) {
		int queue_idx = i >= PACKET_QUEUE_COUNT? i - PACKET_QUEUE_COUNT: i;
		PacketEntry& entry = data->packet_queue.entries[queue_idx];
		if(entry.stream == STREAM_VIDEO && !stop_video) {
			response = decode_packet(data, queue_idx);
			if(response < 0) {
			data->packet_queue.free(queue_idx);
				continue;
			}
			float frame_time = data->playback_start + data->frame->best_effort_timestamp * data->time_base;
			//float frame_time = data->playback_start + av_q2d(data->codec_ctx->time_base) * data->packet->pts;
			if(frame_time > sys_secs()) {
				stop_video = true;
				data->frame_awaiting = true;
				data->frame_awaiting_time = frame_time;
			data->packet_queue.free(queue_idx);
				continue;
			}
			data->frame_awaiting = false;
			int w = data->frame->width, h = data->frame->height;
			if(!data->sws_ctx) {
				data->sws_ctx = sws_getContext(w, h, (AVPixelFormat) data->frame->format,
					w, h, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
				av_image_alloc(data->pic_rgba, data->pic_rgba_linesizes, data->frame->width, data->frame->height, AV_PIX_FMT_RGBA, 1);
			}
			if(data->sws_ctx) {
				sws_scale(data->sws_ctx, data->frame->data, data->frame->linesize, 0, h, data->pic_rgba, data->pic_rgba_linesizes);
				data->video_frame_changed = false;
	  			if(data->texture.data && data->texture.width() == data->frame->width && data->texture.height() == data->frame->height) {
					data->texture.upload_sub(data->pic_rgba[0], 0, 0, data->frame->width, data->frame->height, TEX_FORMAT_RGBA);
				} else {
					data->texture = g_upload_texture(data->pic_rgba[0], data->frame->width, data->frame->height, TEX_FORMAT_RGBA, TEX_DYNAMIC, false);
				}
			} else {
				log_err("error from sws_getContext");
				platform_throw();
			}
			data->packet_queue.free(queue_idx);
		} else if(entry.stream == STREAM_AUDIO && !stop_audio) {
			response = avcodec_send_packet(data->audio_codec_ctx, entry.packet) < 0;
			if(response < 0) {
				log_err("error sending packet to audio decoder: %s", av_err2str(response));
				data->packet_queue.free(queue_idx);
				continue;
			}
			while(response >= 0) {
				response = avcodec_receive_frame(data->audio_codec_ctx, data->audio_frame);
				if(response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
					break;
				} else if(response < 0) {
					log_err("error receiving frame from audio decoder: %s", av_err2str(response));
					continue;
				}
				u32 count = std::min(AUDIO_BUF_SIZE / 4 - data->audio_buf_count, (u32) data->audio_frame->nb_samples);
				if(count < data->audio_frame->nb_samples) {
					stop_audio = true;
					data->audio_frame_awaiting = true;
					break;
				}
				//swr_convert(data->swr_ctx, (u8**) &data->audio_buf, data->audio_buf_count, (const u8**) data->frame->data, data->frame->nb_samples);
				u8* buf_ptr = (u8*) (data->audio_buf + data->audio_buf_count);
				swr_convert(data->swr_ctx, &buf_ptr, count, (const u8**) data->audio_frame->data, count);
				data->audio_buf_count += count;
				//swr_convert(data->swr_ctx, (u8**) &(data->audio_buf + data->audio_buf_count), data->audio_buf_count, (const u8**) data->frame->data, data->frame->nb_samples);
			}
			data->packet_queue.free(queue_idx);
		} else if(entry.stream != STREAM_NONE) break;
	}
}

void VideoPlayback::draw(float x, float y, float w, float h) {
	VideoPlaybackData* data = (VideoPlaybackData*) this->data;
	if(!data) return;
	if(data->texture.data) g_draw_quad(data->texture, x, y, w, h);
}

void VideoPlayback::decode_audio() {
	
}