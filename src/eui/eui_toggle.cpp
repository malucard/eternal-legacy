#include "eui.hpp"

EuiToggle::EuiToggle(std::string text, std::function<void(EuiToggle&, bool)> action, bool activated, std::string id):
	EuiElement(EUI_TOGGLE), text(text), action(action), activated(activated), cur_color(theme->button_color) {
	this->id = id;
}

u16 EuiToggle::get_margin() {
	return theme->button_margin;
}

void EuiToggle::get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) {
	TextInfo ti = g_calc_text(text, max_w - theme->toggle_padding - theme->toggle_dot_size.x, theme->font_size);
	res_w = ti.w + theme->toggle_padding + theme->toggle_dot_size.x;
	res_h = glm::max((u16) ti.h, theme->toggle_dot_size.y);
}

bool EuiToggle::unfocus() {
	pressing = false;
	hovering = false;
	return true;
}

bool EuiToggle::on_cursor(i16 x, i16 y, bool hovering, bool dragging) {
	this->hovering = hovering;
	return hovering;
}

bool EuiToggle::on_input(i16 x, i16 y, u32 input, bool pressed) {
	if(input & INPUT_OK) {
		if(!pressed && pressing && hovering) {
			activated = !activated;
			if(action) action(*this, activated);
		}
		pressing = pressed;
		return pressing;
	} else if(pressed && (input & INPUT_DIRECTIONAL)) {
		hovering = !hovering;
		if(hovering) request_visibility(this, {0, 0, abs_w, abs_h});
		return hovering;
	}
	return false;
}

void EuiToggle::prepare_impl(RectInt bound) {
	abs_w = bound.w;
	abs_h = bound.h;
	i16 off = theme->toggle_dot_size.x + theme->toggle_padding;
	bound.x += off;
	bound.w -= off;
	TextInfo ti = g_calc_text(text, bound.w, theme->font_size);
	text_rect = bound.get_corner_relative(glm::ceil(ti.w), glm::ceil(ti.h), G_TOP_LEFT);
	text_rect.x += off;
	text_rect.y -= 4;
}

void EuiToggle::draw_impl(RectInt bound, const DrawBatchRef& text_batch) {
	cur_color = lerpclamp(cur_color, pressing? theme->button_color_pressed: hovering? theme->button_color_hovered: theme->button_color, theme->fade_speed == (u8) -1? 1: delta * theme->fade_speed);
	g_push_color(cur_color);
	RectInt dot_rect = bound.get_corner(theme->toggle_dot_size.x, theme->toggle_dot_size.y, G_TOP_LEFT);
	theme->toggle_dot.draw(dot_rect, theme->button_halo);
	g_pop_color();
	if(activated) {
		g_push_color(theme->toggle_dot_color);
		theme->toggle_dot.draw(dot_rect.cut_corners(3));
		g_pop_color();
	}
	g_push_batch(text_batch);
	g_push_color(theme->text_color);
	g_draw_text(text.c_str(), bound.x + text_rect.x, bound.y + text_rect.y, text_rect.w, theme->font_size, HaloSettings::with_shadow());
	g_pop_color();
	g_pop_batch();
}
