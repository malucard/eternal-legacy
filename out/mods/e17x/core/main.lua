lib_include("vn")
lib_include("transition")
dofile("core/parser.lua")
dofile("core/game_screen.lua")
dofile("core/title_screen.lua")

local function prep_screen()
	g_set_min_aspect(4, 3)
	g_set_max_aspect(16, 9)
	g_set_virtual_size(-1, 600)
end

function _load()
	sys_set_input(true)
	vn_load({
		prep_screen = prep_screen,
		start_screen = E17XTitleScreen
	})
	--elem_load("bgm")
end

_frame = vn_frame
_input = vn_input
_cursor = vn_cursor
_close = vn_close
