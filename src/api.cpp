// TODO: fix this file by splitting into registerer functions that get called from here

#include "api.hpp"
#include <cstdint>
#ifdef REGULAR_LUA
#include <lua.hpp>
#else
#include <luajit-2.1/lua.hpp>
#endif
#include <graphics.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <audio.hpp>
#include <video.hpp>
#include <util.hpp>
#include <vnscript.hpp>

std::string sys_gfx_vendor;
std::string sys_gfx_version;
std::string sys_gfx_renderer;

lua_State* L = 0;
bool what = false;

char* read_file(const char* path, int& len) {
	FILE* f = fopen(path, "r");
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* d = new char[sz];
	fread(d, sz, 1, f);
	fclose(f);
	return d;
}

char* read_file_str(const char* path) {
	FILE* f = fopen(path, "r");
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	char* d = new char[sz + 1];
	fread(d, sz, 1, f);
	fclose(f);
	d[sz] = 0;
	return d;
}

static int l_DataStream_gc(lua_State* L) {
	delete *(DataStream**) lua_touserdata(L, 1);
	return 0;
}

static int l_Texture_gc(lua_State* L) {
	((TextureRef*) lua_touserdata(L, 1))->~TextureRef();
	return 0;
}

static int l_Mem_gc(lua_State* L) {
	log_info("l_Mem_gc");
	((Mem*) lua_touserdata(L, 1))->~Mem();
	return 0;
}

static int l_mem_alloc(lua_State* L) {
	new((Mem*) lua_newuserdata(L, sizeof(Mem))) Mem(luaL_checknumber(L, 1));
	luaL_setmetatable(L, "Mem");
	return 1;
}

/**
 * make a DataStream from a file
 * @function fs_file
 * @param path File to load into a stream
 * @return DataStream
 */
static int l_fs_file(lua_State* L) {
	const char* s = luaL_checkstring(L, 1);
	*(void**) lua_newuserdata(L, sizeof(void*)) = fs_file(s);
	luaL_setmetatable(L, "DataStream");
	return 1;
}

/**
 * make a DataStream from a memory buffer
 * @function fs_mem
 * @param mem Mem to use in the stream
 * @return DataStream
 */
static int l_fs_mem(lua_State* L) {
	*(void**) lua_newuserdata(L, sizeof(void*)) = new MemDataStream(Mem(luaL_checkudata(L, 1, "Mem")));
	luaL_setmetatable(L, "DataStream");
	return 1;
}

/**
 * @function g_load_texture
 * @param data string to load from file, or any other DataStream
 * @return Texture
 */
static int l_g_load_texture(lua_State* L) {
	int flags = luaL_optinteger(L, 2, 0);
	if(lua_isstring(L, 1)) {
		DataStream* file = fs_file(luaL_checkstring(L, 1));
		new(lua_newuserdata(L, sizeof(TextureRef))) TextureRef(g_load_texture(*file, flags));
		delete file;
	} else {
		new(lua_newuserdata(L, sizeof(TextureRef))) TextureRef(g_load_texture(**(DataStream**) luaL_checkudata(L, 1, "DataStream"), flags));
	}
	luaL_setmetatable(L, "Texture");
	return 1;
}

/**
 * @function g_set_virtual_size
 */
static int l_g_set_virtual_size(lua_State* L) {
	int w = luaL_checkinteger(L, 1);
	int h = luaL_checkinteger(L, 2);
	g_set_virtual_size(w, h);
	return 0;
}

/**
 * @function g_draw
 * @param x corner's position relative to origin
 * @param y corner's position relative to origin
 * @param corner what corner of the texture to place in the position, default G_TOP_LEFT
 * @param origin what corner of the screen to use as (0, 0), default G_TOP_LEFT
 * @return nothing
 */
static int l_g_draw(lua_State* L) {
	TextureRef* tex = (TextureRef*) luaL_checkudata(L, 1, "Texture");
	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	int corner = luaL_optinteger(L, 4, G_TOP_LEFT);
	int origin = luaL_optinteger(L, 5, G_TOP_LEFT);
	g_draw(*tex, x, y, corner, origin);
	return 0;
}

/**
 * draw a texture at a specific size
 * @function g_draw_quad
 * @param x corner's position relative to origin
 * @param y corner's position relative to origin
 * @param w width of the resulting draw
 * @param h height of the resulting draw
 * @param corner what corner of the texture to place in the position, default G_TOP_LEFT
 * @param origin what corner of the screen to use as (0, 0), default G_TOP_LEFT
 * @return nothing
 */
static int l_g_draw_quad(lua_State* L) {
	TextureRef* tex = (TextureRef*) luaL_checkudata(L, 1, "Texture");
	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	float w = luaL_checknumber(L, 4);
	float h = luaL_checknumber(L, 5);
	int corner = luaL_optinteger(L, 6, G_TOP_LEFT);
	int origin = luaL_optinteger(L, 7, G_TOP_LEFT);
	if(lua_istable(L, 8)) {
		HaloSettings halo;
		lua_getfield(L, 8, "halo_alpha");
		if(lua_isnumber(L, -1)) {
			halo.halo_alpha = lua_tonumber(L, -1);
		}
		lua_getfield(L, 8, "halo_lightness");
		if(lua_isnumber(L, -1)) {
			halo.halo_lightness = lua_tonumber(L, -1);
		}
		lua_getfield(L, 8, "halo_distance");
		if(lua_isnumber(L, -1)) {
			halo.halo_distance = lua_tonumber(L, -1);
		}
		lua_getfield(L, 8, "halo_detail");
		if(lua_isnumber(L, -1)) {
			halo.halo_detail = lua_tointeger(L, -1);
		}
		lua_getfield(L, 8, "shadow_alpha");
		if(lua_isnumber(L, -1)) {
			halo.shadow_alpha = lua_tonumber(L, -1);
		}
		lua_getfield(L, 8, "shadow_distance");
		if(lua_isnumber(L, -1)) {
			halo.shadow_distance = lua_tonumber(L, -1);
		}
		lua_getfield(L, 8, "shadow_detail");
		if(lua_isnumber(L, -1)) {
			halo.shadow_detail = lua_tointeger(L, -1);
		}
		lua_pop(L, 7);
		halo.resolve();
		g_draw_quad_crop_halo(*tex, x, y, w, h, 0, 0, tex->data->w, tex->data->h, halo, corner, origin);
	} else {
		g_draw_quad(*tex, x, y, w, h, corner, origin);
	}
	return 0;
}

