#include "vnscript.hpp"
#include <graphics.hpp>
#ifdef REGULAR_LUA
#include <lua.hpp>
#else
#include <luajit-2.1/lua.hpp>
#endif

extern lua_State* L;

void emit_err(const char* msg, const char* msg2 = nullptr) {
	if(msg2) {
		log_err("error in vn script: %s %s", msg, msg2);
	} else {
		log_err("error in vn script: %s", msg? msg: msg2);
	}
	//platform_throw();
}

VnScript::VnScript() {
	buf = (u8*) malloc(capacity);
}

VnScript::~VnScript() {
	/*for(int i = 0; i < lua_resources.size(); i++) {
		if(resources[i].lua_idx) {
			luaL_unref(L, LUA_REGISTRYINDEX, resources[i].lua_idx);
			resources[i].lua_idx = 0;
		}
	}*/
	free(buf);
}

u32 VnScript::make_res(std::string_view res_src, u32 kind) {
	auto it = res_names.find(res_src);
	if(it != res_names.end()) {
		return it->second;
	} else {
		u32 idx = resources.size();
		std::string res_src_str = std::string(res_src);
		res_names[res_src_str] = idx;
		resources.push_back({
			.src = std::move(res_src_str),
			.kind = kind
		});
		return idx;
	}
	/*resources.push_back({
		.src = res_src,
		.lua_idx = 0
	});*/
}

int img_loader(const char* src) {
	DataStream* file = fs_file(src);
	new(lua_newuserdata(L, sizeof(TextureRef))) TextureRef(g_load_texture(*file, 0));
	return luaL_ref(L, LUA_REGISTRYINDEX);
}

void VnScript::push_bgm(u32 res_id) {
	push8(SC_OP_BGM);
	push24(res_id);
}

void VnScript::push_bgm_int(u8 id) {
	push8(SC_OP_BGM_INT);
	push8(id);
}

void VnScript::push_se(u32 res_id) {
	push8(SC_OP_SE);
	push24(res_id);
}

void VnScript::push_se_int(u8 id) {
	push8(SC_OP_SE_INT);
	push8(id);
}

void VnScript::push_stop_se() {
	push8(SC_OP_STOP_SE);
}

void VnScript::push_bg(u32 res_id, u8 trans, u16 duration, bool wait) {
	if(trans >= (1 << 4)) emit_err("bg transition id does not fit in 4 bits");
	if(duration >= (1 << 11)) emit_err("bg transition duration does not fit in 11 bits");
	push8(SC_OP_BG);
	push24(res_id);
	push_trans(trans, duration, wait, "bg");
}

void VnScript::push_sprite(u32 res_id, u8 slot, u8 pos, u8 trans, u16 duration, bool wait) {
	if(slot >= MAX_SPRITES) emit_err("sprite slot does not fit in MAX_SPRITES");
	if(slot >= (1 << 4)) emit_err("sprite slot does not fit in 4 bits");
	if(pos >= (1 << 4)) emit_err("sprite pos does not fit in 4 bits");
	push8(SC_OP_SPRITE);
	push24(res_id);
	push8(slot << 4 | pos);
	push_trans(trans, duration, wait, "sprite");
}

void VnScript::push_msg(u8 speaker_id, const char* speaker_name, const char* msg) {
	push8(SC_OP_MSG);
	push8(speaker_id);
	push_str(speaker_name);
	push_str(msg);
}

void VnScript::push_delay(u32 millis) {
	push8(SC_OP_DELAY);
	push24(millis);
}

void VnScript::push_trans(u8 trans, u16 duration, bool wait, const char* err_ctx) {
	if(trans >= (1 << 4)) emit_err(err_ctx, "transition id does not fit in 4 bits");
	if(duration >= (1 << 11)) emit_err(err_ctx, "transition duration does not fit in 11 bits");
	push16(trans << 12 | ((u16) wait << 11) | duration);
}

void VnScript::grow(u32 sz, u32 cap) {
	if(capacity < size + cap) {
		capacity *= 2;
		buf = (u8*) realloc(buf, capacity);
	}
	size += sz;
}

void VnScript::push8(u8 x) {
	grow(1, 1);
	*(u8*) (buf + size - 1) = x;
}

void VnScript::push16(u16 x) {
	grow(2, 2);
	*(u16*) (buf + size - 2) = x;
}

void VnScript::push24(u32 x) {
	grow(3, 4);
#ifdef IS_BIG_ENDIAN
	*(u32*) (buf + size - 3) = x << 8;
#else
	*(u32*) (buf + size - 3) = x;
#endif
}

void VnScript::push32(u32 x) {
	grow(4, 4);
	*(u32*) (buf + size - 4) = x;
}

void VnScript::push_str(const char* x) {
	u32 sz = strlen(x) + 1;
	grow(sz, sz);
	memcpy(buf + size - sz, x, sz);
}

void VnScript::push_str_view(std::string_view x) {
	u32 sz = x.size() + 1;
	grow(sz, sz);
	memcpy(buf + size - sz, x.data(), sz - 1);
	buf[size - 1] = 0;
}
