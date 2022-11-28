-- Lua 5.4 does not have these while LuaJIT does; we mainly just use getfenv for fs_resolve
setfenv = setfenv or function(f, t)
	f = (type(f) == 'function' and f or debug.getinfo(f + 1, 'f').func)
	local name
	local up = 0
	repeat
		up = up + 1
		name = debug.getupvalue(f, up)
	until name == '_ENV' or name == nil
	if name then
		debug.upvaluejoin(f, up, function() return name end, 1)
		debug.setupvalue(f, up, t)
	end
end

getfenv = function(f)
	if type(f) ~= 'function' then
		f = debug.getinfo(f + 1, 'f')
		if not f then
			return nil
		end
		f = f.func
	end
	local name, val
	local up = 0
	repeat
		up = up + 1
		name, val = debug.getupvalue(f, up)
	until name == '_ENV' or name == nil
	return val
end

local launcher
sys_modules = {}
dofile(ROOT_DIR .. "core/fs.lua")
dofile(ROOT_DIR .. "core/class.lua")
dofile(ROOT_DIR .. "core/color.lua")
dofile(ROOT_DIR .. "core/semver.lua")
dofile(ROOT_DIR .. "core/lang.lua")

-- api

if not bit then
	dofile(ROOT_DIR .. "core/bitop.lua")
end

function g_calc_corner(obj_w, obj_h, bound_x, bound_y, bound_w, bound_h, corner)
	if corner == G_CENTER then
		return bound_x + bound_w / 2 - obj_w / 2, bound_y + bound_h / 2 - obj_h / 2, obj_w, obj_h
	elseif corner == G_TOP_LEFT then
		return bound_x, bound_y, obj_w, obj_h
	elseif corner == G_TOP_RIGHT then
		return bound_x + bound_w - obj_w, bound_y, obj_w, obj_h
	elseif corner == G_BOTTOM_LEFT then
		return bound_x, bound_y + bound_h - obj_h, obj_w, obj_h
	elseif corner == G_BOTTOM_RIGHT then
		return bound_x + bound_w - obj_w, bound_y + bound_h - obj_h, obj_w, obj_h
	elseif corner == G_CENTER_TOP then
		return bound_x + bound_w / 2 - obj_w / 2, bound_y, obj_w, obj_h
	elseif corner == G_CENTER_BOTTOM then
		return bound_x + bound_w / 2 - obj_w / 2, bound_y + bound_h - obj_h, obj_w, obj_h
	elseif corner == G_CENTER_LEFT then
		return bound_x, bound_y + bound_h / 2 - obj_h / 2, obj_w, obj_h
	elseif corner == G_CENTER_RIGHT then
		return bound_x + bound_w - obj_w, bound_y + bound_h / 2 - obj_h / 2, obj_w, obj_h
	end
end

function g_calc_crop(obj_w, obj_h, bound_x, bound_y, bound_w, bound_h)
	if bound_w / bound_h >= obj_w / obj_h then
		obj_h = obj_h * bound_w / obj_w
		return bound_x, bound_y - (obj_h - bound_h) * 0.5, bound_w, obj_h
	else
		obj_w = obj_w * bound_h / obj_h
		return bound_x - (obj_w - bound_w) * 0.5, bound_y, obj_w, bound_h
	end
end

function g_calc_fit(obj_w, obj_h, bound_x, bound_y, bound_w, bound_h)
	if bound_w / bound_h >= obj_w / obj_h then
		obj_w = obj_w * bound_h / obj_h
		return bound_x - (obj_w - bound_w) * 0.5, bound_y, obj_w, bound_h
	else
		obj_h = obj_h * bound_w / obj_w
		return bound_x, bound_y - (obj_h - bound_h) * 0.5, bound_w, obj_h
	end
end

