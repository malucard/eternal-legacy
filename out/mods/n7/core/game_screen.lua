local screen_effects = {
	quake = function(draw, data)
		--math.random() * 
		if data.time and data.time > 0 and sys_secs() - data.start > data.time then
			draw()
			return true
		end
		g_push()
		g_translate(oscillate(data.speed_x, -data.x, data.x), coscillate(data.speed_y, -data.y, data.y))
		draw()
		g_pop()
	end,

	flash = function(draw, data)
		draw()
		local t = sys_secs() - data.start
		if data.time and t > data.time then
			return true
		end
		t = t % (data.in_time + data.wait + data.out_time)
		local a
		if t > data.in_time + data.wait then
			a = 1 - (t - data.in_time - data.wait) / data.out_time
		elseif t > data.in_time then
			a = 1
		else
			a = t / data.in_time
		end
		g_push()
		g_identity()
		g_push_color(data.r, data.g, data.b, data.a * a)
		g_draw_quad(g_white_tex(), 0, 0, g_width, g_height, G_CENTER, G_CENTER)
		g_pop_color()
		g_pop()
	end
}

N7DCFrame = class {
	default_kind = 0;
	style_count = 10;
	font_size = 24;

	load = function(self, style)
		self.tex = g_load_texture("res/dc/window" .. (style or 1) .. ".png")
		self.wait_tex = g_load_texture("res/dc/waiting_icon.png")
		self.midprint_tex = g_load_texture("res/dc/midprint_icon.png")
		self.choice_tex = g_load_texture("res/dc/choice_cursor.png")
		self.beam_tex = g_load_texture("res/dc/beam.png")
	end;

	draw_hourglass = function(self, msg, x, y)
		local time = sys_secs()
		if not self.cursor_time then self.cursor_time = time end
		time = time - self.cursor_time
		if msg.is_midprint then
			local frame = math.floor(time * 10) % 6
			if frame > 3 then frame = 6 - frame end
			g_draw_quad_crop(self.midprint_tex, x + 2, y, 32, 32, 0, frame * 32, 32, 32, G_TOP_LEFT, G_TOP_LEFT, "text_shadow")
		else
			time = time % 6.5
			local frame = 3
			local rot = 0
			if time < 0.25 then
				rot = time * 4 * math.pi
			elseif time < 1.75 then
				frame = 0
			elseif time < 3.25 then
				frame = 1
			elseif time < 4.75 then
				frame = 2
			else
				frame = 3
			end
			g_push()
			g_translate(x + 10 + 8, y + 15 + 16)
			g_rotate(rot)
			g_draw_quad_crop(self.wait_tex, -8, -16, 16, 32, 0, frame * 32, 16, 32, G_TOP_LEFT, G_TOP_LEFT, "text_shadow")
			g_pop()
		end
	end;

	draw = function(self, msg, kind)
		local style = vn_get_setting("dcstyle")
		if style ~= self.style then
			self:load(style)
		end
		if not kind then return end
		local text_info
		if kind == -1 then
			error()
			local x, y, w, h = g_calc(0, 0, g_width, 540, 1280 - 360, 360, G_CENTER, G_CENTER)
			text_info = g_draw_text(msg.text, x + 180 + ox, y + 24 + oy, w - 180 * 2, self.font_size, "text_shadow")
		else
			local x, y, w, h = g_calc(0, -16, 760, 194, 760, 194, G_CENTER_BOTTOM, G_CENTER_BOTTOM)
			if msg.speaker_name ~= "" and msg.speaker_name then
				g_draw_quad_crop(self.tex, x, y, 419, 55, 1281, 110, 419, 55, G_BOTTOM_LEFT)
				local tw = g_calc_text(msg.speaker_name, 800 - 32, 28).w
				g_draw_text(msg.speaker_name, x + 16, y - 48 + 16, tw, self.font_size, "text_shadow_", G_CENTER)
			end
			if japanese then
				x, y, w, h = (800 - 760) / 2, 600 - 194 - 16 + 32, 760, 194 - 32
				g_draw_nine_patch(self.tex, x, y, w, h, 1, 1, 94, 94, 32)
				x, y, w, h = 400 - 752 / 2 + 20, 600 - 196 + 32, 752 - 40, 182 - 24 - 32
			else
				x, y, w, h = (800 - 760) / 2, 600 - 194 - 16, 760, 194
				g_draw_nine_patch(self.tex, x, y, w, h, 1, 1, 94, 94, 32)
				x, y, w, h = 400 - 752 / 2 + 20, 600 - 196, 752 - 40, 182 - 24
			end
			local trail = 8
			local text = g_wrap_text(msg.text, w, self.font_size)
			local j = 1
			local off = msg.progress - trail
			j = 1 - off
			off = utf8.offset(text, clamp(math.floor(off), 1, utf8.len(text)))
			local cur_x, cur_y = x, y
			local line_height = g_text_line_height(self.font_size)
			for p, c in utf8.codes(text) do
				c = utf8.char(c)
				if c == '\n' then
					cur_y = cur_y + line_height
					cur_x = x + 16
				else
					g_push_color(1, 1, 1, 1 - j / trail)
					text_info = g_draw_text(c, cur_x, cur_y, -1, self.font_size, "text_shadow")
					cur_x = text_info.end_x
					g_pop_color()
					j = j + 1
				end
				if j > trail then
					goto done
				end
			end
			if not text_info then
				text_info = g_draw_text("", x, y, w, self.font_size, "text_shadow")
			end
			::done::
		end
		if msg.is_waiting then
			self:draw_hourglass(msg, text_info.end_x - 2, text_info.end_y - 8)
			--g_draw_quad_crop(self.wait_tex, text_info.end_x + 22, text_info.end_y + 22, 16, 32, 0, 0, 16, 32, G_CENTER, G_TOP_LEFT)
		else
			self.cursor_time = nil
		end
	end;
}

