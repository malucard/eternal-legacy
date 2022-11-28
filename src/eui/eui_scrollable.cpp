#include "eui.hpp"

EuiScrollable::EuiScrollable(bool has_window_bg):
	EuiElement(EUI_SCROLLABLE), has_window_bg(has_window_bg) {}

void EuiScrollable::request_visibility(EuiElement* elem, RectInt where) {
	if(parent) parent->request_visibility(this, {0, 0, abs_w, abs_h});
	if(where.y + where.h - abs_h > scroll_y) {
		scroll_y = where.y + where.h - abs_h;
		this->elem->invalidate();
	} else if(where.y < scroll_y) {
		scroll_y = where.y;
		this->elem->invalidate();
	}
}

void EuiScrollable::get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) {
	EUI_CALL(elem.data, get_min_size(max_w, max_h, res_w, res_h));
	if(res_w > max_w) res_w = max_w;
	if(res_h > max_h) res_h = max_h;
}

bool EuiScrollable::on_cursor(i16 x, i16 y, bool hovering, bool dragging) {
	hovering_y_bar = x >= abs_w && x < abs_w + theme->scroll_bar_size &&
		y >= theme->scroll_bar_margin && y < abs_h - theme->scroll_bar_margin;
	hovering_x_bar = x >= theme->scroll_bar_margin && x < abs_w - theme->scroll_bar_margin &&
		y >= abs_h && y < abs_h + theme->scroll_bar_size;
	if(dragging) {
		if(dragging_x_bar) {
			float bar_len = abs_w - theme->scroll_bar_margin * 2;
			float dot_len = glm::min((float) abs_w / content_w, 1.f) * bar_len;
			scroll_x = dragging_bar_start_scroll + (x - dragging_bar_start_cursor) * (content_w - abs_w) / (bar_len - dot_len);
			last_cursor.x = x;
			return true;
		}
		if(dragging_y_bar) {
			float bar_len = abs_h - theme->scroll_bar_margin * 2;
			float dot_len = glm::min((float) abs_h / content_h, 1.f) * bar_len;
			scroll_y = dragging_bar_start_scroll + (y - dragging_bar_start_cursor) * (content_h - abs_h) / (bar_len - dot_len);
			last_cursor.y = y;
			return true;
		}
		if(x_scroll_bar && last_cursor.x != x) {
			if(!dragging_x && glm::abs(last_cursor.x - x) > 8) {
				if(elem->unfocus()) {
					dragging_x = true;
					cur_scroll_x_speed = 0;
					scroll_speed_time_x = cursor_time_x;
				}
			}
			if(dragging_x) {
				i16 max_scroll_x = content_w - abs_w;
				if(scroll_x < 0) {
					scroll_x *= 4;
				} else if(scroll_x > max_scroll_x) {
					scroll_x = max_scroll_x + (scroll_x - max_scroll_x) * 4;
				}
				scroll_x += last_cursor.x - x;
				float time = sys_secs();
				if(time - cursor_time_x < 0.2) {
					float new_speed = (last_cursor.x - x) / (time - cursor_time_x);
					if(glm::abs(new_speed) > glm::abs(cur_scroll_x_speed) ||
						glm::sign(new_speed) != glm::sign(cur_scroll_x_speed) || time - scroll_speed_time_x > 0.2) {
						cur_scroll_x_speed = new_speed;
						scroll_speed_time_x = time;
					}
				}
				last_cursor.x = x;
				cursor_time_x = time;
				if(scroll_x < 0) {
					scroll_x /= 4;
				} else if(scroll_x > max_scroll_x) {
					scroll_x = max_scroll_x + (scroll_x - max_scroll_x) / 4;
				}
			}
		}
		if(y_scroll_bar && last_cursor.y != y) {
			if(!dragging_y && glm::abs(last_cursor.y - y) > 8) {
				if(elem->unfocus()) {
					dragging_y = true;
					cur_scroll_y_speed = 0;
					scroll_speed_time_y = cursor_time_y;
				}
			}
			if(dragging_y) {
				float max_scroll_y = content_h - abs_h;
				if(scroll_y < 0) {
					scroll_y *= 4;
				} else if(scroll_y > max_scroll_y) {
					scroll_y = max_scroll_y + (scroll_y - max_scroll_y) * 4;
				}
				scroll_y += last_cursor.y - y;
				float time = sys_secs();
				if(time - cursor_time_y < 0.2) {
					float new_speed = (last_cursor.y - y) / (time - cursor_time_y);
					if(glm::abs(new_speed) > glm::abs(cur_scroll_y_speed) ||
						glm::sign(new_speed) != glm::sign(cur_scroll_y_speed) || time - scroll_speed_time_y > 0.2) {
						cur_scroll_y_speed = new_speed;
						scroll_speed_time_y = time;
					}
				}
				last_cursor.y = y;
				cursor_time_y = time;
				if(scroll_y < 0) {
					scroll_y /= 4;
				} else if(scroll_y > max_scroll_y) {
					scroll_y = max_scroll_y + (scroll_y - max_scroll_y) / 4;
				}
			}
		}
	} else {
		if(!dragging_x) last_cursor.x = x;
		if(!dragging_y) last_cursor.y = y;
	}
	return dragging_x || dragging_y || hovering_x_bar || hovering_y_bar || elem->on_cursor(x + scroll_x, y + scroll_y, hovering, dragging);
}

