#pragma once
#include <functional>
#include <graphics.hpp>
#ifdef REGULAR_LUA
#include <lua.hpp>
#else
#include <luajit-2.1/lua.hpp>
#endif

enum {
	EUI_ELEMENT,
	EUI_LAYOUT,
	EUI_SCROLLABLE,
	EUI_SEPARATOR,
	EUI_LABEL,
	EUI_BUTTON,
	EUI_TOGGLE
};

#ifdef NO_VIRTUALS
#define EUI_CALL(elem, call) switch(elem->type) {\
	case EUI_ELEMENT: ((EuiElement*) elem)->call; break;\
	case EUI_LAYOUT: ((EuiLayout*) elem)->call; break;\
	case EUI_SCROLLABLE: ((EuiScrollable*) elem)->call; break;\
	case EUI_SEPARATOR: ((EuiSeparator*) elem)->call; break;\
	case EUI_LABEL: ((EuiLabel*) elem)->call; break;\
	case EUI_BUTTON: ((EuiButton*) elem)->call; break;\
	case EUI_TOGGLE: ((EuiToggle*) elem)->call; break;\
}
#define EUI_CALL_RET(elem, call) ({\
	__typeof__(elem->call) ret;\
	switch(elem->type) {\
		case EUI_ELEMENT: ret = ((EuiElement*) elem)->call; break;\
		case EUI_LAYOUT: ret = ((EuiLayout*) elem)->call; break;\
		case EUI_SCROLLABLE: ret = ((EuiScrollable*) elem)->call; break;\
		case EUI_SEPARATOR: ret = ((EuiSeparator*) elem)->call; break;\
		case EUI_LABEL: ret = ((EuiLabel*) elem)->call; break;\
		case EUI_BUTTON: ret = ((EuiButton*) elem)->call; break;\
		case EUI_TOGGLE: ret = ((EuiToggle*) elem)->call; break;\
	}\
	ret;\
})
#else
#define EUI_CALL(elem, call) elem->call
#define EUI_CALL_RET(elem, call) elem->call
#endif

struct EuiTexture {
	TextureRef tex;
	glm::vec4 color = {1, 1, 1, 1};
	// crop into the texture
	u16 crop_x = 0, crop_y = 0, crop_w, crop_h;
	// draw as a nine-patch after cropping- middle will be stretched
	u8 np_top = 0, np_bottom = 0, np_left = 0, np_right = 0;
	u8 np_top_pix = 0, np_bottom_pix = 0, np_left_pix = 0, np_right_pix = 0;
	// add to the coordinates before drawing
	i8 add_x = 0, add_y = 0, add_w = 0, add_h = 0;

	inline EuiTexture() {}
	inline EuiTexture(TextureRef tex): tex(tex), crop_w(tex->w), crop_h(tex->h) {}
	inline EuiTexture(TextureRef tex, u16 crop_x, u16 crop_y, u16 crop_w, u16 crop_h,
		u8 np, u8 np_pix): tex(tex),
			crop_x(crop_x), crop_y(crop_y), crop_w(crop_w), crop_h(crop_h),
			np_top(np), np_bottom(np), np_left(np), np_right(np),
			np_top_pix(np_pix), np_bottom_pix(np_pix), np_left_pix(np_pix), np_right_pix(np_pix) {}
	inline EuiTexture(TextureRef tex, u16 crop_x, u16 crop_y, u16 crop_w, u16 crop_h,
		u8 np_top, u8 np_bottom, u8 np_left, u8 np_right): tex(tex),
			crop_x(crop_x), crop_y(crop_y), crop_w(crop_w), crop_h(crop_h),
			np_top(np_top), np_bottom(np_bottom), np_left(np_left), np_right(np_right),
			np_top_pix(np_top), np_bottom_pix(np_bottom), np_left_pix(np_left), np_right_pix(np_right) {}
	inline EuiTexture(TextureRef tex, u16 crop_x, u16 crop_y, u16 crop_w, u16 crop_h,
		u8 np_top, u8 np_bottom, u8 np_left, u8 np_right,
		u8 np_top_pix, u8 np_bottom_pix, u8 np_left_pix, u8 np_right_pix,
		i8 add_x = 0, i8 add_y = 0, i8 add_w = 0, i8 add_h = 0): tex(tex),
			crop_x(crop_x), crop_y(crop_y), crop_w(crop_w), crop_h(crop_h),
			np_top(np_top), np_bottom(np_bottom), np_left(np_left), np_right(np_right),
			np_top_pix(np_top_pix), np_bottom_pix(np_bottom_pix), np_left_pix(np_left_pix), np_right_pix(np_right_pix),
			add_x(add_x), add_y(add_y), add_w(add_w), add_h(add_h) {}
	void draw(RectInt rect);
	void draw(RectInt rect, HaloSettings halo);
};

