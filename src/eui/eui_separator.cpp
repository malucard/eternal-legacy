#include "eui.hpp"

EuiSeparator::EuiSeparator(): EuiElement(EUI_SEPARATOR) {}

u16 EuiSeparator::get_margin() {
	return theme->separator_margin;
}

void EuiSeparator::get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) {
	res_w = 2;
	res_h = 2;
}

void EuiSeparator::prepare_impl(RectInt bound) {
	EuiLayout* layout_parent = parent->as_layout();
	if(layout_parent) {
		horizontal = layout_parent->horizontal;
	} else {
		horizontal = bound.w > bound.h;
	}
}

void EuiSeparator::draw_impl(RectInt bound, const DrawBatchRef& text_batch) {
	g_push_color(theme->separator_color);
	if(horizontal) {
		g_draw_quad(g_white_tex(), bound.x + bound.w / 2.f - 1, bound.y, 2, bound.h);
	} else {
		g_draw_quad(g_white_tex(), bound.x, bound.y + bound.h / 2.f - 1, bound.w, 2);
	}
	g_pop_color();
}
