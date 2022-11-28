#include "game.hpp"
#include <graphics.hpp>
#include <game/n7/n7.hpp>

GameMeta* cur_game_meta = nullptr;
Game* cur_game = nullptr;

GameMeta* MainMenu::get_meta() {
	return new GameMeta {.basic_name = "Eternal"s, .full_name = "Eternal"s};
}

void MainMenu::load() {
	n7_bg = g_load_texture(FileDataStream("res/n7_bg.png"));
	n7_icon = g_load_texture(FileDataStream("res/n7_icon.png"));
	e17_bg = g_load_texture(FileDataStream("res/e17_bg.png"));
	e17_icon = g_load_texture(FileDataStream("res/e17_icon.png"));
	r11_bg = g_load_texture(FileDataStream("res/r11_bg.png"));
	r11_icon = g_load_texture(FileDataStream("res/r11_icon.png"));
	btn_o = g_load_texture(FileDataStream("res/buttons/o.png"));
	btn_o.data->upscale = false;
	btn_x = g_load_texture(FileDataStream("res/buttons/x.png"));
	btn_x.data->upscale = false;
	btn_square = g_load_texture(FileDataStream("res/buttons/square.png"));
	btn_square.data->upscale = false;
	btn_triangle = g_load_texture(FileDataStream("res/buttons/triangle.png"));
	btn_triangle.data->upscale = false;
}

void MainMenu::frame() {
	int w = g_width(), h = g_height();
	g_draw_quad(selected == 0? n7_bg: selected == 1? e17_bg: r11_bg, 0, 0, -1, h, G_TOP_RIGHT, G_TOP_RIGHT);
	if(selected == 0) {
		g_push_color(0.f, 0.f, 0.f, 0.5f);
		g_draw_quad(n7_icon, -248_600p, 8_600p, -1, 128_600p, G_CENTER, G_CENTER);
		g_pop_color();
		g_draw_quad(n7_icon, -256_600p, 0, -1, 128_600p, G_CENTER, G_CENTER);
	} else {
		g_push_color(1.f, 1.f, 1.f, 0.5f);
		g_draw_quad(n7_icon, -256_600p, 0, -1, 128_600p, G_CENTER, G_CENTER);
		g_pop_color();
	}
	if(selected == 1) {
		g_push_color(0.f, 0.f, 0.f, 0.5f);
		g_draw_quad(e17_icon, 8_600p, 8_600p, -1, 128_600p, G_CENTER, G_CENTER);
		g_pop_color();
		g_draw_quad(e17_icon, 0, 0, -1, 128_600p, G_CENTER, G_CENTER);
	} else {
		g_push_color(1.f, 1.f, 1.f, 0.5f);
		g_draw_quad(e17_icon, 0, 0, -1, 128_600p, G_CENTER, G_CENTER);
		g_pop_color();
	}
	if(selected == 2) {
		g_push_color(0.f, 0.f, 0.f, 0.5f);
		g_draw_quad(r11_icon, 264_600p, 8_600p, -1, 128_600p, G_CENTER, G_CENTER);
		g_pop_color();
		g_draw_quad(r11_icon, 256_600p, 0, -1, 128_600p, G_CENTER, G_CENTER);
	} else {
		g_push_color(1.f, 1.f, 1.f, 0.5f);
		g_draw_quad(r11_icon, 256_600p, 0, -1, 128_600p, G_CENTER, G_CENTER);
		g_pop_color();
	}
	g_draw_quad(btn_o, 16_600p, -16_600p, 32_600p, 32_600p, G_BOTTOM_LEFT, G_BOTTOM_LEFT);
	g_push_color(0.f, 0.f, 0.f, 1.f);
	g_draw_text("Launch"s, 50_480p, -42_480p, 12903, 24_480p, G_TOP_LEFT, G_BOTTOM_LEFT);
	g_pop_color();
	int tw = g_draw_text("Launch"s, 48_480p, -44_480p, 12903, 24_480p, G_TOP_LEFT, G_BOTTOM_LEFT).w;
	g_draw_quad(btn_square, 16_600p + tw, -16_600p, 32_600p, 32_600p, G_BOTTOM_LEFT, G_BOTTOM_LEFT);
	g_push_color(0.f, 0.f, 0.f, 1.f);
	g_draw_text("Options"s, 50_480p + tw, -42_480p, 12903, 24_480p, G_TOP_LEFT, G_BOTTOM_LEFT);
	g_pop_color();
	g_draw_text("Options"s, 48_480p + tw, -44_480p, 12903, 24_480p, G_TOP_LEFT, G_BOTTOM_LEFT);
}

void MainMenu::input(int key, int action) {
	if(action) {
		switch(key) {
			case INPUT_RIGHT:
				selected = selected == 0? 1: selected == 1? 2: 0;
				break;
			case INPUT_LEFT:
				selected = selected == 0? 2: selected == 1? 0: 1;
				break;
			case INPUT_OK:
				if(selected == 0) {
					close();
					delete this;
					delete cur_game_meta;
					cur_game_meta = Never7::get_meta();
					cur_game = new Never7();
					switch_game();
				}
				break;
		}
	}
}

void MainMenu::cursor(int x, int y, bool dragging) {
	int h = g_height();
	int ph = scale(128, h);
	int pw = ph * n7_icon.width() / n7_icon.height();
	int px = scale(-256, h) - pw / 2, py = -ph / 2;
	g_calc_point(&px, &py, G_CENTER);
	if(x >= px && y >= py && x < px + pw && y < py + ph) {
		selected = 0;
	}
	px += scale(256, h);
	if(x >= px && y >= py && x < px + pw && y < py + ph) {
		selected = 1;
	}
	px += scale(256, h);
	if(x >= px && y >= py && x < px + pw && y < py + ph) {
		selected = 2;
	}
}

void MainMenu::close() {}
