#pragma once
#include <string>
#include <vector>

struct Mem {
	Mem(const Mem& o);
	Mem(Mem&& o);
	Mem(usize sz);
	~Mem();
	u32 size();
	u8* get();
	Mem(void* raw);
	void* to_raw();
	void free_raw(void* raw);
	void* data_ptr();

private:
	struct Data {
		u32 size;
		u32 refs;
		u8 data[];
	}* data;
};

struct DataStream {
	virtual ~DataStream() {}
	virtual void read(void* buf, u32 size) = 0;
	virtual u32 tell() = 0;
	virtual void seek(u32 pos) = 0;
	virtual u32 remaining() = 0;
	/// these are for archives only
	virtual std::vector<std::string> list_files() {return {};};
	virtual void select(const char* file) {};
	/// empty string if not a file
	virtual std::string filename() {return "";}

	u8 read8();
	u16 read16();
	u32 read32();
	u64 read64();
	float read_float();
	u8* read_all(u32* size);
	std::string read_line();
	inline void skip(u32 count) {read(nullptr, count);}
};

struct MemDataStream final: DataStream {
	MemDataStream(Mem mem);
	u32 tell() override;
	void seek(u32 pos) override;
	void read(void* buf, u32 size) override;
	u32 remaining() override;

private:
	u32 cursor;
	Mem mem;
};

#define FILE_BUFFER_SIZE (1024*1024)

struct FileDataStream final: DataStream {
	FileDataStream(const char* path);
	FileDataStream(const FileDataStream& o);
	FileDataStream(FileDataStream&& o);
	~FileDataStream() override;
	u32 tell() override;
	void seek(u32 pos) override;
	void read(void* buf, u32 size) override;
	u32 remaining() override;
	std::string filename() override;
	static void mkdir(const char* path);
	static bool exists(const char* path, bool real);
	static std::string to_real_path(const char* path);

private:
	std::string filename_str;
	void* handle;
	u32 cursor;
	u32 size;
	u8* buf;
};

struct ArchiveEntry {
	std::string name;
	int size;
	int flags;
};

struct ArchiveDataStream: DataStream {
	~ArchiveDataStream() override {};
	std::vector<std::string> list_files() override = 0;
	void select(const char* file) override = 0;
};

ArchiveDataStream* fs_archive(const char* path);
/// can open files nested within archives (example, "Model.afs:MU1.vpk:MU1.vmx2")
DataStream* fs_file(const char* path);
bool fs_exists(const char* path, bool real);
