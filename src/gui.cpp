#include "gui.hpp"
#include <codecvt>
#include <locale>
#include <vector>

/*void NinePatch::draw() {
	int ws = 16_480p;
	g_calc(&x, &y, &w, &h, w, h, corner, origin);
	//g_draw_quad_crop(textbox_tex, x, y, 96, 96, 0, 0, 96, 96, corner, origin);
	g_draw_quad_crop(textbox_tex, x + ws, y + ws, w - ws - ws, h - ws - ws, 16, 16, 64, 64);
	g_draw_quad_crop(textbox_tex, x + ws, y, w - ws - ws, ws, 16, 0, 64, 16);
	g_draw_quad_crop(textbox_tex, x + ws, y + h - ws, w - ws - ws, ws, 16, 80, 64, 16);
	g_draw_quad_crop(textbox_tex, x + w - ws, y + ws, ws, h - ws - ws, 80, 16, 16, 64);
	g_draw_quad_crop(textbox_tex, x, y + ws, ws, h - ws - ws, 0, 16, 16, 64);
	g_draw_quad_crop(textbox_tex, x, y, ws, ws, 0, 0, 16, 16);
	g_draw_quad_crop(textbox_tex, x + w - ws, y, ws, ws, 80, 0, 16, 16);
	g_draw_quad_crop(textbox_tex, x, y + h - ws, ws, ws, 0, 80, 16, 16);
	g_draw_quad_crop(textbox_tex, x + w - ws, y + h - ws, ws, ws, 80, 80, 16, 16);
}*/

GuiTheme gui_theme;
GuiState gui_state;
GuiInput gui_input;

void GuiInput::reset() {
	event = GUI_EVENT_NONE;
}
static std::vector<GuiTheme> gui_theme_stack;

void gui_push_theme() {
	gui_theme_stack.push_back(gui_theme);
}

void gui_pop_theme() {
	gui_theme = std::move(gui_theme_stack[gui_theme_stack.size() - 1]);
	gui_theme_stack.pop_back();
}

static std::vector<GuiState> gui_state_stack;

void gui_push() {
	gui_state_stack.push_back(gui_state);
}

GuiState gui_pop() {
	GuiState gs = gui_state;
	gui_state = gui_state_stack[gui_state_stack.size() - 1];
	gui_state_stack.pop_back();
	return gs;
}

int gui_scale(int v) {
	return scale(v, g_height(), gui_state.res);
}

int gui_rscale(int v) {
	return scale(v, gui_state.res, g_height());
}

int gui_rscale_ceil(int v) {
	int sh = g_height();
	return (v * gui_state.res + sh - 1) / sh;
}

void gui_layout(std::function<void()> contents) {
	int ev = gui_input.event;
	gui_input.event = GUI_EVENT_NONE;
	gui_push();
	gui_state.corner = G_TOP_LEFT;
	gui_state.actually_draw = false;
	gui_state.min_w = 0;
	gui_state.min_h = 0;
	contents();
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
		gui_state.w = g.min_w;
		gui_state.h = g.min_h;
		gui_input.event = ev;
		contents();
		gui_pop();
	}
	if(gui_state.actually_draw) {
		gui_push();
		if(gui_state.corner == G_CENTER_TOP || gui_state.corner == G_CENTER || gui_state.corner == G_CENTER_BOTTOM) {
			gx += 400 * gui_state.res / 600;
		}
		if(gui_state.corner == G_CENTER_LEFT || gui_state.corner == G_CENTER || gui_state.corner == G_CENTER_RIGHT) {
			gy += 300 * gui_state.res / 600;
		}
		gui_state.corner = G_TOP_LEFT;
		gui_state.x = gx;
		gui_state.y = gy;
		gui_state.w = g.min_w;
		gui_state.h = g.min_h;
		contents();
		gui_pop();
	}
	gui_state.grow(g.min_w, g.min_h);
}

