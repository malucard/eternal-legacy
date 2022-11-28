#pragma once

#define ROOT_DIR "ms0:/PSP/GAME/eternal/"
#define FONT_ATLAS_SIZE 512
#define NO_COLOR_OFF_ATTRIB
//#define COLOR_OFF_FLUSH
//#define NO_INDEXED_RENDER
#define TWO_VERTEX_QUADS
//#define LOW_BANDWIDTH
#define NO_VIRTUALS
#define HARDWARE_TEX_FORMATS (TEX_FORMAT_ALPHA4 | TEX_FORMAT_4444 | TEX_FORMAT_5551 | TEX_FORMAT_565)
#define TEXTURE_MAX_WIDTH 512
#define TEXTURE_MAX_HEIGHT 512
#define TEXTURE_POWER_OF_TWO true

struct TextureDataExtraFields {
	u16 pot_x;
	u16 pot_y;
	u16 pix_w;
	u16 pix_h;
	u16 format;
	u8 filter;
	u8 ownership;
	bool on_vram;
	bool swizzled;
};

/*#define CUSTOM_BATCH_ALLOC
template<typename T>
struct BatchAllocator {
  using value_type = T;
  BatchAllocator() noexcept {}
  template<typename U>
  BatchAllocator(const BatchAllocator<U>&) noexcept {}
  T* allocate(std::size_t n) {
	//return (T*) sceGuGetMemory(n);
	T* p = (T*) valloc(sizeof(T) * n);
	if(!p) {
		log_err("bye");
		platform_throw();
	}
	return p;
  }
  void deallocate(T* p, std::size_t n) {
	vfree(p);
  }
};

template<class T, class U>
constexpr bool operator==(const BatchAllocator<T>&, const BatchAllocator<U>&) noexcept {
	return true;
}

template<class T, class U>
constexpr bool operator!=(const BatchAllocator<T>&, const BatchAllocator<U>&) noexcept {
	return false;
}*/