/**
 * draw a section of a texture
 * @function g_draw_crop
 * @param x corner's position relative to origin
 * @param y corner's position relative to origin
 * @param tx position in the texture to use
 * @param tx position in the texture to use
 * @param tw size of the texture region and the resulting draw
 * @param th size of the texture region and the resulting draw
 * @param corner what corner of the texture to place in the position, default G_TOP_LEFT
 * @param origin what corner of the screen to use as (0, 0), default G_TOP_LEFT
 * @return nothing
 */
static int l_g_draw_crop(lua_State* L) {
	TextureRef* tex = (TextureRef*) luaL_checkudata(L, 1, "Texture");
	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	float tx = luaL_checknumber(L, 4);
	float ty = luaL_checknumber(L, 5);
	float tw = luaL_checknumber(L, 6);
	float th = luaL_checknumber(L, 7);
	int corner = luaL_optinteger(L, 8, G_TOP_LEFT);
	int origin = luaL_optinteger(L, 9, G_TOP_LEFT);
	g_draw_crop(*tex, x, y, tx, ty, tw, th, corner, origin);
	return 0;
}

static void push_textinfo(TextInfo ti) {
	lua_createtable(L, 0, 4);
	lua_pushnumber(L, ti.w);
	lua_setfield(L, -2, "w");
	lua_pushnumber(L, ti.h);
	lua_setfield(L, -2, "h");
	lua_pushnumber(L, ti.end_x);
	lua_setfield(L, -2, "end_x");
	lua_pushnumber(L, ti.end_y);
	lua_setfield(L, -2, "end_y");
}

static int l_g_draw_text(lua_State* L) {
	const char* s = luaL_checkstring(L, 1);
	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	float w = luaL_optnumber(L, 4, -1);
	float h = luaL_optnumber(L, 5, 16);
	HaloSettings halo;
	if(lua_istable(L, 6)) {
		lua_getfield(L, 6, "halo_alpha");
		if(lua_isnumber(L, -1)) {
			halo.halo_alpha = lua_tonumber(L, -1);
		}
		lua_getfield(L, 6, "halo_lightness");
		if(lua_isnumber(L, -1)) {
			halo.halo_lightness = lua_tonumber(L, -1);
		}
		lua_getfield(L, 6, "halo_distance");
		if(lua_isnumber(L, -1)) {
			halo.halo_distance = lua_tonumber(L, -1);
		}
		lua_getfield(L, 6, "halo_detail");
		if(lua_isnumber(L, -1)) {
			halo.halo_detail = lua_tointeger(L, -1);
		}
		lua_getfield(L, 6, "shadow_alpha");
		if(lua_isnumber(L, -1)) {
			halo.shadow_alpha = lua_tonumber(L, -1);
		}
		lua_getfield(L, 6, "shadow_distance");
		if(lua_isnumber(L, -1)) {
			halo.shadow_distance = lua_tonumber(L, -1);
		}
		lua_getfield(L, 6, "shadow_detail");
		if(lua_isnumber(L, -1)) {
			halo.shadow_detail = lua_tointeger(L, -1);
		}
		lua_pop(L, 7);
		halo.resolve();
	} else if(lua_isstring(L, 6)) {
		size_t len;
		const char* str = lua_tolstring(L, 6, &len);
		if(!memcmp(str, "text_shadow", len)) {
			halo = HaloSettings::with_shadow();
		} else if(!memcmp(str, "text_halo", len)) {
			halo = HaloSettings {.halo_alpha = 0.75, .shadow_detail = 0, .halo_detail = -1};
			halo.resolve();
		} else if(!memcmp(str, "text_shadow_halo", len)) {
			halo = HaloSettings {.halo_alpha = 0.75, .shadow_distance = 2.5, .shadow_detail = 1, .halo_detail = -1};
			halo.resolve();
		}
	}
	int corner = luaL_optinteger(L, 7, G_TOP_LEFT);
	int origin = luaL_optinteger(L, 8, G_TOP_LEFT);
	push_textinfo(g_draw_text(s, x, y, w, h, halo, corner, origin));
	return 1;
}

static int l_sys_millis(lua_State* L) {
	lua_pushnumber(L, sys_millis());
	return 1;
}

static int l_sys_exit(lua_State* L) {
	sys_exit();
	return 0;
}

static int l_log_info(lua_State* L) {
	const char* s = lua_tostring(L, 1);
	log_info("%s", s);
	return 0;
}

static int l_log_err(lua_State* L) {
	const char* s = lua_tostring(L, 1);
	log_err("%s", s);
	return 0;
}

#define PASS(x) lua_pushcfunction(L, l_ ## x); lua_setglobal(L, #x);
#define PASS_ENUM(x) lua_pushinteger(L, x); lua_setglobal(L, #x);
#define PASS_STRING(x) lua_pushlstring(L, x.c_str(), x.size()); lua_setglobal(L, #x);

