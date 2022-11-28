local ok = "【Okuhiko】"
local files = {"hl1d5.dec", "il2d2.dec", "kl2d4.dec", "l1d4n.dec", "spool.dec", "callback.dec", "hl1d6.dec", "il2d3.dec", "kl2d5.dec", "omake.dec", "ycyari.dec", "c_il1d5.dec", "hl2d1.dec", "il2d4.dec", "kl2d6.dec", "onsen.dec", "yhana.dec", "c_il1d6.dec", "hl2d2.dec", "il2d5.dec", "kl2d7.dec", "op.dec", "ykake.dec", "c_il2d1a.dec", "hl2d3.dec", "il2d6.dec", "kpark.dec", "scottage.dec", "ykimo.dec", "c_il2d1b.dec", "hl2d4.dec", "il2d7.dec", "l1d1aa.dec", "scyari.dec", "yl1d5.dec", "c_il2d2.dec", "hl2d5.dec", "iluna.dec", "l1d1ab.dec", "skago.dec", "yl1d6.dec", "c_il2d3.dec", "hl2d6.dec", "init.dec", "l1d1e.dec", "sl1d5a.dec", "yl2d1a.dec", "c_il2d4.dec", "hl2d7.dec", "ipool.dec", "l1d1n.dec", "sl1d5b.dec", "yl2d1b.dec", "c_il2d5.dec", "hpool.dec", "kjinjya.dec", "l1d2a.dec", "sl1d6.dec", "yl2d2.dec", "c_il2d6.dec", "hturi.dec", "kkani.dec", "l1d2e.dec", "sl2d1.dec", "yl2d3.dec", "c_il2d7.dec", "humi.dec", "kkimo.dec", "l1d2n.dec", "sl2d2.dec", "yl2d4.dec", "curry.dec", "ikimo.dec", "kl1d5.dec", "l1d3a.dec", "sl2d3.dec", "yl2d5.dec", "dc_end.dec", "il1d5.dec", "kl1d6.dec", "l1d3e.dec", "sl2d4.dec", "yl2d6.dec", "dc_omake.dec", "il1d6.dec", "kl2d1.dec", "l1d3n.dec", "sl2d5.dec", "yl2d7.dec", "hcyari.dec", "il2d1a.dec", "kl2d2.dec", "l1d4a.dec", "sl2d6.dec", "ypool.dec", "hkimo.dec", "il2d1b.dec", "kl2d3.dec", "l1d4e.dec", "sl2d7.dec"}
local append = {"c_yuka0.dec", "c_yuka2.dec", "c_yuka4.dec", "c_yuka6.dec", "demo1.dec", "testmode.dec", "c_yuka1.dec", "c_yuka3.dec", "c_yuka5.dec", "c_yuka7.dec", "demo2.dec"}
--files = {"l1d1aa.dec"}
for i, v in ipairs(files) do
	local h = io.open("old/" .. v, "r")
	local txt = h:read("*all")
	h:close()
	local last_delay = {}
	local last_voice = ""
	local last_rbg = ""
	local out = ""
	local acceptable_fg = false
	for line in txt:gmatch("([^\r\n]*)[\r\n]?") do
		if line:find("textColor") then
			if line:find("【Okuhiko】") or (line:find("【%?%?%?】") and acceptable_fg) then
				out = out .. line .. "\n"
				last_delay = {}
				last_voice = ""
			elseif line:find("choiceOpt") then
				out = out .. line .. "\n"
				if #last_delay ~= 0 then
					for i = 1, #last_delay do
						out = out:gsub((last_delay[i]:gsub("([%%%.%[%]%{%}%(%)%?])", "%%%1")), "")
					end
					last_delay = {}
				end
				if last_voice ~= "" then
					out = out:gsub((last_voice:gsub("([%%%.%[%]%{%}%(%)%?])", "%%%1")), "")
					last_voice = ""
				end
			else
				if #last_delay ~= 0 then
					for i = 1, #last_delay do
						out = out:gsub((last_delay[i]:gsub("([%%%.%[%]%{%}%(%)%?])", "%%%1")), "")
					end
					last_delay = {}
				end
				if last_voice ~= "" then
					local nout = out:gsub((last_voice:gsub("([%%%.%[%]%{%}%(%)%?])", "%%%1")), "")
					out = nout
					last_voice = ""
				end
			end
		elseif line:find("delay") then
			table.insert(last_delay, line)
			out = out .. line .. "\n"
		elseif line:find("playVoice") then
			last_voice = line
			out = out .. line .. "\n"
		elseif line:find("multifgload2") then
			if line:find(" OK") then
				out = out .. line:sub(1, 16) .. "fgload 0 00000000" .. line:match(" OK.- %d") .. " 2 3\n"
				acceptable_fg = true
			else
				acceptable_fg = false
			end
		elseif line:find("fgload") then
			if line:find(" OK") then
				out = out .. line .. "\n"
				acceptable_fg = true
			else
				acceptable_fg = false
			end
		elseif line:find("removeBG") then
			last_rbg = line
			out = out .. line .. "\n"
		elseif line:find("createPanBG") then
			if last_rbg ~= "" then
				out = out:gsub(last_rbg:gsub("([%%%.%[%]%{%}%(%)%?])", "%%%1"), "")
				last_rbg = ""
			end
		elseif line:find("waitForSFX") then
			table.insert(last_delay, line)
			out = out .. line .. "\n"
		else
			out = out .. line .. "\n"
		end
	end
	h = io.open(v, "w")
	h:write(out)
	h:close()
