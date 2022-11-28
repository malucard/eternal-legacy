#include "eui.hpp"
#ifdef REGULAR_LUA
#include <lua.hpp>
#else
#include <luajit-2.1/lua.hpp>
#endif

struct ClosureWrapper {
	lua_State* L;
	int self_key;
	int closure_key;

	ClosureWrapper(const ClosureWrapper& o): L(o.L) {
		if(o.self_key != LUA_NOREF) {
			lua_rawgeti(o.L, LUA_REGISTRYINDEX, o.self_key);
			self_key = luaL_ref(L, LUA_REGISTRYINDEX);
			lua_rawgeti(o.L, LUA_REGISTRYINDEX, o.closure_key);
			closure_key = luaL_ref(L, LUA_REGISTRYINDEX);
		} else {
			self_key = LUA_NOREF;
			closure_key = LUA_NOREF;
		}
	}

	ClosureWrapper(ClosureWrapper&& o): L(o.L) {
		self_key = o.self_key;
		closure_key = o.closure_key;
		o.self_key = LUA_NOREF;
		o.closure_key = LUA_NOREF;
	}

	ClosureWrapper(lua_State* L, int self_idx, int closure_idx): L(L) {
		lua_pushvalue(L, self_idx);
		self_key = luaL_ref(L, LUA_REGISTRYINDEX);
		lua_pushvalue(L, closure_idx);
		closure_key = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	~ClosureWrapper() {
		if(self_key != LUA_NOREF) {
			luaL_unref(L, LUA_REGISTRYINDEX, self_key);
			luaL_unref(L, LUA_REGISTRYINDEX, closure_key);
			self_key = LUA_NOREF;
			closure_key = LUA_NOREF;
		}
	}
};

#define CHECK_ELEMENT(dst, idx) Rc<EuiElement> dst = *(Rc<EuiElement>*) lua_touserdata(L, idx);\
	if(!dst || !luaL_getmetafield(L, 1, "eui_element_marker")) {\
		luaL_error(L, "self must be an EuiElement");\
	}

static int l_EuiElement_get_min_size(lua_State* L) {
	CHECK_ELEMENT(elem, 1);
	i16 min_w, min_h;
	EUI_CALL(elem.data, get_min_size(luaL_checknumber(L, 2), luaL_checknumber(L, 3), min_w, min_h));
	lua_pushinteger(L, min_w);
	lua_pushinteger(L, min_h);
	return 2;
}

static int l_EuiElement_propagate_theme(lua_State* L) {
	CHECK_ELEMENT(elem, 1);
	EuiThemeRef theme = *(EuiThemeRef*) luaL_checkudata(L, 2, "EuiTheme");
	elem->propagate_theme(theme);
	return 0;
}

static int l_EuiElement_invalidate(lua_State* L) {
	CHECK_ELEMENT(elem, 1);
	elem->invalidate();
	return 0;
}

static int l_EuiElement_draw(lua_State* L) {
	CHECK_ELEMENT(elem, 1);
	elem.data->draw(RectInt(luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checknumber(L, 5)));
	return 0;
}

static int l_EuiElement_on_input(lua_State* L) {
	CHECK_ELEMENT(elem, 1);
	elem->on_input(luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checkinteger(L, 2), lua_toboolean(L, 3));
	return 0;
}

static int l_EuiElement_on_cursor(lua_State* L) {
	CHECK_ELEMENT(elem, 1);
	elem->on_cursor(luaL_checknumber(L, 2), luaL_checknumber(L, 3), lua_toboolean(L, 4), lua_toboolean(L, 5));
	return 0;
}

#define BEGIN(name) luaL_newmetatable(L, #name);\
	lua_pushboolean(L, true);\
	lua_setfield(L, -2, "eui_element_marker");\
	lua_pushcfunction(L, [](lua_State* L) {\
		((Rc<name>*) luaL_checkudata(L, 1, #name))->destroy();\
		return 0;\
	}); lua_setfield(L, -2, "__gc");
#define BIND(name) if(len == sizeof(#name) - 1 && !memcmp(key, #name, len))

#define BIND_METHOD(name) BIND(name) {\
	lua_pushcfunction(L, l_EuiElement_ ## name); return 1;}
#define BIND_FIELD_STR(name) BIND(name) {\
	lua_pushlstring(L, elem->name.c_str(), elem->name.size()); return 1;}
#define BIND_FIELD_NUM(name) BIND(name) {\
	lua_pushnumber(L, elem->name); return 1;}
#define BIND_FIELD_BOOL(name) BIND(name) {\
	lua_pushboolean(L, elem->name); return 1;}
#define BEGIN_INDEX(name) lua_pushcclosure(L, [](lua_State* L) { \
		Rc<name> elem = *(Rc<name>*) luaL_checkudata(L, 1, #name); \
		size_t len;\
		const char* key = luaL_checklstring(L, 2, &len);\
		BIND_METHOD(get_min_size);\
		BIND_METHOD(propagate_theme);\
		BIND_METHOD(draw);\
		BIND_METHOD(invalidate);\
		BIND_METHOD(on_cursor);\
		BIND_METHOD(on_input);\
		BIND_FIELD_BOOL(expand);\
		BIND_FIELD_STR(id);\
		BIND(theme) {\
			new(lua_newuserdata(L, sizeof(void*))) EuiThemeRef(elem->theme.data);\
			luaL_setmetatable(L, "EuiTheme");\
			return 1;\
		}
#define END_INDEX return 0;\
	}, 0); lua_setfield(L, -2, "__index");