void eui_register_api(lua_State* L);
void g_init_batch();
void api_update_screen_size();
void api_init(int argc, char* argv[]) {
#ifdef ROOT_DIR_NOT_CONST
	if(argc >= 2 && !strcmp(argv[1], "dev")) {
		std::string exe_path = argv[0];
		exe_path = exe_path.substr(0, exe_path.find_last_of('/') + 1) + "../out/";
		char* root_dir = (char*) malloc(exe_path.size() + 1);
		memcpy(root_dir, exe_path.c_str(), exe_path.size() + 1);
		ROOT_DIR = root_dir;
		argv[1] = argv[0];
		argv++;
	}
#endif
	//printf("ROOT_DIR is %s\n", ROOT_DIR);
	if(L) return;
	g_init_batch();
	L = luaL_newstate();
	luaL_openlibs(L);
	eui_register_api(L);
// ==================================== fs
	lua_register(L, "fs_read", [](lua_State* L) {
		const char* path = luaL_checkstring(L, 1);
		DataStream* ds = fs_file(path);
		u32 size;
		u8* str = ds->read_all(&size);
		delete ds;
		lua_pushlstring(L, (const char*) str, size);
		return 1;
	});
	PASS(fs_file);
	PASS(fs_mem);
	lua_register(L, "fs_archive", [](lua_State* L) {
		const char* path = luaL_checkstring(L, 1);
		*(void**) lua_newuserdata(L, sizeof(void**)) = fs_archive(path);
		luaL_setmetatable(L, "DataStream");
		return 1;
	});
	lua_register(L, "fs_mkdir", [](lua_State* L) {
		const char* path = luaL_checkstring(L, 1);
		FileDataStream::mkdir(path);
		return 0;
	});
	lua_register(L, "fs_exists", [](lua_State* L) {
		const char* s = luaL_checkstring(L, 1);
		lua_pushboolean(L, fs_exists(s, lua_isboolean(L, 2) && lua_toboolean(L, 2)));
		return 1;
	});
// ==================================== script
	lua_register(L, "vn_script_new", [](lua_State* L) {
		new(lua_newuserdata(L, sizeof(Rc<VnScript>))) Rc<VnScript>(new VnScript);
		luaL_setmetatable(L, "VnScript");
		return 1;
	});
	
	luaL_newmetatable(L, "VnScript");
	lua_pushcfunction(L, [](lua_State* L) {
		((Rc<VnScript>*) luaL_checkudata(L, 1, "VnScript"))->destroy();
		return 0;
	});
	lua_setfield(L, -2, "__gc");
	lua_createtable(L, 0, 5);
	lua_pushcfunction(L, [](lua_State* L) {
		(*(Rc<VnScript>*) luaL_checkudata(L, 1, "VnScript"))->push_msg(luaL_checkinteger(L, 2), luaL_checkstring(L, 3), luaL_checkstring(L, 4));
		return 0;
	}); lua_setfield(L, -2, "push_msg");
	lua_pushcfunction(L, [](lua_State* L) {
		(*(Rc<VnScript>*) luaL_checkudata(L, 1, "VnScript"))->push_delay(luaL_checknumber(L, 2) * 1000);
		return 0;
	}); lua_setfield(L, -2, "push_delay");
	lua_pushcfunction(L, [](lua_State* L) {
		Rc<VnScript>* script = (Rc<VnScript>*) luaL_checkudata(L, 1, "VnScript");
		u32 res = (*script)->make_res(luaL_checkstring(L, 2), SC_RES_BG);
		(*script)->push_bg(res, luaL_optinteger(L, 3, 0), luaL_optnumber(L, 4, 0.5) * 1000, lua_isboolean(L, 5)? lua_toboolean(L, 5): true);
		return 0;
	}); lua_setfield(L, -2, "push_bg");
	lua_pushcfunction(L, [](lua_State* L) {
		Rc<VnScript>* script = (Rc<VnScript>*) luaL_checkudata(L, 1, "VnScript");
		if(lua_isnumber(L, 2)) {
			(*script)->push_bgm_int(lua_tointeger(L, 2));
		} else {
			u32 res = (*script)->make_res(luaL_checkstring(L, 2), SC_RES_BGM);
			(*script)->push_bgm(res);
		}
		return 0;
	}); lua_setfield(L, -2, "push_bgm");
	lua_pushcfunction(L, [](lua_State* L) {
		Rc<VnScript>* script = (Rc<VnScript>*) luaL_checkudata(L, 1, "VnScript");
		if(lua_isnumber(L, 2)) {
			(*script)->push_se_int(lua_tointeger(L, 2));
		} else {
			u32 res = (*script)->make_res(luaL_checkstring(L, 2), SC_RES_SE);
			(*script)->push_se(res);
		}
		//(*script)->push_se_int(luaL_optinteger(L, 3, 0));
		return 0;
	}); lua_setfield(L, -2, "push_se");
	lua_pushcfunction(L, [](lua_State* L) {
		Rc<VnScript>* script = (Rc<VnScript>*) luaL_checkudata(L, 1, "VnScript");
		u32 res = (*script)->make_res(luaL_checkstring(L, 2), SC_RES_CHARA);
		(*script)->push_sprite(res, luaL_checkinteger(L, 3), luaL_checkinteger(L, 4), luaL_optinteger(L, 5, 0), luaL_optnumber(L, 6, 0.5) * 1000, lua_isboolean(L, 7)? lua_toboolean(L, 7): true);
		return 0;
	}); lua_setfield(L, -2, "push_sprite");
	lua_pushcfunction(L, [](lua_State* L) {
		Rc<VnScript>* script = (Rc<VnScript>*) luaL_checkudata(L, 1, "VnScript");
		lua_pushinteger(L, (*script)->size);
		return 1;
	}); lua_setfield(L, -2, "size");
	lua_pushcfunction(L, [](lua_State* L) {
		Rc<VnScript> sc = *(Rc<VnScript>*) luaL_checkudata(L, 1, "VnScript");
		new(lua_newuserdata(L, sizeof(VnExecutor))) VnExecutor(std::move(sc));
		luaL_setmetatable(L, "VnExecutor");
		return 1;
	}); lua_setfield(L, -2, "execute");
	lua_setfield(L, -2, "__index");

	luaL_newmetatable(L, "VnExecutor");
	lua_pushcfunction(L, [](lua_State* L) {
		((VnExecutor*) lua_touserdata(L, 1))->~VnExecutor();
		return 0;
	});
	lua_setfield(L, -2, "__gc");
	lua_createtable(L, 0, 3);
	lua_pushcfunction(L, [](lua_State* L) {
		VnExecutor* ex = (VnExecutor*) luaL_checkudata(L, 1, "VnExecutor");
		ex->process();
		return 0;
	}); lua_setfield(L, -2, "process");
	lua_pushcfunction(L, [](lua_State* L) {
		VnExecutor* ex = (VnExecutor*) luaL_checkudata(L, 1, "VnExecutor");
		ex->next(luaL_optinteger(L, 2, 0));
		return 0;
	}); lua_setfield(L, -2, "next");
	lua_pushcfunction(L, [](lua_State* L) {
		VnExecutor* ex = (VnExecutor*) luaL_checkudata(L, 1, "VnExecutor");
		lua_pushboolean(L, ex->fast_forward());
		return 1;
	}); lua_setfield(L, -2, "fast_forward");
	lua_pushcfunction(L, [](lua_State* L) {
		VnExecutor* ex = (VnExecutor*) luaL_checkudata(L, 1, "VnExecutor");
		lua_pushboolean(L, ex->skip_to_unread());
		return 1;
	}); lua_setfield(L, -2, "skip_to_unread");
	lua_pushcfunction(L, [](lua_State* L) {
		VnExecutor* ex = (VnExecutor*) luaL_checkudata(L, 1, "VnExecutor");
		lua_createtable(L, 0, 8);
		lua_pushinteger(L, ex->cur_msg.frame_type);
		lua_setfield(L, -2, "frame_type");
		lua_pushboolean(L, ex->cur_msg.is_midprint);
		lua_setfield(L, -2, "is_midprint");
		lua_pushboolean(L, ex->cur_msg.is_read);
		lua_setfield(L, -2, "is_read");
		lua_pushboolean(L, ex->cur_msg.is_waiting);
		lua_setfield(L, -2, "is_waiting");
		lua_pushnumber(L, ex->cur_msg.progress);
		lua_setfield(L, -2, "progress");
		lua_pushstring(L, ex->cur_msg.text.c_str());
		lua_setfield(L, -2, "text");
		lua_pushstring(L, ex->cur_msg.speaker_name.c_str());
		lua_setfield(L, -2, "speaker_name");
		lua_pushinteger(L, ex->cur_msg.speaker_id);
		lua_setfield(L, -2, "speaker_id");
		ex->msg_changed = false;
		return 1;
	}); lua_setfield(L, -2, "get_msg");
	lua_pushcfunction(L, [](lua_State* L) {
		VnExecutor* ex = (VnExecutor*) luaL_checkudata(L, 1, "VnExecutor");
		lua_pushboolean(L, ex->msg_changed);
		return 1;
	}); lua_setfield(L, -2, "msg_changed");
	lua_pushcfunction(L, [](lua_State* L) {
		VnExecutor* ex = (VnExecutor*) luaL_checkudata(L, 1, "VnExecutor");
		if(ex->bgm_int_if_changed != 0xFFFFFFFF) {
			lua_pushinteger(L, ex->bgm_int_if_changed);
			ex->bgm_int_if_changed = 0xFFFFFFFF;
			return 1;
		} else if(ex->bgm_if_changed != 0xFFFFFFFF) {
			lua_pushstring(L, ex->sc->resources[ex->bgm_if_changed].src.c_str());
			ex->bgm_if_changed = 0xFFFFFFFF;
			return 1;
		}
		return 0;
	}); lua_setfield(L, -2, "bgm_if_changed");
	lua_pushcfunction(L, [](lua_State* L) {
		VnExecutor* ex = (VnExecutor*) luaL_checkudata(L, 1, "VnExecutor");
		if(ex->se_int_if_changed != 0xFFFFFFFF) {
			lua_pushinteger(L, ex->se_int_if_changed);
			ex->se_int_if_changed = 0xFFFFFFFF;
			return 1;
		} else if(ex->se_if_changed != 0xFFFFFFFF) {
			lua_pushstring(L, ex->sc->resources[ex->se_if_changed].src.c_str());
			ex->se_if_changed = 0xFFFFFFFF;
			return 1;
		}
		return 0;
	}); lua_setfield(L, -2, "se_if_changed");
	lua_pushcfunction(L, [](lua_State* L) {
		VnExecutor* ex = (VnExecutor*) luaL_checkudata(L, 1, "VnExecutor");
		lua_createtable(L, 0, 5);
		if(ex->bg.res == 0xFFFFFFFF) {
			lua_pushnil(L);
		} else {
			lua_pushstring(L, ex->sc->resources[ex->bg.res].src.c_str());
		}
		lua_setfield(L, -2, "src");
		if(ex->bg.prev_res == 0xFFFFFFFF) {
			lua_pushnil(L);
		} else {
			lua_pushstring(L, ex->sc->resources[ex->bg.prev_res].src.c_str());
		}
		lua_setfield(L, -2, "prev_src");
		lua_pushinteger(L, ex->bg.trans_start);
		lua_setfield(L, -2, "trans_start");
		lua_pushinteger(L, ex->bg.trans_duration);
		lua_setfield(L, -2, "trans_duration");
		lua_pushboolean(L, ex->bg.trans_wait);
		lua_setfield(L, -2, "trans_wait");
		return 1;
	}); lua_setfield(L, -2, "get_bg");
	lua_setfield(L, -2, "__index");
// ==================================== audio
	lua_register(L, "a_play", [](lua_State* L) {
		lua_pushinteger(L, a_play(luaL_checkstring(L, 1), A_FMT_UNKNOWN, lua_isboolean(L, 2) && lua_toboolean(L, 2), luaL_optnumber(L, 3, 1.0)));
		return 1;
	});
	lua_register(L, "a_speak", [](lua_State* L) {
		lua_pushinteger(L, a_speak(luaL_checkstring(L, 1), luaL_optnumber(L, 2, 1.0)));
		return 1;
	});
	lua_register(L, "a_destroy", [](lua_State* L) {
		a_destroy(luaL_checkinteger(L, 1));
		return 0;
	});
// ==================================== graphics
	PASS(g_load_texture);
	lua_register(L, "g_load_scene", [](lua_State* L) {
		if(lua_isstring(L, 1)) {
			new(lua_newuserdata(L, sizeof(SceneRef))) SceneRef(g_load_scene(fs_file(luaL_checkstring(L, 1))));
		} else {
			//new(lua_newuserdata(L, sizeof(SceneRef))) SceneRef(g_load_scene((DataStream*) luaL_checkudata(L, 1, "DataStream"), flags));
			platform_throw();
		}
		luaL_setmetatable(L, "Scene");
		return 1;
	});
	lua_register(L, "g_white_tex", [](lua_State* L) {
		new(lua_newuserdata(L, sizeof(TextureRef))) TextureRef(g_white_tex());
		luaL_setmetatable(L, "Texture");
		return 1;
	});
	lua_register(L, "g_grad_tex", [](lua_State* L) {
		new(lua_newuserdata(L, sizeof(TextureRef))) TextureRef(g_grad_tex());
		luaL_setmetatable(L, "Texture");
		return 1;
	});
	lua_register(L, "g_grad_corner_tex", [](lua_State* L) {
		new(lua_newuserdata(L, sizeof(TextureRef))) TextureRef(g_grad_corner_tex());
		luaL_setmetatable(L, "Texture");
		return 1;
	});
	lua_register(L, "g_calc", [](lua_State* L) {
		float x = luaL_checknumber(L, 1);
		float y = luaL_checknumber(L, 2);
		float w = luaL_checknumber(L, 3);
		float h = luaL_checknumber(L, 4);
		float tw = luaL_optnumber(L, 5, w);
		float th = luaL_optnumber(L, 6, h);
		int corner = luaL_optinteger(L, 7, G_TOP_LEFT);
		int origin = luaL_optinteger(L, 8, G_TOP_LEFT);
		g_calc(&x, &y, &w, &h, tw, th, corner, origin);
		lua_pushnumber(L, x);
		lua_pushnumber(L, y);
		lua_pushnumber(L, w);
		lua_pushnumber(L, h);
		return 4;
	});
	lua_register(L, "g_set_min_aspect", [](lua_State* L) {
		float w = luaL_checknumber(L, 1);
		float h = luaL_checknumber(L, 2);
		g_set_min_aspect(w, h);
		return 0;
	});
	lua_register(L, "g_set_max_aspect", [](lua_State* L) {
		float w = luaL_checknumber(L, 1);
		float h = luaL_checknumber(L, 2);
		g_set_max_aspect(w, h);
		return 0;
	});
	lua_register(L, "g_calc_text", [](lua_State* L) {
		const char* text = luaL_checkstring(L, 1);
		float w = luaL_checknumber(L, 2);
		float h = luaL_checknumber(L, 3);
		push_textinfo(g_calc_text(text, w, h));
		return 1;
	});
	lua_register(L, "g_wrap_text", [](lua_State* L) {
		const char* text = luaL_checkstring(L, 1);
		float w = luaL_checknumber(L, 2);
		float h = luaL_checknumber(L, 3);
		std::string str = ustr_to_str(g_wrap_text(strcvt(text), w, h));
		lua_pushlstring(L, str.c_str(), str.size());
		return 1;
	});
	PASS(g_draw_text);
	lua_register(L, "g_text_line_height", [](lua_State* L) {
		lua_pushnumber(L, g_text_line_height(luaL_checknumber(L, 1)));
		return 1;
	});
	PASS(g_draw);
	PASS(g_draw_quad);
	PASS(g_draw_crop);
	lua_register(L, "g_draw_quad_crop", [](lua_State* L) {
		TextureRef* tex = (TextureRef*) luaL_checkudata(L, 1, "Texture");
		float x = luaL_checknumber(L, 2);
		float y = luaL_checknumber(L, 3);
		float w = luaL_checknumber(L, 4);
		float h = luaL_checknumber(L, 5);
		float tx = luaL_checknumber(L, 6);
		float ty = luaL_checknumber(L, 7);
		float tw = luaL_checknumber(L, 8);
		float th = luaL_checknumber(L, 9);
		int corner = luaL_optinteger(L, 10, G_TOP_LEFT);
		int origin = luaL_optinteger(L, 11, G_TOP_LEFT);
		if(lua_istable(L, 12)) {
			HaloSettings halo;
			lua_getfield(L, 12, "halo_alpha");
			if(lua_isnumber(L, -1)) {
				halo.halo_alpha = lua_tonumber(L, -1);
			}
			lua_getfield(L, 12, "halo_lightness");
			if(lua_isnumber(L, -1)) {
				halo.halo_lightness = lua_tonumber(L, -1);
			}
			lua_getfield(L, 12, "halo_distance");
			if(lua_isnumber(L, -1)) {
				halo.halo_distance = lua_tonumber(L, -1);
			}
			lua_getfield(L, 12, "halo_detail");
			if(lua_isnumber(L, -1)) {
				halo.halo_detail = lua_tointeger(L, -1);
			}
			lua_getfield(L, 12, "shadow_alpha");
			if(lua_isnumber(L, -1)) {
				halo.shadow_alpha = lua_tonumber(L, -1);
			}
			lua_getfield(L, 12, "shadow_distance");
			if(lua_isnumber(L, -1)) {
				halo.shadow_distance = lua_tonumber(L, -1);
			}
			lua_getfield(L, 12, "shadow_detail");
			if(lua_isnumber(L, -1)) {
				halo.shadow_detail = lua_tointeger(L, -1);
			}
			lua_pop(L, 7);
			halo.resolve();
			g_draw_quad_crop_halo(*tex, x, y, w, h, tx, ty, tw, th, halo, corner, origin);
		} else {
			g_draw_quad_crop(*tex, x, y, w, h, tx, ty, tw, th, corner, origin);
		}
		return 0;
	});
	PASS_ENUM(G_POINT_LIGHT);
	PASS_ENUM(G_DIRECTIONAL_LIGHT);
	PASS_ENUM(G_SPOT_LIGHT);
	lua_register(L, "g_light", [](lua_State* L) {
		int id = luaL_checkinteger(L, 1);
		int type = luaL_checkinteger(L, 2);
		float x = luaL_checknumber(L, 3);
		float y = luaL_checknumber(L, 4);
		float z = luaL_checknumber(L, 5);
		float r = luaL_checknumber(L, 6);
		float g = luaL_optnumber(L, 7, r);
		float b = luaL_optnumber(L, 8, r);
		g_light(id, type, {x, y, z}, {r, g, b});
		return 0;
	});
	lua_register(L, "g_light_ambient", [](lua_State* L) {
		float r = luaL_checknumber(L, 1);
		float g = luaL_optnumber(L, 2, r);
		float b = luaL_optnumber(L, 3, r);
		g_light_ambient({r, g, b});
		return 0;
	});
	lua_register(L, "g_draw_scene", [](lua_State* L) {
		SceneRef* scene = (SceneRef*) luaL_checkudata(L, 1, "Scene");
		g_draw_scene(*scene);
		return 0;
	});
	lua_register(L, "g_proj_reset", [](lua_State* L) {
		g_proj_reset();
		return 0;
	});
	lua_register(L, "g_proj_perspective", [](lua_State* L) {
		float fov = luaL_checknumber(L, 1);
		float aspect = luaL_checknumber(L, 2);
		float near = luaL_checknumber(L, 3);
		float far = luaL_checknumber(L, 4);
		g_proj_perspective(fov, aspect, near, far);
		return 0;
	});
	lua_register(L, "g_proj_ortho", [](lua_State* L) {
		float left = luaL_optnumber(L, 1, 0.f);
		float right = luaL_optnumber(L, 2, 1.f);
		float bottom = luaL_optnumber(L, 3, 0.f);
		float top = luaL_optnumber(L, 4, 1.f);
		float near = luaL_optnumber(L, 4, -100.f);
		float far = luaL_optnumber(L, 5, 100.f);
		g_proj_ortho(left, right, bottom, top, near, far);
		return 0;
	});
	lua_register(L, "g_identity", [](lua_State* L) {
		g_identity();
		return 0;
	});
	lua_register(L, "g_translate", [](lua_State* L) {
		float x = luaL_optnumber(L, 1, 0);
		float y = luaL_optnumber(L, 2, 0);
		float z = luaL_optnumber(L, 3, 0);
		g_translate(x, y, z);
		return 0;
	});
	lua_register(L, "g_scale", [](lua_State* L) {
		float x = luaL_optnumber(L, 1, 1);
		float y = x;
		float z = x;
		if(!lua_isnoneornil(L, 2) || !lua_isnoneornil(L, 3)) {
			y = luaL_optnumber(L, 2, 1);
			z = luaL_optnumber(L, 3, 1);
		}
		g_scale(x, y, z);
		return 0;
	});
	lua_register(L, "g_rotate", [](lua_State* L) {
		float angle = luaL_optnumber(L, 1, 0);
		float axis_x = luaL_optnumber(L, 2, 0);
		float axis_y = luaL_optnumber(L, 3, 0);
		float axis_z = luaL_optnumber(L, 4, 1);
		g_rotate(angle, {axis_x, axis_y, axis_z});
		return 0;
	});
	lua_register(L, "g_reset_stacks", [](lua_State* L) {
		g_reset_stacks();
		return 0;
	});
	lua_register(L, "g_push", [](lua_State* L) {
		g_push();
		return 0;
	});
	lua_register(L, "g_pop", [](lua_State* L) {
		g_pop();
		return 0;
	});
	lua_register(L, "g_push_color", [](lua_State* L) {
		float r; float g; float b; float a = 1.f;
		if(lua_istable(L, 1)) {
			lua_rawgeti(L, 1, 1);
			r = lua_tonumber(L, -1);
			lua_rawgeti(L, 1, 2);
			g = lua_tonumber(L, -1);
			lua_rawgeti(L, 1, 3);
			b = lua_tonumber(L, -1);
			lua_rawgeti(L, 1, 4);
			a = lua_isnumber(L, -1)? lua_tonumber(L, -1): 1.f;
		} else {
			r = luaL_optnumber(L, 1, 1);
			g = luaL_optnumber(L, 2, 1);
			b = luaL_optnumber(L, 3, 1);
			a = luaL_optnumber(L, 4, 1);
		}
		g_push_color(r, g, b, clamp(a, 0.f, 1.f));
		return 0;
	});
	lua_register(L, "g_pop_color", [](lua_State* L) {
		g_pop_color();
		return 0;
	});
	PASS_ENUM(G_BLEND_SRC);
	PASS_ENUM(G_BLEND_DST);
	PASS_ENUM(G_BLEND_ADD);
	PASS_ENUM(G_BLEND_MUL);
	lua_register(L, "g_set_color_2", [](lua_State* L) {
		float r = luaL_optnumber(L, 1, 1);
		float g = luaL_optnumber(L, 2, 1);
		float b = luaL_optnumber(L, 3, 1);
		float a = luaL_optnumber(L, 4, 1);
		int blend = luaL_optinteger(L, 5, G_BLEND_MUL);
		g_set_color_2(r, g, b, clamp(a, 0.f, 1.f), blend);
		return 0;
	});
	lua_register(L, "g_push_color_off", [](lua_State* L) {
		float r = luaL_optnumber(L, 1, 1);
		float g = luaL_optnumber(L, 2, 1);
		float b = luaL_optnumber(L, 3, 1);
		float a = luaL_optnumber(L, 4, 1);
		g_push_color_off(r, g, b, a);
		return 0;
	});
	lua_register(L, "g_pop_color_off", [](lua_State* L) {
		g_pop_color_off();
		return 0;
	});
	lua_register(L, "g_virtual_to_real", [](lua_State* L) {
		float x = luaL_optnumber(L, 1, 1);
		float y = luaL_optnumber(L, 2, 1);
		g_virtual_to_real(&x, &y);
		lua_pushnumber(L, x);
		lua_pushnumber(L, y);
		return 2;
	});
	lua_register(L, "g_real_to_virtual", [](lua_State* L) {
		float x = luaL_optnumber(L, 1, 1);
		float y = luaL_optnumber(L, 2, 1);
		g_real_to_virtual(&x, &y);
		lua_pushnumber(L, x);
		lua_pushnumber(L, y);
		return 2;
	});
	lua_register(L, "g_cursor_to_virtual", [](lua_State* L) {
		float x = luaL_optnumber(L, 1, 1);
		float y = luaL_optnumber(L, 2, 1);
		x -= (physical_width - g_real_width()) / 2;
		y -= (physical_height - g_real_height()) / 2;
		g_real_to_virtual(&x, &y);
		lua_pushnumber(L, x);
		lua_pushnumber(L, y);
		return 2;
	});
	PASS(g_set_virtual_size);
	lua_register(L, "g_new_batch", [](lua_State* L) {
		new(lua_newuserdata(L, sizeof(DrawBatchRef))) DrawBatchRef(new DrawBatch);
		luaL_setmetatable(L, "DrawBatch");
		return 1;
	});
	lua_register(L, "g_push_batch", [](lua_State* L) {
		g_push_batch(*(DrawBatchRef*) luaL_checkudata(L, 1, "DrawBatch"));
		return 0;
	});
	lua_register(L, "g_pop_batch", [](lua_State* L) {
		g_pop_batch();
		return 0;
	});
	lua_register(L, "g_flush", [](lua_State* L) {
		g_flush();
		return 0;
	});
// ==================================== video
	lua_register(L, "v_load", [](lua_State* L) {
		new(lua_newuserdata(L, sizeof(VideoPlayback))) VideoPlayback(luaL_checkstring(L, 1));
		luaL_setmetatable(L, "VideoPlayback");
		return 1;
	});
	lua_register(L, "v_decode_video", [](lua_State* L) {
		VideoPlayback* vp = (VideoPlayback*) luaL_checkudata(L, 1, "VideoPlayback");
		vp->decode_video();
		return 0;
	});
	lua_register(L, "v_decode_audio", [](lua_State* L) {
		VideoPlayback* vp = (VideoPlayback*) luaL_checkudata(L, 1, "VideoPlayback");
		vp->decode_audio();
		return 0;
	});
	lua_register(L, "v_draw", [](lua_State* L) {
		float x = luaL_checknumber(L, 2);
		float y = luaL_checknumber(L, 3);
		float w = luaL_checknumber(L, 4);
		float h = luaL_checknumber(L, 5);
		VideoPlayback* vp = (VideoPlayback*) luaL_checkudata(L, 1, "VideoPlayback");
		vp->draw(x, y, w, h);
		return 0;
	});
// ==================================== sys
	lua_register(L, "sys_prop", [](lua_State* L) {
		/*
		vsync
		fullscreen
		monitor
		*/
		lua_pushnumber(L, 1);
		return 1;
	});
	PASS(sys_millis);
	lua_register(L, "sys_secs", [](lua_State* L) {
		lua_pushnumber(L, sys_millis() * ((lua_Number) 1 / 1000));
		return 1;
	});
	lua_register(L, "sys_show_logs", [](lua_State* L) {
		sys_show_logs();
		return 0;
	});
	lua_register(L, "sys_set_vsync", [](lua_State* L) {
		unlock_fps = !(lua_isboolean(L, 1) && lua_toboolean(L, 1));
		return 0;
	});
	lua_register(L, "sys_get_vsync", [](lua_State* L) {
		lua_pushboolean(L, !unlock_fps);
		return 1;
	});
	PASS(sys_exit);
	PASS(log_info);
	PASS(log_err);
	PASS_ENUM(G_TOP_LEFT);
	PASS_ENUM(G_TOP_RIGHT);
	PASS_ENUM(G_BOTTOM_LEFT);
	PASS_ENUM(G_BOTTOM_RIGHT);
	PASS_ENUM(G_CENTER_LEFT);
	PASS_ENUM(G_CENTER_RIGHT);
	PASS_ENUM(G_CENTER_TOP);
	PASS_ENUM(G_CENTER_BOTTOM);
	PASS_ENUM(G_CENTER);
	PASS_ENUM(TEX_NEAREST);
	PASS_ENUM(INPUT_OK);
	PASS_ENUM(INPUT_BACK);
	PASS_ENUM(INPUT_MENU);
	PASS_ENUM(INPUT_MENU_BACK);
	PASS_ENUM(INPUT_SYS_MENU);
	PASS_ENUM(INPUT_MORE);
	PASS_ENUM(INPUT_SKIP);
	PASS_ENUM(INPUT_SKIP_INSTANT);
	PASS_ENUM(INPUT_HIDE);
	PASS_ENUM(INPUT_UP);
	PASS_ENUM(INPUT_DOWN);
	PASS_ENUM(INPUT_LEFT);
	PASS_ENUM(INPUT_RIGHT);
	PASS_ENUM(INPUT_CLICK);
	lua_pushstring(L, ROOT_DIR); lua_setglobal(L, "ROOT_DIR");
#ifdef PLATFORM_PC
	lua_pushboolean(L, true); lua_setglobal(L, "PLATFORM_PC");
#ifdef PLATFORM_LINUX
	lua_pushboolean(L, true); lua_setglobal(L, "PLATFORM_LINUX");
#elif defined(PLATFORM_WINDOWS)
	lua_pushboolean(L, true); lua_setglobal(L, "PLATFORM_WINDOWS");
#endif
#elif defined(PLATFORM_ANDROID)
	lua_pushboolean(L, true); lua_setglobal(L, "PLATFORM_ANDROID");
	lua_pushboolean(L, true); lua_setglobal(L, "PLATFORM_MOBILE");
#elif defined(PLATFORM_PSP)
	lua_pushboolean(L, true); lua_setglobal(L, "PLATFORM_PSP");
	lua_pushboolean(L, true); lua_setglobal(L, "PLATFORM_PS");
#elif defined(PLATFORM_SWITCH)
	lua_pushboolean(L, true); lua_setglobal(L, "PLATFORM_SWITCH");
	lua_pushboolean(L, true); lua_setglobal(L, "PLATFORM_NINTENDO");
#elif defined(PLATFORM_XBOX)
	lua_pushboolean(L, true); lua_setglobal(L, "PLATFORM_XBOX");
#endif
// ==================================== metatables
	luaL_newmetatable(L, "DataStream");
	lua_pushcfunction(L, l_DataStream_gc);
	lua_setfield(L, -2, "__gc");
	lua_createtable(L, 0, 4);
	lua_pushcfunction(L, [](lua_State* L) {
		DataStream* tex = *(DataStream**) luaL_checkudata(L, 1, "DataStream");
		lua_pushstring(L, tex->filename().c_str());
		return 1;
	}); lua_setfield(L, -2, "filename");
	lua_pushcfunction(L, [](lua_State* L) {
		DataStream* ds = *(DataStream**) luaL_checkudata(L, 1, "DataStream");
		usize len = ds->remaining();
		char* buf = new char[len + 1];
		ds->read(buf, len);
		buf[len] = 0;
		lua_pushlstring(L, buf, len);
		delete[] buf;
		return 1;
	}); lua_setfield(L, -2, "read_string");
	lua_pushcfunction(L, [](lua_State* L) {
		DataStream* ds = *(DataStream**) luaL_checkudata(L, 1, "DataStream");
		std::string line = ds->read_line();
		lua_pushlstring(L, line.c_str(), line.size());
		return 1;
	}); lua_setfield(L, -2, "read_line");
	lua_pushcfunction(L, [](lua_State* L) {
		DataStream* ds = *(DataStream**) luaL_checkudata(L, 1, "DataStream");
		lua_pushinteger(L, ds->remaining());
		return 1;
	}); lua_setfield(L, -2, "remaining");
	lua_pushcfunction(L, [](lua_State* L) {
		DataStream* ds = *(DataStream**) luaL_checkudata(L, 1, "DataStream");
		lua_pushinteger(L, ds->tell());
		return 1;
	}); lua_setfield(L, -2, "tell");
	lua_pushcfunction(L, [](lua_State* L) {
		DataStream* ds = *(DataStream**) luaL_checkudata(L, 1, "DataStream");
		ds->seek(luaL_checkinteger(L, 2));
		return 0;
	}); lua_setfield(L, -2, "seek");
	lua_pushcfunction(L, [](lua_State* L) {
		DataStream* ds = *(DataStream**) luaL_checkudata(L, 1, "DataStream");
		ds->skip(luaL_checkinteger(L, 2));
		return 0;
	}); lua_setfield(L, -2, "skip");
	lua_pushcfunction(L, [](lua_State* L) {
		DataStream* ds = *(DataStream**) luaL_checkudata(L, 1, "DataStream");
		ds->select(luaL_checkstring(L, 2));
		return 0;
	}); lua_setfield(L, -2, "select");
	lua_pushcfunction(L, [](lua_State* L) {
		DataStream* ds = *(DataStream**) luaL_checkudata(L, 1, "DataStream");
		auto list = ds->list_files();
		lua_createtable(L, list.size(), 0);
		for(int i = 0; i < list.size(); i++) {
			lua_pushinteger(L, i + 1);
			lua_pushstring(L, list[i].c_str());
			lua_settable(L, -3);
		}
		return 1;
	}); lua_setfield(L, -2, "list_files");
	lua_setfield(L, -2, "__index");

	luaL_newmetatable(L, "Texture");
	lua_pushcfunction(L, l_Texture_gc);
	lua_setfield(L, -2, "__gc");
	lua_createtable(L, 0, 2);
	lua_pushcfunction(L, [](lua_State* L) {
		TextureRef* tex = (TextureRef*) luaL_checkudata(L, 1, "Texture");
		lua_pushnumber(L, (float) tex->width() / tex->height());
		return 1;
	}); lua_setfield(L, -2, "aspect");
	lua_pushcfunction(L, [](lua_State* L) {
		TextureRef* tex = (TextureRef*) luaL_checkudata(L, 1, "Texture");
		lua_pushinteger(L, tex->width());
		lua_pushinteger(L, tex->height());
		return 2;
	}); lua_setfield(L, -2, "size");
	lua_setfield(L, -2, "__index");

	luaL_newmetatable(L, "VideoPlayback");
	lua_pushcfunction(L, [](lua_State* L) {
		((VideoPlayback*) lua_touserdata(L, 1))->~VideoPlayback();
		return 0;
	});
	lua_setfield(L, -2, "__gc");

	luaL_newmetatable(L, "DrawBatch");
	lua_pushcfunction(L, [](lua_State* L) {
		((DrawBatchRef*) lua_touserdata(L, 1))->~DrawBatchRef();
		return 0;
	});
	lua_setfield(L, -2, "__gc");

	luaL_newmetatable(L, "Mesh");
	lua_pushcfunction(L, [](lua_State* L) {
		((MeshRef*) lua_touserdata(L, 1))->~MeshRef();
		return 0;
	});
	lua_setfield(L, -2, "__gc");

	luaL_newmetatable(L, "Scene");
	lua_pushcfunction(L, [](lua_State* L) {
		((SceneRef*) lua_touserdata(L, 1))->~SceneRef();
		return 0;
	});
	lua_setfield(L, -2, "__gc");
	api_update_screen_size();
// ==================================== end of definitions
	char path[strlen(ROOT_DIR) + sizeof("core/main.lua")];
	strcpy(path, ROOT_DIR);
	strcat(path, "core/main.lua");
	if(luaL_loadfile(L, path)) {
		log_err("unhandled error in loading main.lua: %s", lua_tostring(L, -1));
		platform_throw();
	}
	for(int i = 0; i < argc; i++) {
		lua_pushstring(L, argv[i]);
	}
	if(lua_pcall(L, argc, 0, 0)) {
		log_err("unhandled error in initializing core: %s", lua_tostring(L, -1));
		platform_throw();
	}
}