bool EuiScrollable::on_input(i16 x, i16 y, u32 input, bool pressed) {
	if(elem->on_input(x, y, input, pressed)) {
		return true;
	} else if(pressed && (input & INPUT_CLICK)) {
		if(hovering_y_bar) {
			dragging_y_bar = true;
			dragging_bar_start_cursor = y;
			dragging_bar_start_scroll = scroll_y;
			return true;
		} else if(hovering_x_bar) {
			dragging_x_bar = true;
			dragging_bar_start_cursor = x;
			dragging_bar_start_scroll = scroll_x;
			return true;
		}
	} else if(!pressed && (input & INPUT_CLICK)) {
		if(dragging_x_bar) {
			dragging_x_bar = false;
			i16 max_scroll_x = content_w - abs_w;
			if(scroll_x < 0) {
				scroll_x /= 4;
			} else if(scroll_x > max_scroll_x) {
				scroll_x = max_scroll_x + (scroll_x - max_scroll_x) / 4;
			}
		}
		if(dragging_y_bar) {
			dragging_y_bar = false;
			i16 max_scroll_y = content_h - abs_h;
			if(scroll_y < 0) {
				scroll_y /= 4;
			} else if(scroll_y > max_scroll_y) {
				scroll_y = max_scroll_y + (scroll_y - max_scroll_y) / 4;
			}
		}
		if(dragging_x) {
			log_info("recvd %u", sys_millis());
			if(sys_secs() - scroll_speed_time_x > 0.2) {
				cur_scroll_x_speed = 0;
			}
			dragging_x = false;
		}
		if(dragging_y) {
			if(sys_secs() - scroll_speed_time_y > 0.2) {
				cur_scroll_y_speed = 0;
			}
			dragging_y = false;
		}
	} else if((input & INPUT_SCROLL_DOWN) && pressed) {
		if(y_scroll_bar) scroll_y += 64;
		else if(x_scroll_bar) scroll_x += 64;
		return true;
	} else if((input & INPUT_SCROLL_UP) && pressed) {
		if(y_scroll_bar) scroll_y -= 64;
		else if(x_scroll_bar) scroll_x -= 64;
		return true;
	}
	return false;
}

void EuiScrollable::propagate_theme(EuiThemeRef theme) {
	this->theme = theme;
	elem->propagate_theme(theme);
	invalidate();
}

void EuiScrollable::prepare_impl(RectInt bound) {
	EUI_CALL(elem.data, get_min_size(bound.w, bound.h, content_w, content_h));
	if(content_h > bound.h) {
		bound.w -= theme->scroll_bar_size + theme->scroll_bar_margin;
		EUI_CALL(elem.data, prepare_impl(bound));
		EUI_CALL(elem.data, get_min_size(bound.w, bound.h, content_w, content_h));
	}
	if(content_w > bound.w) {
		x_scroll_bar = true;
		bound.h -= theme->scroll_bar_size + theme->scroll_bar_margin;
		EUI_CALL(elem.data, prepare_impl(bound));
		EUI_CALL(elem.data, get_min_size(bound.w, bound.h, content_w, content_h));
	} else {
		scroll_x = 0;
		x_scroll_bar = false;
	}
	if(content_h > bound.h) {
		y_scroll_bar = true;
		EUI_CALL(elem.data, get_min_size(bound.w, bound.h, content_w, content_h));
	} else {
		scroll_y = 0;
		y_scroll_bar = false;
	}
	abs_w = bound.w;
	abs_h = bound.h;
}

