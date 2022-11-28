#include "n7.hpp"
#include <audio.hpp>

namespace n7 {

enum {
	SETTINGS_GAME,
	SETTINGS_VISUAL,
	SETTINGS_AUDIO
};

int volume_master = 100;
int volume_bgm = 100;
int volume_voice = 100;
int volume_sfx = 100;
int volume_gui = 100;

static struct SettingsMenu {
	TextureRef bg, label;
	TextureRef btn_o, btn_x, btn_square, btn_triangle, btn_leftright;
	int section = SETTINGS_GAME;
}* settings_menu = nullptr;

void settings_draw() {
	if(!settings_menu) {
		settings_menu = new SettingsMenu();
		settings_menu->bg = g_load_texture(FileDataStream("res/n7/smenubg.png"));
		settings_menu->label = g_load_texture(FileDataStream("res/n7/optionsbg.png"));
		settings_menu->btn_o = g_load_texture(FileDataStream("res/buttons/o.png"), TEX_NEAREST);
		settings_menu->btn_x = g_load_texture(FileDataStream("res/buttons/x.png"), TEX_NEAREST);
		settings_menu->btn_square = g_load_texture(FileDataStream("res/buttons/square.png"), TEX_NEAREST);
		settings_menu->btn_triangle = g_load_texture(FileDataStream("res/buttons/triangle.png"), TEX_NEAREST);
		settings_menu->btn_leftright = g_load_texture(FileDataStream("res/buttons/leftright.png"), TEX_NEAREST);
	}
	g_draw_quad(settings_menu->bg, 0, 0, g_width(), -1, G_CENTER, G_CENTER);
	g_draw_quad(settings_menu->label, 0, 0, g_width(), -1, G_TOP_LEFT, G_TOP_LEFT);
	//gui_state.grow(48);
	//gui_state.pad(24);
	/*gui_label(U"Settings"s, 24);
	gui_state.grow(24);*/
	//gui_slider(__COUNTER__, U"Master Volume"s, 24, 384, &volume_master, 0, 200, 5, 0);
	//gui_slider(__COUNTER__, U"Music Volume"s, 24, 384, &volume_bgm, 0, 200, 5, 0);
	//gui_slider(__COUNTER__, U"Voice Volume"s, 24, 384, &volume_voice, 0, 200, 5, 0);
	//gui_slider(__COUNTER__, U"SFX Volume"s, 24, 384, &volume_sfx, 0, 200, 5, 0);
	//gui_slider(__COUNTER__, U"Interface Volume"s, 24, 384, &volume_gui, 0, 200, 5, 0);
	GameMode* gm = dynamic_cast<GameMode*>(cur_mode);
	if(gm) {
		if(gm->bgm_channel != -1) {
			a_volume(gm->bgm_channel, volume_bgm / 100.f);
		}
		for(int i = 0; i < gm->sfx_channels.size(); i++) {
			a_volume(gm->sfx_channels[i], volume_sfx / 100.f);
		}
		if(gm->voice_channel != -1) {
			a_volume(gm->voice_channel, volume_voice / 100.f);
		}
		a_master_volume(volume_master / 100.f);
	}
	/*gui_label(U"Game"s, 24);
	gui_button(__COUNTER__, U"Nooo"s, 24, []() {});
	gui_state.corner = G_CENTER_TOP;
	gui_label(U"Visual"s, 24);
	gui_state.corner = G_TOP_LEFT;
	gui_button(__COUNTER__, U"Textbox"s, 24, []() {});
	gui_state.corner = G_CENTER_TOP;
	gui_label(U"Audio"s, 24);
	gui_state.corner = G_TOP_LEFT;
	gui_state.corner = G_TOP_LEFT;*/
	g_draw_quad(settings_menu->btn_x, 16_600p, -16_600p, 32_600p, 32_600p, G_BOTTOM_LEFT, G_BOTTOM_LEFT);
	g_push_color(0.f, 0.f, 0.f, 1.f);
	g_draw_text("Quit"s, 50_480p, -42_480p, 12903, 24_480p, G_TOP_LEFT, G_BOTTOM_LEFT);
	g_pop_color();
	int tw = g_draw_text("Quit"s, 48_480p, -44_480p, 12903, 24_480p, G_TOP_LEFT, G_BOTTOM_LEFT).w;
	g_draw_quad(settings_menu->btn_triangle, 16_600p + tw, -16_600p, 32_600p, 32_600p, G_BOTTOM_LEFT, G_BOTTOM_LEFT);
	g_push_color(0.f, 0.f, 0.f, 1.f);
	g_draw_text("Default"s, 50_480p + tw, -42_480p, 12903, 24_480p, G_TOP_LEFT, G_BOTTOM_LEFT);
	g_pop_color();
	tw += g_draw_text("Default"s, 48_480p + tw, -44_480p, 12903, 24_480p, G_TOP_LEFT, G_BOTTOM_LEFT).w;
	g_draw_quad(settings_menu->btn_leftright, 16_600p + tw, -16_600p, 32_600p, 32_600p, G_BOTTOM_LEFT, G_BOTTOM_LEFT);
	g_push_color(0.f, 0.f, 0.f, 1.f);
	g_draw_text("Change"s, 50_480p + tw, -42_480p, 12903, 24_480p, G_TOP_LEFT, G_BOTTOM_LEFT);
	g_pop_color();
	tw += g_draw_text("Change"s, 48_480p + tw, -44_480p, 12903, 24_480p, G_TOP_LEFT, G_BOTTOM_LEFT).w;
}

bool settings_input(int key, int action) {
	return false;
}

void settings_cursor(int x, int y, bool dragging) {

}

void settings_close() {
	delete settings_menu;
	settings_menu = nullptr;
}

}
