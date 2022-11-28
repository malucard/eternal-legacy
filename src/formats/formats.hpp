#pragma once
#include <fs.hpp>
#include <functional>

// when passing any DataStream* to a FormatRegistry make sure
// to not delete it because the format will take ownership

template<typename T>
struct FormatRegistry {
	const char* sig;
	int sig_len;
	std::function<T(DataStream* ds)> fmt_open;

	static std::vector<FormatRegistry> formats;
	static T open(DataStream* ds) {
		for(int i = 0; i < formats.size(); i++) {
			ds->seek(0);
			char sig[formats[i].sig_len];
			ds->read(sig, formats[i].sig_len);
			if(!memcmp(sig, formats[i].sig, formats[i].sig_len)) {
				return formats[i].fmt_open(ds);
			}
		}
		log_err("'%s' is of unknown format", ds->filename().c_str());
		platform_throw();
		return nullptr;
	}
};

struct ArchiveDataStream;
using ArchiveFormat = FormatRegistry<ArchiveDataStream*>;
template<>
std::vector<ArchiveFormat> ArchiveFormat::formats;
struct SceneData;
using SceneFormat = FormatRegistry<Rc<SceneData>>;
template<>
std::vector<SceneFormat> SceneFormat::formats;
struct TextureRef;
using ImageFormat = FormatRegistry<TextureRef>;
template<>
std::vector<ImageFormat> ImageFormat::formats;