#define BIND_SET_STR(name) BIND(name) {\
	const char* str = luaL_checklstring(L, 3, &len);\
	elem->name = std::string(str, len); return 0;}
#define BIND_SET_NUM(name) BIND(name) {\
	elem->name = lua_checknumber(L, 3); return 0;}
#define BIND_SET_BOOL(name) BIND(name) {\
	elem->name = lua_toboolean(L, 3); return 0;}
#define BEGIN_NEWINDEX(name) lua_pushcclosure(L, [](lua_State* L) { \
		Rc<name> elem = *(Rc<name>*) luaL_checkudata(L, 1, #name); \
		size_t len;\
		const char* key = luaL_checklstring(L, 2, &len);\
		BIND_SET_BOOL(expand);\
		BIND_SET_STR(id);\
		BIND(theme) {\
			elem->theme = *(EuiThemeRef*) luaL_checkudata(L, 3, "EuiTheme");\
			return 1;\
		}
#define END_NEWINDEX return 0;\
	}, 0); lua_setfield(L, -2, "__newindex");

#define THEME_HALO(name) lua_getfield(L, 1, #name);\
	if(lua_istable(L, -1)) {\
		lua_getfield(L, -1, "halo_alpha");\
		if(lua_isnumber(L, -1)) {\
			theme->name.halo_alpha = lua_tonumber(L, -1);\
		}\
		lua_getfield(L, -2, "halo_lightness");\
		if(lua_isnumber(L, -1)) {\
			theme->name.halo_lightness = lua_tonumber(L, -1);\
		}\
		lua_getfield(L, -3, "halo_distance");\
		if(lua_isnumber(L, -1)) {\
			theme->name.halo_distance = lua_tonumber(L, -1);\
		}\
		lua_getfield(L, -4, "halo_detail");\
		if(lua_isnumber(L, -1)) {\
			theme->name.halo_detail = lua_tointeger(L, -1);\
		}\
		lua_getfield(L, -5, "shadow_distance");\
		if(lua_isnumber(L, -1)) {\
			theme->name.shadow_distance = lua_tonumber(L, -1);\
		}\
		lua_getfield(L, -6, "shadow_detail");\
		if(lua_isnumber(L, -1)) {\
			theme->name.shadow_detail = lua_tointeger(L, -1);\
		}\
		lua_pop(L, 6);\
	} else if(lua_isnil(L, -1)) {\
		theme->name = {};\
	}\
	lua_pop(L, 1);