// margin = space around borders of elements
// padding = space between bounds and inner content of each element
// textures for the default theme are set in eui_load_default_theme
// this struct uses compact types and is ordered for padding due to how big it is
struct EuiTheme {
	u32 rc = 0;
	u8 fade_speed = 30;
	u8 font_size = 24;
	u8 layout_margin_x = 8;
	u8 layout_margin_y = 8;
	u8 button_margin = 8;
	u8 button_padding = 8;
	u8 button_corner = G_CENTER;
	u8 separator_margin = 4;
	u8 toggle_padding = 20;
	u8 scroll_bar_size = 8;
	u8 scroll_bar_margin = 4;
	u8 scroll_bar_padding = 0;
	glm::u16vec2 toggle_dot_size = {22, 22};
	HaloSettings button_halo = HaloSettings {.halo_distance = 2, .shadow_distance = 4, .shadow_detail = 1, .halo_detail = 8};
	glm::vec4 button_color = {0.5, 0.4, 0.45, 1};
	glm::vec4 button_color_pressed = {0.4, 0.3, 0.35, 1};
	glm::vec4 button_color_hovered = {0.6, 0.5, 0.55, 1};
	glm::vec4 text_color = {1, 1, 1, 1};

	EuiTexture button_bg;
	EuiTexture toggle_bg;
	EuiTexture toggle_dot;
	EuiTexture scroll_bar_bg;
	EuiTexture scroll_bar_dot;
	EuiTexture window_bg;
	glm::vec4 toggle_dot_color = {0.3, 0.2, 0.25, 1};

	glm::vec4 separator_color = {0.2, 0.2, 0.2, 1.0};

	static Rc<EuiTheme> default_theme;
};
using EuiThemeRef = Rc<EuiTheme>;

extern lua_State* L;

struct EuiLayout;

struct EuiElement {
	EuiElement* parent = nullptr;
	EuiThemeRef theme = EuiTheme::default_theme;
	std::string id;
	u32 rc = 0;
	int lua_obj_ref = LUA_NOREF;
	bool expand = false;
	bool dirty = true;
	u8 type = EUI_ELEMENT;

	inline EuiElement(u8 type = EUI_ELEMENT): type(type) {}

	virtual ~EuiElement() {
		if(lua_obj_ref != LUA_NOREF) {
			luaL_unref(L, LUA_REGISTRYINDEX, lua_obj_ref);
			lua_obj_ref = LUA_NOREF;
		}
	}
	virtual EuiLayout* as_layout() {return nullptr;}
	// mainly useful to make EuiScrollables scroll to make this visible
	virtual void request_visibility(EuiElement* elem, RectInt where) {
		if(parent) parent->request_visibility(elem, where);
	}
	virtual u16 get_margin() {return 0;}
	virtual bool is_focusable() {return false;}
	// may return false to deny an unfocus, for example sliders when engaged by a click
	virtual bool unfocus() {return true;}
	virtual bool on_input(i16 x, i16 y, u32 input, bool pressed) {return false;}
	// may return false if hovering is false to deny an unhover, like elements engaged by a click
	virtual bool on_cursor(i16 x, i16 y, bool hovering, bool dragging) {return false;}
	virtual void get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h);
	void draw(RectInt bound, const DrawBatchRef& text_batch);
	void draw(RectInt bound);
	void invalidate();
	virtual void propagate_theme(EuiThemeRef theme) {this->theme = theme; invalidate();}
	virtual void prepare_impl(RectInt bound) {}
	// text is drawn in a separate batch to reduce draw calls
	virtual void draw_impl(RectInt bound, const DrawBatchRef& text_batch) {}
};

using EuiElementRef = Rc<EuiElement>;

struct EuiLayout final: EuiElement {
	EuiTexture background;
	EuiTexture foreground;
	// store the last computed location of each child
	struct Placement {
		EuiElementRef elem;
		RectInt rect;
	};
	std::vector<Placement> children;
	i16 cur_hovered = -1;
	i16 cur_pressed = -1;
	bool horizontal = false;
	bool has_window_bg = false;
	
	EuiLayout(bool horizontal = false, bool has_window_bg = false);
	template<typename T>
	T* get_child(const std::string_view& id) {
		for(int i = 0; i < children.size(); i++) {
			if(children[i].elem->id == id) {
				return (T*) children[i].elem.data;
			}
		}
		for(int i = 0; i < children.size(); i++) {
			EuiLayout* lay = children[i].elem->as_layout();
			if(lay) {
				T* child = lay->get_child<T>(id);
				if(child) return child;
			}
		}
		return nullptr;
	}
	void add_child(EuiElementRef elem);
	void remove_child(EuiElementRef elem);
	EuiLayout* as_layout() override {return this;}
	u16 get_margin() override;
	bool is_focusable() override {
		return first_focusable_elem != -1;
	}
	bool unfocus() override {
		if(cur_hovered != -1) {
			if(!children[cur_hovered].elem->unfocus()) {
				cur_hovered = -1;
				return false;
			}
			cur_hovered = -1;
		}
		if(cur_pressed != -1) {
			if(!children[cur_pressed].elem->unfocus()) {
				cur_pressed = -1;
				return false;
			}
			cur_pressed = -1;
		}
		return true;
	}
	void request_visibility(EuiElement* elem, RectInt where) override;
	void get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) override;
	bool on_cursor(i16 x, i16 y, bool hovering, bool dragging) override;
	bool on_input(i16 x, i16 y, u32 input, bool pressed) override;
	void propagate_theme(EuiThemeRef theme) override;
	void prepare_impl(RectInt bound) override;
	void draw_impl(RectInt bound, const DrawBatchRef& text_batch) override;
