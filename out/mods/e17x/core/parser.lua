local names = {
	[-1] = "",
	[0] = "BW",
	[1] = "Takeshi",
	[2] = "Tsugumi",
	[3] = "Sora",
	[4] = "You",
	[5] = "Kid",
	[6] = "Coco",
	[7] = "Sara",
	[8] = "Pipi",
	[9] = "Researcher",
	[10] = "Suzuki",
	[11] = "Sato",
	[12] = "Staff",
	[13] = "Chami",
	[14] = "Control",
	[15] = "Boy",
	[16] = "???",
	[17] = "Girl",
	[18] = "Lemur",
	[19] = "Lady",
	[20] = "Sora A",
	[21] = "Sora B",
	[22] = "Sora C",
	[23] = "Announcer",
	[24] = "Staff Girl",
	[25] = "I",
	[26] = "Mayo",
	[27] = "Matsunaga",
	[28] = "Popo?",
	[29] = "Man",
	[30] = "Woman",
	[31] = "Puppy",
	[32] = "Teacher",
	[33] = "Doctor",
	[34] = "Gentleman",
	[35] = "Supervisor",
	[36] = "Subordinate",
	[37] = "Professor Morino",
	[38] = "You-Haru",
	[39] = "You-Aki",
	[40] = "Kaburaki",
	[41] = "Hokuto",
	[42] = "Takeshi Yagami",
	[43] = "Yoichi Tanaka",
	[44] = "Dr. Tanaka",
	[45] = "Researcher A",
	[46] = "Researcher B",
	[47] = "Researcher C",
	[48] = "Researcher D",
	[49] = "Researcher E",
	[50] = "Researcher F",
	[51] = "Researcher G",
	[52] = "Control A",
	[53] = "Control B",
	[54] = "Doctor",
	[55] = "Announcer",
	[56] = "Everyone",
	[57] = "Operator",
	[58] = "Labcoat Man",
	[59] = "Voice on TV",
	[60] = "Assistant",
	[61] = "Researcher A",
	[62] = "Researcher B",
	[63] = "Takeshi",
	[64] = "Kid",
	[65] = "Spare",
	[66] = "Kid (Unvoiced)",
	[67] = "Takeshi (Unvoiced)",
	[68] = "Anchorwoman",
	[69] = "Instructor"
}

local function remove_comment(line)
	local comment = line:find("%s*//")
	if comment then
		line = line:sub(1, comment - 1)
	end
	return line
end

