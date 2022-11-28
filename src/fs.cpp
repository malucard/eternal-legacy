#include "fs.hpp"
#include <formats/formats.hpp>
#include <cstddef>
#include <cstring>
#include <string>
#include <algorithm>

Mem::Mem(const Mem& o): data(o.data) {
	if(data) data->refs++;
}

Mem::Mem(Mem&& o): data(o.data) {
	o.data = nullptr;
}

Mem::Mem(usize sz) {
	data = (Data*) malloc(offsetof(Data, data) + sz);
	data->size = sz;
	data->refs = 1;
}

Mem::Mem(void* raw): data((Data*) raw) {
	if(data) data->refs++;
}

Mem::~Mem() {
	free_raw(data);
}

u32 Mem::size() {
	return data? data->size: 0;
}

u8* Mem::get() {
	return data? data->data: nullptr;
}

void* Mem::to_raw() {
	if(data) data->refs++;
	return data;
}

void Mem::free_raw(void* raw) {
	if(raw) {
		Data* data = (Data*) raw;
		if(data->refs <= 1) {
			free(data);
		} else {
			data->refs--;
		}
	}
}

void* Mem::data_ptr() {
	return data? data->data: nullptr;
}

u8 DataStream::read8() {
	u8 x;
	read((void*) &x, 1);
	return x;
}

u16 DataStream::read16() {
#ifdef IS_BIG_ENDIAN
	u8 x[2];
	read((void*) x, 2);
	return (u16) x[0] << 0 | (u16) x[1] << 8;
#else
	u16 x;
	read((void*) &x, 2);
	return x;
#endif
}

u32 DataStream::read32() {
#ifdef IS_BIG_ENDIAN
	u8 x[4];
	read((void*) x, 4);
	return (u32) x[0] << 0 | (u32) x[1] << 8 | (u32) x[2] << 16 | (u32) x[3] << 24;
#else
	u32 x;
	read((void*) &x, 4);
	return x;
#endif
}

u64 DataStream::read64() {
#ifdef IS_BIG_ENDIAN
	u8 x[8];
	read((void*) x, 8);
	return (u64) x[0] << 0 | (u64) x[1] << 8 | (u64) x[2] << 16 | (u64) x[3] << 24
		| (u64) x[4] << 32 | (u64) x[5] << 40 | (u64) x[6] << 48 | (u64) x[7] << 56;
#else
	u64 x;
	read((void*) &x, 8);
	return x;
#endif
}

float DataStream::read_float() {
#ifdef IS_BIG_ENDIAN
	u8 x[4];
	read((void*) x, 4);
	u32 y = (u32) x[0] << 0 | (u32) x[1] << 8 | (u32) x[2] << 16 | (u32) x[3] << 24;
	return (float*) &y;
#else
	float x;
	read((void*) &x, 4);
	return x;
#endif
}

u8* DataStream::read_all(u32* size) {
	*size = remaining();
	u8* buf = (u8*) malloc(*size);
	read(buf, *size);
	return buf;
}

std::string DataStream::read_line() {
	std::string s;
	while(remaining()) {
		char ch = read8();
		if(ch != '\r') {
			if(ch == '\n') {
				if(s.size()) {
					return s;
				}
			} else {
				s += ch;
			}
		}
	}
	return s;
}

MemDataStream::MemDataStream(Mem mem): mem(mem), cursor(0) {}

u32 MemDataStream::tell() {
	return cursor;
}

void MemDataStream::seek(u32 pos) {
	cursor = pos;
}

void MemDataStream::read(void* buf, u32 size) {
	if(cursor + size > mem.size()) {
		log_err("tried to read past buffer end");
		platform_throw();
	}
	if(buf) {
		memcpy(buf, mem.get() + cursor, size);
	}
	cursor += size;
}

u32 MemDataStream::remaining() {
	return mem.size() - cursor;
}

ArchiveDataStream* fs_archive(const char* path) {
	return ArchiveFormat::open(fs_file(path));
}

DataStream* fs_file(const char* path_) {
	std::string path = FileDataStream::to_real_path(path_);
	DataStream* ds = nullptr;
	int last_colon = 0;
	int i = 0;
	for(char c; (c = path[i]); i++) {
		if(i > 5 && c == ':' && path[i + 1] != '/') {
			if(ds) {
				int len = i - last_colon;
				char fpath[len + 1];
				fpath[len] = 0;
				memcpy(fpath, path.c_str() + last_colon, len);
				ds->select(fpath);
				ds = ArchiveFormat::open(ds);
			} else {
				char fpath[i + 1];
				fpath[i] = 0;
				memcpy(fpath, path.c_str(), i);
				ds = ArchiveFormat::open(new FileDataStream(fpath));
			}
			last_colon = i + 1;
		}
	}
	if(ds) {
		int len = i - last_colon;
		char fpath[len + 1];
		fpath[len] = 0;
		memcpy(fpath, path.c_str() + last_colon, len);
		ds->select(fpath);
	} else {
		ds = new FileDataStream(path.c_str());
	}
	return ds;
}

bool fs_exists(const char* path_, bool real) {
	std::string path = real? path_: FileDataStream::to_real_path(path_);
	DataStream* ds = nullptr;
	int last_colon = 0;
	int i = 0;
	for(char c; (c = path[i]); i++) {
		if(i > 5 && c == ':' && path[i + 1] != '/') {
			if(ds) {
				int len = i - last_colon;
				char fpath[len + 1];
				fpath[len] = 0;
				memcpy(fpath, path.c_str() + last_colon, len);
				ds->select(fpath);
				ds = ArchiveFormat::open(ds);
			} else {
				char fpath[i + 1];
				fpath[i] = 0;
				memcpy(fpath, path.c_str(), i);
				ds = ArchiveFormat::open(new FileDataStream(fpath));
			}
			last_colon = i + 1;
		}
	}
	bool exists;
	if(ds) {
		int len = i - last_colon;
		char fpath[len + 1];
		fpath[len] = 0;
		memcpy(fpath, path.c_str() + last_colon, len);
		auto list = ds->list_files();
		exists = std::find(list.begin(), list.end(), (const char*) fpath) != list.end();
	} else {
		exists = FileDataStream::exists(path.c_str(), true);
	}
	delete ds;
	return exists;
}
