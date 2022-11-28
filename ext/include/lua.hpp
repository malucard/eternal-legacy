extern "C" {
#ifdef REGULAR_LUA
#include "luainclude/lua.h"
#include "luainclude/lauxlib.h"
#include "luainclude/lualib.h"
#else
#include "luajitinclude/lua.h"
#include "luajitinclude/lauxlib.h"
#include "luajitinclude/lualib.h"
#include "luajit.h"
#endif
}