N7GameScreen = class(VnScreen) {
	trans_in = TRANS_FADE_IN;
	trans_out = TRANS_FADE_OUT;

	_init = function(self) end;
	
	load = function(self)
		self.frame = N7DCFrame()
		self.frame:load()
		self.skip_icons = g_load_texture("res/dc/skip.png")
	end;

	unload = function(self)
		self.bg = nil
	end;

	input = function(self, inp)
		if inp.pressed and not inp.repeated then
			if inp:is(INPUT_OK) then
				cur_vn.cur_executor:next()
			elseif inp:is(INPUT_BACK) then
				sys_close_game()
			end
		end
	end;

	skip_icon_alpha = 0.0;
	draw = function(self)
		local is_skipping
		if sys_is_down(INPUT_SKIP_INSTANT) then
			self.skip_icon_alpha = 2.0
			is_skipping = cur_vn.cur_executor:skip_to_unread()
		elseif sys_is_down(INPUT_SKIP) then
			self.skip_icon_alpha = 2.0
			is_skipping = cur_vn.cur_executor:fast_forward()
		end
		cur_vn.cur_executor:process()
		local bgm = cur_vn.cur_executor:bgm_if_changed()
		if bgm then
			if self.bgm_channel then
				a_destroy(self.bgm_channel)
			end
			print(vn_find_res("bgm", bgm))
			self.bgm_channel = a_play(vn_find_res("bgm", bgm), true)
		end
		local bg = cur_vn.cur_executor:get_bg()
		if bg.prev_src then
			local bg_img, w, h = vn_find_res("bg", bg.prev_src)
			if w and h then
				g_draw_quad(g_load_texture(bg_img), 0, 0, w, h, G_CENTER, G_CENTER)
			else
				g_draw(g_load_texture(bg_img), 0, 0, G_CENTER, G_CENTER)
			end
		end
		if bg.src then
			local bg_img, w, h = vn_find_res("bg", bg.src)
			local a = clamp((sys_millis() - bg.trans_start) / bg.trans_duration, 0, 1)
			g_push_color(1, 1, 1, a)
			if w and h then
				g_draw_quad(g_load_texture(bg_img), 0, 0, w, h, G_CENTER, G_CENTER)
			else
				g_draw(g_load_texture(bg_img), 0, 0, G_CENTER, G_CENTER)
			end
			g_pop_color()
		end
		local msg = cur_vn.cur_executor:get_msg()
		self.frame:draw(msg, 0, 0)
		if self.skip_icon_alpha > 0 then
			g_push_color(1, 1, 1, self.skip_icon_alpha)
			if is_skipping then
				local idx = math.floor(sys_millis() / 200) % 3
				g_draw_crop(self.skip_icons, 16, 16, 0, 32 * idx, 32, 32)
			else
				g_draw_crop(self.skip_icons, 16, 16, 0, 96, 32, 32)
			end
			g_pop_color()
			self.skip_icon_alpha = math.max(self.skip_icon_alpha - 4 * delta, 0)
		end
	end;
}