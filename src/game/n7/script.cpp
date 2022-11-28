#include "n7.hpp"
#include <algorithm>
#include <cctype>
#include <codecvt>
#include <locale>

using namespace n7;

Cmd::Cmd() {}

Cmd::Cmd(const Cmd& o) {
	kind = o.kind;
	switch(kind) {
		case CMD_MESSAGE:
			new(&cmd_msg.msg) std::u32string(o.cmd_msg.msg);
			cmd_msg.shadow_color = o.cmd_msg.shadow_color;
			cmd_msg.speaker = o.cmd_msg.speaker;
			break;
		case CMD_BG:
			new(&cmd_bg.filename) std::string(o.cmd_bg.filename);
			cmd_bg.lnk = o.cmd_bg.lnk;
			cmd_bg.transition = o.cmd_bg.transition;
			break;
		case CMD_FG:
			new(&cmd_fg.filename) std::string(o.cmd_fg.filename);
			cmd_fg.where = o.cmd_fg.where;
			cmd_fg.who = o.cmd_fg.who;
			cmd_fg.slot = o.cmd_fg.slot;
			cmd_fg.light = o.cmd_fg.light;
			break;
		case CMD_VOICE:
			new(&cmd_voice.filename) std::string(o.cmd_voice.filename);
			cmd_voice.who = o.cmd_voice.who;
			break;
		case CMD_JUMP_FILE:
			new(&cmd_jump_file.name) std::string(o.cmd_jump_file.name);
			break;
		default:
			memcpy(this, &o, sizeof(Cmd));
			break;
	}
}

Cmd::~Cmd() {
	switch(kind) {
		case CMD_MESSAGE:
			cmd_msg.msg.~basic_string();
			break;
		case CMD_BG:
			cmd_bg.filename.~basic_string();
			break;
		case CMD_FG:
			cmd_fg.filename.~basic_string();
			break;
		case CMD_VOICE:
			cmd_voice.filename.~basic_string();
			break;
		case CMD_JUMP_FILE:
			cmd_jump_file.name.~basic_string();
			break;
	}
}