void gui_label(const std::u32string& text, int height) {
	int my, mx = g_calc_text(text, gui_scale(gui_state.w), gui_scale(height)).mx;
	if(gui_state.actually_draw) {
		g_push_color(gui_theme.shadow_color);
		g_draw_text(text, gui_scale(gui_state.x + 2), gui_scale(gui_state.y + 2), mx, gui_scale(height), gui_state.corner, gui_state.corner);
		g_pop_color();
		g_push_color(gui_theme.text_color);
		if(gui_theme.text_color_gradient) {
			g_draw_text_alpha_gradient(text, gui_scale(gui_state.x), gui_scale(gui_state.y), mx, gui_scale(height), [](auto c) {return 1.f;}, gui_theme.text_color_gradient.value(), gui_state.corner, gui_state.corner);
		} else {
			g_draw_text(text, gui_scale(gui_state.x), gui_scale(gui_state.y), mx, gui_scale(height), gui_state.corner);
		}
		g_pop_color();
	}
	gui_state.grow(gui_rscale_ceil(mx), gui_rscale_ceil(my));
}

void gui_button(int id, const std::u32string& text, int height, std::function<void()> click) {
	int my, mx = g_calc_text(text, gui_scale(gui_state.w), gui_scale(height), &my);
	int w = gui_state.horizontal? mx: gui_scale(gui_state.w), h = gui_state.horizontal? gui_scale(gui_state.h): my;
	if(gui_input.event == GUI_EVENT_OK && gui_input.selected == id) {
		click();
		gui_input.event = GUI_EVENT_NONE;
	}
	if(gui_input.event == GUI_EVENT_MOUSE) {
		int gx = gui_scale(gui_state.x), gy = gui_scale(gui_state.y);
		g_calc(&gx, &gy, &w, &h, w, h, gui_state.corner, gui_state.corner);
		if(gui_input.mouse_x >= gx && gui_input.mouse_x < gx + w && gui_input.mouse_y >= gy && gui_input.mouse_y < gy + h) {
			gui_input.selected = id;
			gui_input.event = GUI_EVENT_NONE;
		}
	}
	if(gui_state.actually_draw) {
		if(gui_input.selected == id) {
			gui_theme.hover_button(w, mx, my);
		}
		g_push_color(gui_theme.shadow_color);
		g_draw_text(text, gui_scale(gui_state.x + 2), gui_scale(gui_state.y + 2), mx, gui_scale(height), gui_state.corner, gui_state.corner);
		g_pop_color();
		g_push_color(gui_theme.text_color);
		if(gui_theme.text_color_gradient) {
			g_draw_text_alpha_gradient(text, gui_scale(gui_state.x), gui_scale(gui_state.y), mx, gui_scale(height), [](auto c) {return 1.f;}, gui_theme.text_color_gradient.value(), gui_state.corner, gui_state.corner);
		} else {
			g_draw_text(text, gui_scale(gui_state.x), gui_scale(gui_state.y), mx, gui_scale(height), gui_state.corner, gui_state.corner);
		}
		g_pop_color();
	}
	gui_state.grow(gui_rscale_ceil(mx), gui_rscale_ceil(my));
}