void EuiScrollable::draw_impl(RectInt bound, const DrawBatchRef& text_batch) {
	if(has_window_bg) {
		theme->window_bg.draw(bound);
	} else {
		background.draw(bound);
	}
	if(x_scroll_bar && !dragging_x) {
		float max_scroll_x = glm::max(content_w - abs_w, 0);
		if(scroll_x < 0 || scroll_x > max_scroll_x) {
			scroll_x += cur_scroll_x_speed * delta * 0.25f;
		} else {
			scroll_x += cur_scroll_x_speed * delta;
		}
		cur_scroll_x_speed = lerpclamp(cur_scroll_x_speed, 0.f, 8 * delta);
		if(glm::abs(cur_scroll_x_speed) < 200.f) {
			cur_scroll_x_speed = 0.f;
			if(scroll_x < 0) {
				scroll_x = 0;
			} else if(scroll_x > max_scroll_x) {
				scroll_x = max_scroll_x;
			}
		}
	}
	if(y_scroll_bar && !dragging_y) {
		float max_scroll_y = glm::max(content_h - abs_h, 0);
		if(scroll_y < 0 || scroll_y > max_scroll_y) {
			scroll_y += cur_scroll_y_speed * delta * 0.25f;
		} else {
			scroll_y += cur_scroll_y_speed * delta;
		}
		cur_scroll_y_speed = lerpclamp(cur_scroll_y_speed, 0.f, 8 * delta);
		if(glm::abs(cur_scroll_y_speed) < 200.f) {
			cur_scroll_y_speed = 0.f;
			if(scroll_y < 0) {
				scroll_y = 0;
			} else if(scroll_y > max_scroll_y) {
				scroll_y = max_scroll_y;
			}
		}
	}
	if(x_scroll_bar) {
		float bar_len = abs_w - theme->scroll_bar_margin * 2;
		theme->scroll_bar_bg.draw({(i16) (bound.x + theme->scroll_bar_margin), (i16) (bound.y + bound.h - theme->scroll_bar_size - theme->scroll_bar_margin), (i16) bar_len, (i16) theme->scroll_bar_size});
		float dot_len = glm::min((float) abs_w / content_w, 1.f) * bar_len;
		float offset = (float) scroll_x / (content_w - abs_w) * (bar_len - dot_len);
		if(offset < 0.f) {
			dot_len += offset;
			offset = 0.f;
		} else if(offset > bar_len - dot_len) {
			dot_len -= offset - (bar_len - dot_len);
		}
		if(dot_len > 0.f) {
			if(dragging_x_bar) {
				g_push_color(theme->button_color_pressed);
			} else if(hovering_x_bar) {
				g_push_color(theme->button_color_hovered);
			} else {
				g_push_color(theme->button_color);
			}
			theme->scroll_bar_dot.draw({(i16) (bound.x + theme->scroll_bar_margin + offset), (i16) (bound.y + bound.h - theme->scroll_bar_size - theme->scroll_bar_margin), (i16) dot_len, (i16) theme->scroll_bar_size});
			g_pop_color();
			bound.h -= theme->scroll_bar_size + theme->scroll_bar_margin;
		}
	}
	if(y_scroll_bar) {
		float bar_len = abs_h - theme->scroll_bar_margin * 2;
		theme->scroll_bar_bg.draw({(i16) (bound.x + bound.w - theme->scroll_bar_size - theme->scroll_bar_margin), (i16) (bound.y + theme->scroll_bar_margin), (i16) theme->scroll_bar_size, (i16) bar_len});
		float dot_len = glm::min((float) abs_h / content_h, 1.f) * bar_len;
		float offset = (float) scroll_y / (content_h - abs_h) * (bar_len - dot_len);
		if(offset < 0.f) {
			dot_len += offset;
			offset = 0.f;
		} else if(offset > bar_len - dot_len) {
			dot_len -= offset - (bar_len - dot_len);
		}
		if(dot_len > 0.f) {
			if(dragging_y_bar) {
				g_push_color(theme->button_color_pressed);
			} else if(hovering_y_bar) {
				g_push_color(theme->button_color_hovered);
			} else {
				g_push_color(theme->button_color);
			}
			theme->scroll_bar_dot.draw({(i16) (bound.x + bound.w - theme->scroll_bar_size - theme->scroll_bar_margin), (i16) (bound.y + theme->scroll_bar_margin + offset), (i16) theme->scroll_bar_size, (i16) dot_len});
			g_pop_color();
			bound.w -= theme->scroll_bar_size + theme->scroll_bar_margin;
		}
	}
	cur_scroll_x = lerpclamp(cur_scroll_x, scroll_x, delta * 30);
	cur_scroll_y = lerpclamp(cur_scroll_y, scroll_y, delta * 30);
	g_flush();
	g_flush(text_batch);
	g_push_scissor(bound);
	elem.data->draw(bound.move(-cur_scroll_x, -cur_scroll_y), text_batch);
	g_flush();
	g_flush(text_batch);
	g_pop_scissor();
	foreground.draw(bound);
}
