local RUNNING = "running" -- will execute next command on next frame
local DELAYING = "delaying" -- will execute next command when sys_millis() reaches self.delay_done
local CALLBACK = "callback" -- will execute next command when self.awaiting_callbacks is 0
local WAITING = "waiting" -- will execute next command on next("advance")
local WAITING_SFX = "waiting_sfx" -- will execute next command when no sfx is still playing
local HALTED = "halted" -- will not execute

VnExecutor = class {
	state = RUNNING;
	cur_speaker_name = "";
	cur_text = "";
	pos = 1;
	awaiting_callbacks = 0;
	is_midprint = false;
	clear_after = false;
	
	_init = function(self, game_screen, parser, data, loader, frame, screen_effects)
		self.game_screen = game_screen
		self.parser = parser
		self.script = parser(data, game_screen)
		self.loader = loader
		self.frame = frame
		self.frame_kind = frame.default_kind
		frame:load()
		self.screen_effects = screen_effects
		-- functions to call every frame, removed when returning true
		-- mostly used for animations and transitions
		self.callbacks = {}
		self.imgs = {}
		self.active_effects = {}
		self.active_sfx = {}
		self.vars = {}
	end;

	destroy = function(self)
		if self.bgm then
			a_destroy(self.bgm)
			self.bgm = nil
		end
		if self.voice_playing then
			a_destroy(self.voice_playing)
			self.voice_playing = nil
		end
	end;

	next = function(self, kind)
		local state = self.state
		if state == WAITING then
			if kind == "advance" or kind == "fast" or kind == "instant" then
				if self.clear_after then
					self.cur_speaker_name = ""
					self.cur_text = ""
				end
				self:next_cmd()
			end
		elseif state == DELAYING then
			if kind == "frame" then
				if sys_millis() >= self.delay_done then
					self:next_cmd()
				end
			elseif kind == "fast" or kind == "instant" then
				for i = #self.callbacks, 1, -1 do
					if self.callbacks[i](true) then
						table.remove(self.callbacks, i)
					end
				end
				self.delay_done = 0
				self:next_cmd()
			end
		elseif state == RUNNING then
			if kind == "frame" then
				self:next_cmd()
			end
		elseif state == CALLBACK then
			if kind == "frame" then
				for i = #self.callbacks, 1, -1 do
					if self.callbacks[i]() then
						table.remove(self.callbacks, i)
					end
				end
			elseif kind == "fast" or kind == "instant" then
				for i = #self.callbacks, 1, -1 do
					if self.callbacks[i](true) then
						table.remove(self.callbacks, i)
					end
				end
			end
			if self.awaiting_callbacks == 0 then
				self:next_cmd()
			end
		elseif state == WAITING_SFX then
			self.state = RUNNING
			for i = #self.active_sfx, 1, -1 do
				if a_playing(self.active_sfx[i].audio) then
					if kind == "instant" then
						a_destroy(self.active_sfx[i].audio)
						table.remove(self.active_sfx, i)
					else
						self.state = WAITING_SFX
					end
				else
					table.remove(self.active_sfx, i)
				end
			end
		end
	end;

	await_callback = function(self, cb)
		self.state = CALLBACK
		self.awaiting_callbacks = self.awaiting_callbacks + 1
		self.callbacks[#self.callbacks + 1] = cb
	end;

	next_cmd = function(self)
		local executed = 0
		self.state = RUNNING
		repeat
			local cmd = self.script.cmds[self.pos]
			if not cmd then
				log_info("libvn: script halted")
				self.state = HALTED
				return
			end
			local op = cmd[1]
			if op == "msg" then
				if self.voice_playing and not cmd.keep_voice and not self.is_midprint then
					a_destroy(self.voice_playing)
					self.voice_playing = nil
				end
				self.cur_speaker_name = cmd.speaker.display
				if self.is_midprint then
					self.cur_text = self.cur_text .. cmd.msg
				else
					self.cur_text = cmd.msg
				end
				if cmd.voice then
					self.voice_playing = a_play(cmd.voice)
				end
				self.is_midprint = not not cmd.midprint
				self.clear_after = not not cmd.clear_after
				self.state = WAITING
			elseif op == "voice" then
				if self.voice_playing then
					a_destroy(self.voice_playing)
					self.voice_playing = nil
				end
				if cmd.voice then
					self.voice_playing = a_play(cmd.voice)
				end
			elseif op == "delay" then
				self.delay_done = sys_millis() + cmd[2] * 1000
				self.state = DELAYING
			elseif op == "frame" then
				self.frame_kind = cmd.kind
			elseif op == "img" then
				local img
				if cmd.draw then
					img = {
						tex, src = cmd.src, id = cmd.id, tags = cmd.tags,
						active_effects = {},
						draw = cmd.draw, args = cmd.args,
						x = cmd.x, y = cmd.y, w = cmd.w, h = cmd.h, corner = cmd.corner, origin = cmd.origin,
						scale = cmd.scale or 1, priority = cmd.priority or 1,
						alpha = 1
					}
				else
					local tex, w, h = self.loader(cmd.src)
					img = {
						tex, src = cmd.src, id = cmd.id, tags = cmd.tags,
						active_effects = {},
						x = cmd.x, y = cmd.y, w = w or cmd.w, h = h or cmd.h, corner = cmd.corner, origin = cmd.origin,
						scale = cmd.scale or 1, priority = cmd.priority or 1,
						alpha = 1
					}
				end
				for i = 1, #self.imgs do
					if self.imgs[i].priority < img.priority then
						table.insert(self.imgs, i, img)
						goto placed
					end
				end
				table.insert(self.imgs, img)
				::placed::
				if cmd.fade_in and cmd.fade_in > 0 then
					img.alpha = 0
					local start = sys_millis()
					local finish = start + cmd.fade_in * 1000
					if cmd.fade_wait then
						self:await_callback(function(skip)
							local time = sys_millis()
							if skip or time >= finish then
								self.awaiting_callbacks = self.awaiting_callbacks - 1
								img.alpha = 1
								return true
							end
							img.alpha = (time - start) / (finish - start)
						end)
					else
						self.callbacks[#self.callbacks + 1] = function(skip)
							local time = sys_millis()
							if skip or time >= finish then
								img.alpha = 1
								return true
							end
							img.alpha = (time - start) / (finish - start)
						end
					end
				end
			elseif op == "img_set_tags" then
				for i = #self.imgs, 1, -1 do
					local v = self.imgs[i]
					if v.id == cmd.id then
						v.tags = {unpack(cmd.tags)}
					end
				end
			elseif op == "img_add_tags" then
				for i = #self.imgs, 1, -1 do
					local v = self.imgs[i]
					if v.id == cmd.id then
						local j = #v.tags
						for k = 1, #cmd.tags do
							v.tags[j + k] = cmd.tags[k]
						end
					end
				end
			elseif op == "img_move" then
				for i = #self.imgs, 1, -1 do
					local v = self.imgs[i]
					if v.id == cmd.id then
						if cmd.time == 0 then
							v.x = cmd.x
							v.y = cmd.y
							v.scale = cmd.scale
						else
							local prev_x, prev_y, prev_scale = v.x, v.y, v.scale
							local dst_x, dst_y, dst_scale = cmd.x, cmd.y, cmd.scale or prev_scale
							local start = sys_millis()
							local finish = start + cmd.time * 1000
							self:await_callback(function(skip)
								local time = sys_millis()
								if skip or time >= finish then
									self.awaiting_callbacks = self.awaiting_callbacks - 1
									v.x = dst_x
									v.y = dst_y
									v.scale = dst_scale
									return true
								end
								time = (time - start) / (finish - start)
								v.x = cerp(prev_x, dst_x, time)
								v.y = cerp(prev_y, dst_y, time)
								v.scale = cerp(prev_scale, dst_scale, time)
							end)
						end
					end
				end
			elseif op == "img_rm" then
				for i = #self.imgs, 1, -1 do
					local v = self.imgs[i]
					if v.id == cmd.id then
						if cmd.fade_time then
							local start = sys_millis()
							local finish = start + cmd.fade_time * 1000
							self:await_callback(function(skip)
								local time = sys_millis()
								if skip or time >= finish then
									self.awaiting_callbacks = self.awaiting_callbacks - 1
									for i = #self.imgs, 1, -1 do
										local v = self.imgs[i]
										if v.id == cmd.id then
											table.remove(self.imgs, i)
										end
									end
									return true
								end
								local img = v
								time = (time - start) / (finish - start)
								img.alpha = 1 - time
							end, cmd.fade_wait)
						else
							table.remove(self.imgs, i)
						end
					end
				end
			elseif op == "img_rm_tag" then
				for i = #self.imgs, 1, -1 do
					local v = self.imgs[i]
					for j = 1, #v.tags do
						if v.tags[j] == cmd.tag then
							if cmd.fade_time then
								local start = sys_millis()
								local finish = start + cmd.fade_time * 1000
								self:await_callback(function(skip)
									local time = sys_millis()
									if skip or time >= finish then
										self.awaiting_callbacks = self.awaiting_callbacks - 1
										for i = #self.imgs, 1, -1 do
											local v = self.imgs[i]
											for j = 1, #v.tags do
												if v.tags[j] == cmd.tag then
													table.remove(self.imgs, i)
												end
											end
										end
										return true
									end
									local img = v
									time = (time - start) / (finish - start)
									img.alpha = 1 - time
								end, cmd.fade_wait)
							else
								table.remove(self.imgs, i)
							end
						end
					end
				end
			elseif op == "effect" then
				cmd.data.start = sys_secs()
				if cmd.img_id then
					for i = #self.imgs, 1, -1 do
						local img = self.imgs[i]
						if img.id == cmd.img_id then
							img.active_effects[#img.active_effects + 1] = {self.screen_effects[cmd.name], cmd.data, cmd.name}
						end
					end
				else
					self.active_effects[#self.active_effects + 1] = {self.screen_effects[cmd.name], cmd.data, cmd.name}
				end
			elseif op == "effect_rm" then
				for i = #self.active_effects, 1, -1 do
					if self.active_effects[i][3] == cmd.name then
						table.remove(self.active_effects, i)
					end
				end
			elseif op == "fadeout" then
				local start = sys_millis()
				local finish = start + cmd.time * 1000
				local r, g, b, a = 0, 0, 0, 0
				if cmd.r then
					r, g, b = cmd.r, cmd.g, cmd.b
					if cmd.a then
						a = cmd.a
					end
				end
				local img = {
					g_white_tex(), id = "fade", tags = {"fade", "fadeout"},
					active_effects = {},
					draw = cmd.draw, args = cmd.args,
					x = 0, y = 0, w = -2, h = -2, corner = G_CENTER, origin = G_CENTER,
					scale = cmd.scale or 1, priority = cmd.priority or 2000,
					r = r, g = g, b = b, alpha = a
				}
				self.imgs[#self.imgs + 1] = img
				self:await_callback(function(skip)
					local time = sys_millis()
					if skip or time >= finish then
						img.alpha = 1
						self.awaiting_callbacks = self.awaiting_callbacks - 1
						return true
					end
					img.alpha = (time - start) / (finish - start)
				end)
			elseif op == "fadein" then
				local start = sys_millis()
				local finish = start + cmd.time * 1000
				local r, g, b, a = 0, 0, 0, 1
				for i = #self.imgs, 1, -1 do
					local v = self.imgs[i]
					if v.id == "fade" then
						table.remove(self.imgs, i)
						r, g, b = v.r, v.g, v.b
					end
				end
				if cmd.r then
					r, g, b = cmd.r, cmd.g, cmd.b
					if cmd.a then
						a = cmd.a
					end
				end
				local img = {
					g_white_tex(), id = "fade", tags = {"fade", "fadein"},
					active_effects = {},
					x = 0, y = 0, w = -2, h = -2, corner = G_CENTER, origin = G_CENTER,
					scale = cmd.scale or 1, priority = cmd.priority or 2000,
					r = r, g = g, b = b, alpha = a
				}
				self.imgs[#self.imgs + 1] = img
				self:await_callback(function(skip)
					local time = sys_millis()
					if skip or time >= finish then
						for i = #self.imgs, 1, -1 do
							local v = self.imgs[i]
							if v.id == "fade" then
								table.remove(self.imgs, i)
							end
						end
						self.awaiting_callbacks = self.awaiting_callbacks - 1
						return true
					end
					img.alpha = 1 - (time - start) / (finish - start)
				end)
			elseif op == "bgm" then
				if self.bgm_src ~= cmd.src then
					if self.bgm then
						a_destroy(self.bgm)
						self.bgm_src = nil
					end
					self.bgm = a_play(fs_resolve(cmd.src), true, 0.5)
					self.bgm_src = cmd.src
				end
			elseif op == "sfx" then
				self.active_sfx[#self.active_sfx + 1] = {audio = a_play(fs_resolve(cmd.src)), src = cmd.src, id = cmd.id}
			elseif op == "sfx_stop" then
				for i = #self.active_sfx, 1, -1 do
					if self.active_sfx[i].id == cmd.id then
						a_destroy(self.active_sfx[i].audio)
						table.remove(self.active_sfx, i)
					end
				end
			elseif op == "sfx_wait" then
				for i = #self.active_sfx, 1, -1 do
					if a_playing(self.active_sfx[i].audio) then
						self.state = WAITING_SFX
					else
						table.remove(self.active_sfx, i)
					end
				end
			-- control flow
			elseif op == "set" then
				self.vars[cmd.var] = cmd.val
			elseif op == "jump" then
				if cmd.path then
					self.script = self.parser(fs_file(cmd.path):read_string(), self.game_screen)
				end
				if cmd.label then
					self.pos = self.script.labels[cmd.label]
					if not self.pos then
						log_err("label %s not found", cmd.label)
					end
				else
					self.pos = 1
				end
			end
			self.pos = self.pos + 1
			executed = executed + 1
			if self.state == CALLBACK then
				for i = #self.callbacks, 1, -1 do
					if self.callbacks[i]() then
						table.remove(self.callbacks, i)
					end
				end
				if self.awaiting_callbacks == 0 then
					self.state = RUNNING
				end
			end
		until self.state ~= RUNNING or executed > 128
	end;

	save = function(self)
		for i = #self.callbacks, 1, -1 do
			self.callbacks[i](true)
		end
		local sav_imgs = {}
		for i = 1, #self.imgs do
			local img = self.imgs[i]
			sav_imgs[i] = {
				src = img.src, effects = img.effects,
				x = img.x, y = img.y, w = img.w, h = img.h, scale = img.scale,
				r = img.r, g = img.g, b = img.b, alpha = img.alpha,
			}
		end
		local sav = {
			time = os.time(), -- os.date("%Y/%m/%d %H:%M:%S", sav.time)
			bgm = self.bgm_src
		}
	end;

	draw = function(self)
		for i = #self.callbacks, 1, -1 do
			if self.callbacks[i]() then
				table.remove(self.callbacks, i)
			end
		end
		local function draw()
			for i = 1, #self.imgs do
				local v = self.imgs[i]
				if v.r then
					g_push_color(v.r, v.g, v.b, v.alpha)
				else
					g_push_color(1, 1, 1, v.alpha)
				end
				local function draw()
					if v.draw then
						v.draw(unpack(v.args))
					else
						local w, h = v.w, v.h
						if w < 0 then
							local nw, nh = v[1]:size()
							w = nw * -w
							if h < 0 then
								h = nh * -h
							end
						elseif h < 0 then
							local nw, nh = v[1]:size()
							h = nh * -h
						end
						g_draw_quad(v[1], v.x, v.y, w * (v.scale or 1), h * (v.scale or 1), v.corner, v.origin)
					end
				end
				local i = #v.active_effects
				local e = v.active_effects[i]
				if e then
					local function draw_next()
						local e = v.active_effects[i]
						if e then
							i = i - 1
							e[1](draw_next, e[2])
						else
							draw()
						end
					end
					i = i - 1
					e[1](draw_next, e[2])
				else
					draw()
				end
				g_pop_color()
				--g_draw(v[1], 0, 0, G_CENTER, G_CENTER)
			end
		end
		local i = #self.active_effects
		local e = self.active_effects[i]
		if e then
			local function draw_next()
				local e = self.active_effects[i]
				if e then
					i = i - 1
					local j = i
					if e[1](draw_next, e[2]) then
						self.active_effects[j] = nil
					end
				else
					draw()
				end
			end
			i = i - 1
			local j = i
			if e[1](draw_next, e[2]) then
				self.active_effects[j] = nil
			end
			for j = #self.active_effects, 1, -1 do
				if self.active_effects[j] == nil then
					table.remove(self.active_effects, j)
				end
			end
		else
			draw()
		end
		self.frame:draw(self, self.frame_kind)
	end;
}