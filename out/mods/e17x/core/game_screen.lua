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

E17XFrame = class {
	default_kind = nil;

	load = function(self, style)
		if not self.tex then
			self.tex = g_load_texture("et:/user/e17x/SystemData.afs:meswin.png")
			self.choice_tex = g_load_texture("et:/user/e17x/SystemData.afs:choicea.png")
		end
	end;

	draw = function(self, msg, kind, style)
		if not kind then return end
		local text_info
		if kind == "nvl" then
			local x, y, w, h = g_calc(0, 0, g_width, 540, 1280 - 360, 360, G_CENTER, G_CENTER)
			text_info = g_draw_text(msg.text, x + 180 + ox, y + 24 + oy, w - 180 * 2, 28, "text_shadow")
		else
			local x, y, w, h = g_calc(0, 0, g_width, 185, 1280, 185, G_CENTER_BOTTOM, G_CENTER_BOTTOM)
			if msg.speaker_name ~= "" and msg.speaker_name then
				g_draw_quad_crop(self.tex, x, y, 419, 55, 1281, 110, 419, 55, G_BOTTOM_LEFT)
				local tw = g_calc_text(msg.speaker_name, 419, 28).w
				g_draw_text(msg.speaker_name, x + 128, y - 48 + 16, tw, 28, "text_shadow", G_CENTER)
			end
			g_draw_quad_crop(self.tex, x, y, w, h, 0, 372, 1280, 185)
			text_info = g_draw_text(msg.text, x + 180, y + 24, w - 360, 28, "text_shadow")
		end
		if msg.progress == 1.0 then
			g_push()
			g_translate(text_info.end_x + 22, text_info.end_y + 22)
			g_rotate(sys_millis() / 500.0 % (math.pi * 2))
			g_draw_quad_crop(self.choice_tex, 0, 0, 32, 26, 856, 660, 32, 26, G_CENTER, G_TOP_LEFT)
			g_pop()
		end
	end;
}

local function loader(src)
	local tex = g_load_texture("et:/user/e17x/CG.afs:" .. src .. ".png")
	local w, h = tex:size()
	if h == 272 then
		return tex, 1280, 720
	end
	return tex, w, h
end

GameScreen = class(VnScreen) {
	trans_in = TRANS_FADE_IN;
	trans_out = TRANS_FADE_OUT;

	_init = function(self) end;
	
	load = function(self)
		self.frame = E17XFrame()
		self.frame:load()
		self.parser = Parser("script/OP00")
		self.parser:update()
		self.executor = self.parser.script:execute()
		--VnExecutor(self, parse_script, fs_file("script_en/OP00"):read_string(), loader, E17XboxFrame(), screen_effects)
		self.executor:process()
		self.models = {}
	end;

	unload = function(self)
		self.bg = nil
	end;

	input = function(self, inp)
		if inp.pressed then
			if inp:is(INPUT_OK) then
				self.executor:next()
			elseif inp:is(INPUT_BACK) then
				sys_close_game()
			end
		end
	end;

	draw_model = function(self, src, preset)
		src = src:upper()
		if src:find("SO") or src:find("YU2") then return nil end
		local model = self.models[src]
		if not model then
			if src:find("MU") then
				model = g_load_scene("et:/user/e17x/Model.afs:" .. src .. ".vpk:Users/izutsu_sana/Desktop/VPK0902/" .. src .. "/" .. src .. ".vmx2")
			else
				model = g_load_scene("et:/user/e17x/Model.afs:" .. src .. ".vpk:Users/izutsu_sana/Desktop/VPK0926_FIX/" .. src .. "/" .. src .. ".vmx2")
			end
			--model = g_load_scene("mods/e17x-sprites/models/" .. src .. "/" .. src .. ".vmx2")
			self.models[src] = model
		end
		--local model = g_load_scene(fs_resolve("pkg:/sprite/" .. src .. "/" .. src .. ".vmx2"))
		if not model then return end
		g_push()
		g_proj_perspective(math.rad(70), 16 / 9, 0.1, 1000)
		--g_light(0, G_POINT_LIGHT, 0, 0, 0, 15)
		local model_light = self.executor.vars.model_light
		g_identity()
		g_rotate(math.rad(model_light.AngleY), 0, 1, 0)
		g_rotate(math.rad(model_light.AngleX), 1, 0, 0)
		--g_translate(preset[])
		--g_light(0, G_DIRECTIONAL_LIGHT, 0, -1, 0, model_light.Diffuse[1], model_light.Diffuse[2], model_light.Diffuse[3])
		g_light(0, G_DIRECTIONAL_LIGHT, 0, 0, 1, model_light.Diffuse[1] / 4, model_light.Diffuse[2] / 4, model_light.Diffuse[3] / 4)
		g_identity()
		g_light_ambient(model_light.Ambient[1], model_light.Ambient[2], model_light.Ambient[3])
		g_translate(preset[1] * 8, preset[2] - 12, preset[3] - 10)
		--g_rotate(math.rad(180), 0, 1, 0)
		g_rotate(math.rad((preset[7] or 0) + 180), 0, 1, 0)
		g_rotate(math.rad(preset[8] or 0), 0, 0, 1)
		g_rotate(math.rad(preset[6] or 0), 1, 0, 0)
		--g_scale(preset[4], preset[5], 1)
		g_draw_scene(model)
		g_proj_reset()
		g_pop()
	end;

	draw = function(self)
		if sys_is_down(INPUT_SKIP) then
			self.executor:fast_forward()
		end
		--self.executor:next("frame")
		--self.executor:draw()
		self.executor:process()
		local bg = self.executor:get_bg()
		if bg.prev_src then
			local bg_img, w, h = loader(bg.prev_src)
			g_draw_quad(bg_img, 0, 0, w, h, G_CENTER, G_CENTER)
		end
		if bg.src then
			local bg_img, w, h = loader(bg.src)
			local a = clamp((sys_millis() - bg.trans_start) / bg.trans_duration, 0, 1)
			g_push_color(1, 1, 1, a)
			g_draw_quad(bg_img, 0, 0, w, h, G_CENTER, G_CENTER)
			g_pop_color()
		end
		local msg = self.executor:get_msg()
		self.frame:draw(msg, 0, 0)
	end;
}
