#include "formats.hpp"
#include <strings.h>

// Ever17 Xbox archive format in which the models are stored

/* QuickBMS script
idstring "DYNAPACK"
get FILES long
for i = 0 < FILES
	get SIZE long
	getdstring NAME 255
	savepos OFFSET
	log NAME OFFSET SIZE
	goto SIZE 0 SEEK_CUR
next i
*/

struct VpkDataStream: ArchiveDataStream {
	struct Entry {
		std::string name;
		u32 pos;
		u32 size;
	};
	std::vector<Entry> entries;
	u32 selected = 0;
	u32 cursor = 0;
	DataStream* ds;

	VpkDataStream(DataStream* ds): ds(ds) {
		ds->seek(8);
		u32 count = ds->read32();
		for(int i = 0; i < count; i++) {
			u32 size = ds->read32();
			char filename[255];
			ds->read(filename, 255);
			// for some reason, these are full paths to someone's desktop
			// so we do a little sanitizing (C:\Users\... -> Users/...)
			std::string name = filename;
			size_t start_idx = name.find_first_of(':');
			if(start_idx == std::string::npos) {
				start_idx = 0;
			} else {
				start_idx += 2;
			}
			name = name.substr(start_idx);
			for(int i = 0; i < name.size(); i++) {
				if(name[i] == '\\') {
					name[i] = '/';
				}
			}
			u32 pos = ds->tell();
			ds->skip(size);
			entries.push_back(Entry {
				.name = name,
				.pos = pos,
				.size = size
			});
		}
	}

	~VpkDataStream() override {
		if(ds) delete ds;
	}

	std::vector<std::string> list_files() override {
		std::vector<std::string> list;
		for(int i = 0; i < entries.size(); i++) {
			list.push_back(entries[i].name);
		}
		return list;
	}

	void select(const char* name) override {
		for(int i = 0; i < entries.size(); i++) {
			if(!strcasecmp(entries[i].name.c_str(), name)) {
				selected = i;
				cursor = 0;
				ds->seek(entries[i].pos);
				return;
			}
		}
		log_err("archive '%s' has no entry '%s'", ds->filename().c_str(), name);
		platform_throw();
	}

	u32 tell() override {
		return cursor;
	}

	void seek(u32 pos) override {
		if(pos > entries[selected].size) {
			log_err("tried to seek past end of entry '%s' in '%s'", entries[selected].name.c_str(), ds->filename().c_str());
			platform_throw();
		}
		cursor = pos;
		ds->seek(entries[selected].pos + cursor);
	}

	void read(void* buf, u32 size) override {
		if(cursor + size > entries[selected].size) {
			log_err("tried to read past end of entry '%s' in '%s'", entries[selected].name.c_str(), ds->filename().c_str());
			platform_throw();
		}
		ds->seek(entries[selected].pos + cursor);
		ds->read(buf, size);
		cursor += size;
	}

	u32 remaining() override {
		return entries[selected].size - cursor;
	}

	std::string filename() override {
		return ds->filename() + ':' + entries[selected].name;
	}
};

ArchiveDataStream* load_vpk(DataStream* ds) {
	return new VpkDataStream(ds);
}
