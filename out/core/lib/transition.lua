local slots = {}

--- default transition duration (not to be changed)
TRANS_DURATION = 0.15

function TRANS_WAIT_OUT()
end

function TRANS_WAIT_IN(t, draw)
	draw()
end

function TRANS_FADE(t, draw1, draw2)
	draw1()
	g_push_color(1, 1, 1, t)
	draw2()
	g_pop_color()
end

function TRANS_FADE_IN(t, draw)
	g_push_color(1, 1, 1, t)
	draw()
	g_pop_color()
end

function TRANS_FADE_OUT(t, draw)
	g_push_color(1, 1, 1, 1 - t)
	if t ~= 1.0 then
		draw()
	end
	g_pop_color()
end

function TRANS_ENTER_IN(t, draw1, draw2)
	g_push()
	g_translate(g_width / 2, g_height / 2)
	g_scale(t / 2 + 0.5)
	g_translate(g_width / -2, g_height / -2)
	g_push_color(1, 1, 1, t)
	draw1()
	g_pop_color()
	g_pop()
end

function TRANS_ENTER_OUT(t, draw1, draw2)
	g_push()
	g_translate(g_width / 2, g_height / 2)
	g_scale(t + 1)
	g_translate(g_width / -2, g_height / -2)
	g_push_color(1, 1, 1, 1 - t)
	if t ~= 1.0 then
		draw1()
	end
	g_pop_color()
	g_pop()
end

function TRANS_EXIT_IN(t, draw1, draw2)
	g_push()
	g_translate(g_width / 2, g_height / 2)
	g_scale(2 - t)
	g_translate(g_width / -2, g_height / -2)
	g_push_color(1, 1, 1, t)
	draw1()
	g_pop_color()
	g_pop()
end

function TRANS_EXIT_OUT(t, draw1, draw2)
	g_push()
	g_translate(g_width / 2, g_height / 2)
	g_scale(1 - t / 2)
	g_translate(g_width / -2, g_height / -2)
	g_push_color(1, 1, 1, 1 - t)
	if t ~= 1.0 then
		draw1()
	end
	g_pop_color()
	g_pop()
end

function TRANS_LTR(t, draw1, draw2)
	g_push_color(1, 1, 1, t)
	if t ~= 1.0 then
		draw()
	end
	g_pop_color()
end

function TRANS_RTL(t, draw1, draw2)
	g_push_color(1, 1, 1, t)
	if t ~= 1.0 then
		draw()
	end
	g_pop_color()
end

local slot = {}

function trans_start(trans, on_done, time)
	slot = {trans, sys_millis(), time or 0.15, on_done}
end

function trans_skip()
	slot[3] = 0
	if slot[4] then
		slot[4]()
	end
end

function trans_check_draw(draw1, draw2, opt) -- will be just draw, opt for single-draw transitions
	--[[if not slot then
		draw1()
		return
	end
	local slot = type(slot) == "table" and slot or slots[slot] ]]
	if slot[1] then
		local t = (sys_millis() - slot[2]) / (slot[3] * 1000)
		if t >= 1.0 or t < 0.0 then
			slot[1](1.0, draw1, draw2, opt)
			if slot[4] then
				cb = slot[4]
				slot[4] = nil
				cb()
			end
			--slot[1] = nil
			return true
		elseif t < 0.0 then
			slot[1] = nil
			draw1()
			if opt then
				draw2()
			end
		else
			slot[1](t, draw1, draw2, opt)
		end
	else
		draw1()
		if opt then
			draw2()
		end
	end
end

local trans_check_draw = trans_check_draw

function trans_frame(frame, env, key)
	return function()
		trans_check_draw(frame)
	end
end