local function remove_comment(line)
	--local comment = line:find("%s*//")
	--if comment then
	--	line = line:sub(1, comment - 1)
	--end
	return line
end

DecParser = class(VnParser) {
	scr_line_i = 1;

	_init = function(self, stream)
		self.stream = stream
		self.labels = {}
		self.script = vn_script_new()
	end;

	check = function(stream)
		while stream:remaining() > 0 do
			local line = stream:read_line()
			if line:find("^%[[0-9a-f]+%] ") or line:find("{textColor ") then
				return true
			end
		end
	end;

	update = function(self)
		local ex = self.script
		--ex:begin_parse()
		while self.stream:remaining() > 0 do --and ex:should_run() do
			local line = remove_comment(self.stream:read_line())
			local label, cmd, rest = line:match("^%[([0-9a-f]+)%]%s*[0-9a-f]*:?%s*([a-zA-Z0-9]+)")
			if label then
				line = line:sub(16)
				self.labels[label] = ex:size()
				if cmd == "bgload" then
					local s = line:split(" ")
					local name = s[3]:lower()
					if s[4] == "0" or s[4] == "4" or s[4] == "5" or s[4] == "6" then
						--out = out .. "\nbgload " .. name .. ".png " .. frames .. " " .. s[4]
						ex:push_bg(name, s[4], 0.5, true)
					elseif s[4] == "2" then
						--out = out .. "\npanbgload " .. name
						ex:push_bg(name, s[4], 0.5, true) -- TODO: real pan bg
					else
						error("unknown BG transition " .. s[4] .. " in " .. filename)
					end
					--if starts(name, "ev_") and not starts(name, "ev_al") and name ~= "ev_ha02b" and name ~= "ev_iz06b" then
					--	--out = out .. "\ngsetvar cg_" .. name .. " = 1"
					--end
				elseif cmd == "playBGM" then
					local n = line:after(" ")
					--n = bgm_map[n] or n
					--if tonumber(n) < 10 then
					--	n = "0" .. n
					--end
					--out = out .. "\nmusic track_" .. n .. ".ogg\ngsetvar track_" .. n .. " = 1"
					ex:push_bgm(tonumber(n))
				end
			else -- not a command; may be a message (but make sure it's valid)
				local shadow_color = line:match("^{textColor [0-9]+ ([0-9]+)}")
				if shadow_color then-- line ~= "" and not line:find("^%s*#") then
					--out = out .. "\ntext " .. line:gsub("】 \"(.*)\" ?", "】 ｢%1｣")
					line = line:sub(line:find("}") + 1)
					ex:push_msg(-1, "",
						line:gsub("  ", "\n"):gsub("¥n", "\n"):trim():gsub("{textColor.-}", "%%K"))
				end
			end
		end
	end;
}