local RUNNING = "running" -- will execute next command on next frame
local SCROLLING = "scrolling" -- will interpolate next command when sys_millis() reaches self.delay_done
local DELAYING = "delaying" -- will execute next command when sys_millis() reaches self.delay_done
local CALLBACK = "callback" -- will execute next command when self.awaiting_callbacks is 0
local WAITING = "waiting" -- will execute next command on next("advance")
local WAITING_SFX = "waiting_sfx" -- will execute next command when no sfx is still playing
local HALTED = "halted" -- will not execute

VnExecutor = class {
	state = RUNNING;
	delay_done = nil;
	imgs_dirty = false;

	_init = function(self)
		self.vars = {}
		self.read_msgs = {}
		self.fields = {}
		self.loaded = {}
		self.renderers = {}
		self.screen_effects = {}
		self.layer_order = {"bg", "sprite", "effect"}
	end;
	
	make_state_obj = function(self)
	end;

	set_state = function(self, field, val)
		self.fields[field] = val
	end;

	serialize = function(self)

	end;

	delay = function(self, secs)
		if self.delay_done then
			self.delay_done = self.delay_done + secs * 1000
		else
			self.delay_done = sys_millis() + secs * 1000
		end
	end;

	add_img = function(self, layer_name, id, src, params)
		local img = {
			layer = layer_name,
			src = src,
			id = id,
			params = params,
			ephemeral = {} -- not saved
		}
		vn_run_hooks("add_img", img)
		if not img.src then
			return
		end
		self.imgs[#self.imgs + 1] = img
		self.imgs_dirty = true
	end;

	rm_img = function(self, id)
		local imgs = self.imgs
		for i = 1, #imgs do
			if imgs[i].id == id then
				table.remove(imgs, i)
				return
			end
		end
	end;

	rm_img_layer = function(self, layer)
		local imgs = self.imgs
		for i = 1, #imgs do
			if imgs[i].layer == layer then
				table.remove(imgs, i)
			end
		end
	end;

	submit_msg = function(self, text, speaker_id, speaker_name, midprint, clear_after, seen)
		self.state = SCROLLING
		local msg = {
			text = text,
			speaker_id = speaker_id,
			speaker_name = speaker_name,
			midprint = midprint == nil and true or midprint,
			clear_after = clear_after == nil and true or clear_after,
			seen = false
		}
		vn_run_hooks("message", msg)
		if self.msg.skip then
			self.state = RUNNING
		else
			self.msg = msg
		end
	end;

	draw = function(self)
		if self.bg then	
			g_draw_crop(self.bg)
		end
		if self.imgs_dirty then
			for i = 1, #self.layer_order do
				local layer = self.layer_order[i]
				self.layer_order[img] = 1
			end
		end
		for i = 1, #self.layer_order do
			local layer = self.layer_order[i]
			self.layer_order[img] = 1
			for j = 1, #self.imgs do
				local img = layer[j]
				self.layer_order[img] = 1
				self.renderers[img.kind](img.src, img.params, img.ephemeral)
			end
		end
	end;
}