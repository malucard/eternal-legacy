#include "n7.hpp"
#include <audio.hpp>

using namespace n7;

TitleMode::TitleMode() {
	bg[0] = g_load_texture(FileDataStream("res/n7/dc/title1.png"));
	bg[1] = g_load_texture(FileDataStream("res/n7/dc/title2.png"));
	bg[2] = g_load_texture(FileDataStream("res/n7/dc/title3.png"));
	bg[3] = g_load_texture(FileDataStream("res/n7/dc/title4.png"));
	logo = g_load_texture(FileDataStream("res/n7/dc/logo.png"));
	buttons = g_load_texture(FileDataStream("res/n7/dc/title_buttons.png"));
	grayed = g_load_texture(FileDataStream("res/n7/special/grayed.png"));
	yuka = g_load_texture(FileDataStream("res/n7/special/yuka.png"));
}

void TitleMode::input(int key, int action) {
	switch(state) {
		case STATE_NORMAL:
			if(action) {
				switch(key) {
					case INPUT_UP:
						selected--;
						if(selected == -1) selected = 6;
						break;
					case INPUT_DOWN:
						selected++;
						if(selected == 7) selected = 0;
						break;
					case INPUT_OK:
						if(selected == 0) {
							delete this;
							cur_mode = new GameMode();
						} else if(selected == 5) {
							state = STATE_SETTINGS;
							a_play("res/n7/dc/chosen.wav", A_FMT_WAV, false, volume_gui / 100.f);
						} else if(selected == 6) {
							state = STATE_MODS;
							a_play("res/n7/dc/chosen.wav", A_FMT_WAV, false, volume_gui / 100.f);
						}
						break;
				}
			}
			break;
		case STATE_MODS:
			if(action) {
				switch(key) {
					case INPUT_OK: case INPUT_RIGHT: case INPUT_LEFT: case INPUT_UP: case INPUT_DOWN:
						mods_menu_input(action, key);
						break;
					case INPUT_BACK: case INPUT_MENU_BACK:
						if(!mods_menu_input(action, key)) {
							state = STATE_NORMAL;
							a_play("res/n7/dc/back.wav", A_FMT_WAV, false, volume_gui / 100.f);
							mods_menu_close();
						}
						break;
				}
			}
			break;
		case STATE_SETTINGS:
			if(action) {
				switch(key) {
					case INPUT_OK: case INPUT_RIGHT: case INPUT_LEFT: case INPUT_UP: case INPUT_DOWN:
						settings_input(action, key);
						break;
					case INPUT_BACK: case INPUT_MENU_BACK:
						if(!settings_input(action, key)) {
							state = STATE_NORMAL;
							a_play("res/n7/dc/back.wav", A_FMT_WAV, false, volume_gui / 100.f);
							settings_close();
						}
						break;
				}
			}
			break;
	}
}

void TitleMode::cursor(int x, int y, bool dragging) {
	if(state == STATE_NORMAL) {
		int w = g_width(), h = g_height();
		if(x >= w / 2 - scale(64, h, 480) && x < w / 2 + scale(64, h, 480)) {
			for(int i = 0; i < 7; i++) {
				if(y >= scale(272 + 24 * i, h, 480) && y < scale(272 + 24 * (i + 1), h, 480)) {
					selected = i;
					break;
				}
			}
		}
	} else if(state == STATE_MODS) {
		mods_menu_cursor(x, y, dragging);
	} else if(state == STATE_SETTINGS) {
		settings_cursor(x, y, dragging);
	}
}

void TitleMode::draw() {
	if(state == STATE_NORMAL) {
		int w = g_width();
		int h = g_height();
		u32 time = g_get_millis();
		u32 bgt = time % 4000;
		int bgstep = bgt / 1000;
		float bgp = bgt % 1000 / 1000.f;
		g_draw_quad(bg[bgstep], 0, 0, w, h, G_TOP_LEFT, G_TOP_LEFT);
		g_push_color(1.f, 1.f, 1.f, bgp);
		bgstep++;
		g_draw_quad(bg[bgstep > 3? 0: bgstep], 0, 0, w, h, G_TOP_LEFT, G_TOP_LEFT);
		g_pop_color();
		int lw = 256;
		g_draw_quad(logo, scale(32, h, 480), scale(32, h, 480), -1, scale(256, h, 480), G_BOTTOM_LEFT, G_CENTER_LEFT);
		for(int i = 0; i < 7; i++) {
			int j = i * 24;
			if(i > 3) j += 88 + 9 * 24;
			float p = (float) i / 6.f;
			if(selected == i) {
				button_last_highlighted[i] = time;
				g_push_color(lerp(235.f, 250.f, p) / 255.f, lerp(115.f, 20.f, p) / 255.f, lerp(15.f, 0.f, p) / 255.f, 1.f);
			} else if(time - button_last_highlighted[i] < 500) {
				float tp = (time - button_last_highlighted[i]) / 500.f;
				float r = lerp(lerp(235.f, 250.f, p), 15.f, tp) / 255.f;
				float g = lerp(lerp(115.f, 20.f, p), lerp(75.f, 20.f, p), tp) / 255.f;
				float b = lerp(lerp(15.f, 0.f, p), lerp(255.f, 105.f, p), tp) / 255.f;
				g_push_color(r, g, b, 1.f);
			} else {
				g_push_color(15.f / 255.f, lerp(75.f, 20.f, p) / 255.f, lerp(1.f, 105.f / 255.f, p), 1.f);
			}
			g_draw_quad_crop(buttons, scale(32 + lw, h, 480), _480p(32 + 24 * i), -1, 24_480p, 0, j, 256, 24, G_CENTER_TOP, G_CENTER_LEFT);
			g_pop_color();
		}
		g_draw_quad(grayed, 0, 0, -1, h, G_TOP_RIGHT, G_TOP_RIGHT);
		g_draw_quad(yuka, 0, 0, -1, h, G_TOP_RIGHT, G_TOP_RIGHT);
	} else if(state == STATE_SETTINGS) {
		settings_draw();
	} else if(state == STATE_MODS) {
		mods_menu_draw();
	}
}
