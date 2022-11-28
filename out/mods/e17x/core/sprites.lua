og_root = "et:/user/e17x"

vn_add_loader("sprite", function(id)
	id = id:upper()
	return g_load_scene(fs_resolve("pkg:/sprite/" .. id .. "/" .. id .. ".vmx2"))
end, function(id)
	if src:find("SO") then return nil end
	local model = self.models[src]
	if not model then
		if src:find("MU") then
			model = g_load_scene(og_root .. "mods/e17x-sprites/Model.afs:" .. src .. ".vpk:Users/izutsu_sana/Desktop/VPK0902/" .. src .. "/" .. src .. ".vmx2")
		else
			model = g_load_scene(og_root .. "media/Model.afs:" .. src .. ".vpk:Users/izutsu_sana/Desktop/VPK0926_FIX/" .. src .. "/" .. src .. ".vmx2")
		end
		--model = g_load_scene("mods/e17x-sprites/models/" .. src .. "/" .. src .. ".vmx2")
		self.models[src] = model
	end
	id = id:upper()
	return g_load_scene(fs_resolve("pkg:/sprite/" .. id .. "/" .. id .. ".vmx2"))
end, function(model)
	if not model then return end
	g_push()
	g_proj_perspective(math.rad(70), 16 / 9, 0.1, 1000)
	--g_light(0, G_POINT_LIGHT, 0, 0, 0, 15)
	local model_light = self.executor.vars.model_light
	g_identity()
	g_rotate(math.rad(model_light.AngleY), 0, 1, 0)
	g_rotate(math.rad(model_light.AngleX), 1, 0, 0)
	--g_translate(preset[])
	--g_light(0, G_DIRECTIONAL_LIGHT, 0, -1, 0, model_light.Diffuse[1], model_light.Diffuse[2], model_light.Diffuse[3])
	g_light(0, G_DIRECTIONAL_LIGHT, 0, 0, 1, model_light.Diffuse[1] / 4, model_light.Diffuse[2] / 4, model_light.Diffuse[3] / 4)
	g_identity()
	g_light_ambient(model_light.Ambient[1], model_light.Ambient[2], model_light.Ambient[3])
	g_translate(preset[1] * 8, preset[2] - 12, preset[3] - 10)
	--g_rotate(math.rad(180), 0, 1, 0)
	g_rotate(math.rad((preset[7] or 0) + 180), 0, 1, 0)
	g_rotate(math.rad(preset[8] or 0), 0, 0, 1)
	g_rotate(math.rad(preset[6] or 0), 1, 0, 0)
	--g_scale(preset[4], preset[5], 1)
	g_draw_scene(model)
	g_proj_reset()
	g_pop()
end)