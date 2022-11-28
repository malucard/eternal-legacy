return {
	uuid = "d406f132-a386-4f0f-bc6e-ea414f0311ea",
	name = "Ever17 -the out of infinity- Eternal Edition",
	desc = "Ever17.",
	id = "e17",
	provides = {{
		type = "game",
		name = "Ever17 -the out of infinity- Eternal Edition",
		short_name = "Ever17",
		original_release = "2002-08-28",
		id = "e17",
		bg = PLATFORM_PSP and "meta/bg.png" or "meta/bg_hd.png",
		icon = "meta/icon.png",
		main = "core/main.lua",
		parts = {"e17:script", "e17:bg", "e17:sprite", "e17:bgm", "e17:sfx", "e17:voice"},
	}}
}
