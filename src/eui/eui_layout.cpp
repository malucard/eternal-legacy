#include "eui.hpp"

EuiLayout::EuiLayout(bool horizontal, bool has_window_bg):
	EuiElement(EUI_LAYOUT), horizontal(horizontal), has_window_bg(has_window_bg) {}

u16 EuiLayout::get_margin() {
	return 0; //theme->layout_margin;
}

void EuiLayout::add_child(EuiElementRef elem) {
	elem->invalidate();
	elem->parent = this;
	children.push_back(Placement {.elem = elem});
	invalidate();
}

void EuiLayout::remove_child(EuiElementRef elem) {
	elem->parent = nullptr;
	for(int i = 0; i < children.size(); i++) {
		if(children[i].elem.data == elem.data) {
			children.erase(children.begin() + i);
			break;
		}
	}
	elem->invalidate();
}

void EuiLayout::request_visibility(EuiElement* elem, RectInt where) {
	for(int i = 0; i < children.size(); i++) {
		Placement& p = children[i];
		if(p.elem.data == elem) {
			if(parent) parent->request_visibility(elem, where.move(p.rect.x, p.rect.y));
		}
	}
}

void EuiLayout::get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) {
	max_w -= theme->layout_margin_x;
	max_h -= theme->layout_margin_y;
	res_w = 0, res_h = 0;
	for(int i = 0; i < children.size(); i++) {
		EuiElementRef elem = children[i].elem;
		i16 elem_w, elem_h;
		EUI_CALL(elem.data, get_min_size(max_w, max_h, elem_w, elem_h));
		if(horizontal) {
			res_w += elem_w + elem->get_margin() * 2;
			if(elem_h > res_h) res_h = elem_h;
		} else {
			res_h += elem_h + elem->get_margin() * 2;
			if(elem_w > res_w) res_w = elem_w;
		}
	}
	res_w += theme->layout_margin_x * 2;
	res_h += theme->layout_margin_y * 2;
}

bool EuiLayout::on_cursor(i16 x, i16 y, bool hovering, bool dragging) {
	// clear the hovered and the pressed if it isn't still engaged by this cursor position
	if(cur_hovered != -1) {
		Placement& p = children[cur_hovered];
		if(!p.elem->on_cursor(x - p.rect.x, y - p.rect.y, p.rect.contains(x, y), dragging) && p.elem->unfocus()) {
			cur_hovered = -1;
		}
	}
	if(cur_pressed != -1) {
		Placement& p = children[cur_pressed];
		if(!p.elem->on_cursor(x - p.rect.x, y - p.rect.y, p.rect.contains(x, y), dragging) && p.elem->unfocus()) {
			cur_pressed = -1;
		}
	}
	// only spread the event if not engaged to prevent engaging multiple elements at once
	if(cur_hovered == -1 && cur_pressed == -1) {
		for(int i = 0; i < children.size(); i++) {
			Placement& p = children[i];
			if(p.rect.contains(x, y) && p.elem->on_cursor(x - p.rect.x, y - p.rect.y, true, dragging)) {
				cur_hovered = i;
				return true;
			}
		}
	}
	return hovering;
}

bool EuiLayout::on_input(i16 x, i16 y, u32 input, bool pressed) {
	if(children.size() == 0) return false;
	if(input & INPUT_OK) {
		if(pressed) {
			if(cur_hovered != -1) {
				cur_pressed = cur_hovered;
				Placement& p = children[cur_pressed];
				return p.elem->on_input(x - p.rect.x, y - p.rect.y, input, pressed);
			}
		} else {
			if(cur_hovered != -1) {
				Placement& p = children[cur_hovered];
				p.elem->on_input(x - p.rect.x, y - p.rect.y, input, pressed);
			}
			if(cur_pressed != -1) {
				Placement& p = children[cur_pressed];
				if(!p.elem->on_input(x - p.rect.x, y - p.rect.y, input, pressed)) {
					cur_pressed = -1;
				}
				return true;
			}
		}
	} else if(pressed && cur_pressed == -1) {
		if(input & (INPUT_RIGHT | INPUT_DOWN)) {
			i16 prev = cur_hovered;
			if(prev != -1) {
				if(!children[prev].elem->on_input(0, 0, input, pressed)) {
					if(input & (horizontal? INPUT_RIGHT: INPUT_DOWN)) {
						do {
							cur_hovered++;
							if(cur_hovered == children.size()) {
								cur_hovered = 0;
							}
							if(cur_hovered == prev) {
								cur_hovered = -1;
								break;
							}
						} while(!children[cur_hovered].elem->on_input(0, 0, input, pressed));
					} else {
						cur_hovered = -1;
					}
				}
			} else {
				cur_hovered = 0;
				while(!children[cur_hovered].elem->on_input(0, 0, input, pressed)) {
					cur_hovered++;
					if(cur_hovered == children.size()) {
						cur_hovered = -1;
						break;
					}
				}
			}
			if(cur_hovered != -1) {
				if(cur_hovered == first_focusable_elem && parent) parent->request_visibility(this, {0, 0, 0, 0});
				if(cur_hovered == last_focusable_elem && parent) parent->request_visibility(this, {0, abs_h, 0, 0});
				return true;
			} else {
				return false;
			}
		} else if(input & (INPUT_LEFT | INPUT_UP)) {
			i16 prev = cur_hovered;
			if(prev != -1) {
				if(!children[prev].elem->on_input(0, 0, input, pressed)) {
					if(input & (horizontal? INPUT_LEFT: INPUT_UP)) {
						do {
							cur_hovered--;
							if(cur_hovered == -1) {
								cur_hovered = children.size() - 1;
							}
							if(cur_hovered == prev) {
								cur_hovered = -1;
								break;
							}
						} while(!children[cur_hovered].elem->on_input(0, 0, input, pressed));
					} else {
						cur_hovered = -1;
					}
				}
			} else {
				cur_hovered = children.size() - 1;
				while(!children[cur_hovered].elem->on_input(0, 0, input, pressed)) {
					cur_hovered--;
					if(cur_hovered == -1) {
						cur_hovered = -1;
						break;
					}
				}
			}
			if(cur_hovered != -1) {
				if(cur_hovered == first_focusable_elem && parent) parent->request_visibility(this, {0, 0, 0, 0});
				if(cur_hovered == last_focusable_elem && parent) parent->request_visibility(this, {0, abs_h, 0, 0});
				return true;
			} else {
				return false;
			}
		}
	}
	if(cur_pressed != -1) {
		Placement& p = children[cur_pressed];
		return p.elem->on_input(x - p.rect.x, y - p.rect.y, input, pressed);
	} else if(cur_hovered != -1) {
		Placement& p = children[cur_hovered];
		return p.elem->on_input(x - p.rect.x, y - p.rect.y, input, pressed);
	}
	return false;
}

