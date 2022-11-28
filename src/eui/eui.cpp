#include "eui.hpp"

void EuiTexture::draw(RectInt rect) {
	if(!tex) return;
	if(!crop_w) crop_w = tex->w;
	if(!crop_h) crop_h = tex->h;
	rect = RectInt(rect.vec + glm::i16vec4(add_x, add_y, add_w, add_h));
	i16 x = rect.x, y = rect.y, w = rect.w, h = rect.h;
	g_push_color(color);
	if(np_top) {
		if(np_left) {
			g_draw_quad_crop(tex, x, y, np_left, np_top, crop_x, crop_y, np_left_pix, np_top_pix);
		}
		g_draw_quad_crop(tex, x + np_left, y, w - np_left - np_right, np_top, crop_x + np_left_pix, crop_y, crop_w - np_left_pix - np_right_pix, np_top_pix);
	}
	if(np_left) {
		if(np_bottom) {
			g_draw_quad_crop(tex, x, y + h - np_bottom, np_left, np_bottom, crop_x, crop_y + crop_h - np_bottom_pix, np_left_pix, np_top_pix);
		}
		g_draw_quad_crop(tex, x, y + np_top, np_left, h - np_top - np_bottom, crop_x, crop_y + np_top_pix, np_left_pix, crop_h - np_top_pix - np_bottom_pix);
	}
	if(np_bottom) {
		if(np_right) {
			g_draw_quad_crop(tex, x + w - np_right, y + h - np_bottom, np_right, np_bottom, crop_x + crop_w - np_right_pix, crop_y + crop_h - np_bottom_pix, np_right_pix, np_bottom_pix);
		}
		g_draw_quad_crop(tex, x + np_left, y + h - np_bottom, w - np_left - np_right, np_bottom, crop_x + np_left_pix, crop_y + crop_h - np_bottom_pix, crop_w - np_left_pix - np_right_pix, np_bottom_pix);
	}
	if(np_right) {
		if(np_top) {
			g_draw_quad_crop(tex, x + w - np_right, y, np_right, np_top, crop_x + crop_w - np_right_pix, crop_y, np_right_pix, np_top_pix);
		}
		g_draw_quad_crop(tex, x + w - np_right, y + np_top, np_right, h - np_top - np_bottom, crop_x + crop_w - np_right_pix, crop_y + np_top_pix, np_right_pix, crop_h - np_top_pix - np_bottom_pix);
	}
	g_draw_quad_crop(tex, x + np_left, y + np_top, w - np_left - np_right, h - np_top - np_bottom, crop_x + np_left_pix, crop_y + np_top_pix, crop_w - np_left_pix - np_right_pix, crop_h - np_top_pix - np_bottom_pix);
	g_pop_color();
}

// from glm::radians implementation
constexpr float full_turn = 360.f * 0.01745329251994329576923690768489f;

void EuiTexture::draw(RectInt rect, HaloSettings halo) {
	float shadow_detail = halo.get_shadow_detail();
	float shadow_step = (float) halo.shadow_distance / shadow_detail;
	float halo_detail = halo.get_halo_detail();
	float halo_step = (float) halo.halo_distance / halo_detail;
	if(shadow_detail) {
		g_push_color(0, 0, 0, halo.shadow_alpha);
		for(float offset = halo.shadow_distance; offset > 0.f; offset -= shadow_step) {
			draw(rect.move(offset, offset));
		}
		g_pop_color();
	}
	if(halo_detail) {
		g_push_color(0, 0, 0, halo.halo_alpha);
		g_push_color_off(halo.halo_lightness, halo.halo_lightness, halo.halo_lightness, 0);
		for(float r = 0; r < full_turn; r += full_turn / halo_detail) {
			float off_x = glm::cos(r) * halo.halo_distance;
			float off_y = glm::sin(r) * halo.halo_distance;
			draw(rect.move(off_x, off_y));
		}
		g_pop_color_off();
		g_pop_color();
	}
	draw(rect);
}

EuiThemeRef EuiTheme::default_theme = Rc(new EuiTheme);

// this is called by api_load because we can only make textures after the renderer is working
void eui_load_default_theme() {
	EuiTheme::default_theme->button_bg = g_white_tex();
	EuiTheme::default_theme->toggle_bg = g_white_tex();
	EuiTheme::default_theme->toggle_dot = g_white_tex();
	EuiTheme::default_theme->window_bg = g_white_tex();
	EuiTheme::default_theme->window_bg.color = {0.15, 0.05, 0.15, 0.9};
	EuiTheme::default_theme->scroll_bar_bg = g_white_tex();
	EuiTheme::default_theme->scroll_bar_bg.color = EuiTheme::default_theme->toggle_dot_color;
	EuiTheme::default_theme->scroll_bar_dot = g_white_tex();
	//EuiTheme::default_theme->scroll_bar_dot.color = EuiTheme::default_theme->button_color;
}

void eui_load_default_thesme() {
	EuiTheme::default_theme->font_size = 30;
	EuiTheme::default_theme->text_color = {0x77 / 255.f, 0x99 / 255.f, 0xbb / 255.f, 1};
	EuiTheme::default_theme->layout_margin_x = 34;
	EuiTheme::default_theme->layout_margin_y = 24;
	EuiTheme::default_theme->button_margin = 0;
	EuiTheme::default_theme->button_corner = G_CENTER_LEFT;
	EuiTheme::default_theme->button_padding = 0;
	EuiTheme::default_theme->button_halo.shadow_detail = 0;
	EuiTheme::default_theme->button_halo.halo_detail = 0;
	EuiTheme::default_theme->button_bg = EuiTexture(g_load_texture(FileDataStream("et:/res/n7/dc/beam.png")), 0, 32, 256, 32, 0, 0, 0, 0, 0, 0, 0, 0, -20, -12, 40, 26);
	EuiTheme::default_theme->button_color_hovered = {0xe0 / 255.f, 0xa0 / 255.f, 0xb0 / 255.f, 1};
	//EuiTheme::default_theme->button_color_hovered = {0xe0 / 255.f, 0x90 / 255.f, 0xf0 / 255.f, 1};
	EuiTheme::default_theme->button_color = EuiTheme::default_theme->button_color_hovered * glm::vec4(1, 1, 1, 0);
	EuiTheme::default_theme->button_color_pressed = EuiTheme::default_theme->button_color_hovered * glm::vec4(0.5, 0.5, 0.5, 1);
	EuiTheme::default_theme->toggle_bg = g_white_tex();
	EuiTheme::default_theme->toggle_dot = g_white_tex();
	EuiTheme::default_theme->window_bg = EuiTexture(g_load_texture(FileDataStream("et:/res/n7/dc/window10.png")), 1, 1, 94, 94, 44, 32);
}

void EuiElement::get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) {
	res_w = max_w;
	res_h = max_h;
}

void EuiElement::draw(RectInt bound, const DrawBatchRef& text_batch) {
	if(dirty) {
		EUI_CALL(this, prepare_impl(bound));
		dirty = false;
	}
	EUI_CALL(this, draw_impl(bound, text_batch));
}

void EuiElement::draw(RectInt bound) {
	if(dirty) {
		prepare_impl(bound);
		dirty = false;
	}
	DrawBatchRef text_batch = g_new_batch();
	EUI_CALL(this, draw_impl(bound, text_batch));
	g_flush();
	g_push_batch(text_batch);
	g_flush();
	g_pop_batch();
}

void EuiElement::invalidate() {
	dirty = true;
	if(parent) parent->invalidate();
}