DecScriptStream::DecScriptStream(const char* path) {
	FileDataStream data(path);
	log_info("loading %s", path);
	while(data.remaining()) {
		std::string line = data.read_line();
		Cmd cmd;
		if(line[0] == '[') {
			jumps.insert(std::make_pair(std::stoul(line.substr(1, 8), nullptr, 16), cmds.size()));
			if(starts_with(line, "jump "s, 15)) {
				std::string name = line.substr(20);
				std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {return tolower(c);});
				cmd.kind = CMD_JUMP_FILE;
				new(&cmd.cmd_jump_file.name) std::string(name);
				cmds.push_back(cmd);
			} else if(starts_with(line, "goto7 "s, 15)) {
				u32 addr = std::stoul(line.substr(21, 8), nullptr, 16);
				cmd.kind = CMD_JUMP;
				cmd.cmd_jump.addr = addr;
				cmds.push_back(cmd);
			} else if(starts_with(line, "bgload "s, 15)) {
				int end = line.find(' ', 31);
				cmd.kind = CMD_BG;
				new(&cmd.cmd_bg.filename) std::string(line.substr(31, end - 31));
				cmd.cmd_bg.lnk = true;
				int transition = std::stoi(line.substr(end + 1, 1));
				if(transition == 0 || transition == 4 || transition == 5 || transition == 6) {
				} else if(transition == 2) {
					// pan
				} else {
					log_err("unknown BG transition %d in %s", transition, filename.c_str());
					platform_throw();
				}
				cmd.cmd_bg.transition = transition;
				cmds.push_back(cmd);
			} else if(starts_with(line, "removeBG "s, 15)) {
				cmd.kind = CMD_BG;
				bool white = line[24] == '1';
				cmd.cmd_bg.transition = std::stoi(line.substr(26, 1));
				cmd.cmd_bg.lnk = false;
				if(cmd.cmd_bg.transition != 1 || !white) {
					new(&cmd.cmd_bg.filename) std::string(white? "res/n7/white.png": "res/n7/black.png");
				}
				if(cmd.cmd_bg.transition == 0 || cmd.cmd_bg.transition == 3 || cmd.cmd_bg.transition == 4 || cmd.cmd_bg.transition == 5 or cmd.cmd_bg.transition == 6) {
				} else if(cmd.cmd_bg.transition == 1) {
					if(white) {
						new(&cmd.cmd_bg.filename) std::string("res/n7/mask.png");
					} else {
						cmd.cmd_bg.transition = 3;
					}
				} else if(cmd.cmd_bg.transition == 2) {
					cmd.cmd_bg.transition = 3;
				} else {
					log_err("unknown BG transition %d in %s", cmd.cmd_bg.transition, filename.c_str());
					platform_throw();
				}
				cmds.push_back(cmd);
			} else if(starts_with(line, "fgload "s, 15)) {
				// [000001a3] 11: fgload 0 00000000 YU01CA 1 3
				//		internal slot? ^	 screen pos? ^
				int end = line.find(' ', 33);
				std::string filename = line.substr(33, end - 33);
				std::transform(filename.begin(), filename.end(), filename.begin(), [](unsigned char c) {return tolower(c);});
				if(ends_with(filename, "d"s)) {
					filename = filename.substr(0, filename.size() - 1);
					cmd.cmd_fg.light = LIGHT_EVENING;
				} else if(ends_with(filename, "n"s)) {
					filename = filename.substr(0, filename.size() - 1);
					cmd.cmd_fg.light = LIGHT_NIGHT;
				} else {
					cmd.cmd_fg.light = LIGHT_DAY;
				}
				cmd.kind = CMD_FG;
				new(&cmd.cmd_fg.filename) std::string(filename);
				cmd.cmd_fg.where = std::stoi(line.substr(end + 1, 1));
				if(cmd.cmd_fg.where == 0) cmd.cmd_fg.where = 2;
				else if(cmd.cmd_fg.where == 2) cmd.cmd_fg.where = 0;
				cmd.cmd_fg.slot = std::stoi(line.substr(22, 1));
				if(starts_with(filename, "yu"s)) {
					cmd.cmd_fg.who = CHR_YU;
				} else if(starts_with(filename, "ha"s)) {
					cmd.cmd_fg.who = CHR_HA;
				} else if(starts_with(filename, "ku"s)) {
					cmd.cmd_fg.who = CHR_KU;
				} else if(starts_with(filename, "iz"s)) {
					cmd.cmd_fg.who = CHR_IZ;
				} else if(starts_with(filename, "ok"s)) {
					cmd.cmd_fg.who = CHR_OK;
				} else if(starts_with(filename, "sa"s)) {
					cmd.cmd_fg.who = CHR_SA;
				} else {
					cmd.cmd_fg.who = CHR_NONE;
				}
				cmds.push_back(cmd);
			} else if(starts_with(line, "moveFG "s, 15)) {
				cmd.kind = CMD_MVFG;
				cmd.cmd_mvfg.slot = std::stoi(line.substr(22, 1));
				cmd.cmd_mvfg.where = std::stoi(line.substr(24, 1));
				if(cmd.cmd_mvfg.where == 0) cmd.cmd_mvfg.where = 2;
				else if(cmd.cmd_mvfg.where == 2) cmd.cmd_mvfg.where = 0;
				cmds.push_back(cmd);
			} else if(starts_with(line, "removeFG "s, 15)) {
				cmd.kind = CMD_RMFG;
				cmd.cmd_rmfg.slot = std::stoi(line.substr(24, 1));
				cmds.push_back(cmd);
			} else if(starts_with(line, "removeAllFG"s, 15)) {
				cmd.kind = CMD_RMFG;
				cmd.cmd_rmfg.slot = -1;
				cmds.push_back(cmd);
			} else if(starts_with(line, "playBGM "s, 15)) {
				cmd.kind = CMD_MUSIC;
				cmd.cmd_music.track = std::stoi(line.substr(23, line.size() - 23));
				cmds.push_back(cmd);
			} else if(starts_with(line, "stopBGM "s, 15)) {
				cmd.kind = CMD_MUSIC;
				cmd.cmd_music.track = 0;
				cmds.push_back(cmd);
			} else if(starts_with(line, "playSFX "s, 15)) {
				cmd.kind = CMD_SFX;
				if(line[23] == 'S') {
					cmd.cmd_sfx.sfx = std::stoi(line.substr(25, line.size() - 25));
				} else {
					cmd.cmd_sfx.sfx = std::stoi(line.substr(23, line.find(' ', 23) - 23));
				}
				cmds.push_back(cmd);
			} else if(starts_with(line, "waitForSFX "s, 15)) {
				cmd.kind = CMD_SFX;
				cmd.cmd_sfx.sfx = -1;
				cmds.push_back(cmd);
			} else if(starts_with(line, "playVoice "s, 15)) {
				cmd.kind = CMD_VOICE;
				std::string fn = line.substr(25, line.size() - 25);
				std::transform(fn.begin(), fn.end(), fn.begin(), [](char c) {return tolower(c);});
				new(&cmd.cmd_voice.filename) std::string(std::move(fn));
				cmds.push_back(cmd);
			} else if(starts_with(line, "delay "s, 15)) {
				cmd.kind = CMD_DELAY;
				cmd.cmd_jump.addr = std::stoi(line.substr(21, 8));
				cmds.push_back(cmd);
			}
		} else if(starts_with(line, "{textColor")) {
			std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
			std::u32string uline = converter.from_bytes(line);
			if(uline[uline.size() - 1] == U' ') {
				uline.erase(uline.size() - 1);
			}
			//log_info("%s", line.substr(15).c_str());
			cmd.kind = CMD_MESSAGE;
			new(&cmd.cmd_msg.msg) std::u32string(uline.substr(15));
			if(starts_with(line, "0 2}"s, 11)) {
				cmd.cmd_msg.speaker = CHR_NONE;
				cmd.cmd_msg.shadow_color = 2;
			} else if(starts_with(line, "1 4}"s, 11)) {
				cmd.cmd_msg.speaker = CHR_YU;
				cmd.cmd_msg.shadow_color = 4;
			} else if(starts_with(line, "1 8}"s, 11)) {
				cmd.cmd_msg.speaker = CHR_IZ;
				cmd.cmd_msg.shadow_color = 8;
			} else if(starts_with(line, "1 7}"s, 11)) {
				cmd.cmd_msg.speaker = CHR_KU;
				cmd.cmd_msg.shadow_color = 7;
			} else if(starts_with(line, "1 6}"s, 11)) {
				cmd.cmd_msg.speaker = CHR_SA;
				cmd.cmd_msg.shadow_color = 6;
			} else if(starts_with(line, "0 0}【Makoto"s, 11)) {
				cmd.cmd_msg.speaker = CHR_MA;
				cmd.cmd_msg.shadow_color = 0;
			} else if(starts_with(line, "0 0}"s, 11)) {
				cmd.cmd_msg.speaker = CHR_OK;
				cmd.cmd_msg.shadow_color = 0;
			} else {
				cmd.cmd_msg.speaker = CHR_NONE;
				cmd.cmd_msg.shadow_color = -1;
			}
			cmds.push_back(cmd);
		}
	}
	log_info("done");
}

Cmd DecScriptStream::next() {
	log_info("cmd %u", cmds[idx].kind);
	return cmds[idx++];
}

void DecScriptStream::jump(u32 addr) {
	idx = jumps[addr];
}
