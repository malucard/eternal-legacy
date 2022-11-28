#include <map>
#include <fs.hpp>
#include <algorithm>
#include <strings.h>

// KID archive format, used prior to Remember11

void unpack_lnd(u8* buf, u8* out, u32 size, u32 unpacked_size);

struct LnkDataStream: public ArchiveDataStream {
	struct Entry {
		u32 pos;
		u32 size;
		char name[0x18];
		bool packed;
		Entry(): pos(0), size(0) {}
		Entry(u32 pos, u32 size): pos(pos), size(size) {}
	};
	std::vector<Entry> entries;
	int selected = -1;
	u8* unpacked = nullptr;
	int unpacked_len;
	int cursor = 0;
	DataStream* file;

	LnkDataStream(DataStream* file): file(file) {
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
			entries.push_back(e);
			index_off += 0x20;
		}
	}

	~LnkDataStream() override {
		if(unpacked) delete[] unpacked;
		if(file) delete file;
	}

	std::vector<std::string> list_files() override {
		std::vector<std::string> list;
		for(int i = 0; i < entries.size(); i++) {
			list.push_back(entries[i].name);
		}
		return list;
	}

	void select(const char* name) override {
		if(unpacked) {
			delete[] unpacked;
			unpacked = nullptr;
		}
		for(int i = 0; i < entries.size(); i++) {
			if(!strcasecmp(entries[i].name, name)) {
				selected = i;
				cursor = 0;
				Entry& e = entries[selected];
				if(e.packed) {
					log_info("entry '%s' is packed", e.name);
					u8* buf = new u8[e.size];
					file->seek(e.pos);
					file->read(buf, e.size);
					unpacked_len = le32(*(u32*) (buf + 8));
					unpacked = new u8[unpacked_len + 22];
					unpack_lnd(buf + 16, unpacked + 22, e.size - 16, unpacked_len);
					delete[] buf;
				} else {
					//unpacked_len = 
				}
				return;
			}
		}
		log_err("archive '%s' has no entry '%s'", file->filename().c_str(), name);
		platform_throw();
	}

	u32 tell() override {
		return cursor;
	}

	void seek(u32 pos) override {
		if(pos > unpacked_len) {
			log_err("tried to seek past end of entry '%s' in '%s'", entries[selected].name, file->filename().c_str());
			platform_throw();
		}
		cursor = pos;
	}

	void read(void* buf, u32 size) override {
		if(entries[selected].packed) {
			if(cursor + size > unpacked_len) {
				log_err("tried to read past end of entry '%s' in '%s'", entries[selected].name, file->filename().c_str());
				platform_throw();
			}
			memcpy(buf, unpacked + cursor, size);
		} else {
			file->seek(entries[selected].pos + cursor);
			file->read(buf, size);
		}
		cursor += size;
	}

	u32 remaining() override {
		return entries[selected].size - cursor;
	}

	std::string filename() override {
		return file->filename() + ':' + entries[selected].name;;
	}
};

ArchiveDataStream* load_lnk(DataStream* ds) {
	return new LnkDataStream(ds);
}
