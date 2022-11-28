#pragma once
#include <fs.hpp>
#include <map>
#include <glm/vec3.hpp>
#include <string_view>

enum {
	SC_RES_BG,
	SC_RES_CHARA,
	SC_RES_BGM,
	SC_RES_SE,
	SC_RES_VOICE,
};

enum {
	SC_OP_FETCH,
	SC_OP_HALT,
	SC_OP_BG,
	SC_OP_SPRITE,
	SC_OP_EFFECT,
	SC_OP_FADEOUT,
	SC_OP_DELAY,
	SC_OP_MSG_MARKER,
	SC_OP_MSG,
	SC_OP_FRAME,
	SC_OP_BGM,
	SC_OP_BGM_INT,
	SC_OP_SE,
	SC_OP_SE_INT,
	SC_OP_STOP_SE
};

constexpr u32 MAX_LOADED_RES = 8;

struct VnScript {
	VnScript();
	~VnScript();
	u32 make_str(const char* str);
	u32 make_res(std::string_view res_src, u32 kind);
	void push_bgm(u32 res); // op-1 res-3 total-4
	void push_bgm_int(u8 bgm_id); // op-1 id-1 total-2
	void push_se(u32 res); // op-1 res-3 total-4
	void push_se_int(u8 bgm_id); // op-1 id-1 total-2
	void push_stop_se();
	void push_stopse();
	void push_bg(u32 res_id, u8 trans, u16 duration, bool wait);
	void push_bg_effect(u32 str_id, u8 slot, int lua_params);
	void push_sprite(u32 res_id, u8 slot, u8 pos, u8 trans, u16 duration, bool wait);
	void push_sprite_effect(u32 res_id, u8 slot);
	void push_voice(const char* src); // op-1 src-str total-variable
	u32 make_msg_group(const char* name); // a script ID, as marker numbers are often script-local
	void push_msg_marker(u32 group, u32 id); // op-1 group-3 id-3 total-7
	void push_msg(u8 speaker_id, const char* speaker_name, const char* msg);
	void push_delay(u32 millis);
	void push_set_int_global(u32 field, u32 val);
	void push_set_float_global(u32 field, float val);
	void push_set_int(u32 field, u32 val);
	void push_set_float(u32 field, float val);
	Mem serialize(); // expose the internal buffer for caching

	struct Resource {
		std::string src;
		u32 kind;
	};
	std::vector<Resource> resources;
	std::map<std::string, u32, std::less<>> res_names;
	std::vector<std::string> strings;
	std::vector<std::string> msg_groups;
	u32 rc = 0;
	u32 size = 0;
	u8* buf;
private:
	u32 capacity = 1024;

	void grow(u32 sz, u32 cap);
	void push8(u8 x);
	void push16(u16 x);
	void push24(u32 x);
	void push32(u32 x);
	void push_str(const char* x);
	void push_str_view(std::string_view x);
	void push_trans(u8 id, u16 duration, bool wait, const char* err_ctx);
};

enum {
	EX_RUNNING,
	EX_WAITING,
	EX_PRINTING,
	EX_DELAY,
	EX_TRANSITION,
	EX_HALTED
};

constexpr u32 MAX_SPRITES = 1 << 4;

enum {
	TYPE_NULL,
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_BOOL,
	TYPE_STRING
};

struct VnSettings {
	float text_speed = 32.f;
	bool fast_forward_all;
	bool instant_skip_all;
	char language[4];
};

struct VnVal {
	u8 type = TYPE_NULL;
	union {
		i32 intv;
		float floatv;
		bool boolv;
		const char* stringv;
	};

	VnVal();
	VnVal(const VnVal& o);
	VnVal(VnVal&& o);
	~VnVal();
	std::string to_string();
	std::string serialize();
	static VnVal deserialize(const std::string_view& str);
};

struct VnExecutor {
	struct MsgBox {
		i8 frame_type = 0;
		bool is_midprint = false;
		bool is_read = false;
		bool is_waiting = false;
		float progress;
		u8 speaker_id;
		std::string speaker_name;
		std::string text;
		u16 len_without_newlines = 0;
		glm::vec3 text_shadow_color;
	} cur_msg;
	bool msg_changed = false;
	struct Object {
		u32 prev_res = 0xFFFFFFFF;
		u32 res = 0xFFFFFFFF;
		u32 trans_start = 0;
		u16 trans_duration = 0;
		u8 trans_id = 0;
		bool trans_wait = false;
		u8 prev_pos;
		u8 pos;
		u32 move_start = 0;
		u32 move_duration = 0; // todo think about this
	};
	Rc<VnScript> sc;
	u32 cursor = 0;
	u8 state = EX_RUNNING;
	u32 bgm_if_changed = 0xFFFFFFFF;
	u32 bgm_int_if_changed = 0xFFFFFFFF;
	u32 se_if_changed = 0xFFFFFFFF;
	u32 se_int_if_changed = 0xFFFFFFFF;
	Object bg;
	Object sprites[MAX_SPRITES];
	std::map<std::string, VnVal> vars;
	u32 delay_until = 0;
	VnSettings settings;

	VnExecutor(Rc<VnScript> sc);
	void process();
	void next(u32 choice_idx);
	bool fast_forward();
	bool skip_to_unread();

private:
	u8 read8();
	u16 read16();
	u32 read24();
	u32 read32();
	u32 readres();
	void read_trans(Object& into);
	const char* read_str();
	void clear_delays();
};
