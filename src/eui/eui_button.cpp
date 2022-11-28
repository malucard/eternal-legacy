#include "eui.hpp"

EuiButton::EuiButton(std::string text, std::function<void(EuiButton&)> action, std::string id):
	EuiElement(EUI_BUTTON), text(text), action(action), cur_color(theme->button_color) {
	this->id = id;
}

u16 EuiButton::get_margin() {
	return theme->button_margin;
}

void EuiButton::get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) {
	TextInfo ti = g_calc_text(text, max_w - theme->button_padding * 2, theme->font_size);
	res_w = ti.w + theme->button_padding * 2;
	res_h = ti.h + theme->button_padding * 2;
}

bool EuiButton::unfocus() {
	pressing = false;
	hovering = false;
	return true;
}

bool EuiButton::on_cursor(i16 x, i16 y, bool hovering, bool dragging) {
	this->hovering = hovering;
	return hovering || pressing;
}

bool EuiButton::on_input(i16 x, i16 y, u32 input, bool pressed) {
	if(input & INPUT_OK) {
		if(!pressed && pressing && hovering) {
			if(action) action(*this);
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

void EuiButton::prepare_impl(RectInt bound) {
	abs_w = bound.w;
	abs_h = bound.h;
	bound = bound.cut_corners(theme->button_padding);
	TextInfo ti = g_calc_text(text, bound.w, theme->font_size);
	text_rect = bound.get_corner_relative(glm::ceil(ti.w), glm::ceil(ti.h), theme->button_corner);
	text_rect.x += theme->button_padding;
	text_rect.y += theme->button_padding - 2;
}

void EuiButton::draw_impl(RectInt bound, const DrawBatchRef& text_batch) {
	cur_color = lerpclamp(cur_color,
		pressing? theme->button_color_pressed: hovering? theme->button_color_hovered: theme->button_color,
		theme->fade_speed == (u8) -1? 1: delta * theme->fade_speed);
	g_push_color(cur_color);
	theme->button_bg.draw(bound, theme->button_halo);
	g_pop_color();
	g_push_batch(text_batch);
	g_push_color(theme->text_color);
	g_draw_text(text.c_str(), bound.x + text_rect.x, bound.y + text_rect.y, text_rect.w, theme->font_size, HaloSettings::with_shadow());
	g_pop_color();
	g_pop_batch();
}
