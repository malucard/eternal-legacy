lib_include("halo")

local pc_map = {
	[INPUT_OK] = "res/buttons/enter.png",
	--[INPUT_BACK] = "res/buttons/backspace.png",
	[INPUT_BACK] = "res/buttons/esc.png",
	[INPUT_MENU] = "res/buttons/esc.png",
	[INPUT_MENU_BACK] = "res/buttons/esc.png",
	[INPUT_SYS_MENU] = "res/buttons/del.png",
	[INPUT_MORE] = "res/buttons/keyb.png",
	[INPUT_SKIP] = "res/buttons/ctrl.png",
	[INPUT_SKIP_INSTANT] = "res/buttons/keyk.png",
	[INPUT_HIDE] = "res/buttons/f1.png",
	[INPUT_UP] = "res/buttons/dpadu.png",
	[INPUT_DOWN] = "res/buttons/dpadd.png",
	[INPUT_UP | INPUT_DOWN] = "res/buttons/dpadud.png",
	[INPUT_LEFT] = "res/buttons/dpadl.png",
	[INPUT_RIGHT] = "res/buttons/dpadr.png",
	[INPUT_LEFT | INPUT_RIGHT] = "res/buttons/dpadlr.png",
	[INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT] = "res/buttons/dpad.png"
}

local xbox_map = {
	[INPUT_OK] = "res/buttons/xboxa.png",
	[INPUT_BACK] = "res/buttons/xboxb.png",
	[INPUT_MENU] = "res/buttons/xboxx.png",
	[INPUT_MENU_BACK] = "res/buttons/xboxx.png",
	[INPUT_MORE] = "res/buttons/xboxy.png",
	[INPUT_SKIP] = "res/buttons/xboxb.png",
	[INPUT_SKIP_INSTANT] = "res/buttons/rt.png", -- should be menu
	[INPUT_HIDE] = "res/buttons/lb.png",
	[INPUT_UP] = "res/buttons/dpadu.png",
	[INPUT_DOWN] = "res/buttons/dpadd.png",
	[INPUT_UP | INPUT_DOWN] = "res/buttons/dpadud.png",
	[INPUT_LEFT] = "res/buttons/dpadl.png",
	[INPUT_RIGHT] = "res/buttons/dpadr.png",
	[INPUT_LEFT | INPUT_RIGHT] = "res/buttons/dpadlr.png",
	[INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT] = "res/buttons/dpad.png"
}

local psp_map = {
	[INPUT_OK] = "res/buttons/pscross.png",
	[INPUT_BACK] = "res/buttons/pscircle.png",
	[INPUT_MENU] = "res/buttons/pssquare.png",
	[INPUT_MENU_BACK] = "res/buttons/pssquare.png",
	[INPUT_SYS_MENU] = "res/buttons/pspselect.png",
	[INPUT_MORE] = "res/buttons/pstriangle.png",
	[INPUT_SKIP] = "res/buttons/pscircle.png",
	[INPUT_SKIP_INSTANT] = "res/buttons/pspstart.png",
	[INPUT_HIDE] = "res/buttons/l.png",
	[INPUT_UP] = "res/buttons/dpadu.png",
	[INPUT_DOWN] = "res/buttons/dpadd.png",
	[INPUT_UP | INPUT_DOWN] = "res/buttons/dpadud.png",
	[INPUT_LEFT] = "res/buttons/dpadl.png",
	[INPUT_RIGHT] = "res/buttons/dpadr.png",
	[INPUT_LEFT | INPUT_RIGHT] = "res/buttons/dpadlr.png",
	[INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT] = "res/buttons/dpad.png"
}

local nint_map = {
	[INPUT_OK] = "res/buttons/a.png",
	[INPUT_BACK] = "res/buttons/b.png",
	[INPUT_MENU] = "res/buttons/y.png",
	[INPUT_MENU_BACK] = "res/buttons/y.png",
	[INPUT_SYS_MENU] = "res/buttons/minus.png",
	[INPUT_MORE] = "res/buttons/x.png",
	[INPUT_SKIP] = "res/buttons/b.png",
	[INPUT_SKIP_INSTANT] = "res/buttons/plus.png",
	[INPUT_HIDE] = "res/buttons/l.png",
	[INPUT_UP] = "res/buttons/dpadu.png",
	[INPUT_DOWN] = "res/buttons/dpadd.png",
	[INPUT_UP | INPUT_DOWN] = "res/buttons/dpadud.png",
	[INPUT_LEFT] = "res/buttons/dpadl.png",
	[INPUT_RIGHT] = "res/buttons/dpadr.png",
	[INPUT_LEFT | INPUT_RIGHT] = "res/buttons/dpadlr.png",
	[INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT] = "res/buttons/dpad.png"
}

function bh_load_icon(input)
	if PLATFORM_PC then
		return g_load_texture("et:/" .. pc_map[input], TEX_NEAREST)
	elseif PLATFORM_PSP then
		return g_load_texture("et:/" .. psp_map[input], TEX_NEAREST)
	elseif PLATFORM_PS then
		return g_load_texture("et:/" .. ps_map[input], TEX_NEAREST)
	elseif PLATFORM_NINTENDO then
		return g_load_texture("et:/" .. nint_map[input], TEX_NEAREST)
	end
end

function bh_draw(inputs)
	local x = 16
	for _, inp in ipairs(inputs) do
		if type(inp) == "string" then
			local textinfo = g_draw_text(inp, x, -22, -1, 24, {halo_alpha = 0.75, halo_detail = -1, shadow_distance = 2.5, shadow_detail = 1}, G_BOTTOM_LEFT, G_BOTTOM_LEFT)
			x = x + textinfo.w + 16
		else
			local icon = bh_load_icon(inp)
			g_push_color(0, 0, 0, 0.5)
			g_draw_quad(icon, x + 4, -12, 32, 32, G_BOTTOM_LEFT, G_BOTTOM_LEFT)
			g_pop_color()
			g_draw_quad(icon, x, -16, 32, 32, G_BOTTOM_LEFT, G_BOTTOM_LEFT)
			x = x + 40
		end
	end
end