#define THEME_COLOR(name) lua_getfield(L, 1, #name);\
	if(lua_istable(L, -1)) {\
		lua_rawgeti(L, -1, 1);\
		theme->name.r = lua_tonumber(L, -1);\
		lua_rawgeti(L, -2, 2);\
		theme->name.g = lua_tonumber(L, -1);\
		lua_rawgeti(L, -3, 3);\
		theme->name.b = lua_tonumber(L, -1);\
		lua_rawgeti(L, -4, 4);\
		theme->name.a = lua_tonumber(L, -1);\
		lua_pop(L, 4);\
	}\
	lua_pop(L, 1);
#define THEME_INT(name) lua_getfield(L, 1, #name);\
	if(lua_isnumber(L, -1)) {\
		theme->name = lua_tointeger(L, -1);\
	}\
	lua_pop(L, 1);
#define THEME_TEX(name) lua_getfield(L, 1, #name);\
	if(lua_istable(L, -1)) {\
		lua_rawgeti(L, -1, 1);\
		TextureRef* tex = (TextureRef*) luaL_checkudata(L, -1, "Texture");\
		theme->name.tex = *tex;\
		lua_rawgeti(L, -2, 2);\
		theme->name.crop_x = lua_tonumber(L, -1);\
		lua_rawgeti(L, -3, 3);\
		theme->name.crop_y = lua_tonumber(L, -1);\
		lua_rawgeti(L, -4, 4);\
		theme->name.crop_w = lua_tonumber(L, -1);\
		lua_rawgeti(L, -5, 5);\
		theme->name.crop_h = lua_tonumber(L, -1);\
		lua_rawgeti(L, -6, 6);\
		theme->name.np_top = lua_tonumber(L, -1);\
		lua_rawgeti(L, -7, 7);\
		theme->name.np_bottom = lua_tonumber(L, -1);\
		lua_rawgeti(L, -8, 8);\
		theme->name.np_left = lua_tonumber(L, -1);\
		lua_rawgeti(L, -9, 9);\
		theme->name.np_right = lua_tonumber(L, -1);\
		lua_rawgeti(L, -10, 10);\
		theme->name.np_top_pix = lua_tonumber(L, -1);\
		lua_rawgeti(L, -11, 11);\
		theme->name.np_bottom_pix = lua_tonumber(L, -1);\
		lua_rawgeti(L, -12, 12);\
		theme->name.np_left_pix = lua_tonumber(L, -1);\
		lua_rawgeti(L, -13, 13);\
		theme->name.np_right_pix = lua_tonumber(L, -1);\
		lua_rawgeti(L, -14, 14);\
		theme->name.add_x = lua_tonumber(L, -1);\
		lua_rawgeti(L, -15, 15);\
		theme->name.add_y = lua_tonumber(L, -1);\
		lua_rawgeti(L, -16, 16);\
		theme->name.add_w = lua_tonumber(L, -1);\
		lua_rawgeti(L, -17, 17);\
		theme->name.add_h = lua_tonumber(L, -1);\
		theme->name.color = {1, 1, 1, 1};\
		lua_pop(L, 17);\
	} else if(lua_isuserdata(L, -1)) {\
		TextureRef* tex = (TextureRef*) luaL_checkudata(L, -1, "Texture");\
		theme->name = *tex;\
	}\
	lua_pop(L, 1);

