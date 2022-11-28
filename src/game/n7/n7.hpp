#pragma once
#include <game/game.hpp>
#include <vector>
#include <map>
#include <optional>
#include <functional>

namespace n7 {

struct Mode {
	virtual ~Mode() {}
	virtual void input(int key, int action) = 0;
	virtual void cursor(int x, int y, bool dragging) = 0;
	virtual void draw() = 0;
};

extern Mode* cur_mode;

enum {
	LNK_BG,
	LNK_CHR,
	LNK_VOICE,
	LNK_SE,
	LNK_COUNT
};

TextureRef load_lnk_texture(int lnk, const std::string& path);
int play_lnk_audio(int lnk, const std::string& path, float volume = 1.f);

enum {
	CMD_NONE,
	CMD_MESSAGE,
	CMD_BG,
	CMD_FG,
	CMD_MVFG,
	CMD_RMFG,
	CMD_MUSIC,
	CMD_SFX,
	CMD_VOICE,
	CMD_DELAY,
	CMD_JUMP_FILE,
	CMD_JUMP
};

enum {
	CHR_NONE,
	CHR_MA,
	CHR_OK,
	CHR_YU,
	CHR_HA,
	CHR_SA,
	CHR_KU,
	CHR_IZ,
	CHR_COUNT
};

enum {
	LIGHT_DAY,
	LIGHT_EVENING,
	LIGHT_NIGHT
};

enum {
	STATE_NORMAL,
	STATE_PAUSE,
	STATE_CLOSING_PAUSE,
	STATE_SETTINGS,
	STATE_MODS
};

extern int volume_master;
extern int volume_bgm;
extern int volume_voice;
extern int volume_sfx;
extern int volume_gui;

struct Cmd {
	union {
		struct {
			std::u32string msg;
			u8 speaker;
			int shadow_color;
		} cmd_msg;
		struct {
			std::string filename;
			u8 transition;
			bool lnk;
		} cmd_bg;
		struct {
			std::string filename;
			u8 where;
			u8 who;
			i8 slot;
			u8 light;
		} cmd_fg;
		struct {
			i8 slot;
			u8 where;
		} cmd_mvfg;
		struct {
			i8 slot;
		} cmd_rmfg;
		struct {
			u8 track;
		} cmd_music;
		struct {
			i16 sfx;
		} cmd_sfx;
		struct {
			std::string filename;
			u8 who;
		} cmd_voice;
		struct {
			u32 frames;
		} cmd_delay;
		struct {
			std::string name;
		} cmd_jump_file;
		struct {
			u32 addr;
		} cmd_jump;
	};
	u8 kind = CMD_NONE;

	Cmd();
	Cmd(const Cmd& o);
	~Cmd();
};

struct ScriptStream {
	virtual ~ScriptStream() {};
	virtual Cmd next() = 0;
	virtual void jump(u32 addr) = 0;
};

struct DecScriptStream: ScriptStream {
	DataStream* data;
	std::string filename;
	std::vector<Cmd> cmds;
	std::map<u32, u32> jumps;
	u32 idx = 0;

	DecScriptStream(const char* path);
	Cmd next() override;
	void jump(u32 addr) override;
};

struct Particle {
	bool (*frame)(Particle* p, float dt);
	int x, y, time;
};

struct SpriteSlot {
	TextureRef tex;
	i8 slot;
	u8 who;
	u8 light;
	i16 off;
	u32 fade_in_until;
	u32 fade_out_until;
	std::optional<TextureRef> fancy_transition_from;
	u32 fancy_transition_until;
};

struct GameMode: Mode {
	int state = STATE_NORMAL;
	std::optional<TextureRef> bg_transition_from;
	std::optional<TextureRef> bg;
	int bg_transition;
	u32 bg_transition_until;
	std::optional<SpriteSlot> sprites[3];
	std::u32string textbox;
	int shadow_color = -1;
	Particle particles[128];
	int n_particles = 0;
	ScriptStream* script;
	bool waiting = false;
	bool skipping = false;
	bool scrolling = false;
	bool midprint = false;
	u32 pause_anim_start = 0;
	int pause_selected = 0;
	bool pause_clicked = 0;
	u32 scrolling_after;
	u32 scrolling_since;
	bool let_voice_play = false;
	int bgm_channel = -1;
	int voice_channel = -1;
	std::vector<int> sfx_channels;
	std::function<void()> execute_cmd;
	TextureRef midprint_icon, waiting_icon;
	u32 waiting_icon_start;

	GameMode();
	~GameMode() override;
	void input(int key, int action) override;
	void cursor(int x, int y, bool dragging) override;
	void draw_waiting_icon(TextInfo text_info);
	void draw_menu();
	void draw() override;
};

struct TitleMode: Mode {
	int state = STATE_NORMAL;
	int selected = 0;
	float seconds;
	TextureRef bg[4];
	TextureRef buttons;
	TextureRef logo;
	TextureRef grayed;
	TextureRef yuka;
	u32 button_last_highlighted[7] = {};

	TitleMode();
	void input(int key, int action) override;
	void cursor(int x, int y, bool dragging) override;
	void draw() override;
};

void draw_beam(int x, int y, int w, int h);
void draw_beam_2(int x, int y, int w, int h);
void draw_window(int x, int y, int w, int h, int corner = G_TOP_LEFT, int origin = G_TOP_LEFT);
Color get_text_color(int which = 0);

int gui_to_screen(int v);
int screen_to_gui(int v);
void settings_draw();
bool settings_input(int key, int action);
void settings_cursor(int x, int y, bool dragging);
void settings_close();
void mods_menu_draw();
bool mods_menu_input(int key, int action);
void mods_menu_cursor(int x, int y, bool dragging);
void mods_menu_close();

}
