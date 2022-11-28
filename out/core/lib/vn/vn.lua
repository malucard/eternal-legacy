function vn_load(opt)
	opt.hooks = {}
	cur_vn = opt
	cur_vn.res_finders = {}
	cur_vn.parsers = {}
	cur_vn.settings = {}
	cur_vn.langs = {}
	vn_switch_screen(cur_vn.start_screen())
	vn_make_option("skip", "skipread", true)
	vn_make_option("ff_mode", "skipall")
end

function vn_unload()
	if cur_vn and cur_vn.cur_screen then
		cur_vn.cur_screen:unload()
	end
	cur_vn = nil
end

function vn_switch_screen(screen)
	sys_set_input(false)
	if cur_vn.cur_screen then
		local prev_screen = cur_vn.cur_screen
		trans_start(cur_vn.cur_screen.trans_out, function()
			screen:load()
			cur_vn.cur_screen = screen
			sys_set_input(true)
			trans_start(screen.trans_in, function()
				if screen.trans_in_done then
					screen:trans_in_done()
				end
				prev_screen:unload()
			end, screen.trans_in_time)
		end)
	else
		screen:load()
		cur_vn.cur_screen = screen
		sys_set_input(true)
		trans_start(screen.trans_in, function()
			if screen.trans_in_done then
				screen:trans_in_done()
			end
		end, screen.trans_in_time)
	end
end