void eui_register_api(lua_State* L) {
	BEGIN(EuiElement);

	BEGIN(EuiLayout);
	BEGIN_INDEX(EuiLayout);
		if(key[0] == '#') {
			EuiElement* child = elem->get_child<EuiElement>(std::string_view(key + 1, len - 1));
			if(!child) return 0;
			lua_rawgeti(L, LUA_REGISTRYINDEX, child->lua_obj_ref);
			return 1;
		}
		BIND(add_child) {
			lua_pushcfunction(L, [](lua_State* L) {
				Rc<EuiLayout> lay = *(Rc<EuiLayout>*) luaL_checkudata(L, 1, "EuiLayout");
				CHECK_ELEMENT(child, 2);
				lay->add_child(child);
				return 0;
			});
			return 1;
		}
		BIND(remove_child) {
			lua_pushcfunction(L, [](lua_State* L) {
				Rc<EuiLayout> lay = *(Rc<EuiLayout>*) luaL_checkudata(L, 1, "EuiLayout");
				CHECK_ELEMENT(child, 2);
				lay->remove_child(child);
				return 0;
			});
			return 1;
		}
		BIND_FIELD_BOOL(horizontal);
	END_INDEX;
	BEGIN_NEWINDEX(EuiLayout);
		BIND_SET_BOOL(horizontal);
	END_NEWINDEX;
	lua_register(L, "EuiLayout", [](lua_State* L) {
		EuiLayout* elem = new EuiLayout(lua_isboolean(L, 1) && lua_toboolean(L, 1), lua_isboolean(L, 2) && lua_toboolean(L, 2));
		new(lua_newuserdata(L, sizeof(void*))) Rc(elem);
		luaL_setmetatable(L, "EuiLayout");
		lua_pushvalue(L, -1);
		elem->lua_obj_ref = luaL_ref(L, LUA_REGISTRYINDEX);
		return 1;
	});

	BEGIN(EuiLabel);
	BEGIN_INDEX(EuiLabel);
		BIND_FIELD_STR(text);
	END_INDEX;
	BEGIN_NEWINDEX(EuiLabel);
		BIND_SET_STR(text);
	END_NEWINDEX;
	lua_register(L, "EuiLabel", [](lua_State* L) {
		EuiLabel* elem = new EuiLabel(luaL_checkstring(L, 1), lua_isstring(L, 2)? lua_tostring(L, 2): "");
		new(lua_newuserdata(L, sizeof(void*))) Rc(elem);
		luaL_setmetatable(L, "EuiLabel");
		lua_pushvalue(L, -1);
		elem->lua_obj_ref = luaL_ref(L, LUA_REGISTRYINDEX);
		return 1;
	});

	BEGIN(EuiButton);
	BEGIN_INDEX(EuiButton);
		BIND_FIELD_STR(text);
	END_INDEX;
	BEGIN_NEWINDEX(EuiButton);
		BIND_FIELD_STR(text);
	END_NEWINDEX;
	lua_register(L, "EuiButton", [](lua_State* L) {
		EuiButton* elem;
		if(lua_isfunction(L, 2)) {
			ClosureWrapper closure(L, 1, 2);
			elem = new EuiButton(luaL_checkstring(L, 1), 
				[closure](EuiButton&) {
					lua_rawgeti(closure.L, LUA_REGISTRYINDEX, closure.closure_key);
					lua_rawgeti(closure.L, LUA_REGISTRYINDEX, closure.self_key);
					lua_call(closure.L, 1, 0);
				}, lua_isstring(L, 3)? lua_tostring(L, 3): "");
		} else {
			elem = new EuiButton(luaL_checkstring(L, 1), 
				[](EuiButton&) {}, lua_isstring(L, 3)? lua_tostring(L, 3): "");
		}
		new(lua_newuserdata(L, sizeof(void*))) Rc(elem);
		luaL_setmetatable(L, "EuiButton");
		lua_pushvalue(L, -1);
		elem->lua_obj_ref = luaL_ref(L, LUA_REGISTRYINDEX);
		return 1;
	});

	BEGIN(EuiToggle);
	BEGIN_INDEX(EuiToggle);
		BIND_FIELD_STR(text);
		BIND_FIELD_BOOL(activated);
	END_INDEX;
	BEGIN_NEWINDEX(EuiToggle);
		BIND_SET_STR(text);
		BIND_SET_BOOL(activated);
	END_NEWINDEX;
	lua_register(L, "EuiToggle", [](lua_State* L) {
		EuiToggle* elem;
		if(lua_isfunction(L, 2)) {
			ClosureWrapper closure(L, 1, 2);
			elem = new EuiToggle(luaL_checkstring(L, 1), 
				[closure](EuiToggle&, bool activated) {
					lua_rawgeti(closure.L, LUA_REGISTRYINDEX, closure.closure_key);
					lua_rawgeti(closure.L, LUA_REGISTRYINDEX, closure.self_key);
					lua_pushboolean(closure.L, activated);
					lua_call(closure.L, 2, 0);
				}, lua_toboolean(L, 3), lua_isstring(L, 4)? lua_tostring(L, 4): "");
		} else {
			elem = new EuiToggle(luaL_checkstring(L, 1), 
				[](EuiToggle&, bool) {}, lua_toboolean(L, 3), lua_isstring(L, 4)? lua_tostring(L, 4): "");
		}
		new(lua_newuserdata(L, sizeof(void*))) Rc(elem);
		luaL_setmetatable(L, "EuiToggle");
		lua_pushvalue(L, -1);
		elem->lua_obj_ref = luaL_ref(L, LUA_REGISTRYINDEX);
		return 1;
	});

	BEGIN(EuiSeparator);
	BEGIN_INDEX(EuiSeparator);
	END_INDEX;
	BEGIN_NEWINDEX(EuiSeparator);
	END_NEWINDEX;
	lua_register(L, "EuiSeparator", [](lua_State* L) {
		EuiSeparator* elem = new EuiSeparator;
		new(lua_newuserdata(L, sizeof(void*))) Rc(elem);
		luaL_setmetatable(L, "EuiSeparator");
		lua_pushvalue(L, -1);
		elem->lua_obj_ref = luaL_ref(L, LUA_REGISTRYINDEX);
		return 1;
	});

	luaL_newmetatable(L, "EuiTheme");
	lua_pushcfunction(L, [](lua_State* L) {
		((EuiThemeRef*) luaL_checkudata(L, 1, "EuiTheme"))->destroy();
		return 0;
	}); lua_setfield(L, -2, "__gc");
	constexpr int a = sizeof(EuiElement);
	lua_register(L, "EuiTheme", [](lua_State* L) {
		EuiTheme* theme = new EuiTheme(*EuiTheme::default_theme.data);
		theme->rc = 0;
		if(lua_istable(L, 1)) {
			THEME_INT(font_size);
			THEME_INT(layout_margin_x);
			THEME_INT(layout_margin_y);
			THEME_INT(button_margin);
			THEME_INT(button_padding);
			THEME_INT(button_corner);
			THEME_COLOR(text_color);
			THEME_COLOR(button_color);
			THEME_COLOR(button_color_pressed);
			THEME_COLOR(button_color_hovered);
			THEME_HALO(button_halo);
			THEME_TEX(button_bg);
			THEME_TEX(toggle_bg);
			THEME_TEX(toggle_dot);
			THEME_TEX(window_bg);
		}
		new(lua_newuserdata(L, sizeof(void*))) EuiThemeRef(theme);
		luaL_setmetatable(L, "EuiTheme");
		return 1;
	});

	BEGIN(EuiScrollable);
	BEGIN_INDEX(EuiScrollable);
		if(key[0] == '#') {
			EuiLayout* lay = elem->elem->as_layout();
			if(!lay) {
				return 0;
			}
			EuiElement* child = lay->get_child<EuiElement>(std::string_view(key + 1, len - 1));
			if(!child) return 0;
			lua_rawgeti(L, LUA_REGISTRYINDEX, child->lua_obj_ref);
			return 1;
		}
		BIND(elem) {
			if(elem->elem) {
				lua_rawgeti(L, LUA_REGISTRYINDEX, elem->elem->lua_obj_ref);
			} else {
				lua_pushnil(L);
			}
			return 1;
		}
	END_INDEX;
	BEGIN_NEWINDEX(EuiScrollable);
		BIND(elem) {
			CHECK_ELEMENT(child, 3);
			elem->elem = child;
			child->parent = elem.data;
			elem->invalidate();
			return 0;
		}
	END_NEWINDEX;
	lua_register(L, "EuiScrollable", [](lua_State* L) {
		EuiScrollable* elem = new EuiScrollable(lua_isboolean(L, 1) && lua_toboolean(L, 1));
		new(lua_newuserdata(L, sizeof(void*))) Rc(elem);
		luaL_setmetatable(L, "EuiScrollable");
		lua_pushvalue(L, -1);
		elem->lua_obj_ref = luaL_ref(L, LUA_REGISTRYINDEX);
		return 1;
	});
}
