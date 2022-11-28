VnTitleScreen = class(VnScreen) {
	_init = function(self, opt)
		self.opt = opt
	end;
	load = function(self)
		self.loaded_bg = self.opt.load_bg(self)
	end;
	unload = function(self)
		self.loaded_bg = nil
	end;
	input = function(self, inp)
		if inp.pressed then
			if inp:is(INPUT_OK) then
				vn_switch_screen(E17GameScreen())
			elseif inp:is(INPUT_BACK) then
				sys_close_game()
			end
		end
	end;
	draw = function(self)
		g_draw_quad(self.loaded_bg, 0, 0, -1, g_height, G_CENTER, G_CENTER)
	end;
}