return {
	uuid = "5e63254b-2864-45b1-97c2-4b685ed18ee6",
	name = "Ever17 (Xbox 360) Eternal Edition",
	desc = "Ever17's Xbox 360 remake.",
	id = "e17x",
	provides = {{
		type = "game",
		name = "Ever17 (Xbox 360) Eternal Edition",
		short_name = "Ever17 (Xbox 360)",
		original_release = "2011-00-00",
		id = "e17x",
		bg = PLATFORM_PSP and "meta/bg.png" or "meta/bg_hd.png",
		icon = "meta/icon.png",
		main = "core/main.lua",
		parts = {"e17x:script", "e17x:bg", "e17x:sprite", "e17x:bgm", "e17x:sfx", "e17x:voice"},
	}}
}