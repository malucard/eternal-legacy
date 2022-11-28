function bgm_resolve(name)
	return "et:/user/e17x/BGM.afs:" .. name
end

E17XTitleScreen = class(VnScreen) {
	trans_in = TRANS_FADE_IN;
	trans_in_time = 0.5;
	trans_in_done = function(self)
		if self.show_logos then
			trans_start(TRANS_WAIT_IN, function()
				trans_start(TRANS_FADE_OUT, function()
					self.bg = g_load_texture(vn_source_root .. "img/logo/title_logo_01.png")
					trans_start(TRANS_FADE_IN, function()
						trans_start(TRANS_WAIT_IN, function()
							trans_start(TRANS_FADE_OUT, function()
								self.show_logos = false
								self.time_opened = sys_millis() + TRANS_DURATION
								self.bg = g_load_texture(vn_source_root .. "SystemData.afs:title_bg.png")
								trans_start(TRANS_FADE_IN)
								self.bgm = a_play(bgm_resolve("bgm21"), true)
							end, 0.5)
						end, 1)
					end, 0.5)
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
			self.bg = g_load_texture(vn_source_root .. "img/logo/title_logo_02.png")
			self.bgm = nil
		else
			self.bg = g_load_texture(vn_source_root .. "SystemData.afs:title_bg.png")
			self.bgm = a_play(vn_find_res("bgm", "bgm21"), true)
		end
	end;

	unload = function(self)
		self.bg = nil
		if self.bgm then
			a_destroy(self.bgm)
			self.bgm = nil
		end
	end;

	input = function(self, inp)
		if inp.pressed then
			if inp:is(INPUT_OK) then
				if self.show_logos then
					while self.show_logos do
						trans_skip()
					end
				else
					vn_switch_screen(E17XGameScreen())
				end
			elseif inp:is(INPUT_BACK) then
				sys_close_game()
			end
		end
	end;

	draw = function(self)
		if self.show_logos then
			g_draw_quad(self.bg, 0, 0, -1, g_height, G_CENTER, G_CENTER)
		else
			g_draw_quad_crop(self.bg, 0, 0, -1, g_height, 0, 0, 1280, 719.5, G_TOP_LEFT, G_TOP_LEFT)
			g_draw_quad_crop(self.bg, 45, -80, -2, -2, 650, 723 + 40, 340, 40, G_BOTTOM_LEFT, G_BOTTOM_LEFT)
		end
		--self.ui:draw(0, 0, g_width, g_height)
	end;
}