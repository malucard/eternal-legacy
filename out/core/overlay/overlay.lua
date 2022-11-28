dofile("et:/core/overlay/settings.lua")
local showing_overlay = false

local opened_at
overlay_layout = EuiLayout(false)
overlay_layout:add_child(EuiLabel("System Menu"))
overlay_layout:add_child(EuiButton("Return", function()
	opened_at = sys_millis()
	showing_overlay = false
end))
overlay_layout:add_child(EuiLabel("", "game_name"))
overlay_layout:add_child(EuiLabel("", "fps_counter"))
overlay_layout:add_child(EuiSeparator())
overlay_layout:add_child(EuiToggle("VSync", function(self, activated)
	sys_set_vsync(activated)
end, sys_get_vsync()))
overlay_layout:add_child(EuiToggle("Test KO1", function(self, activated)
	test_model = activated
end))
overlay_layout:add_child(EuiButton("Show logs", function(self)
	sys_show_logs()
end))
overlay_layout:add_child(EuiButton("Exit to launcher", function(self)
	log_info("game close requested in overlay, exiting to launcher")
	load_game("core", "launcher.lua", "launcher", "Launcher")
end))
overlay_layout:add_child(EuiButton("Exit", function(self)
	sys_exit()
end))
overlay_layout:add_child(EuiSeparator())
overlay_layout:add_child(EuiLabel("", "gfx_vendor"))
overlay_layout:add_child(EuiLabel("", "gfx_renderer"))
overlay_layout:add_child(EuiLabel("", "gfx_version"))
overlay_layout:add_child(EuiButton("What!", function(self) log_info("jojo!!") end))
overlay_layout:add_child(EuiButton("What!", function(self) end))
overlay_layout:add_child(EuiButton("What!", function(self) end))
overlay_layout:add_child(EuiButton("What!", function(self) end))
overlay_layout:add_child(EuiButton("What!", function(self) end))
overlay_layout:add_child(EuiButton("What!", function(self) end))
overlay_layout:add_child(EuiButton("What!", function(self) end))
overlay_layout:add_child(EuiButton("What!", function(self) end))
overlay_layout:add_child(EuiButton("What!", function(self) end))
overlay_layout:add_child(EuiButton("What!", function(self) end))
overlay_layout:add_child(EuiButton("What!", function(self) end))
overlay_layout:add_child(EuiButton("Last", function(self) end))

local overlay_scrollable = EuiScrollable(true)
overlay_scrollable.elem = overlay_layout
overlay_layout = overlay_scrollable

--overlay_layout = EuiLayout(false, true)
--overlay_layout:add_child(EuiButton("セーブ", function() end))
--overlay_layout:add_child(EuiButton("ロード", function() end))
--overlay_layout:add_child(EuiButton("クイックセーブ", function() end))
--overlay_layout:add_child(EuiButton("クイックロード", function() end))
--overlay_layout:add_child(EuiButton("オプション", function() end))
--overlay_layout:add_child(EuiButton("操作説明", function() end))
--overlay_layout:add_child(EuiButton("マップ", function() end))
--overlay_layout:add_child(EuiButton("ゲームをやめる", function() end))
--overlay_layout:add_child(EuiButton("もどる", function() end))

local time = 1000 * 8 / 60

function overlay_frame()
	if not showing_overlay then
		if opened_at then
			if sys_millis() - opened_at >= time then
				opened_at = nil
				return
			end
		else
			return
		end
	elseif not opened_at then
		opened_at = sys_millis()
		return
	end
	if g_real_height <= 320 then
		g_set_virtual_size(g_real_width * 2, g_real_height * 2)
	elseif g_real_height <= 480 then
		g_set_virtual_size(math.floor(g_real_width * 1.5), math.floor(g_real_height * 1.5))
	end
	overlay_layout["#gfx_vendor"].text = sys_gfx_vendor
	overlay_layout["#gfx_version"].text = sys_gfx_version
	overlay_layout["#gfx_renderer"].text = sys_gfx_renderer
	overlay_layout["#game_name"].text = "Running " .. (CUR_GAME and CUR_GAME.GAME_SHORT_NAME or "unknown")
	overlay_layout["#fps_counter"].text = sys_fps .. " fps, " .. sys_draw_calls .. " draw calls, " .. sys_polys .. " polys"
	local w = 200 * g_height / 600 * 2
	local x
	if showing_overlay then
		x = cerp(-w, 0, clamp(sys_millis() - opened_at, 0, time) / time)
	else
		x = cerp(0, -w, clamp(sys_millis() - opened_at, 0, time) / time)
	end
	g_push()
	g_translate(x, 0)
	overlay_layout:draw(0, 0, w, g_height)
	g_pop()
	--draw_settings_menu()
end

local cursor_x, cursor_y = 0, 0

function overlay_cursor(x, y, dragging)
	if not showing_overlay then
		return false
	end
	if g_real_height <= 320 then
		g_set_virtual_size(g_real_width * 2, g_real_height * 2)
	elseif g_real_height <= 480 then
		g_set_virtual_size(g_real_width * 1.5, g_real_height * 1.5)
	end
	local w = 200 * g_height / 600 * 2
	cursor_x, cursor_y = g_cursor_to_virtual(x, y)
	overlay_layout:on_cursor(cursor_x, cursor_y, cursor_x < w, dragging)
	return true
end

function overlay_input(inp)
	if inp:is_down(INPUT_SYS_MENU) then
		showing_overlay = not showing_overlay
		opened_at = sys_millis()
		return true
	elseif not showing_overlay then
		return false
	elseif inp:is_down(INPUT_BACK) then
		showing_overlay = false
		opened_at = sys_millis()
	end
	if g_real_height <= 320 then
		g_set_virtual_size(g_real_width * 2, g_real_height * 2)
	elseif g_real_height <= 480 then
		g_set_virtual_size(g_real_width * 1.5, g_real_height * 1.5)
	end
	local cx, cy = g_cursor_to_virtual(inp.x, inp.y)
	overlay_layout:on_input(inp.key, inp.pressed, cx, cy)
	return true
end

function overlay_invalidate()
	overlay_layout:invalidate()
	settings_menu:invalidate()
end