protected:
	i16 abs_w, abs_h;
	i16 first_focusable_elem = -1;
	i16 last_focusable_elem = -1;
};

struct EuiScrollable final: EuiElement {
	EuiTexture background;
	EuiTexture foreground;
	EuiElementRef elem;
	float scroll_x = 0, scroll_y = 0;
	bool has_window_bg = false;
	
	EuiScrollable(bool has_window_bg = false);
	EuiLayout* as_layout() override {return elem->as_layout();}
	bool is_focusable() override {return elem->is_focusable();}
	bool unfocus() override {
		return !(dragging_x || dragging_y || dragging_x_bar || dragging_y_bar) && elem->unfocus();
	}
	void request_visibility(EuiElement* elem, RectInt where) override;
	void get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) override;
	bool on_cursor(i16 x, i16 y, bool hovering, bool dragging) override;
	bool on_input(i16 x, i16 y, u32 input, bool pressed) override;
	void propagate_theme(EuiThemeRef theme) override;
	void prepare_impl(RectInt bound) override;
	void draw_impl(RectInt bound, const DrawBatchRef& text_batch) override;
protected:
	glm::i16vec2 last_cursor;
	float cursor_time_x, cursor_time_y;
	i16 abs_w, abs_h;
	i16 content_w, content_h;
	float cur_scroll_x = 0, cur_scroll_y = 0;
	// set on click release, then reduced each frame while adding to scroll_x/y
	float cur_scroll_x_speed = 0, cur_scroll_y_speed = 0;
	// if > 0.2 seconds after last scroll check, 
	float scroll_speed_time_x = 0, scroll_speed_time_y = 0;
	bool x_scroll_bar = false, y_scroll_bar = false;
	bool dragging_x = false, dragging_y = false;
	bool dragging_x_bar = false, dragging_y_bar = false;
	bool hovering_x_bar = false, hovering_y_bar = false;
	i16 dragging_bar_start_cursor = 0, dragging_bar_start_scroll = 0;
	glm::vec4 x_bar_dot_color, y_bar_dot_color;
};

struct EuiSeparator final: EuiElement {
	EuiSeparator();
	u16 get_margin() override;
	void get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) override;
	void prepare_impl(RectInt bound) override;
	void draw_impl(RectInt bound, const DrawBatchRef& text_batch) override;
protected:
	bool horizontal = false;
};

struct EuiLabel final: EuiElement {
	std::string text;
	u8 corner = G_CENTER;
	EuiLabel(std::string text, std::string id = "");
	void get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) override;
	void prepare_impl(RectInt bound) override;
	void draw_impl(RectInt bound, const DrawBatchRef& text_batch) override;
protected:
	RectInt text_rect;
};

struct EuiButton final: EuiElement {
	std::string text;
	glm::vec4 cur_color;
	std::function<void(EuiButton&)> action;
	EuiButton(std::string text, std::function<void(EuiButton&)> action = nullptr, std::string id = "");
	u16 get_margin() override;
	bool is_focusable() override {return true;}
	bool unfocus() override;
	bool on_cursor(i16 x, i16 y, bool hovering, bool dragging) override;
	bool on_input(i16 x, i16 y, u32 input, bool pressed) override;
	void get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) override;
	void prepare_impl(RectInt bound) override;
	void draw_impl(RectInt bound, const DrawBatchRef& text_batch) override;
protected:
	i16 abs_w, abs_h;
	RectInt text_rect;
	bool hovering = false;
	bool pressing = false;
};

struct EuiToggle final: EuiElement {
	std::string text;
	glm::vec4 cur_color;
	u8 corner = G_TOP_LEFT;
	bool activated = false;
	std::function<void(EuiToggle&, bool)> action;
	EuiToggle(std::string text, std::function<void(EuiToggle&, bool)> action = nullptr, bool activated = false, std::string id = "");
	u16 get_margin() override;
	bool is_focusable() override {return true;}
	bool unfocus() override;
	bool on_cursor(i16 x, i16 y, bool hovering, bool dragging) override;
	bool on_input(i16 x, i16 y, u32 input, bool pressed) override;
	void get_min_size(i16 max_w, i16 max_h, i16& res_w, i16& res_h) override;
	void prepare_impl(RectInt bound) override;
	void draw_impl(RectInt bound, const DrawBatchRef& text_batch) override;
protected:
	i16 abs_w, abs_h;
	RectInt text_rect;
	bool hovering = false;
	bool pressing = false;
};
