local full_turn = math.rad(360)

function hili_draw_shadow_halo(draw, halo_alpha, halo_lightness, halo_detail, halo_distance, shadow_distance, shadow_detail)
	if not alpha then
		alpha = 0.5
	end
	if not halo_distance then
		halo_distance = 2
	end
	if not halo_detail then
		local rh = g_real_height
		if rh <= 360 then
			halo_detail = 3 --3 * (halo_distance * g_real_width / g_width) / 2
			--halo_detail = 3
		elseif g_real_height < 480 then
			halo_detail = 4 * (halo_distance * g_real_width / g_width) / 2
			--halo_detail = 4
		elseif g_real_height >= 720 then
			local scale = math.floor(g_real_height / 720) -- scale infinitely on each multiple of 720
			halo_detail = 5 * (halo_distance * g_real_width / g_width) / 2 * scale
		else
			halo_detail = 5 * (halo_distance * g_real_width / g_width) / 2
		end
		--halo_detail = 4 * (halo_distance * g_real_width / g_width) / 2
	end
	if not shadow_detail then
		shadow_detail = 1
	end
	if shadow_detail ~= 0 then
		g_push_color(0, 0, 0, 0.5)
		if not shadow_distance then
			shadow_distance = 2
		end
		local shadow_step = shadow_distance / shadow_detail
		for dist = shadow_step, shadow_detail * shadow_step, shadow_step do
			draw(dist, dist)
		end
		g_pop_color()
	end
	if halo_detail ~= 0 then
		g_push_color(0, 0, 0, halo_alpha) -- make the halo half as opaque as the shadow and the normal one
		g_push_color_off(halo_lightness, halo_lightness, halo_lightness, 0)
		for r = 0, full_turn, full_turn / halo_detail do
			draw(math.cos(r) * halo_distance, math.sin(r) * halo_distance)
		end
		g_pop_color_off()
		g_pop_color()
	end
	return draw(0, 0)
end