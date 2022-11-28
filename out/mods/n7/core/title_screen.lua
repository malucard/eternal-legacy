--[[VnTitleScreen {
	logos = {"", ""},
	press_start_img = "",
	press_start_x = 0,
	press_start_y = 0,
	press_start_corner = 0,
	press_start_origin = 0,
	buttons = {
		"",

	},
}

VnScreenGraph {
	press_start = VnAcceptScreen {
		trans_in = TRANS_FADE_IN;
		trans_in_time = 0.5;
		display = 
	}
}]]

never7_theme = EuiTheme {
	font_size = 30;
	text_color = {0x77 / 255, 0x99 / 255, 0xbb / 255, 1};
	layout_margin_x = 34;
	layout_margin_y = 24;
	button_margin = 0;
	button_padding = 0;
	button_corner = G_CENTER_LEFT;
	button_halo = nil;
	button_bg = {g_load_texture("et:/res/n7/dc/beam.png"), 0, 32, 256, 32, 0, 0, 0, 0, 0, 0, 0, 0, -20, -12, 40, 26};
	button_color = {0xe0 / 255, 0xa0 / 255, 0xb0 / 255, 0};
	button_color_pressed = {0xe0 / 255 / 2, 0xa0 / 255 / 2, 0xb0 / 255 / 2, 1};
	button_color_hovered = {0xe0 / 255, 0xa0 / 255, 0xb0 / 255, 1};
	--toggle_bg = g_white_tex();
	--toggle_dot = g_white_tex();
	--toggle_dot_color = {0.23, 0.2, 0.25, 1};
	window_bg = {g_load_texture("et:/res/n7/dc/window10.png"), 1, 1, 94, 94, 44, 44, 44, 44, 32, 32, 32, 32, 0, 0, 0, 0};
}

pause_menu = EuiScrollable(true)
local pause_menu_lay = EuiLayout(false, false)
pause_menu_lay:add_child(EuiButton("セーブ", function() end))
pause_menu_lay:add_child(EuiButton("ロード", function() end))
pause_menu_lay:add_child(EuiButton("クイックセーブ", function() end))
pause_menu_lay:add_child(EuiButton("クイックロード", function() end))
pause_menu_lay:add_child(EuiButton("オプション", function() end))
pause_menu_lay:add_child(EuiButton("操作説明", function() end))
pause_menu_lay:add_child(EuiButton("マップ", function() end))
pause_menu_lay:add_child(EuiButton("ゲームをやめる", function() end))
pause_menu_lay:add_child(EuiButton("もどる", function() end))
pause_menu.elem = pause_menu_lay
pause_menu:propagate_theme(never7_theme)
--pause_menu = EuiButton("ゲームをやめる", function() end)

