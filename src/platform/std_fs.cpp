#include <fs.hpp>
#include <graphics.hpp>
#include <cstdio>
#include <filesystem>

FileDataStream::FileDataStream(const char* path): filename_str(path) {
	std::string npath = to_real_path(path);
	handle = fopen(npath.c_str(), "rb");
	if(!handle) {
		log_err("could not open file %s", path);
		platform_throw();
	}
	cursor = 0;
	fseek((FILE*) handle, 0, SEEK_END);
	size = ftell((FILE*) handle);
	fseek((FILE*) handle, 0, SEEK_SET);
}

FileDataStream::FileDataStream(const FileDataStream& o): filename_str(o.filename_str.c_str()) {
	std::string npath = to_real_path(o.filename_str.c_str());
	handle = fopen(npath.c_str(), "rb");
	if(!handle) {
		log_err("could not open file %s", o.filename_str.c_str());
		platform_throw();
	}
	cursor = o.cursor;
	fseek((FILE*) handle, cursor, SEEK_SET);
	size = o.size;
}

FileDataStream::FileDataStream(FileDataStream&& o): filename_str(o.filename_str), handle(o.handle), cursor(o.cursor), size(o.size) {
	o.handle = nullptr;
}

FileDataStream::~FileDataStream() {
	if(handle) {
		fclose((FILE*) handle);
	}
}

u32 FileDataStream::remaining() {
	return size - cursor;
}

u32 FileDataStream::tell() {
	return cursor;
}

void FileDataStream::seek(u32 pos) {
	fseek((FILE*) handle, pos, SEEK_SET);
	cursor = pos;
}

void FileDataStream::read(void* buf, u32 size) {
	if(cursor + size > this->size) {
		log_err("tried to read past file end");
		platform_throw();
	}
	if(buf) {
		fread(buf, size, 1, (FILE*) handle);
	} else {
		fseek((FILE*) handle, size, SEEK_CUR);
	}
	cursor += size;
}

bool FileDataStream::exists(const char* path, bool real) {
	FILE* test;
	if(real) {
		test = fopen(path, "rb");
	} else {
		std::string npath = to_real_path(path);
		test = fopen(npath.c_str(), "rb");
	}
	if(test) {
		fclose(test);
		return true;
	} else return false;
}

std::string FileDataStream::filename() {
	return filename_str;
}

void FileDataStream::mkdir(const char* path) {
	std::filesystem::create_directories(path);
}
