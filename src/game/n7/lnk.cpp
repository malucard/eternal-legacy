#include "n7.hpp"
#include <audio.hpp>
#include <map>

namespace n7 {

struct Lnk {
	struct Entry {
		u32 pos;
		u32 size;
		char name[0x18];
		bool packed;
		Entry(): pos(0), size(0) {}
		Entry(u32 pos, u32 size): pos(pos), size(size) {}
	};
	FileDataStream* file = nullptr;
	std::map<std::string, Entry> entries;
	std::string path;

	Lnk() {}
	~Lnk() {
		delete file;
	}

	void load(const char* path) {
		this->path = path;
		file = new FileDataStream(path);
		file->read(nullptr, 4);
		u32 count = file->read32();
		u32 index_off = 0x10;
		u32 data_off = index_off + count * 32;
		for(int i = 0; i < count; i++) {
			Entry e;
			file->seek(index_off);
			file->read(&e, 0x20);
			e.pos = le32(e.pos) + data_off;
			e.packed = le32(e.size) & 1;
			e.size = le32(e.size) >> 1;
			std::string name = (const char*) e.name;
			std::transform(name.begin(), name.end(), name.begin(), [](char c) {return tolower(c);});
			entries[name] = e;
			index_off += 0x20;
		}
	}
} lnks[LNK_COUNT];

u8* load_cps(u8* data, int size, int* width, int* height, bool no_alpha);

TextureRef load_lnk_texture(int lnk, const std::string& path) {
	std::string npath = path;
	std::transform(npath.begin(), npath.end(), npath.begin(), [](char c) {return tolower(c);});
	Lnk& l = lnks[lnk];
	if(lnk == LNK_BG) {
		std::string fpath = "res/n7/packs/test/bg/"s + npath + ".png"s;
		if(FileDataStream::exists(fpath.c_str())) {
			return g_load_texture(FileDataStream(fpath.c_str()));
		}
		if(!l.file) {
			l.load("n7/bg.dat");
		}
	} else if(lnk == LNK_CHR) {
		std::string fpath = "res/n7/packs/test/chara/"s + npath + ".png"s;
		if(FileDataStream::exists(fpath.c_str())) {
			return g_load_texture(FileDataStream(fpath.c_str()));
		}
		if(!l.file) {
			l.load("n7/chara.dat");
		}
	}
	npath += ".cps";
	auto iter = l.entries.find(npath);
	if(iter == l.entries.end()) {
		log_err("%s not found in %s\n", npath.c_str(), lnk == LNK_BG? "bg.dat": "chara.dat");
		platform_throw();
	}
	Lnk::Entry e = iter->second;
	u8* buf = new u8[e.size];
	l.file->seek(e.pos);
	l.file->read(buf, e.size);
	int w, h;
	u8* res = load_cps(buf, e.size, &w, &h, lnk == LNK_BG);
	TextureRef t = g_upload_texture(res, w, h, TEX_FORMAT_RGBA);
	delete[] res;
	t.data->filename = npath;
	return t;
}

void unpack_lnd(u8* buf, u8* out, u32 size, u32 unpacked_size);

int play_lnk_audio(int lnk, const std::string& path, float volume) {
	Lnk& l = lnks[lnk];
	if(lnk == LNK_VOICE) {
		std::string npath = "res/n7/packs/test/voice/"s + path + ".ogg"s;
		if(FileDataStream::exists(npath.c_str())) {
			return a_play(npath.c_str(), A_FMT_OGG, false, volume);
		}
		if(!l.file) {
			l.load("n7/wave.dat");
		}
	} else if(lnk == LNK_SE) {
		std::string npath = "res/n7/packs/test/se/"s + path + ".ogg"s;
		if(FileDataStream::exists(npath.c_str())) {
			return a_play(npath.c_str(), A_FMT_OGG, false, volume);
		}
		if(!l.file) {
			l.load("n7/se.dat");
		}
	}
	std::string npath = path + (lnk == LNK_SE? ".wav": ".waf");
	std::transform(npath.begin(), npath.end(), npath.begin(), [](char c) {return tolower(c);});
	auto iter = l.entries.find(npath);
	if(iter == l.entries.end()) {
		log_err("%s not found in %s\n", npath.c_str(), lnk == LNK_VOICE? "voice.dat": "se.dat");
		platform_throw();
	}
	Lnk::Entry e = iter->second;
	log_info("%s", npath.c_str());
	u8* out;
	u32 size;
	if(e.packed) {
		u8* buf = new u8[e.size];
		l.file->seek(e.pos);
		l.file->read(buf, e.size);
		size = le32(*(u32*) (buf + 8));
		out = new u8[size + 22];
		unpack_lnd(buf + 16, out + 22, e.size - 16, size);
		delete[] buf;
	} else {
		out = new u8[e.size + 22];
		l.file->seek(e.pos);
		l.file->read(out + 22, e.size);
		size = e.size + 22;
	}
	out += 22;
	u16 channels = le16(*(u16*) (out + 6));
	u32 sample_rate = le32(*(u32*) (out + 8));
	u32 bytes_per_second = le32(*(u32*) (out + 12));
	u16 block = le16(*(u16*) (out + 0x10));
	u16 bits = le16(*(u16*) (out + 0x12));
	u32 adpcm_size = le32(*(u32*) (out + 0x34));
	log_info("%d %d %d %d %d %d", channels, sample_rate, bytes_per_second, block, bits, adpcm_size);
	out -= 22;
	*(u32*) (out) = le32(0x46464952);
	*(u32*) (out + 4) = le32(adpcm_size + 0x46);
	*(u32*) (out + 8) = le32(0x45564157);
	*(u32*) (out + 12) = le32(0x20746d66);
	*(u32*) (out + 16) = le32(0x32);
	*(u16*) (out + 20) = le16(2);
	*(u16*) (out + 22) = le16(channels);
	*(u32*) (out + 24) = le32(sample_rate);
	*(u32*) (out + 28) = le32(bytes_per_second);
	*(u16*) (out + 32) = le16(block);
	*(u16*) (out + 34) = le16(bits);
	*(u16*) (out + 36) = le16(0x20);
	memcpy(out + 38, out + 22 + 0x14, 0x20);
	*(u32*) (out + 0x20 + 38) = le32(0x61746164);
	*(u32*) (out + 0x20 + 42) = le32(adpcm_size);
	FILE* f = fopen("test.wav", "wb");
	fwrite(out, size, 1, f);
	fclose(f);
	return a_play_mem(out, size, A_FMT_WAV, false, volume);
}

}