Parser = class {
	scr_line_i = 1;

	_init = function(self, path)
		self.script = vn_script_new()
		self.scr_lines = fs_file(path):read_string():split("\n")
	end;

	update = function(self)
		local ex = self.script
		local lines = self.scr_lines
		local linec = #lines
		while self.scr_line_i < linec do --and ex:should_run() do
			local i = self.scr_line_i
			local line = remove_comment(lines[i])
			local taskn = line:match("%[Task([0-9]+)%]")
			if taskn then
				line = remove_comment(lines[i + 2])
				local tasktype = line:match(":Tasktype%s+\"(.+)\"")
				i = i + 3
				local attr = {id = tonumber(taskn)}
				local texture -- treat :Texture specially because the filename is always included in a comment
				repeat
					line = remove_comment(lines[i])
					local key, val = line:match(":([A-Za-z]+)%s+(.-)%s*$")
					if key then
						if key == "Texture" or key == "BGM" or key == "SE" or key == "Voice" then
							texture = lines[i]:match("<<([a-zA-Z0-9_]+)>>")
						elseif key == "Time" and val == "1" then
							val = "0"
						elseif key == "FadeIn" then -- and val == "1" then
							--val = "0" --val = "30"
						end
						attr[key] = val
					end
					i = i + 1
				until lines[i]:match("^%s*}%s*$")
				self.scr_line_i = i
				if tasktype == "Message" then
					if attr.Frame == "-1" then
						--ex:push_frame(-1)
					elseif attr.Frame == "0" then
						--ex:push_frame(0)
					elseif attr.Frame == "1" then
						--ex:push_frame(1)
					elseif attr.Frame == "2" then
						--ex:push_frame(2)
					end
					local char = attr.Character and tonumber(attr.Character) or -1
					local name = attr.Name and tonumber(attr.Name) or char
					if attr.Voice then
						--ex:submit_voice("xbox/voice/" .. texture:upper() .. ".ahx.wav")
					end
					--print(attr.Message:sub(2, -2):gsub("\\n", "\n"):gsub("¥n", "\n"):trim())
					ex:push_msg(
						char, names[name] or names[-1],
						attr.Message:sub(2, -2):gsub("\\n", "\n"):gsub("¥n", "\n"):trim()
						--attr.ChangingPage == "0",
						--attr.ChangingPage == "1"
					)
				elseif tasktype == "TimeWait" then
					--ex:push_delay(tonumber(attr.Time) / 30)
				-- effects
				elseif tasktype == "SpriteQuake" then
					local p1, p2, p3, p4, p5, p6 = attr.Param:match("([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)")
					p1, p2, p3, p4, p5, p6 = tonumber(p1), tonumber(p2), tonumber(p3), tonumber(p4), tonumber(p5), tonumber(p6)
					--ex:add_img_effect(attr.SpriteNumber, "quake", {speed_x = p5, speed_y = p6, x = p3, y = p4, off_x = 0, off_y = 0})
				elseif tasktype == "EffectQuake" then
					local p1, p2, p3, p4, p5, p6 = attr.Param:match("([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+)")
					p1, p2, p3, p4, p5, p6 = tonumber(p1), tonumber(p2), tonumber(p3), tonumber(p4), tonumber(p5), tonumber(p6)
					--ex:add_screen_effect("quake", {
					--	speed_x = p5, speed_y = p6,
					--	x = p3, y = p4, off_x = 0, off_y = 0,
					--	time = attr.Time and (tonumber(attr.Time) / 60) or nil
					--})
				elseif tasktype == "EffectQuakeEnd" then
					--ex:rm_screen_effect("quake")
				elseif tasktype == "Flash" then
					local r, g, b, a = attr.Color:match("([0-9]+),([0-9]+),([0-9]+),([0-9]+)")
					r, g, b, a = tonumber(r) / 255, tonumber(g) / 255, tonumber(b) / 255, tonumber(a) / 255
					--ex:add_screen_effect("flash", {
					--	r = r, g = g, b = b, a = a,
					--	in_time = tonumber(attr.InTime) / 60,
					--	out_time = tonumber(attr.OutTime) / 60,
					--	wait = tonumber(attr.TimeWait or 0) / 60,
					--	time = (not attr.TimeWait) and ((tonumber(attr.InTime) + tonumber(attr.OutTime)) / 60)
					--})
				elseif tasktype == "FlashStop" then
					--ex:rm_screen_effect("flash")
				-- visual
				elseif tasktype == "ChrModelOpen" then
					--local src = model_files[attr.Name:sub(2, -2)]
					--ex:add_img("sprite", attr.Name, src,
					--	{
					--		priority = tonumber(attr.Priority),
					--		preset = model_presets[attr.Name:sub(2, -2)][tonumber(attr.Preset)]
					--	},
					--	attr.FadeIn and (tonumber(attr.Time or 30) / 60) or 0,
					--	attr.FadeWait ~= "0"
					--)
				elseif tasktype == "ChrModelLight" then
					local diffuse = {attr.Diffuse:match("([0-9.]+),([0-9.]+),([0-9.]+),")}
					for i = 1, 3 do
						diffuse[i] = tonumber(diffuse[i]) / 255
					end
					local ambient = {attr.Ambient:match("([0-9.]+),([0-9.]+),([0-9.]+),")}
					for i = 1, 3 do
						ambient[i] = tonumber(ambient[i]) / 255
					end
					local spec = {attr.Spec:match("([0-9.]+),([0-9.]+),([0-9.]+),")}
					local angle_x, angle_y = attr.Angle:match("(%-?[0-9.]+),(%-?[0-9.]+)")
					--ex.fields.model_light = {
					--	Diffuse = diffuse, Ambient = ambient, Spec = spec,
					--	AngleX = math.rad(tonumber(angle_x)), AngleY = math.rad(tonumber(angle_y))
					--}
				elseif tasktype == "ChrModelClose" then
					--if attr.Name then
					--	ex:rm_img("sprite", attr.Name, attr.Fade == "1" and (tonumber(attr.Time or 30) / 60) or 0, true)
					--else
					--	ex:rm_img_layer("sprite", attr.Fade == "1" and (tonumber(attr.Time or 30) / 60) or 0, true)
					--end
				elseif tasktype == "Sprite" or tasktype == "SpriteBG" then
					texture = texture:upper()
					-- priorities go from 1 to 10, except for one ":Priority 60" in X_CCBW01
					-- because there are only 16 layers, we change that index to 15
					ex:push_bg(texture, 0, attr.FadeIn and (tonumber(attr.Time or 30) / 60 * 1000) or 0, attr.FadeWait ~= "0")
					--if fs_exists("et:/user/e17x/CG.afs:" .. texture .. ".png") or fs_exists("xbox/bg_psp/" .. texture .. ".png") then
						--local x, y = attr.Position:match("([0-9]+),([0-9]+)")
						--res.cmds[#res.cmds + 1] = {
						--	"img",
						--	src = texture,
						--	id = attr.Texture,
						--	x = tonumber(x) - 640, y = tonumber(y) - 360, w = -1, h = -1,
						--	corner = G_CENTER, origin = G_CENTER,
						--	priority = tonumber(attr.Priority),
						--	fade_in = attr.FadeIn and (tonumber(attr.Time or 30) / 60) or 0,
						--	fade_wait = attr.FadeWait ~= "0",
						--	tags = tasktype == "SpriteBG" and {"all", "bg"} or {"all"}
						--}
						--if attr.SpriteDelete == "1" then
						--	res.cmds[#res.cmds + 1] = {
						--		"img_set_tags", id = attr.Texture, tags = {}
						--	}
						--	res.cmds[#res.cmds + 1] = {
						--		"img_rm_tag", tag = tasktype == "SpriteBG" and "bg" or "all"
						--	}
						--	res.cmds[#res.cmds + 1] = {
						--		"img_set_tags", id = attr.Texture, tags = tasktype == "SpriteBG" and {"all", "bg"} or {"all"}
						--	}
						--end
					--else
					--	log_err("texture not found " .. texture)
					--end
				elseif tasktype == "SpriteDelete" then
					--res.cmds[#res.cmds + 1] = {
					--	"img_rm",
					--	id = attr.SpriteNumber,
					--	fade_time = attr.FadeOut == "1" and (tonumber(attr.Time or 30) / 60) or 0,
					--	fade_wait = attr.FadeWait == "1"
					--}
				elseif tasktype == "SpriteDeleteBG" then
					--res.cmds[#res.cmds + 1] = {
					--	"img_rm",
					--	id = attr.SpriteNumber,
					--	fade_time = attr.FadeOut == "1" and (tonumber(attr.Time or 30) / 60) or 0,
					--	fade_wait = attr.FadeWait == "1"
					--}
				elseif tasktype == "FadeOut" then
					--local r, g, b, a = attr.Color:match("([0-9]+),([0-9]+),([0-9]+),([0-9]+)")
					--res.cmds[#res.cmds + 1] = {
					--	"fadeout",
					--	r = tonumber(r) / 255, g = tonumber(g) / 255, b = tonumber(b) / 255, a = tonumber(a) / 255,
					--	time = tonumber(attr.Time or "30") / 60
					--}
					--if attr.SpriteDelete == "1" then
					--	res.cmds[#res.cmds + 1] = {
					--		"img_rm_tag",
					--		tag = "all"
					--	}
					--end
				elseif tasktype == "FadeIn" then
					--if attr.SpriteDelete == "1" then
					--	res.cmds[#res.cmds + 1] = {
					--		"img_rm_tag",
					--		tag = "all"
					--	}
					--end
					--local r, g, b, a = attr.Color:match("([0-9]+),([0-9]+),([0-9]+),([0-9]+)")
					--res.cmds[#res.cmds + 1] = {
					--	"fadein",
					--	r = tonumber(r) / 255, g = tonumber(g) / 255, b = tonumber(b) / 255, a = tonumber(a) / 255,
					--	time = tonumber(attr.Time or "30") / 60
					--}
				elseif tasktype == "SpriteMove" then
					--local x, y = attr.Position:match("([0-9]+),([0-9]+)")
					--res.cmds[#res.cmds + 1] = {
					--	"img_move",
					--	id = attr.SpriteNumber,
					--	x = tonumber(x) - 640, y = tonumber(y) - 360,
					--	scale = tonumber(attr.Scale) or nil,
					--	time = attr.Wait == "1" and (tonumber(attr.Time) / 60) or 0
					--}
				-- sound
				elseif tasktype == "BgmPlay" then
					--local bgm_path = bgm_resolve(texture:lower())
					--print(bgm_path)
					--if fs_exists(bgm_path) then
					--	res.cmds[#res.cmds + 1] = {
					--		"bgm",
					--		src = bgm_path,
					--		id = attr.BGM
					--	}
					--end
				elseif tasktype == "BgmStop" then
					--res.cmds[#res.cmds + 1] = {"bgm_stop"}
				elseif tasktype == "SePlay" then
					--texture = texture:lower()
					--if fs_exists("xbox/se/" .. texture .. ".adx.ogg") then
					--	res.cmds[#res.cmds + 1] = {
					--		"sfx",
					--		src = "xbox/se/" .. texture .. ".adx.ogg",
					--		id = attr.SE
					--	}
					--end
				elseif tasktype == "SeStop" then
					--res.cmds[#res.cmds + 1] = {"sfx_stop", id = attr.SE}
				-- control flow
				elseif tasktype == "Tag" then
					--res.labels[attr.Name:sub(2, -2)] = #res.cmds
				elseif tasktype == "Jump_Tag" then
					--res.cmds[#res.cmds + 1] = {"jump", label = attr.NextTag:sub(2, -2)}
				elseif tasktype == "NextScene" then
					--local file, tag = attr.NextScene:match("\"(.-)\",\"(.-)\"")
					--if file then
					--	res.cmds[#res.cmds + 1] = {"jump", path = "xbox/script_en/" .. file, label = tag}
					--else
					--	res.cmds[#res.cmds + 1] = {"jump", path = "xbox/script_en/" .. attr.NextScene:sub(2, -2)}
					--end
				end
			else
				self.scr_line_i = i + 1
			end
		end
	end;
}