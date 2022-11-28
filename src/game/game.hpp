#pragma once

#include <graphics.hpp>

enum {
	ACTION_RELEASE = 0,
	ACTION_PRESS,
	ACTION_REPEAT
};

struct GameMeta {
	std::string basic_name;
	std::string full_name;
};

struct Game {
	virtual ~Game() {}
	virtual void load() = 0;
	virtual void frame() = 0;
	virtual void close() = 0;
	virtual void input(int key, int action) = 0;
	virtual void cursor(int x, int y, bool dragging) = 0;
};

struct MainMenu: Game {
	static GameMeta* get_meta();

	TextureRef bg;
	TextureRef n7_bg, e17_bg, r11_bg, n7_icon, e17_icon, r11_icon;
	TextureRef btn_o, btn_x, btn_square, btn_triangle;
	int selected = 0;

	void load() override;
	void frame() override;
	void close() override;
	void input(int key, int action) override;
	void cursor(int x, int y, bool dragging) override;
};

struct Never7: Game {
	TextureRef textbox_tex, beam_tex;
	Color beam_color;
	
	static GameMeta* get_meta();

	void load() override;
	void frame() override;
	void close() override;
	void input(int key, int action) override;
	void cursor(int x, int y, bool dragging) override;
};

extern GameMeta* cur_game_meta;
extern Game* cur_game;

template<typename T>
void launch_game();
