-- independent game file referenced in main.lua but not included

_G.GLOBAL_ROOT = "../"
lib_include("buttonhint")
lib_include("halo")
lib_include("transition")
local selected_game_i
local selected_game
local selected_game_prev
local audio_playing
local selected_bg
local selected_bg_prev
local games
local icons
local assets = {}
local trans

local function set_selected_game(game)
	if game == selected_game then return end
	selected_game_prev = selected_game
	selected_game = game
	selected_bg_prev = selected_bg
	log_info("et:/" .. selected_game._root .. selected_game.bg)
	log_info(fs_resolve("et:/" .. selected_game._root .. selected_game.bg))
	selected_bg = game.bg and g_load_texture("et:/" .. selected_game._root .. selected_game.bg)
	if audio_playing then
		a_destroy(audio_playing)
	end
	audio_playing = selected_game.snd and a_play(selected_game._root .. selected_game.snd, true)
	--a_speak(selected_game.name)
end

local video_playback
function _load()
	--video_playback = v_load("mods/e17x/xbox/video/OP00.wmv")
	_resize(g_real_width, g_real_height)
	if audio_playing then
		a_destroy(audio_playing)
		audio_playing = nil
	end
	selected_game_prev = nil
	selected_game = nil
	games = {}
	icons = {}
	for _, mod in pairs(sys_modules) do
		if mod.provides then
			if not mod.provides[1] then
				mod.provides = {mod.provides}
			end
			for _, p in ipairs(mod.provides) do
				if p.type == "game" then
					games[#games + 1] = p
					p._root = mod._root
					p._alpha = 0
					if p.icon then
						icons[p._root .. p.icon] = g_load_texture(p._root .. p.icon)
					end
				end
			end
		end
	end
	for i = 1, #games - 1 do
		local date = date_to_num(games[i].original_release)
		for i2 = i + 1, #games do
			local date2 = date_to_num(games[i2].original_release)
			if date2 < date then
				games[i], games[i2] = games[i2], games[i]
				date = date2
			end
		end
	end
	selected_game_i = 1
	set_selected_game(games[selected_game_i])
	trans = trans_start(TRANS_FADE_IN)
end

local vw, vh = -1, -1
local icon_stride = 360
local icon_width = 256
local halo_detail = 9
local halo_distance = 3
function _resize(rw, rh)
	halo_distance = 3
	if rh <= 360 then
		vw, vh = rw * 2, rh * 2
		halo_detail = -1
		halo_distance = 4
		icon_width = 256
		icon_stride = 272
	elseif g_real_height <= 600 then
		--vw, vh = rw * 2, rh * 2
		vw, vh = -1, -1
		halo_detail = 4
		icon_width = 200
		icon_stride = 220
	elseif g_real_height >= 720 then
		local scale = math.floor(g_real_height / 720) -- scale infinitely on each multiple of 720
		halo_detail = 6 * scale
		vw, vh = math.floor(rw / scale), math.floor(rh / scale)
		icon_width = 256
		icon_stride = 360
	else
		vw, vh = -1, -1
		halo_detail = 6
		--g_set_virtual_size(g_real_width, g_real_height)
	end
end

local model --= g_load_scene("et:/mods/e17x-sprites/Model.afs:TU1.vpk:Users/izutsu_sana/Desktop/VPK0926_FIX/TU1/TU1.vmx2")
_frame = trans_frame(function()
	g_set_virtual_size(vw, vh)
	local sw, sh = g_width, g_height
	if selected_bg_prev then
		if sw / sh > selected_bg_prev:aspect() then
			g_draw_quad(selected_bg_prev, 0, 0, sw, -1, G_BOTTOM_RIGHT, G_BOTTOM_RIGHT)
		else
			g_draw_quad(selected_bg_prev, 0, 0, -1, sh, G_BOTTOM_RIGHT, G_BOTTOM_RIGHT)
		end
	end
	if selected_game then
		g_push_color(1.0, 1.0, 1.0, selected_game._alpha)
		if selected_bg then
			if sw / sh > selected_bg:aspect() then
				g_draw_quad(selected_bg, 0, 0, sw, -1, G_BOTTOM_RIGHT, G_BOTTOM_RIGHT)
			else
				g_draw_quad(selected_bg, 0, 0, -1, sh, G_BOTTOM_RIGHT, G_BOTTOM_RIGHT)
			end
		end
		g_pop_color()
	end
	local width = g_width
	local count = #games
	local start_x = width / 2 - icon_stride * (#games - 1) / 2
	for i = 1, count do
		local game = games[i]
		if selected_game == game then
			game._alpha = lerp(game._alpha, 1, delta * 12)
			g_draw_text(game.name, 16, 8, -1, 18, {halo_alpha = 0.75, halo_detail = -1, shadow_distance = 2.5, shadow_detail = 1})
		else
			game._alpha = lerp(game._alpha, 0, delta * 12)
		end
		local x = start_x + icon_stride * (i - 1)
		if icons[game._root .. game.icon] then
			g_push_color(1, 1, 1, game._alpha * 0.5 + 0.5)
			g_draw_quad(icons[game._root .. game.icon], x, 0, icon_width, -1, G_CENTER, G_CENTER_LEFT,
				{halo_alpha = game._alpha * 0.5 * oscillate(2, 0, 1), halo_lightness = 1, halo_detail = -1, halo_distance = halo_distance * 1.5,
				shadow_alpha = game._alpha * 0.5, shadow_distance = 8, shadow_detail = 1})
			--[[hili_draw_shadow_halo(function(offx, offy)
				if offx == 0 and offy == 0 then
					g_pop_color()
					g_push_color(1.0, 1.0, 1.0, game._alpha * 0.5 + 0.5)
				end
				g_draw_quad(icons[game._root .. game.icon], x + offx, offy, icon_width, -1, G_CENTER, G_CENTER_LEFT)
			end, ]]
			g_pop_color()
			--g_push_color(1.0, 1.0, 1.0, game._alpha * 0.5 + 0.5)
			--g_draw_quad(icons[game._root .. game.icon], x, 0, icon_width, -1, G_CENTER, G_CENTER_LEFT)
			--g_pop_color()
		else
			g_draw_text(game.name, x, 200, 128, 16)
		end
	end
	g_draw_text("Eternal v" .. sys_prop("core_version"), -16, 8, -1, 18, {halo_alpha = 0.75, halo_detail = -1, shadow_distance = 2.5, shadow_detail = 1}, G_TOP_RIGHT, G_TOP_RIGHT)
	bh_draw({INPUT_OK, "Start", INPUT_SYS_MENU, "System Menu", INPUT_MORE, "Store", INPUT_BACK, "Exit"})
	--v_decode_video(video_playback)
	--v_decode_audio(video_playback)
	local vx, vy, vw, vh = g_calc_fit(1280, 720, 0, 0, g_width, g_height)
	--v_draw(video_playback, vx, vy, vw, vh)
	--g_draw(g_white_tex(), 0, 0, g_width, g_height)

	if not model then return end
	g_push()
	g_proj_perspective(math.rad(70), 16 / 9, 0.1, 1000)
	local model_light = {Diffuse = {1, 1, 1}, Ambient = {0.1, 0.1, 0.1}, AngleX = 0, AngleY = 0}
	g_identity()
	g_rotate(math.rad(model_light.AngleY), 0, 1, 0)
	g_rotate(math.rad(model_light.AngleX), 1, 0, 0)
	g_light(0, G_DIRECTIONAL_LIGHT, 0, 0, 1, model_light.Diffuse[1] / 4, model_light.Diffuse[2] / 4, model_light.Diffuse[3] / 4)
	g_identity()
	g_light_ambient(model_light.Ambient[1], model_light.Ambient[2], model_light.Ambient[3])
	g_translate(0, -8, -15)
	g_rotate(math.rad(170), 0, 1, 0)
	g_rotate(math.rad(-10), 1, 0, 0)
	g_draw_scene(model)
	g_proj_reset()
	g_pop()
end)

function _input(inp)
	if inp.pressed then
		if inp:is(INPUT_MORE) then
			trans = trans_start(TRANS_FADE_OUT, function()
				load_game("core/store", "store.lua", "store", "Store")
			end)
		elseif inp:is(INPUT_OK) then
			sys_set_input(false)
			trans = trans_start(TRANS_ENTER_OUT, function()
				sys_set_input(true)
				load_game(selected_game._root, selected_game.main, selected_game.id, selected_game.short_name or selected_game.name)
			end)
		elseif inp:is(INPUT_BACK) then
			sys_exit()
		elseif inp:is(INPUT_RIGHT) then
			selected_game_i = selected_game_i + 1
			if selected_game_i > #games then
				selected_game_i = 1
			end
			set_selected_game(games[selected_game_i])
		elseif inp:is(INPUT_LEFT) then
			selected_game_i = selected_game_i - 1
			if selected_game_i < 1 then
				selected_game_i = #games
			end
			set_selected_game(games[selected_game_i])
		end
	end
end

function _cursor(x, y, dragging)
	g_set_virtual_size(vw, vh)
	x, y = g_cursor_to_virtual(x, y)
	local width = g_width
	local icon_y = g_height * 0.5 - icon_width * 0.25
	local count = #games
	local start_x = width / 2 - icon_stride * (#games - 1) / 2
	for i = 1, count do
		local game = games[i]
		local icon_x = start_x + icon_stride * (i - 1) - icon_width * 0.5
		if x >= icon_x and x < icon_x + icon_width and y >= icon_y and y < icon_y + icon_width * 0.5 then
			selected_game_i = i
			set_selected_game(games[selected_game_i])
		end
	end
end

function _close()
end