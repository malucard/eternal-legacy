#include "eui.hpp"

EuiLabel::EuiLabel(std::string text, std::string id):
	EuiElement(EUI_LABEL), text(text) {
	this->id = id;
}

void EuiLabel::get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) {
	TextInfo ti = g_calc_text(text, max_w, theme->font_size);
	res_w = ti.w;
	res_h = ti.h;
}

void EuiLabel::prepare_impl(RectInt bound) {
	get_min_size(bound.w, bound.h, text_rect.w, text_rect.h);
	text_rect = bound.get_corner_relative(text_rect.w, text_rect.h, corner);
	text_rect.y -= 2;
}

void EuiLabel::draw_impl(RectInt bound, const DrawBatchRef& text_batch) {
	g_push_batch(text_batch);
	g_push_color(theme->text_color);
	g_draw_text(text.c_str(), bound.x + text_rect.x, bound.y + text_rect.y, text_rect.w, theme->font_size, HaloSettings::with_shadow());
	g_pop_color();
	g_pop_batch();
}