N7TitleScreen = class(VnScreen) {
	trans_in = TRANS_FADE_IN;
	trans_in_time = 0.5;
	trans_in_done = function(self)
		if self.show_logos then
			trans_start(TRANS_WAIT_IN, function()
				trans_start(TRANS_FADE_OUT, function()
					self.show_logos = false
					self.time_opened = sys_millis() + TRANS_DURATION
					self.bgs = {
						g_load_texture("res/dc/title1.png"),
						g_load_texture("res/dc/title2.png"),
						g_load_texture("res/dc/title3.png"),
						g_load_texture("res/dc/title4.png")
					}
					self.bgs[5] = self.bgs[3]
					self.bgs[6] = self.bgs[4]
					self.bgs[7] = g_white_tex()
					self.button_last_highlighted = {0, 0, 0, 0, 0, 0, 0}
					self.logo = g_load_texture("res/dc/logo.png")
					self.buttons = g_load_texture("res/dc/title_buttons.png")
					self.grayed = g_load_texture("res/special/grayed.png")
					self.yuka = g_load_texture("res/special/yuka.png")
					self.haruka = g_load_texture("res/special/haruka.png")
					self.saki = g_load_texture("res/special/saki.png")
					self.kurumi = g_load_texture("res/special/kurumi.png")
					self.izumi = g_load_texture("res/special/izumi.png")
					self.selected = 0
					trans_start(TRANS_FADE_IN)
					self.bgm = a_play("music/pc/track_21.ogg", true)
				end, 0.5)
			end, 1)
		end
	end;
	trans_out = TRANS_FADE_OUT;

	_init = function(self)
		self.show_logos = true
	end;
	
	load = function(self)
		if self.show_logos then
			self.bg = g_load_texture("res/kid_logo.png")
			self.bgm = nil
		else
			self.logo = g_load_texture("res/dc/logo.png")
			self.bgm = a_play("music/pc/track_21.ogg", true)
		end
	end;

	unload = function(self)
		self.bg = nil
		if self.bgm then
			a_destroy(self.bgm)
			self.bgm = nil
		end
	end;

	resize = function(self)
		pause_menu:invalidate()
	end;

	input = function(self, inp)
		--if pause_menu:on_input(inp.key, inp.pressed, inp.x - 16, inp.y - 16) then return end
		if inp.pressed then
			if inp:is(INPUT_OK) then
				if self.show_logos then
					while self.show_logos do
						trans_skip()
					end
				else
					vn_start_game("op")
				end
			elseif inp:is(INPUT_BACK) then
				sys_close_game()
			elseif inp:is(INPUT_UP) then
				self.selected = self.selected - 1
				if self.selected == -1 then
					self.selected = 6
				end
			elseif inp:is(INPUT_DOWN) then
				self.selected = self.selected + 1
				if self.selected == 7 then
					self.selected = 0
				end
			end
		end
	end;

	buttons_x = 0;
	buttons_w = 0;
	buttons_y = 0;
	buttons_stride = 0;
	buttons_count = 6;
	cursor = function(self, x, y, dragging)
		if x >= self.buttons_x and x < self.buttons_x + self.buttons_w and y >= self.buttons_y then
			local id = math.floor((y - self.buttons_y) / self.buttons_stride)
			if id < self.buttons_count then
				self.selected = id
			end
		end
		pause_menu:on_cursor(x - 16, y - 16, true, dragging)
	end;

	draw = function(self)
		if self.show_logos then
			local w, h, x, y = self.bg:size()
			x, y, w, h = g_calc_crop(w, h, 0, 0, g_width, g_height)
			g_draw_quad(self.bg, x, y, w, h, G_TOP_LEFT, G_TOP_LEFT)
		elseif self.bgs then
			local time = sys_millis() - self.time_opened
			local bgt = time % 14000
			local bgstep = math.floor(bgt / 2000) + 1
			local bgp = bgt % 2000 / 2000.0
			g_draw_quad(self.bgs[bgstep], 0, 0, g_width, g_width * 480 / 640, G_CENTER, G_CENTER)
			g_push_color(1, 1, 1, bgp)
			bgstep = bgstep + 1
			g_draw_quad(self.bgs[bgstep > 7 and 1 or bgstep], 0, 0, g_width, g_width * 480 / 640, G_CENTER, G_CENTER)
			g_pop_color()
			g_draw_quad(self.grayed, 0, 0, -1, g_height, G_TOP_RIGHT, G_TOP_RIGHT)
			g_draw_quad(self.yuka, 0, 0, -1, g_height, G_TOP_RIGHT, G_TOP_RIGHT)
			g_draw_quad(self.haruka, 0, 0, -1, g_height, G_TOP_RIGHT, G_TOP_RIGHT)
			g_draw_quad(self.saki, 0, 0, -1, g_height, G_TOP_RIGHT, G_TOP_RIGHT)
			--g_draw_quad(self.kurumi, 0, 0, -1, g_height, G_TOP_RIGHT, G_TOP_RIGHT)
			--g_draw_quad(self.izumi, 0, 0, -1, g_height, G_TOP_RIGHT, G_TOP_RIGHT)
			local chars_w = 300 * g_height / 480
			local menu_w = 512 * g_height / 480
			local menu_x = (g_width - chars_w) / 2
			g_push()
			g_translate(menu_x, g_height / 4 + 32 * g_height / 480)
			if g_width - chars_w < menu_w then
				local menu_h = g_height * ((g_width - chars_w) / menu_w)
				g_scale(menu_h / g_height)
			end
			g_draw_quad(self.logo, 0, 0, -1, 256 * g_height / 480, G_CENTER, G_TOP_LEFT)
			g_pop()
			self.buttons_x = menu_x - 66 * g_height / 480
			self.buttons_w = 132 * g_height / 480
			self.buttons_y = 270 * g_height / 480 --g_height * 0.5 + 32 * g_height / 480
			self.buttons_stride = 24 * g_height / 480
			for i = 0, self.buttons_count - 1 do
				local j = i * 24
				if i > 4 then
					j = j + 88 + 9 * 24
				end
				local p = i / (self.buttons_count - 1)
				if self.selected == i then
					self.button_last_highlighted[i + 1] = time
					g_push_color(lerp(235, 250, p) / 255, lerp(115, 20, p) / 255, lerp(15, 0, p) / 255, 1)
				elseif time - self.button_last_highlighted[i + 1] < 500 then
					local tp = (time - self.button_last_highlighted[i + 1]) / 500
					local r = lerp(lerp(235, 250, p), 15, tp) / 255
					local g = lerp(lerp(115, 20, p), lerp(75, 20, p), tp) / 255
					local b = lerp(lerp(15, 0, p), lerp(255, 105, p), tp) / 255
					g_push_color(r, g, b, 1.0)
				else
					g_push_color(15.0 / 255, lerp(75, 20, p) / 255, lerp(1.0, 105 / 255, p), 1)
				end
				g_draw_quad_crop(self.buttons, menu_x, (32 + 24 * i) * g_height / 480, -1, 24 * g_height / 480, 0, j, 256, 24, G_CENTER_TOP, G_CENTER_LEFT)
				g_pop_color()
			end
		end
		local min_w, min_h = pause_menu:get_min_size(g_width, g_height)
		pause_menu:draw(16, 16, min_w, min_h)
	end;
}