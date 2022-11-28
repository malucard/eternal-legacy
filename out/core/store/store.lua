_G.GLOBAL_ROOT = "../../"
dofile("elements/package_small.lua")
dofile("home.lua")
dofile("top_bar.lua")

local store_screen = EuiLayout {
	store_top_bar;
	EuiSeparator {};
	store_home;
	background = EuiTexView {
		src = "menubg.png";
		crop = true;
	};
	foreground = EuiButtonHints {
		INPUT_MENU, "Search", INPUT_MORE, "Bookmarked", INPUT_BACK, "Exit"
	};
}

trans = nil

function _load()
	trans = trans_start(TRANS_FADE_IN)
end

local vw, vh = -1, -1
function _resize(rw, rh)
	halo_distance = 3
	if rh <= 360 then
		vw, vh = rw * 2, rh * 2
	elseif g_real_height <= 576 then
		vw, vh = rw * 1.25, rh * 1.25
	elseif g_real_height <= 600 then
		--vw, vh = rw * 2, rh * 2
		vw, vh = -1, -1
	elseif g_real_height >= 720 then
		local scale = g_real_height / 720 -- scale infinitely on each multiple of 720
		vw, vh = math.floor(rw / scale), math.floor(rh / scale)
	else
		vw, vh = -1, -1
	end
	g_set_virtual_size(vw, vh)
	store_screen:invalidate()
end

_frame = trans_frame(function()
	store_screen:draw(0, 0, g_width, g_height)
end, _ENV, "trans")

local cursor_x, cursor_y = 0, 0
function _cursor(x, y, dragging)
	if not dragging then
		store_screen:on_hover(true, x, y)
	end
	cursor_x, cursor_y = x, y
end

function _input(inp)
	if inp.pressed and inp:is(INPUT_BACK) then
		sys_set_input(false)
		trans = trans_start(TRANS_FADE_OUT, function()
			sys_close_game()
		end)
	elseif inp:is(INPUT_CLICK) then
		store_screen:on_click(inp.pressed, cursor_x, cursor_y)
	end
end
