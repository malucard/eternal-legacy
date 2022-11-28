lib_include("vn")
lib_include("transition")
dofile("core/decparser.lua")
dofile("core/game_screen.lua")
dofile("core/title_screen.lua")
dofile("core/assets.lua")

local function prep_screen()
	g_set_min_aspect(4, 3)
	if vn_get_setting("unlock_aspect") then
		g_set_max_aspect(16, 9)
	else
		g_set_max_aspect(4, 3)
	end
	g_set_virtual_size(-1, 600)
end

function _load()
	sys_set_input(true)
	vn_source_root = "et:/user/n7/"
	vn_load({
		prep_screen = prep_screen,
		start_screen = N7TitleScreen,
		game_screen = N7GameScreen,
	})
	vn_add_res_finder("bgm", "ja", function(name)
		if name == "infinity" then
			return "pkg:/music/infinity.ogg"
		end
		local ost = vn_get_setting("ost")
		name = tostring(name)
		if ost == "ps2" then
			return (#name == 1 and "pkg:/music/ps2/track_0" or "pkg:/music/ps2/track_") .. name .. ".ogg"
		elseif ost == "pc" then
			return (#name == 1 and "pkg:/music/pc/track_0" or "pkg:/music/pc/track_") .. name .. ".ogg"
		end
	end)
	vn_add_res_path("bg", vn_source_root .. "bg.dat:", ".cps", "ja", "lower")
	vn_add_res_path("chara", vn_source_root .. "chara.dat:", ".cps", "ja", "lower")
	vn_add_res_path("se", vn_source_root .. "se.dat:", ".wav", "ja", "upper")
	vn_add_res_path("voice", vn_source_root .. "wave.dat:", ".waf", "ja", "upper")
	vn_add_res_path("script", "pkg:/scriptja/", ".dec", "ja", "lower")
	vn_add_res_path("script", "pkg:/scripten/", ".dec", "en-us", "lower")
	vn_add_res_path("script", "pkg:/scriptpt/", ".dec", "pt-br", "lower")
	vn_add_res_path("script", "pkg:/scriptzhcn.dat:", ".scr", "zh-cn", "upper")
	vn_add_res_path("script", "pkg:/scriptzhtw.dat:", ".scr", "zh-tw", "upper")
--	vn_add_lang("pkg:/menu/ja.txt", "ja")
--	vn_add_lang("pkg:/menu/en.txt", "en-us")
--	vn_add_lang("pkg:/menu/pt.txt", "pt")
	vn_make_toggle("unlock_aspect", false)
	vn_make_option("bg_fit", "zoom", true)
	vn_make_option("bg_fit", "keep")
	vn_make_option("bg_fit", "stretch")
	vn_make_toggle("fast_forward_mode", false, "read", "all")
	vn_make_toggle("instant_skip_mode", false, "read", "all")
	vn_make_option("ost", "ps2", true)
	vn_make_option("ost", "pc")
	vn_make_num_setting("dcstyle", 1, 15)
	print("res: " .. vn_find_res("bgm", "15"))
	vn_add_parser(DecParser)
	--vn_add_parser(SnrParser)
end

_frame = vn_frame
_input = vn_input
_cursor = vn_cursor
_resize = vn_resize
_close = vn_close

--[[

local bg, sprite
function _load()
	local bglnk = fs_archive("lnk/bg.dat")
	bglnk:select("ev_ha02a.cps")
	bg = g_load_texture(bglnk)
	local chara = fs_archive("lnk/chara.dat")
	chara:select("ha01aa.cps")
	sprite = g_load_texture(chara)
end
]]