void EuiLayout::propagate_theme(EuiThemeRef theme) {
	this->theme = theme;
	for(int i = 0; i < children.size(); i++) {
		children[i].elem->propagate_theme(theme);
	}
	invalidate();
}

void EuiLayout::prepare_impl(RectInt bound) {
	first_focusable_elem = -1;
	last_focusable_elem = -1;
	for(int i = 0; i < children.size(); i++) {
		if(children[i].elem->is_focusable()) {
			first_focusable_elem = i;
			break;
		}
	}
	for(int i = children.size() - 1; i >= 0; i--) {
		if(children[i].elem->is_focusable()) {
			last_focusable_elem = i;
			break;
		}
	}
	int slack = (horizontal? bound.w - theme->layout_margin_x * 2: bound.h - theme->layout_margin_y * 2);
	u32 expand_count = 0;
	for(int i = 0; i < children.size(); i++) {
		Placement& p = children[i];
		//p.elem->invalidate();
		if(p.elem->expand) {
			expand_count++;
			slack -= p.elem->get_margin() * 2;
		} else if(horizontal) {
			EUI_CALL(p.elem.data, get_min_size(slack, bound.h, p.rect.w, p.rect.h));
			p.rect.h = bound.h - theme->layout_margin_y * 2;
			slack -= p.rect.w;
		} else {
			EUI_CALL(p.elem.data, get_min_size(bound.w, slack, p.rect.w, p.rect.h));
			p.rect.w = bound.w - theme->layout_margin_x * 2;
			slack -= p.rect.h;
		}
	}
	u16 expand_size = expand_count && slack > 0? slack / expand_count: 0;
	i16 x = theme->layout_margin_x, y = theme->layout_margin_y;
	bound = bound.grow(theme->layout_margin_x * -2, theme->layout_margin_y * -2);
	for(int i = 0; i < children.size(); i++) {
		Placement& p = children[i];
		if(expand_size && p.elem->expand) {
			if(horizontal) {
				p.rect.w = expand_size;
				p.rect.h = bound.h - theme->layout_margin_y * 2;
			} else {
				p.rect.w = bound.w - theme->layout_margin_x * 2;
				p.rect.h = expand_size;
			}
		}
		if(horizontal) {
			x += EUI_CALL_RET(p.elem.data, get_margin());
			p.rect.x = x;
			p.rect.y = y;
			EUI_CALL(p.elem.data, prepare_impl({0, y, p.rect.w, p.rect.h}));
			bound.w -= p.rect.w;
			x += p.rect.w + p.elem->get_margin();
		} else {
			y += EUI_CALL_RET(p.elem.data, get_margin());
			p.rect.x = x;
			p.rect.y = y;
			EUI_CALL(p.elem.data, prepare_impl({x, 0, p.rect.w, p.rect.h}));
			bound.h -= p.rect.h;
			y += p.rect.h + EUI_CALL_RET(p.elem.data, get_margin());
		}
	}
	get_min_size(bound.w, bound.h, abs_w, abs_h);
}

void EuiLayout::draw_impl(RectInt bound, const DrawBatchRef& text_batch) {
	if(has_window_bg) {
		theme->window_bg.draw(bound);
	} else {
		background.draw(bound);
	}
	RectInt scissor = g_virtual_scissor;
	for(int i = 0; i < children.size(); i++) {
		Placement& p = children[i];
		RectInt rect = bound.to_absolute(p.rect);
		if(rect.x + rect.w < scissor.x || rect.y + rect.h < scissor.y) {
			continue;
		}
		if(rect.x > scissor.x + scissor.w || rect.y > scissor.y + scissor.h) {
			break;
		}
		p.elem.data->draw(rect, text_batch);
	}
	foreground.draw(bound);
}

	//float scissor_y_begin = scissor.y - bound_y;
	//float scissor_y_end = scissor.y + scissor.w - bound_y;
	//float scissor_x_begin = scissor.x - bound_x;
	//float scissor_x_end = scissor.x + scissor.z - bound_x;
		/*if(p.y + p.h < rel_scissor.scissor_y_begin || p.x + p.w < scissor_x_begin) {
			continue;
		}
		if(p.y > scissor_y_end || p.x > scissor_x_end) {
			break;
		}*/