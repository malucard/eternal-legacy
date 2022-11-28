#include <fs.hpp>
#include <cstdio>

std::string root_path;

FileDataStream::FileDataStream(const char* path) {
	handle = fopen((root_path + "/eternal/" + path).c_str(), "rb");
	if(!handle) {
		log_err("could not open file %s", (root_path + "/eternal/" + path).c_str());
		throw;
	}
	cursor = 0;
	fseek((FILE*) handle, 0, SEEK_END);
	size = ftell((FILE*) handle);
	fseek((FILE*) handle, 0, SEEK_SET);
}

FileDataStream::~FileDataStream() {
	fclose((FILE*) handle);
}

u32 FileDataStream::remaining() {
	return size - cursor;
}

void FileDataStream::seek(u32 pos) {
	fseek((FILE*) handle, pos, SEEK_SET);
}

void FileDataStream::read(void* buf, u32 size) {
	if(cursor + size > this->size) {
		log_err("tried to read past file end");
		throw;
	}
	if(buf) {
		fread(buf, size, 1, (FILE*) handle);
	} else {
		fseek((FILE*) handle, size, SEEK_CUR);
	}
	cursor += size;
}

bool FileDataStream::exists(const char* path) {
	FILE* test = fopen(path, "rb");
	if(test) {
		fclose(test);
		return true;
	} else return false;
}

std::string FileDataStream::to_real_path(const char* path) {
	return root_path + "/eternal/" + path;
}