local tex_cache = setmetatable({}, {__mode = "v"})
-- hold textures in the cache for 1 frame by moving _hold to _hold_prev each frame
local tex_cache_hold_prev
local tex_cache_hold = {}
local ogg_load_texture = g_load_texture
function g_load_texture(source, flags) -- change loader to cache textures until garbage collected
	local name
	if type(source) == "string" then
		source = fs_resolve(source)
		name = source
	else
		name = source:filename()
		if name == "" then
			name = tostring(source)
		else
			name = fs_resolve(name)
		end
	end
	local tex = tex_cache[name]
	if tex then
		tex_cache_hold[#tex_cache_hold + 1] = tex
		return tex
	end
	tex = ogg_load_texture(source, flags)
	log_info("newly loaded " .. name)
	tex_cache[name] = tex
	tex_cache_hold[#tex_cache_hold + 1] = tex
	return tex
end

local ogg_draw = g_draw
function g_draw(tex, ...)
	tex_cache_hold[#tex_cache_hold + 1] = tex
	ogg_draw(tex, ...)
end

local ogg_draw_quad = g_draw_quad
function g_draw_quad(tex, ...)
	tex_cache_hold[#tex_cache_hold + 1] = tex
	ogg_draw_quad(tex, ...)
end

local ogg_draw_quad_crop = g_draw_quad_crop
function g_draw_quad_crop(tex, ...)
	tex_cache_hold[#tex_cache_hold + 1] = tex
	ogg_draw_quad_crop(tex, ...)
end

function g_draw_nine_patch(tex, x, y, w, h, tx, ty, tw, th, sides, corner, origin)
	g_draw_quad_crop(tex, x, y, sides, sides, tx, ty, sides, sides, corner, origin) -- tl
	g_draw_quad_crop(tex, x + sides, y, w - sides - sides, sides, tx + sides, ty, tw - sides - sides, sides, corner, origin) -- top
	g_draw_quad_crop(tex, x + w - sides, y, sides, sides, tx + tw - sides, ty, sides, sides, corner, origin) -- tr
	g_draw_quad_crop(tex, x, y + sides, sides, h - sides - sides, tx, ty + sides, sides, th - sides - sides, corner, origin) -- left
	g_draw_quad_crop(tex, x + sides, y + sides, w - sides - sides, h - sides - sides, tx + sides, ty + sides, tw - sides - sides, th - sides - sides, corner, origin) -- center
	g_draw_quad_crop(tex, x + w - sides, y + sides, sides, h - sides - sides, tx + tw - sides, ty + sides, sides, th - sides - sides, corner, origin) -- right
	g_draw_quad_crop(tex, x, y + h - sides, sides, sides, tx, ty + th - sides, sides, sides, corner, origin) -- bl
	g_draw_quad_crop(tex, x + sides, y + h - sides, w - sides - sides, sides, tx + sides, ty + th - sides, tw - sides - sides, sides, corner, origin) -- bottom
	g_draw_quad_crop(tex, x + w - sides, y + h - sides, sides, sides, tx + tw - sides, ty + th - sides, sides, sides, corner, origin) -- br
end

function clamp(x, min, max)
	if x > max then
		return max
	elseif x < min then
		return min
	end
	return x
end

local clamp = clamp
function lerp(from, to, t)
	from = from + (to - from) * (t > 1 and 1 or (t < 0 and 0 or t))
	return from
end

function cubic(p1, p2, p3, p4, t)
	local m = t * t
	local a = p4 - p3 - p1 + p2
	local b = p1 - p2 - a
	local c = p3 - p1
	local d = p2
	return a * t * m + b * m + c * t + d
end

function cerp(a, b, t)
	local v = t * t * (3 - 2 * t)
	return a + v * (b - a)
end

function lerp_free(from, to, t)
	from = from + (to - from) * t
	return from
end

function oscillate(speed, from, to, off)
	return lerp(from, to, math.sin((sys_secs() + (off or 0)) * speed) * 0.5 + 0.5)
end

function coscillate(speed, from, to, off)
	return lerp(from, to, math.cos((sys_secs() + (off or 0)) * speed) * 0.5 + 0.5)
end

local close_requested_obj = {}
local exit_requested_obj = {}

function sys_close_game() -- must be called from within a game
	error(close_requested_obj)
end

local ogsys_exit = sys_exit
function sys_exit()
	error(exit_requested_obj)
end

local ogpcall = pcall
local loaded_libs = {}
function lib_include(name)
	if not loaded_libs[name] then
		local f, err = loadfile("core/lib/" .. name .. ".lua", "t", _G)
			or loadfile("core/lib/" .. name .. "/" .. name .. ".lua", "t", _G)
		if not f then
			if err then
				error("error loading lib '" .. name .. "': " .. err)
			else
				error("could not find lib '" .. name .. "'")
			end
		end
		loaded_libs[name] = true
		f()
	end
end

function date_to_num(date)
	if not date:match("[0-9][0-9][0-9][0-9]%-[0-9][0-9]%-[0-9][0-9]") then
		error("date_to_num expects format YYYY-MM-DD")
	end
	return tonumber((date:gsub("%-", "")))
end

local oglog_info = log_info
function log_info(fmt, ...)
	oglog_info(fmt and string.format(fmt, ...) or tostring(fmt))
end

local oglog_err = log_err
function log_err(fmt, ...)
	oglog_err(fmt and string.format(fmt, ...) or tostring(fmt))
end

function print(...)
	local v = {...}
	for i = 1, #v do
		v[i] = tostring(v[i])
	end
	oglog_info(table.concat(v, "\t"))
end

function string.split(self, div)
	local res = {}
	local i = 1
	while true do
		local i2 = self:find(div, i)
		if i2 then
			res[#res + 1] = self:sub(i, i2 - 1)
			i = i2 + 1
		else
			return res
		end
	end
end

function string.trim(self)
	return self:match("^%s*(.-)%s*$")
end

function string.before(self, s)
	local i = self:find(s)
	return i and self:sub(1, i - 1) or self
end

function string.after(self, s)
	local _, i = self:find(s)
	return i and self:sub(i + 1) or self
end

function pcall(...)
	local res = {ogpcall(...)}
	if not res[1] then
		if res[2] == close_requested_obj then
			error(close_requested_obj)
		else
			--error(res[2])
		end
	end
	return unpack(res)
end

function strace(start_at)
	start_at = start_at and start_at + 1 or 3
	local stack = ""
	for i = start_at, start_at + 10 do
		local info = debug.getinfo(i, "Sln")
		if not info then
			break
		end
		if i ~= start_at then
			stack = stack .. "\n"
		end
		stack = stack .. (i - start_at + 1) .. ". "
		if info.what == "C" then
			stack = stack .. "C function " .. (info.name or "<unknown>")
		else
			stack = stack .. (info.short_src or "<unknown>") .. ":" .. info.currentline
			if info.what == "main" then
				stack = stack .. " in main chunk"
			elseif info.namewhat ~= "" then
				stack = stack .. " in " .. info.namewhat .. " function " .. info.name
			else
				stack = stack .. " in anonymous function"
			end
			if info.source:sub(1, 1) == "@" then
				local src = fs_file(info.source:sub(2)):read_string()
				stack = stack .. "\n" .. src:split("\n")[info.currentline]
			end
		end
	end
	return stack
end

function sys_set_input(can_input)
	CUR_GAME.DISABLE_INPUT = not can_input
end

local props = {core_version = "1.0-dev"}

function sys_prop(name)
	return props[name]
end

function sys_make_prop(name, val)
	props[name] = val
end

-- if we encounter a subcommand, run it and do not run games

do
	local args = {...}
	if #args > 1 then
		local path = fs_resolve("et:/core/command/" .. args[2] .. ".lua")
		if fs_exists(path) then
			table.remove(args, 1)
			table.remove(args, 1)
			dofile(path, unpack(args))
		else
			log_err("no command found at " .. path)
		end
		ogsys_exit()
		return
	end
end

-- operation of the core

dofile(ROOT_DIR .. "core/overlay/overlay.lua")

local function safely_handler(res)
	if CUR_GAME.GAME_ROOT == "core/" and CUR_GAME.GAME_MAIN == "launcher.lua" then
		if res == close_requested_obj then
			log_info("game close requested in launcher, exiting")
		elseif res == exit_requested_obj then
			log_info("exit requested in launcher, exiting")
		else
			log_err("Lua stack trace:\n" .. strace())
			log_err(res)
			log_err("unhandled error in launcher, exiting")
		end
		ogsys_exit()
	else
		if res == close_requested_obj then
			log_info("game close requested in game, exiting to launcher")
			load_game("core", "launcher.lua", "launcher", "Launcher")
		elseif res == exit_requested_obj then
			log_info("exit requested in game, exiting")
			ogsys_exit()
		else
			log_err("Lua stack trace:\n" .. strace())
			log_err(res)
			log_err("unhandled error in game, exiting to launcher")
			load_game("core", "launcher.lua", "launcher", "Launcher")
		end
	end
end

local function safely(f, ...)
	return xpcall(f, safely_handler, ...)
end

local function load_module(root, main)
	root = "mods/" .. root
	local index = dofile(root .. "/index.lua")
	index._root = root .. "/"
	sys_modules[index.uuid] = index
end

function load_game(root, main, id, short_name)
	if CUR_GAME then
		if CUR_GAME._close then
			safely(CUR_GAME._close)
		end
		CUR_GAME = nil
	end
	a_destroy(-1)
	local env = setmetatable({
		GAME_ROOT = root .. "/", GAME_MAIN = main, GAME_ID = id, GAME_SHORT_NAME = short_name,
		_load = false, _resize = false, _frame = false, _input = false, _cursor = false, _close = false
	}, {__index = _G})
	main = fs_resolve(root .. "/" .. main)
	CUR_GAME = env
	log_info("launching " .. main)
	local f, err = loadfile(main, "bt", env)
	if not f then
		log_err(err)
		error("could not load game at " .. main)
	end
	g_set_min_aspect(0, 0)
	g_set_max_aspect(0, 0)
	g_set_virtual_size(-1, -1)
	if safely(f) and env._load then
		safely(env._load)
	end
	env.GAME_LOADED = true
end

function _load()
	vidd = v_load("et:/user/e17x/data/OP00.wmv")
	load_module("n7")
	load_module("e17")
	load_module("e17x")
	--load_module("e17x-bgm")
	load_game("core", "launcher.lua", "launcher", "Launcher")
	--g_scale(0.5)
	--g_translate(100, 50)
	--g_rotate(math.rad(-45))
end

function _resize(rw, rh)
	if CUR_GAME._resize and CUR_GAME.GAME_LOADED then
		safely(CUR_GAME._resize, rw, rh)
	end
	overlay_invalidate()
end

local pressed_list = {}
local prev_pressed_keys = 0
local pressed_keys = 0

function sys_is_just_down(key)
	return bit.band(pressed_keys, key) > bit.band(prev_pressed_keys, key)
end

function sys_is_just_up(key)
	return bit.band(pressed_keys, key) < bit.band(prev_pressed_keys, key)
end

function sys_is_down(key)
	return bit.band(pressed_keys, key) ~= 0
end

local function input_is(self, key)
	return bit.band(self.key, key) ~= 0
end

local function input_is_down(self, key)
	return self.pressed and bit.band(self.key, key) ~= 0
end

local cursor_x, cursor_y = 0, 0
function _input(key, pressed, repeating, strength)
	if pressed then
		if not repeating then
			pressed_list[#pressed_list + 1] = key
		end
	else
		for i = #pressed_list, 1, -1 do
			if pressed_list[i] == key then
				table.remove(pressed_list, i)
				break
			end
		end
	end
	pressed_keys = 0
	for i = 1, #pressed_list do
		pressed_keys = bit.bor(pressed_keys, pressed_list[i])
	end
	local _, handled = safely(overlay_input, {
		key = key,
		strength = strength,
		pressed = pressed,
		repeating = repeating,
		is = input_is,
		is_down = input_is_down,
		x = cursor_x,
		y = cursor_y
	})
	if not handled then
		if CUR_GAME._input and CUR_GAME.GAME_LOADED and not CUR_GAME.DISABLE_INPUT then
			safely(CUR_GAME._input, {
				key = key,
				strength = strength,
				pressed = pressed,
				repeating = repeating,
				is = input_is,
				is_down = input_is_down,
				x = cursor_x,
				y = cursor_y
			})
		end
	end
end

function _cursor(x, y, dragging)
	cursor_x, cursor_y = x, y
	local _, handled = safely(overlay_cursor, x, y, dragging)
	if not handled then
		if CUR_GAME._cursor and CUR_GAME.GAME_LOADED and not CUR_GAME.DISABLE_INPUT then
			safely(CUR_GAME._cursor, x, y, dragging)
		end
	end
end

function _close()
	if CUR_GAME._close and CUR_GAME.GAME_LOADED then
		safely(CUR_GAME._close)
	end
end

local src = "KO1"
local model
local tried_load_model = false

delta = 0.1
sys_fps = 1
local last_sec
local last_frame = sys_millis()
local frames = 0
function _frame()
	if not last_sec then last_sec = sys_millis() end
	tex_cache_hold_prev = tex_cache_hold
	tex_cache_hold = {}
	g_reset_stacks()
	if CUR_GAME._frame and CUR_GAME.GAME_LOADED then
		g_push()
		safely(CUR_GAME._frame)
		g_pop()
	end
	g_flush()
	g_reset_stacks()
	safely(overlay_frame)
	g_flush()
	g_reset_stacks()
	--v_decode_video(vidd)
	--v_decode_audio(vidd)
	--v_draw(vidd, 0, 0, g_width, g_height)

	if test_model and not model and not tried_load_model then
		model = g_load_scene("et:/user/e17x/Model.afs:" .. src .. ".vpk:Users/izutsu_sana/Desktop/VPK0926_FIX/" .. src .. "/" .. src .. ".vmx2")
		tried_load_model = true
	end
	if model and test_model then
		g_push()
		g_proj_perspective(math.rad(70), g_width / g_height, 0.1, 1000)
		--g_light(0, G_POINT_LIGHT, 0, 0, 0, 15)
		local AngleY, AngleX = 0, 0
		g_identity()
		g_rotate(math.rad(AngleY), 0, 1, 0)
		g_rotate(math.rad(AngleX), 1, 0, 0)
		g_light(0, G_DIRECTIONAL_LIGHT, 0, 0, 1, 0.5, 0.5, 0.5)
		g_identity()
		local Ambient = {0.5, 0.1, 0.1}
		g_light_ambient(Ambient[1], Ambient[2], Ambient[3])
		g_translate(0, -12, -10)
		--g_rotate(math.rad(180), 0, 1, 0)
		g_rotate(math.rad(0 + 180), 0, 1, 0)
		g_rotate(math.rad(0), 0, 0, 1)
		g_rotate(math.rad(0), 1, 0, 0)
		--g_scale(preset[4], preset[5], 1)
		g_draw_scene(model)
		g_proj_reset()
		g_flush()
		g_pop()
		g_reset_stacks()
	end
	g_set_virtual_size(-1, -1)
	g_set_max_aspect(0, 0)
	g_set_min_aspect(0, 0)
	local t = sys_millis()
	delta = (t - last_frame) / 1000
	--if delta > 0.9 then delta = 0.9 end
	last_frame = t
	if t >= last_sec + 1000 then
		sys_fps = frames
		--log_info(frames .. " fps, " .. sys_draw_calls .. " calls, " .. sys_polys .. " polys")
		frames = 1
		last_sec = t
	else
		frames = frames + 1
	end
	prev_pressed_keys = pressed_keys
end