void eui_load_default_theme();
void api_load() {
	eui_load_default_theme();
	PASS_STRING(sys_gfx_vendor);
	PASS_STRING(sys_gfx_version);
	PASS_STRING(sys_gfx_renderer);
	lua_getglobal(L, "_load");
	if(lua_pcall(L, 0, 0, 0)) {
		log_err("unhandled error in core _load: %s", lua_tostring(L, -1));
		platform_throw();
	}
}

void api_update_screen_size() {
	lua_pushinteger(L, g_width());
	lua_setglobal(L, "g_width");
	lua_pushinteger(L, g_height());
	lua_setglobal(L, "g_height");
	lua_pushinteger(L, g_real_width());
	lua_setglobal(L, "g_real_width");
	lua_pushinteger(L, g_real_height());
	lua_setglobal(L, "g_real_height");
}

void api_resize(int rw, int rh) {
	physical_width = rw;
	physical_height = rh;
	update_screen_size();
	lua_getglobal(L, "_resize");
	lua_pushinteger(L, real_width);
	lua_pushinteger(L, real_height);
	if(lua_pcall(L, 2, 0, 0)) {
		log_err("unhandled error in core _resize: %s", lua_tostring(L, -1));
		platform_throw();
	}
}

float last_frame_time = 0.0;
float delta;
void api_frame() {
	lua_pushinteger(L, draw_calls);
	lua_setglobal(L, "sys_draw_calls");
	lua_pushinteger(L, polys);
	lua_setglobal(L, "sys_polys");
	draw_calls = 0;
	polys = 0;
	float cur_time = sys_secs();
	delta = cur_time - last_frame_time;
	last_frame_time = cur_time;
	lua_getglobal(L, "_frame");
	if(lua_pcall(L, 0, 0, 0)) {
		log_err("unhandled error in core _frame: %s", lua_tostring(L, -1));
		platform_throw();
	}
	g_flush();
	glyph_cache_frame_end();
}