function vn_add_hook(kind, callback)
	local list = cur_vn.hooks[kind]
	if list then
		list[#list + 1] = callback
	else
		cur_vn.hooks[kind] = {callback}
	end
end

function vn_run_hooks(kind, data, ctx)
	local list = cur_vn.hooks[kind]
	if list then
		for i = #list, 1, -1 do
			list[i](data, ctx)
		end
	end
end

local function inner_frame()
	cur_vn.cur_screen:draw()
end

function vn_frame()
	cur_vn:prep_screen()
	trans_check_draw(inner_frame)
end

function vn_input(input)
	if cur_vn and cur_vn.cur_screen then
		cur_vn:prep_screen()
		cur_vn.cur_screen:input(input)
	end
end

function vn_cursor(x, y, ...)
	if cur_vn and cur_vn.cur_screen then
		cur_vn:prep_screen()
		x, y = g_cursor_to_virtual(x, y)
		cur_vn.cur_screen:cursor(x, y, ...)
	end
end

function vn_close()
	if cur_vn and cur_vn.cur_screen then
		cur_vn:prep_screen()
		cur_vn.cur_screen:unload()
	end
end

function vn_resize()
	if cur_vn and cur_vn.cur_screen then
		cur_vn:prep_screen()
		cur_vn.cur_screen:resize()
	end
end


dofile("et:/core/lib/vn/screen/screen.lua")
dofile("et:/core/lib/vn/executor.lua")
--dofile("et:/core/lib/vn/screen/title_screen.lua")

function vn_find_res(res_kind, name)
	local langs = cur_vn.res_finders[res_kind]
	local finders = langs[CUR_LANG]
	if finders then
		for j = #finders, 1, -1 do
			local path = finders[j](name)
			if path then
				return path
			end
		end
	end
	for i = 1, #LANG_LIST do
		local lang_code = LANG_LIST[i]
		finders = langs[lang_code]
		if finders then
			for j = #finders, 1, -1 do
				local path = finders[j](name)
				if path then
					return path
				end
			end
		end
	end
	-- if no exact match, look again but ignore the countries
	for i = 1, #LANG_LIST do
		local lang_code = LANG_LIST[i]:sub(1, 2)
		finders = langs[lang_code]
		if finders then
			for j = #finders, 1, -1 do
				local path = finders[j](name)
				if path then
					return path
				end
			end
		end
	end
	-- if no match, see if there's a finder for the "" language
	finders = langs[""]
	if finders then
		for i = #finders, 1, -1 do
			local path = finders[i](name)
			if path then
				return path
			end
		end
	end
	-- if still nothing, just grab the first one that works
	for _, finders in pairs(langs) do
		for i = #finders, 1, -1 do
			local path = finders[i](name)
			if path then
				return path
			end
		end
	end
end

function vn_open_res(res_kind, name)
	local path = vn_find_res(res_kind, name)
	return path and fs_file(path)
end

--- add a custom resource finder
--- @param res_kind string arbitrary, for example "bg", "chara", "se", "script"
--- @param finder function 
function vn_add_res_finder(res_kind, lang, finder)
	lang = lang or ""
	local langs = cur_vn.res_finders[res_kind]
	if not langs then
		langs = {}
		cur_vn.res_finders[res_kind] = langs
	end
	local finders = langs[lang]
	if not finders then
		finders = {}
		langs[lang] = finders
	end
	finders[#finders + 1] = finder
end

--- add a simple resource finder for a defined path
--- @param res_kind string arbitrary, for example "bg", "chara", "se", "script"
--- @param root string root path for the files, can use the : separator to read inside archives
--- @param extension string will be added to the end of the string unmodified
--- @param lang string language code for this resource
--- @param modifier? string "upper", "lower" or nothing
function vn_add_res_path(res_kind, root, extension, lang, modifier)
	local finder
	if modifier == "upper" then
		finder = function(name)
			return root .. name:upper() .. extension
		end
	elseif modifier == "lower" then
		finder = function(name)
			return root .. name:lower() .. extension
		end
	else
		finder = function(name)
			return root .. name .. extension
		end
	end
	return vn_add_res_finder(res_kind, lang, finder)
end

--- *caller* is responsible for resetting the stream position between check_file and _init
-- must have:
-- _init(self, stream: DataStream)
-- check_file(stream: DataStream) return false if file does not fit this parser's format
-- update(self, limit: integer)
VnParser = class {}

--- add a parser class to the current vn
-- @class class inheriting VnParser
function vn_add_parser(parser)
	cur_vn.parsers[#cur_vn.parsers + 1] = parser
end

function vn_set_script(name)
	local path = vn_find_res("script", name)
	if not path then return end
	local stream = fs_file(path)
	if not stream then return end
	local parsers = cur_vn.parsers
	for i = #parsers, 1, -1 do
		local parser = parsers[i]
		stream:seek(0)
		if parser.check(stream) then
			stream:seek(0)
			cur_vn.cur_parser = parser(stream)
			cur_vn.cur_script = cur_vn.cur_parser.script
			cur_vn.cur_parser:update()
			cur_vn.cur_executor = cur_vn.cur_script:execute()
			return true
		end
	end
	error("no parser found for script")
end

function vn_start_game(start_script)
	vn_set_script(start_script)
	vn_switch_screen(cur_vn.game_screen)
end

function vn_get_setting(id)
	local setting = cur_vn.settings[id]
	return setting and setting.value
end

function vn_make_option(id, option_id, is_default)
	local setting = cur_vn.settings[id]
	if not setting then
		setting = {}
		cur_vn.settings[id] = setting
		setting.value = option_id
	end
	setting.kind = "option"
	local alts = setting.alts
	if alts then
		alts[#alts + 1] = option_id
	else
		setting.alts = {option_id}
	end
	if is_default then
		setting.value = option_id
	end
end

function vn_make_toggle(id, default, on_id, off_id)
	vn_make_option(id, off_id, not default)
	vn_make_option(id, on_id, default)
end

function vn_make_num_setting(id, min, max, step)
	local setting = cur_vn.settings[id]
	if not setting then
		setting = {}
		cur_vn.settings[id] = setting
		setting.value = min
	end
	setting.kind = "num"
	setting.min = setting.min and math.min(min, setting.min) or min
	setting.max = setting.max and math.max(max, setting.max) or max
	setting.step = step or 1
end

function vn_add_lang(path, lang)
	local ds = fs_file(path)
	local table = cur_vn.langs[lang] or {}
	while ds:remaining() > 0 do
		local line = ds:read_line()
		local id, tl = line:match("^([a-zA-Z0-9_]+)=(.*)$")
		if id then
			table[id] = tl
		end
	end
	cur_vn.langs[lang] = table
end

function vn_translate(str)
	local langs = cur_vn.langs
	local lang = langs[CUR_LANG]
	if lang then
		local tl = lang[str]
		if tl then
			return tl
		end
	end
	for i = 1, #LANG_LIST do
		local lang_code = LANG_LIST[i]
		lang = langs[lang_code]
		if lang then
			local tl = lang[str]
			if tl then
				return tl
			end
		end
	end
	-- if no exact match, look again but ignore the countries
	for i = 1, #LANG_LIST do
		local lang_code = LANG_LIST[i]:sub(1, 2)
		lang = langs[lang_code]
		if lang then
			local tl = lang[str]
			if tl then
				return tl
			end
		end
	end
	-- if no match, see if there's a finder for the "" language
	lang = langs[""]
	if lang then
		local tl = lang[str]
		if tl then
			return tl
		end
	end
	-- if still nothing, just grab the first one that works
	for _, lang in pairs(langs) do
		local tl = lang[str]
		if tl then
			return tl
		end
	end
end
