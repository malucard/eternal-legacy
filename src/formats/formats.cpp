#include "formats.hpp"
#include <graphics.hpp>

//template<typename T>
//std::vector<FormatRegistry<T>> FormatRegistry<T>::formats;

ArchiveDataStream* load_lnk(DataStream* ds);
ArchiveDataStream* load_afs(DataStream* ds);
ArchiveDataStream* load_vpk(DataStream* ds);

template<>
std::vector<ArchiveFormat> ArchiveFormat::formats {{
	.sig = "LNK\0", .sig_len = 4,
	.fmt_open = [](DataStream* ds) -> ArchiveDataStream* {
		return load_lnk(ds);
	}
}, {
	.sig = "AFS\0", .sig_len = 4,
	.fmt_open = [](DataStream* ds) -> ArchiveDataStream* {
		return load_afs(ds);
	}
}, {
	.sig = "DYNAPACK", .sig_len = 8,
	.fmt_open = [](DataStream* ds) -> ArchiveDataStream* {
		return load_vpk(ds);
	}
}};

Rc<SceneData> load_vmx2(DataStream* ds);

template<>
std::vector<SceneFormat> SceneFormat::formats {{
	.sig = "VMX2", .sig_len = 4,
	.fmt_open = [](DataStream* ds) -> SceneRef {
		return load_vmx2(ds);
	}
}};
