#include "n7.hpp"

namespace n7 {

Mode* cur_mode;

static void* cached_colors_textbox = nullptr;
static Color text_colors[19];

Color get_text_color(int which) {
	if(cached_colors_textbox != ((Never7*) cur_game)->textbox_tex.data) {
		for(int i = 0; i < 18; i++) {
			text_colors[i] = ((Never7*) cur_game)->textbox_tex.get_pixel(8 + 16 * (i % 6), 98 + 4 * (i / 6), 128, 128);
		}
		text_colors[18] = ((Never7*) cur_game)->textbox_tex.get_pixel(112, 16, 128, 128);
		cached_colors_textbox = ((Never7*) cur_game)->textbox_tex.data;
	}
	return text_colors[which];
}

void draw_window(int x, int y, int w, int h, int corner, int origin) {
	int ws = 32_600p;
	g_calc(&x, &y, &w, &h, w, h, corner, origin);
	//g_draw_quad_crop(textbox_tex, x, y, 96, 96, 0, 0, 96, 96, corner, origin);
	g_draw_quad_crop(((Never7*) cur_game)->textbox_tex, x + ws, y + ws, w - ws - ws, h - ws - ws, 32, 32, 32, 32);
	g_draw_quad_crop(((Never7*) cur_game)->textbox_tex, x + ws, y, w - ws - ws, ws, 32, 0, 32, 32);
	g_draw_quad_crop(((Never7*) cur_game)->textbox_tex, x + ws, y + h - ws, w - ws - ws, ws, 32, 64, 32, 32);
	g_draw_quad_crop(((Never7*) cur_game)->textbox_tex, x + w - ws, y + ws, ws, h - ws - ws, 64, 32, 32, 32);
	g_draw_quad_crop(((Never7*) cur_game)->textbox_tex, x, y + ws, ws, h - ws - ws, 0, 32, 32, 32);
	g_draw_quad_crop(((Never7*) cur_game)->textbox_tex, x, y, ws, ws, 0, 0, 32, 32);
	g_draw_quad_crop(((Never7*) cur_game)->textbox_tex, x + w - ws, y, ws, ws, 64, 0, 32, 32);
	g_draw_quad_crop(((Never7*) cur_game)->textbox_tex, x, y + h - ws, ws, ws, 0, 64, 32, 32);
	g_draw_quad_crop(((Never7*) cur_game)->textbox_tex, x + w - ws, y + h - ws, ws, ws, 64, 64, 32, 32);
}

/*
void gui_n7_layout(std::function<void()> contents) {
	int ev = gui_input.event;
	gui_input.event = GUI_EVENT_NONE;
	gui_push();
	gui_state.corner = G_TOP_LEFT;
	gui_state.actually_draw = false;
	gui_state.pad(24);
	gui_state.min_w = 0;
	gui_state.min_h = 0;
	contents();
	gui_state.min_w += 48;
	gui_state.min_h += 48;
	GuiState g = gui_pop();
	int gx = gui_state.x, gy = gui_state.y;
	if(gui_state.corner == G_CENTER_TOP || gui_state.corner == G_CENTER || gui_state.corner == G_CENTER_BOTTOM) {
		gx -= g.min_w / 2;
	}
	if(gui_state.corner == G_CENTER_LEFT || gui_state.corner == G_CENTER || gui_state.corner == G_CENTER_RIGHT) {
		gy -= g.min_h / 2;
	}
	if(ev != GUI_EVENT_NONE) {
		gui_push();
		gui_state.corner = G_TOP_LEFT;
		gui_state.actually_draw = false;
		gui_state.x = gx;
		gui_state.y = gy;
		gui_state.pad(24);
		gui_state.w = g.min_w - 48;
		gui_state.h = g.min_h - 48;
		gui_input.event = ev;
		contents();
		gui_pop();
	}
	if(gui_state.actually_draw) {
		gui_push();
		draw_window(gui_scale(gui_state.x), gui_scale(gui_state.y), gui_scale(g.min_w), gui_scale(g.min_h), gui_state.corner, gui_state.corner);
		if(gui_state.corner == G_CENTER_TOP || gui_state.corner == G_CENTER || gui_state.corner == G_CENTER_BOTTOM) {
			gx += 400 * gui_state.res / 600;
		}
		if(gui_state.corner == G_CENTER_LEFT || gui_state.corner == G_CENTER || gui_state.corner == G_CENTER_RIGHT) {
			gy += 300 * gui_state.res / 600;
		}
		gui_state.corner = G_TOP_LEFT;
		gui_state.x = gx;
		gui_state.y = gy;
		gui_state.pad(24);
		gui_state.w = g.min_w - 48;
		gui_state.h = g.min_h - 48;
		contents();
		gui_pop();
	}
	gui_state.grow(g.min_w, g.min_h);
}*/

void draw_beam(int x, int y, int w, int h) {
	Color c = get_text_color(18);
	g_push_color(c);
	g_draw_quad_crop(((Never7*) cur_game)->beam_tex, x, y, w, h, 0, 0, 256, 32);
	g_pop_color();
}

void draw_beam_2(int x, int y, int w, int h) {
	Color c = get_text_color(18);
	g_push_color(c);
	g_draw_quad_crop(((Never7*) cur_game)->beam_tex, x, y, w, h, 0, 32, 256, 32);
	g_pop_color();
}

}

using namespace n7;

GameMeta* Never7::get_meta() {
	return new GameMeta {.basic_name = "Never7"s, .full_name = "Never7 -the end of infinity- Eternal Edition"s};
}

void Never7::load() {
	textbox_tex = g_load_texture(FileDataStream("res/n7/dc/window1.png"));
	beam_tex = g_load_texture(FileDataStream("res/n7/dc/beam.png"));
	textbox_tex->upscale = false;
	beam_tex->upscale = false;
	cur_mode = new TitleMode();
	//gui_push_theme();
	//gui_theme.text_color = get_text_color(0);
	//gui_theme.text_color_gradient = get_text_color(6);
	//gui_theme.shadow_color = get_text_color(12);
	beam_color = get_text_color(18);
	/*gui_theme.hover_button = [](int w, int tw, int th) {
		Color c = get_text_color(18);
		g_push_color(c);
		if(gui_state.horizontal) {
			g_draw_quad_crop(((Never7*) cur_game)->beam_tex, gui_scale(gui_state.x - 12), gui_scale(gui_state.y + 4), w + gui_scale(24), th, 0, 0, 256, 32, gui_state.corner, gui_state.corner);
		} else {
			g_draw_quad_crop(((Never7*) cur_game)->beam_tex, gui_scale(gui_state.x - 12), gui_scale(gui_state.y + 4), w + gui_scale(24), th, 0, 32, 256, 32, gui_state.corner, gui_state.corner);
		}
		g_pop_color();
	};*/
	/*gui_theme.draw_slider = [&](int x, int y, int w, int h, int v, int min, int max) {
		g_push_color(beam_color);
		g_draw_quad_crop(beam_tex, x, y + gui_scale(6), w, h, 0, 32, 256, 32, gui_state.corner, gui_state.corner);
		g_draw_quad_crop(beam_tex, x, y + gui_scale(6), w * (v - min) / (max - min), h, 0, 32, 256, 32, gui_state.corner, gui_state.corner);
		g_pop_color();
	};*/
}

void Never7::frame() {
	cur_mode->draw();
}

void Never7::input(int key, int action) {
	cur_mode->input(key, action);
}

void Never7::cursor(int x, int y, bool dragging) {
	int h = g_height();
	cur_mode->cursor(x, y, dragging);
}

void Never7::close() {
}