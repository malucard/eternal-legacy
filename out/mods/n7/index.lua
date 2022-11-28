return {
	uuid = "3f0ce6d4-e1f0-4c84-b4a0-0b3b80e72ac5",
	name = "Never7 -the end of infinity- Eternal Edition",
	desc = "Never7.",
	id = "n7",
	provides = {{
		type = "game",
		name = "Never7 -the end of infinity- Eternal Edition",
		short_name = "Never7",
		original_release = "2000-03-23",
		id = "n7",
		bg = PLATFORM_PSP and "core/meta/n7_bg.png" or "core/meta/n7_bg_hd.png",
		icon = "core/meta/n7_icon.png",
		snd = "core/meta/n7_snd.ogg",
		snd = "music/pc/track_21.ogg",
		main = "core/main.lua",
		files = {"core"},
		parts = {"n7:script", "n7:bg", "n7:sprite", "n7:bgm", "n7:sfx", "n7:voice"},
	}}
}