void gui_slider(int id, const std::u32string& text, int height, int width, int* value, int min, int max, int step, std::optional<int> reset) {
	int my, mx = g_calc_text(text, gui_scale(gui_state.w), gui_scale(height), &my);
	int w = gui_state.horizontal? mx: gui_scale(gui_state.w), h = gui_state.horizontal? gui_scale(gui_state.h): my;
	if(gui_input.selected == id) {
		if(gui_input.event == GUI_EVENT_OK || gui_input.event == GUI_EVENT_DRAG) {
			int gx = gui_scale(gui_state.x + gui_state.w - width), gy = gui_scale(gui_state.y);
			int slw = gui_scale(width);
			g_calc(&gx, &gy, &w, &h, w, h, gui_state.corner, gui_state.corner);
			if(gui_input.mouse_x >= gx && gui_input.mouse_x < gx + slw && gui_input.mouse_y >= gy && gui_input.mouse_y < gy + h) {
				*value = (gui_input.mouse_x - gx) * (max - min) / slw + min;
				if(*value > max) *value = min;
				if(*value < min) *value = max;
			} else if(gui_input.event == GUI_EVENT_OK) {
				*value += step;
				if(*value > max) *value = min;
				if(*value < min) *value = max;
			} else {
				*value = (gui_input.mouse_x - gx) * (max - min) / slw + min;
				if(*value > max) *value = max;
				if(*value < min) *value = min;
			}
			gui_input.event = GUI_EVENT_NONE;
		} else if(gui_input.event == GUI_EVENT_RIGHT) {
			*value += step;
			if(*value > max) *value = min;
			if(*value < min) *value = max;
			gui_input.event = GUI_EVENT_NONE;
		} else if(gui_input.event == GUI_EVENT_LEFT) {
			*value -= step;
			if(*value > max) *value = min;
			if(*value < min) *value = max;
			gui_input.event = GUI_EVENT_NONE;
		}
	}
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
	std::u32string val_text = converter.from_bytes(std::to_string(*value));
	int vmy, vmx = g_calc_text(val_text, gui_scale(gui_state.w), gui_scale(height), &vmy);
	if(gui_input.event == GUI_EVENT_MOUSE) {
		int gx = gui_scale(gui_state.x), gy = gui_scale(gui_state.y);
		g_calc(&gx, &gy, &w, &h, w, h, gui_state.corner, gui_state.corner);
		if(gui_input.mouse_x >= gx && gui_input.mouse_x < gx + w && gui_input.mouse_y >= gy && gui_input.mouse_y < gy + h) {
			gui_input.selected = id;
			gui_input.event = GUI_EVENT_NONE;
		}
	}
	if(gui_state.actually_draw) {
		if(gui_input.selected == id) {
			gui_theme.hover_button(w, mx, my);
		}
		g_push_color(gui_theme.shadow_color);
		g_draw_text(text, gui_scale(gui_state.x + 2), gui_scale(gui_state.y + 2), mx, gui_scale(height), gui_state.corner, gui_state.corner);
		g_draw_text(val_text, gui_scale(gui_state.x + gui_state.w - width - 8 + 2) - vmx, gui_scale(gui_state.y + 2), vmx, gui_scale(height), gui_state.corner, gui_state.corner);
		g_pop_color();
		g_push_color(gui_theme.text_color);
		if(gui_theme.text_color_gradient) {
			g_draw_text_alpha_gradient(text, gui_scale(gui_state.x), gui_scale(gui_state.y), mx, gui_scale(height), [](auto c) {return 1.f;}, gui_theme.text_color_gradient.value(), gui_state.corner, gui_state.corner);
			g_draw_text_alpha_gradient(val_text, gui_scale(gui_state.x + gui_state.w - width - 8) - vmx, gui_scale(gui_state.y), vmx, gui_scale(height), [](auto c) {return 1.f;}, gui_theme.text_color_gradient.value(), gui_state.corner, gui_state.corner);
		} else {
			g_draw_text(text, gui_scale(gui_state.x), gui_scale(gui_state.y), mx, gui_scale(height), gui_state.corner, gui_state.corner);
		}
		g_pop_color();
	}
	gui_theme.draw_slider(gui_scale(gui_state.x + gui_state.w - width), gui_scale(gui_state.y), gui_scale(width), gui_scale(height), *value, min, max);
	if(gui_state.horizontal) {
		gui_state.grow(gui_rscale_ceil(mx + vmx), gui_rscale_ceil(my));
	} else {
		gui_state.grow(gui_rscale_ceil(mx + vmx), gui_rscale_ceil(my));
	}
}