end

for i, v in ipairs(append) do
	local h = io.open("old/append/" .. v, "r")
	local txt = h:read("*all")
	h:close()
	local last_delay = {}
	local last_voice = ""
	local last_rbg = ""
	local out = ""
	local acceptable_fg = false
	for line in txt:gmatch("([^\r\n]*)[\r\n]?") do
		if line:find("@text") then
			if line:find("【Okuhiko】") or (line:find("【%?%?%?】") and acceptable_fg) then
				out = out .. line .. "\n"
				last_delay = {}
				last_voice = ""
			else
				if #last_delay ~= 0 then
					for i = 1, #last_delay do
						out = out:gsub((last_delay[i]:gsub("([%%%.%[%]%{%}%(%)%?])", "%%%1")), "")
					end
					last_delay = {}
				end
				if last_voice ~= "" then
					local nout = out:gsub((last_voice:gsub("([%%%.%[%]%{%}%(%)%?])", "%%%1")), "")
					out = nout
					last_voice = ""
				end
			end
		elseif line:find("@choice") then
			out = out .. line .. "\n"
			if #last_delay ~= 0 then
				for i = 1, #last_delay do
					out = out:gsub((last_delay[i]:gsub("([%%%.%[%]%{%}%(%)%?])", "%%%1")), "")
				end
				last_delay = {}
			end
			if last_voice ~= "" then
				out = out:gsub((last_voice:gsub("([%%%.%[%]%{%}%(%)%?])", "%%%1")), "")
				last_voice = ""
			end
		elseif line:find("delay") then
			table.insert(last_delay, line)
			out = out .. line .. "\n"
		elseif line:find("playVoice") then
			last_voice = line
			out = out .. line .. "\n"
		elseif line:find("fgload") then
			if line:find(" OK") then
				out = out .. line .. "\n"
				acceptable_fg = true
			else
				acceptable_fg = false
			end
		elseif line:find("removeBG") then
			last_rbg = line
			out = out .. line .. "\n"
		elseif line:find("createPanBG") then
			if last_rbg ~= "" then
				out = out:gsub(last_rbg:gsub("([%%%.%[%]%{%}%(%)%?])", "%%%1"), "")
				last_rbg = ""
			end
		elseif line:find("waitForSFX") then
			table.insert(last_delay, line)
			out = out .. line .. "\n"
		else
			out = out .. line .. "\n"
		end
	end
	h = io.open("append/" .. v, "w")
	h:write(out)
	h:close()
end


