#include <fs.hpp>

struct AfsDataStream: public ArchiveDataStream {
	struct Entry {
		u32 pos;
		u32 size;
		char name[32];
	};
	std::vector<Entry> entries;
	int selected = 0;
	u32 cursor = 0;
	DataStream* ds;

	AfsDataStream(DataStream* ds): ds(ds) {
		u32 file_size = ds->remaining() + 4;
		u32 count = ds->read32();
		assert(count * 8 + 8 <= file_size);
		for(int i = 0; i < count; i++) {
			u32 entry_pos = ds->read32();
			u32 entry_size = ds->read32();
			assert(entry_pos + entry_size <= file_size);
			entries.push_back(Entry {
				.pos = entry_pos,
				.size = entry_size
			});
		}
		u32 table_pos = ds->read32();
		u32 table_size = ds->read32();
		if(table_pos == 0) {
			ds->seek(entries[0].pos - 8);
			table_pos = ds->read32();
			table_size = ds->read32();
		}
		assert(table_size == count * 48 || table_size == 0);
		assert(table_pos + table_size <= file_size);
		bool has_table = table_pos && table_size >= count * 48 && table_pos + table_size <= file_size;
		if(has_table) ds->seek(table_pos);
		for(int i = 0; i < count; i++) {
			if(entries[i].pos == 0) {
				log_err("skipping empty entry %d in afs %s", i, ds->filename().c_str());
				continue;
			}
			if(has_table) {
				ds->read(entries[i].name, 32);
				u32 year = ds->read16();
				u32 month = ds->read16();
				u32 day = ds->read16();
				u32 hour = ds->read16();
				u32 minute = ds->read16();
				u32 second = ds->read16();
				u32 entry_size = ds->read32();
				//log_info("%s %d %d %d %d", entries[i].name, filetype, unk1, unk2, entry_size);
			} else {
				strcpy(entries[i].name, std::to_string(i).c_str());
			}
		}
	}

	~AfsDataStream() override {
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
			if(!strcmp(entries[i].name, name)) {
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
			log_err("tried to seek past end of entry '%s' in '%s'", entries[selected].name, ds->filename().c_str());
			platform_throw();
		}
		cursor = pos;
		ds->seek(entries[selected].pos + cursor);
	}

	void read(void* buf, u32 size) override {
		if(cursor + size > entries[selected].size) {
			log_err("tried to read past end of entry '%s' in '%s'", entries[selected].name, ds->filename().c_str());
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

ArchiveDataStream* load_afs(DataStream* ds) {
	return new AfsDataStream(ds);
}