void api_input(int key, bool pressed, bool repeating, float strength) {
	lua_getglobal(L, "_input");
	lua_pushinteger(L, key);
	lua_pushboolean(L, pressed);
	lua_pushboolean(L, repeating);
	lua_pushboolean(L, strength);
	if(lua_pcall(L, 4, 0, 0)) {
		log_err("unhandled error in core _input: %s", lua_tostring(L, -1));
		platform_throw();
	}
}

void api_input(int key, bool pressed, bool repeating) {
	api_input(key, pressed, repeating, pressed? 1.f: 0.f);
}

void api_cursor(float x, float y, bool dragging) {
	lua_getglobal(L, "_cursor");
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	lua_pushboolean(L, dragging);
	if(lua_pcall(L, 3, 0, 0)) {
		log_err("unhandled error in core _cursor: %s", lua_tostring(L, -1));
		platform_throw();
	}
}

void api_close() {
	lua_getglobal(L, "_close");
	if(lua_pcall(L, 0, 0, 0)) {
		log_err("unhandled error in core _close: %s", lua_tostring(L, -1));
		platform_throw();
	}
}

std::string FileDataStream::to_real_path(const char* path) {
	if(!L) {
		return std::string(ROOT_DIR) + path;
	}
	lua_getglobal(L, "fs_resolve");
	lua_pushstring(L, path);
	if(lua_pcall(L, 1, 1, 0)) {
		log_err("unhandled error in core fs_resolve: %s with %s", lua_tostring(L, -1), path);
		platform_throw();
	}
	std::string ret = luaL_checkstring(L, -1);
	lua_pop(L, 1);
	return ret;
}
