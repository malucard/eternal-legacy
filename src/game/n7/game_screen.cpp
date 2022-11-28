#define _USE_MATH_DEFINES
#include "n7.hpp"
#include <audio.hpp>
#include <coroutine.hpp>
#include <locale>
#include <cmath>

int a_speak(const char* text);

namespace n7 {

constexpr int SLIDE_TIME = 200;
constexpr int BG_TRANSITION_TIME = 200;
constexpr int CHAR_TIME = 100;
constexpr int CHAR_WAIT = 10;

GameMode::GameMode() {
	script = new DecScriptStream("res/n7/script/init.dec");
	waiting_icon = g_load_texture(FileDataStream("res/n7/dc/waiting_icon.png"));
	midprint_icon = g_load_texture(FileDataStream("res/n7/dc/midprint_icon.png"));
	Cmd cmd;
	u32 time = g_get_millis();
	u32 ntime;
	int i;
	u32 ch;
	int delay;
	int transition;
	std::optional<TextureRef> transition_from;
	execute_cmd = coroutine()
		while(waiting) {
			yield;
		}
		cmd.~Cmd();
		new(&cmd) Cmd(script->next());
		if(cmd.kind == CMD_MESSAGE) {
			if(let_voice_play) {
				let_voice_play = false;
			} else {
				if(voice_channel != -1) {
					a_destroy(voice_channel);
					voice_channel = -1;
				}
			}
			/*{
				std::string a;
				for(int i = 0; i < cmd.cmd_msg.msg.size(); i++) {
					char32_t ch = cmd.cmd_msg.msg[i];
					while(true) {
						if(ch == ' ') {
							if(cmd.cmd_msg.msg[i + 1] == ' ') {
								ch = '\n';
								i++;
							} else break;
						} else if(ch == '{') {
							while(cmd.cmd_msg.msg[++i] != '}') {}
							ch = cmd.cmd_msg.msg[++i];
							time = g_get_millis();
						} else if(ch == U'【') {
							int j = cmd.cmd_msg.msg.find(U'】');
							i = j + 2;
							ch = cmd.cmd_msg.msg[i];
						} else break;
					}
					a.push_back(ch);
				}
				voice_channel = a_speak(a.c_str());
			}*/
			time = g_get_millis();
			scrolling_since = time;
			scrolling_after = 0;
			shadow_color = cmd.cmd_msg.shadow_color;
			textbox = std::u32string();
			for(i = 0; i < cmd.cmd_msg.msg.size(); i++) {
				ch = cmd.cmd_msg.msg[i];
				delay = 0;
				while(true) {
					if(ch == U' ') {
						if(cmd.cmd_msg.msg[i + 1] == U' ') {
							ch = U'\n';
							i++;
						} else break;
					} else if(ch == U'{') {
						{
							int j = i + 1;
							while(cmd.cmd_msg.msg[++i] != U'}') {}
							ch = cmd.cmd_msg.msg[++i];
							std::u32string sub = cmd.cmd_msg.msg.substr(j, i - 2);
							if(starts_with(sub, U"delay ")) {
								std::u32string num = sub.substr(6);
								delay = std::stoi(std::string(num.begin(), num.end())) * 1000 / 30;
								goto do_delay;
							}
						}
						if(!skipping) {
							while(scrolling) {
								yield;
							}
							waiting = true;
							midprint = true;
							while(waiting) {
								yield;
							}
							midprint = false;
						}
						if(false) {
							do_delay:
							if(!skipping) {
								scrolling = true;
								while(scrolling) {
									yield;
								}
							}
						}
						time = g_get_millis();
					} else if(ch == U'【') {
						int j = cmd.cmd_msg.msg.find(U'】');
						textbox += cmd.cmd_msg.msg.substr(i, j + 2);
						i = j + 2;
						ch = cmd.cmd_msg.msg[i];
					} else break;
				}
				scrolling = true;
				if(skipping) delay = 0;
				while(true) {
					ntime = g_get_millis();
					if(ntime - time >= delay) {
						time += delay;
						textbox += ch;
						break;
					}
					yield;
				}
			}
			while(scrolling) {
				yield;
			}
			waiting = true;
		} else if(cmd.kind == CMD_BG) {
			if(bg) {
				bg_transition_from = bg.value();
				bg_transition_until = g_get_millis() + BG_TRANSITION_TIME;
			} else {
				bg_transition_from.reset();
				bg_transition_until = 0;
			}
			bg_transition = cmd.cmd_bg.transition;
			bg = cmd.cmd_bg.lnk? load_lnk_texture(LNK_BG, cmd.cmd_bg.filename): g_load_texture(FileDataStream(cmd.cmd_bg.filename.c_str()));
			for(int i = 0; i < 3; i++) {
				if(sprites[i]) {
					sprites[i].reset();
				}
			}
		} else if(cmd.kind == CMD_FG) {
			transition_from.reset();
			transition = 0;
			for(i = 0; i < 3; i++) {
				if(sprites[i]) {
					if(sprites[i].value().who == cmd.cmd_fg.who) {
						transition_from.emplace(sprites[i].value().tex);
						transition = 1;
						sprites[i].reset();
					} else if(i == cmd.cmd_fg.where || sprites[i].value().slot == cmd.cmd_fg.slot) {
						time = g_get_millis();
						ntime = time;
						sprites[i].value().fade_out_until = time + SLIDE_TIME;
						while(ntime - time < SLIDE_TIME) {
							ntime = g_get_millis();
							yield;
						}
						sprites[i].reset();
					}
					/*if(sprites[i].value().slot == cmd.cmd_fg.slot) {
						sprites[i].value().slot = -1;
					}*/
				}
			}
			{
				SpriteSlot sp;
				sp.tex = load_lnk_texture(LNK_CHR, cmd.cmd_fg.filename);
				g_ensure_upscale(sp.tex, -1, g_height());
				sp.who = cmd.cmd_fg.who;
				sp.light = cmd.cmd_fg.light;
				sp.slot = cmd.cmd_fg.slot;
				sp.off = 0;
				sp.fade_in_until = transition == 0? g_get_millis() + SLIDE_TIME: 0;
				sp.fade_out_until = 0;
				if(transition == 1) {
					sp.fancy_transition_from = transition_from;
					sp.fancy_transition_until = g_get_millis() + SLIDE_TIME;
					transition_from.reset();
				} else {
					sp.fancy_transition_until = 0;
				}
				sprites[cmd.cmd_fg.where].emplace(sp);
			}
			// if we're in the left or right and there's someone in the middle of the screen, slide them to their new place
			if(sprites[0] && sprites[1] && !sprites[2]) {
				sprites[2].emplace(std::move(sprites[1].value()));
				sprites[1].reset();
				sprites[2].value().off = -200;
				time = g_get_millis();
				ntime = time;
				while(ntime - time < SLIDE_TIME) {
					yield;
					ntime = g_get_millis();
					sprites[2].value().off = -200 + (i32) (ntime - time) * 200 / SLIDE_TIME;
				}
				sprites[2].value().off = 0;
			} else if(sprites[2] && sprites[1] && !sprites[0]) {
				sprites[0].emplace(std::move(sprites[1].value()));
				sprites[1].reset();
				sprites[0].value().off = 200;
				time = g_get_millis();
				ntime = time;
				while(ntime - time < SLIDE_TIME) {
					yield;
					ntime = g_get_millis();
					sprites[0].value().off = 200 - (i32) (ntime - time) * 200 / SLIDE_TIME;
				}
				sprites[0].value().off = 0;
			}
		} else if(cmd.kind == CMD_MVFG) {
			for(i = 0; i < 3; i++) {
				if(sprites[i] && sprites[i].value().slot == cmd.cmd_mvfg.slot) {
					if(i == cmd.cmd_mvfg.where) break;
					if(sprites[cmd.cmd_mvfg.where]) {
						time = g_get_millis();
						ntime = time;
						sprites[cmd.cmd_mvfg.where].value().fade_out_until = time + SLIDE_TIME;
						while(ntime - time < SLIDE_TIME) {
							ntime = g_get_millis();
							yield;
						}
						sprites[cmd.cmd_mvfg.where].reset();
					}
					sprites[cmd.cmd_mvfg.where].emplace(std::move(sprites[i].value()));
					sprites[i].reset();
					sprites[cmd.cmd_mvfg.where].value().off = (i - cmd.cmd_mvfg.where) * 200;
					time = g_get_millis();
					ntime = time;
					while(ntime - time < SLIDE_TIME) {
						yield;
						ntime = g_get_millis();
						sprites[cmd.cmd_mvfg.where].value().off = (i - cmd.cmd_mvfg.where) * 200 + (i32) (ntime - time) * (cmd.cmd_mvfg.where - i) * 200 / SLIDE_TIME;
					}
					sprites[cmd.cmd_mvfg.where].value().off = 0;
				}
			}
		} else if(cmd.kind == CMD_RMFG) {
			for(i = 0; i < 3; i++) {
				if(sprites[i]) {
					if(cmd.cmd_rmfg.slot == -1) {
						sprites[i].reset();
					} else if(sprites[i].value().slot == cmd.cmd_rmfg.slot) {
						time = g_get_millis();
						ntime = time;
						sprites[i].value().fade_out_until = time + SLIDE_TIME;
						while(ntime - time < SLIDE_TIME) {
							ntime = g_get_millis();
							yield;
						}
						sprites[i].reset();
					}
				}
			}
		} else if(cmd.kind == CMD_MUSIC) {
			if(bgm_channel != -1) {
				a_destroy(bgm_channel);
			}
			if(cmd.cmd_music.track == 0) {
				if(bgm_channel != -1) {
					a_destroy(bgm_channel);
					bgm_channel = -1;
				}
			} else {
				bgm_channel = a_play(((cmd.cmd_music.track < 10? "res/n7/music/ps2/track_0"s: "res/n7/music/ps2/track_"s) + std::to_string(cmd.cmd_music.track) + ".ogg"s).c_str(), A_FMT_OGG, true, volume_bgm / 100.f);
			}
		} else if(cmd.kind == CMD_SFX) {
			if(cmd.cmd_sfx.sfx == -1) {
				for(i = 0; i < sfx_channels.size(); i++) {
					if(!skipping) {
						while(a_playing(sfx_channels[i])) {
							yield;
						}
					}
					a_destroy(sfx_channels[i]);
				}
				sfx_channels.clear();
			} else {
				int ch = play_lnk_audio(LNK_SE, (cmd.cmd_sfx.sfx < 10? "se00"s: cmd.cmd_sfx.sfx < 100? "se0"s: "se"s) + std::to_string(cmd.cmd_music.track), volume_sfx / 100.f);
				sfx_channels.push_back(ch);
			}
		} else if(cmd.kind == CMD_VOICE) {
			if(voice_channel != -1) {
				a_destroy(voice_channel);
			}
			//log_info("%s", cmd.cmd_voice.filename.c_str());
			voice_channel = play_lnk_audio(LNK_VOICE, cmd.cmd_voice.filename, volume_voice / 100.f);
			let_voice_play = true;
		} else if(cmd.kind == CMD_DELAY) {
			if(!skipping) {
				time = g_get_millis() + cmd.cmd_delay.frames * 1000 / 30;
				while(g_get_millis() < time) {
					yield;
				}
			}
		} else if(cmd.kind == CMD_JUMP_FILE) {
			delete script;
			script = new DecScriptStream(("res/n7/script/"s + cmd.cmd_jump_file.name + ".dec"s).c_str());
		} else if(cmd.kind == CMD_JUMP) {
			script->jump(cmd.cmd_jump.addr);
		}
	coend;
}

GameMode::~GameMode() {
	if(voice_channel != -1) {
		a_destroy(voice_channel);
	}
	if(bgm_channel != -1) {
		a_destroy(bgm_channel);
	}
	for(int i = 0; i < sfx_channels.size(); i++) {
		a_destroy(sfx_channels[i]);
	}
}

void GameMode::input(int key, int action) {
	switch(state) {
		case STATE_NORMAL:
			if(action) {
				switch(key) {
					case INPUT_OK:
						if(scrolling) {
							scrolling = false;
						} else if(waiting) {
							waiting = false;
						}
						break;
					case INPUT_SKIP:
						skipping = true;
						waiting = false;
						break;
					case INPUT_MENU:
					case INPUT_MENU_BACK:
						state = STATE_PAUSE;
						a_play("res/n7/dc/chosen.wav", A_FMT_WAV, false, volume_gui / 100.f);
						pause_anim_start = g_get_millis();
						break;
				}
			} else {
				switch(key) {
					case INPUT_SKIP:
						skipping = false;
						break;
				}
			}
			break;
		case STATE_PAUSE:
			if(action) {
				switch(key) {
					case INPUT_OK:
						pause_clicked = true;
						break;
					case INPUT_UP:
						pause_selected--;
						if(pause_selected == -1) {
							pause_selected = 6;
						}
						break;
					case INPUT_DOWN:
						pause_selected++;
						if(pause_selected == 7) {
							pause_selected = 0;
						}
						break;
					case INPUT_MENU:
					case INPUT_MENU_BACK:
						state = STATE_CLOSING_PAUSE;
						a_play("res/n7/dc/back.wav", A_FMT_WAV, false, volume_gui / 100.f);
						pause_anim_start = g_get_millis();
						break;
				}
			}
			break;
		case STATE_SETTINGS:
			if(action) {
				switch(key) {
					case INPUT_OK:
						//gui_input.event = GUI_EVENT_OK;
						break;
					case INPUT_RIGHT:
						//gui_input.event = GUI_EVENT_RIGHT;
						break;
					case INPUT_LEFT:
						//gui_input.event = GUI_EVENT_LEFT;
						break;
					case INPUT_UP:
						//gui_input.event = GUI_EVENT_UP;
						break;
					case INPUT_DOWN:
						//gui_input.event = GUI_EVENT_DOWN;
						break;
					case INPUT_MENU:
					case INPUT_MENU_BACK:
						state = STATE_PAUSE;
						a_play("res/n7/dc/back.wav", A_FMT_WAV, false, volume_gui / 100.f);
						settings_close();
						break;
				}
			}
			break;
	}
}

void GameMode::cursor(int x, int y, bool dragging) {
	if(state == STATE_PAUSE) {
		if(x >= 34_600p && x < 214_600p && y >= 40_600p && y < 32_600p * 7 + 40_600p) {
			pause_selected = (y - 40_600p) / 32_600p;
		}
	}
	//gui_input.event = dragging? GUI_EVENT_DRAG: GUI_EVENT_MOUSE;
	//gui_input.mouse_x = x;
	//gui_input.mouse_y = y;
}

void do_button(GameMode* game, int id, std::u32string text, int x, int y, int w, int h, std::function<void()> HOW) {
	if(id == game->pause_selected) {
		draw_beam_2(x, y + 4_600p, w, h);
		if(game->pause_clicked) {
			game->pause_clicked = false;
			HOW();
		}
	}
	g_push_color(get_text_color(12));
	g_draw_text(text, x + 17_600p, y + 2_600p, w - 17_600p, 26_600p);
	g_pop_color();
	g_push_color(get_text_color(0));
	g_draw_text(text, x + 15_600p, y, w - 17_600p, 26_600p);
	g_pop_color();
}

void GameMode::draw_menu() {
	u32 time = g_get_millis() - pause_anim_start;
	if(state == STATE_CLOSING_PAUSE) {
		if(time > 100) {
			time = 100;
			state = STATE_NORMAL;
		}
		time = 100 - time;
	} else if(time > 100) {
		time = 100;
	}
	//gui_state.pad(24);
	//local cy = y + (i - 1) * sep + sep / 4 + 8
	int ww = 200_600p, wh = 32_600p * 7 + 38_600p;
	int y = 40_600p;
	int offx = _600p(time * (36_600p + ww)) / 100 - (36_600p + ww);
	draw_window(offx + 24_600p, 24_600p, ww, wh);
	int wl = offx + 34_600p, wcw = ww - 20_600p;
	do_button(this, 0, U"Resume"s, wl, y, wcw, 32_600p, [&]() {
		state = STATE_CLOSING_PAUSE;
		a_play("res/n7/dc/back.wav", A_FMT_WAV, false, volume_gui / 100.f);
		pause_anim_start = g_get_millis();
	});
	y += 32_600p;
	do_button(this, 1, U"Save"s, wl, y, wcw, 32_600p, []() {});
	y += 32_600p;
	do_button(this, 2, U"Load"s, wl, y, wcw, 32_600p, []() {});
	y += 32_600p;
	do_button(this, 3, U"Shortcut"s, wl, y, wcw, 32_600p, []() {});
	y += 32_600p;
	do_button(this, 4, U"Playing Log"s, wl, y, wcw, 32_600p, []() {});
	y += 32_600p;
	do_button(this, 5, U"Options"s, wl, y, wcw, 32_600p, [&]() {
		state = STATE_SETTINGS;
		a_play("res/n7/dc/chosen.wav", A_FMT_WAV, false, volume_gui / 100.f);
	});
	y += 32_600p;
	do_button(this, 6, U"Title"s, wl, y, wcw, 32_600p, []() {
		delete cur_mode;
		cur_mode = new TitleMode();
		a_play("res/n7/dc/chosen.wav", A_FMT_WAV, false, volume_gui / 100.f);
	});
}

void GameMode::draw_waiting_icon(TextInfo text_info) {
	u32 time = g_get_millis();
	if(waiting_icon_start) {
		time -= waiting_icon_start;
	} else {
		waiting_icon_start = time;
		time = 0;
	}
	if(midprint) {
		int frame = time / 100 % 6;
		if(frame > 3) frame = 6 - frame;
		g_push_color(0, 0, 0, 1);
		g_draw_quad_crop(midprint_icon, text_info.end_x + 8_600p, text_info.end_y + 6_600p, 32_600p, 32_600p, 0, frame * 32, 32, 32);
		g_pop_color();
		g_draw_quad_crop(midprint_icon, text_info.end_x + 6_600p, text_info.end_y + 4_600p, 32_600p, 32_600p, 0, frame * 32, 32, 32);
	} else {
		time = time % 6500;
		int frame = 3;
		float rot = 0;
		if(time < 250) {
			rot = time * (0.004f * M_PI);
		} else if(time < 1750) {
			frame = 0;
		} else if(time < 3250) {
			frame = 1;
		} else if(time < 4750) {
			frame = 2;
		} else {
			frame = 3;
		}
		g_push_color(0, 0, 0, 1);
		g_draw_quad_crop_rot(waiting_icon, text_info.end_x + 8_600p, text_info.end_y + 6_600p, 16_600p, 32_600p, 0, frame * 32, 16, 32, rot, 8_600pf, 16_600pf);
		g_pop_color();
		g_draw_quad_crop_rot(waiting_icon, text_info.end_x + 6_600p, text_info.end_y + 4_600p, 16_600p, 32_600p, 0, frame * 32, 16, 32, rot, 8_600pf, 16_600pf);
	}
}

void GameMode::draw() {
	execute_cmd();
	if(skipping) {
		scrolling = false;
		waiting = false;
		execute_cmd();
		scrolling = false;
		waiting = false;
	}
	u32 curtime = g_get_millis();
	int h = g_height();
	if(bg) { // time for transitions...........
		if(bg_transition_until > curtime) {
			g_draw_quad(bg_transition_from.value(), 0, 0, -1, h, G_CENTER, G_CENTER);
			float t = ((float) (curtime - (bg_transition_until - BG_TRANSITION_TIME))) / BG_TRANSITION_TIME;
			switch(bg_transition) {
				case 0:
					g_push_color(1.f, 1.f, 1.f, t);
					g_draw_quad(bg.value(), 0, 0, -1, h, G_CENTER, G_CENTER);
					g_pop_color();
					break;
				case 1:
					g_push_color_off(0.f, 0.f, 0.f, t);
					g_draw_quad(bg.value(), 0, 0, -1, h, G_CENTER, G_CENTER);
					g_pop_color_off();
					break;
				case 2:
					g_push_color(1.f, 1.f, 1.f, t);
					g_draw_quad(bg.value(), 0, 0, -1, h, G_CENTER, G_CENTER);
					g_pop_color();
					break;
				case 4:
					for(int tx = 0; tx < 800; tx += 32) {
						float tt = t * 2.f - tx / 800.f;
						if(tt > 1.f) tt = 1.f;
						if(tt > 0.f) {
							g_push_color(1.f, 1.f, 1.f, tt);
							g_draw_quad_crop(bg.value(), _600p(tx - 400), 0, -1, h, tx, 0, tt * 32, 600, G_TOP_LEFT, G_CENTER_TOP);
							g_pop_color();
						}
					}
					break;
				case 5:
					for(int tx = 0; tx < 800; tx += 32) {
						float tt = t * 2.f - (800 - tx) / 800.f;
						if(tt > 1.f) tt = 1.f;
						if(tt > 0.f) {
							g_push_color(1.f, 1.f, 1.f, tt);
							float tw = tt * 32;
							g_draw_quad_crop(bg.value(), _600pf(tx - 368), 0, -1, h, tx + 32 - tw, 0, tw, 600, G_TOP_RIGHT, G_CENTER_TOP);
							g_pop_color();
						}
					}
					break;
			}
		} else {
			if(bg_transition_until != 0) {
				bg_transition_until = 0;
				bg_transition_from.reset();
				if(bg_transition == 1) {
					bg.emplace(g_load_texture(FileDataStream("res/n7/white.png")));
				}
			}
			g_draw_quad(bg.value(), 0, 0, -1, h, G_CENTER, G_CENTER);
		}
	}
	for(int j = 0; j < 3; j++) {
		int i = j == 2? 1: j == 1? 2: j;
		if(sprites[i]) {
			SpriteSlot& sp = sprites[i].value();
			float alpha = 1.f;
			if(sp.fade_in_until > curtime) {
				alpha = ((float) (curtime - (sp.fade_in_until - SLIDE_TIME))) / SLIDE_TIME;
			} else if(sp.fade_out_until != 0) {
				alpha = 1.f - ((float) (curtime - (sp.fade_out_until - SLIDE_TIME))) / SLIDE_TIME;
			}
			switch(sp.light) {
				case LIGHT_DAY:
					g_push_color(1.f, 1.f, 1.f, alpha);
					break;
				case LIGHT_EVENING:
					g_push_color(0xed / 255.f, 0xb8 / 255.f, 0x7c / 255.f, alpha);
					break;
				case LIGHT_NIGHT:
					g_push_color(0x7e / 255.f, 0x99 / 255.f, 0xff / 255.f, alpha);
					break;
			}
			if(sp.fancy_transition_until > curtime) {
				int up_w = (h * sp.tex.width() + sp.tex.height() - 1) / sp.tex.height();
				g_ensure_upscale(sp.tex, up_w, h);
				int from_up_w = (h * sp.tex.width() + sp.tex.height() - 1) / sp.tex.height();
				g_ensure_upscale(sp.fancy_transition_from.value(), from_up_w, h);
				sp.tex->upscale = false;
				sp.fancy_transition_from.value().data->upscale = false;
				float t = ((float) (curtime - (sp.fancy_transition_until - SLIDE_TIME))) / SLIDE_TIME;
				int part_h = 24_600p;
				int cur_h = clamp(24 * t + 0.5f, 0.f, 24.f);
				int tw = sp.tex.width(), th = sp.tex.height();
				int ptw = sp.fancy_transition_from.value().width(), pth = sp.fancy_transition_from.value().height();
				int mth = _600p(std::max(pth, th));
				int x = _600p(-200 + i * 200 + sp.off);
				for(int sy = h - part_h; sy >= h - mth; sy -= 24) {
					float ty = (float) (sy - (h - th)) * 600_600p / h, pty = (float) (sy - (h - th));
					g_draw_quad_crop(sp.tex, x, _600p(sy), up_w, cur_h, 0, ty, tw, cur_h, G_CENTER_TOP, G_CENTER_TOP);
					g_draw_quad_crop(sp.fancy_transition_from.value(), x, sy + cur_h, from_up_w, part_h - cur_h, 0, pty + cur_h * 600_600p / h, ptw, part_h - cur_h, G_CENTER_TOP, G_CENTER_TOP);
				}
				sp.tex->upscale = true;
				sp.fancy_transition_from.value().data->upscale = true;
			} else {
				sp.fancy_transition_until = 0;
				sp.fancy_transition_from.reset();
				g_draw_quad(sp.tex, _600p(-200 + i * 200 + sp.off), 0, -1, _600p(sp.tex.height()), G_CENTER_BOTTOM, G_CENTER_BOTTOM);
			}
			g_pop_color();
		}
	}
	draw_window(0, -16_600p, 760_600p, 194_600p, G_CENTER_BOTTOM, G_CENTER_BOTTOM);
	Color text_color = get_text_color(0), text_color_2 = get_text_color(6);
	Color shadow = get_text_color(12);
	if(shadow_color == 0) {
		shadow.r = 0.35f; shadow.g = 0.35f; shadow.b = 0.45f;
	} else if(shadow_color == 4) {
		shadow.r = 0.45f; shadow.g = 0.25f; shadow.b = 0.25f;
	} else if(shadow_color == 2 || shadow_color == -1) {
		//shadow_r = 0.f; shadow_g = 0.f; shadow_b = 0.f;
	} else if(shadow_color == 5) {
		shadow.r = 0.45f; shadow.g = 0.35f; shadow.b = 0.35f;
	} else if(shadow_color == 7) {
		shadow.r = 0.45f; shadow.g = 0.45f; shadow.b = 0.15f;
	} else if(shadow_color == 1) {
		shadow.r = 0.45f; shadow.g = 0.25f; shadow.b = 0.45f;
	} else if(shadow_color == 8) {
		shadow.r = 0.35f; shadow.g = 0.4f; shadow.b = 0.5f;
	} else if(shadow_color == 6) {
		shadow.r = 0.55f; shadow.g = 0.45f; shadow.b = 0.35f;
	} else {
		shadow.r = 0.f; shadow.g = 0.f; shadow.b = 0.f;
		log_err("unknown shadow color %d", shadow_color);
	}
	std::u32string rest = textbox;
	if(starts_with(textbox, U"【"s)) {
		int i = textbox.find(U"】"s);
		rest = textbox.substr(i + 2);
		if(rest[0] == U'"') {
			rest[0] = U'「';
			if(rest[rest.size() - 1] == U'"') {
				rest[rest.size() - 1] = U'」';
			} else {
				rest += U'」';
			}
		}
		std::u32string speaker_name = textbox.substr(1, i - 1);
		int w = g_calc_text(speaker_name, 12124, 26_600p).w;
		draw_window(-380_600p, -210_600p, w + 64_600p, 64_600p, G_BOTTOM_LEFT, G_CENTER_BOTTOM);
		g_push_color(shadow);
		g_draw_text(speaker_name, -346_600p, -258_600p, w, 26_600p, G_TOP_LEFT, G_CENTER_BOTTOM);
		g_pop_color();
		g_push_color(text_color);
		g_draw_text(speaker_name, -348_600p, -260_600p, w, 26_600p, G_TOP_LEFT, G_CENTER_BOTTOM);
		g_pop_color();
	}
	g_push_color(shadow);
	int i = 0;
	g_draw_text_alpha(rest, 2_600p, -194_600p, 712_600p, 26_600p, [&](auto c) {
		if(!scrolling || i < scrolling_after) {
			i++;
			return 1.f;
		} else if(curtime - scrolling_since < i * CHAR_WAIT) {
			i++;
		return 0.f;
		} else {
			return clamp((curtime - (scrolling_since + i++ * CHAR_WAIT)) / (float) CHAR_TIME, 0.f, 1.f);
		}
	}, G_CENTER_TOP, G_CENTER_BOTTOM);
	i = 0;
	g_pop_color();
	g_push_color(text_color);
	TextInfo text_info = g_draw_text_alpha_gradient(rest, 0, -196_600p, 712_600p, 26_600p, [&](auto c) {
		if(!scrolling || i < scrolling_after) {
			i++;
			return 1.f;
		} else if(curtime - scrolling_since < i * CHAR_WAIT) {
			i++;
			return 0.f;
		} else {
			return clamp((curtime - (scrolling_since + i++ * CHAR_WAIT)) / (float) CHAR_TIME, 0.f, 1.f);
		}
	}, text_color_2, G_CENTER_TOP, G_CENTER_BOTTOM);
	g_pop_color();
	if(curtime - scrolling_since >= i * CHAR_WAIT) {
		scrolling = false;
		scrolling_after = i;
		scrolling_since = curtime - i * CHAR_WAIT;
	}
	if(waiting) {
		draw_waiting_icon(text_info);
	} else {
		waiting_icon_start = 0;
	}
	if(state == STATE_PAUSE || state == STATE_CLOSING_PAUSE) {
		draw_menu();
	} else if(state == STATE_SETTINGS) {
		settings_draw();
	}
	//g_draw_effect(EFFECT_N7_CALENDAR, std::sin(curtime % 8000 / 4000.f * M_PI), bg.value(), 0, 0, -1, h, 0, 0, -1, -1, G_CENTER, G_CENTER);
}

}
