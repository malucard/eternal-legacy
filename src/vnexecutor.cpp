#include <vnscript.hpp>
#include <graphics.hpp>
#include <algorithm>

VnVal::VnVal() {}

VnVal::VnVal(const VnVal& o) {
	// just to be safe with all the architectures
	if(type == TYPE_STRING) {
		int sz = strlen(o.stringv) + 1;
		char* str = (char*) malloc(sz);
		memcpy(str, o.stringv, sz);
		stringv = str;
	} else if(type == TYPE_INT) {
		intv = o.intv;
	} else if(type == TYPE_FLOAT) {
		floatv = o.floatv;
	} else if(type == TYPE_BOOL) {
		boolv = o.boolv;
	}
	type = o.type;
}

VnVal::VnVal(VnVal&& o) {
	// just to be safe with all the architectures
	if(type == TYPE_STRING) {
		stringv = o.stringv;
	} else if(type == TYPE_INT) {
		intv = o.intv;
	} else if(type == TYPE_FLOAT) {
		floatv = o.floatv;
	} else if(type == TYPE_BOOL) {
		boolv = o.boolv;
	}
	type = o.type;
	o.type = TYPE_NULL;
}

VnVal::~VnVal() {
	if(type == TYPE_STRING) {
		delete stringv;
	}
	type = TYPE_NULL;
}

std::string VnVal::to_string() {
	if(type == TYPE_STRING) {
		return stringv;
	} else if(type == TYPE_INT) {
		return std::to_string(intv);
	} else if(type == TYPE_FLOAT) {
		return std::to_string(floatv);
	} else if(type == TYPE_BOOL) {
		return std::to_string(boolv);
	} else {
		return "null";
	}
}

std::string VnVal::serialize() {
	if(type == TYPE_STRING) {
		return std::string('"', 1) + stringv + '"';
	} else if(type == TYPE_INT) {
		return std::to_string(intv);
	} else if(type == TYPE_FLOAT) {
		return std::to_string(floatv);
	} else if(type == TYPE_BOOL) {
		return std::to_string(boolv);
	} else {
		return "null";
	}
}

VnVal VnVal::deserialize(const std::string_view& str) {
	VnVal val;
	if(str[0] == '"') {
		char* str2 = (char*) malloc(str.size() - 2);
		memcpy(str2, str.substr(1, str.size() - 2).data(), str.size() - 2);
		str2[str.size() - 3] = 0;
		val.stringv = str2;
		val.type = TYPE_STRING;
	} else if(str.find('.')) {
		val.floatv = std::stod(str.data());
		val.type = TYPE_FLOAT;
	} else if(str[0] >= '0' && str[0] <= '9') {
		val.intv = std::stol(str.data());
		val.type = TYPE_INT;
	} else if(str == "true") {
		val.boolv = true;
		val.type = TYPE_BOOL;
	} else if(str == "false") {
		val.boolv = false;
		val.type = TYPE_BOOL;
	} else {
		val.type = TYPE_NULL;
	}
	return val;
}

VnExecutor::VnExecutor(Rc<VnScript> sc): sc(sc) {
	
}

void VnExecutor::process() {
	cur_msg.progress += settings.text_speed * delta;
	if(state == EX_PRINTING) {
		u32 len = cur_msg.len_without_newlines;
		if(cur_msg.progress >= len) {
			cur_msg.is_waiting = true;
			state = EX_WAITING;
		}
	}
	while(cursor < sc->size && state == EX_RUNNING) {
		u32 time = sys_millis();
		if(time < delay_until || (bg.trans_wait && time < bg.trans_start + bg.trans_duration)) {
			return;
		}
		for(int i = 0; i < MAX_SPRITES; i++) {
			if(sprites[i].trans_wait && time < sprites[i].trans_start + sprites[i].trans_duration) {
				return;
			}
		}
		u8 op = read8();
		switch(op) {
			case SC_OP_BG: {
				bg.prev_res = bg.res;
				bg.res = readres();
				read_trans(bg);
				break;
			}
			case SC_OP_SPRITE: {
				u32 res = readres();
				u8 slot_and_pos = read8();
				Object& sprite = sprites[slot_and_pos >> 4];
				sprite.prev_res = sprite.res;
				sprite.res = res;
				sprite.pos = slot_and_pos & 0xF;
				read_trans(sprite);
				break;
			}
			case SC_OP_BGM: {
				bgm_int_if_changed = 0xFFFFFFFF;
				bgm_if_changed = readres();
				break;
			}
			case SC_OP_BGM_INT: {
				bgm_if_changed = 0xFFFFFFFF;
				bgm_int_if_changed = read8();
				break;
			}
			case SC_OP_SE: {
				se_int_if_changed = 0xFFFFFFFF;
				se_if_changed = readres();
				break;
			}
			case SC_OP_SE_INT: {
				se_if_changed = 0xFFFFFFFF;
				se_int_if_changed = read8();
				break;
			}
			case SC_OP_DELAY: {
				delay_until = sys_millis() + read24();
				break;
			}
			case SC_OP_MSG_MARKER: {
				break;
			}
			case SC_OP_FRAME: {
				cur_msg.frame_type = (i8) read8();
				break;
			}
			case SC_OP_MSG: {
				cur_msg.speaker_id = read8();
				cur_msg.speaker_name = read_str();
				cur_msg.text = read_str();
				std::u32string wide = strcvt(cur_msg.text);
				std::remove(wide.begin(), wide.end(), U'\n');
				cur_msg.len_without_newlines = wide.size();
				cur_msg.progress = 0.0;
				cur_msg.is_midprint = false;
				cur_msg.is_waiting = false;
				cur_msg.is_read = false;
				msg_changed = true;
				state = EX_PRINTING;
				break;
			}
			default: {
				log_err("unknown op %u in vn", op);
			}
		}
	}
}

void VnExecutor::next(u32 choice_idx) {
	if(state == EX_PRINTING) {
		cur_msg.progress = cur_msg.len_without_newlines;
		cur_msg.is_waiting = true;
		state = EX_WAITING;
	} else if(state == EX_WAITING) {
		state = EX_RUNNING;
	}
	process();
}

bool VnExecutor::fast_forward() {
	if(cur_msg.is_read) {
		if(state == EX_WAITING || state == EX_PRINTING) {
			state = EX_RUNNING;
		}
		clear_delays();
		process();
		return true;
	}
	return false;
}

bool VnExecutor::skip_to_unread() {
	if(!cur_msg.is_read) return false;
	while(cur_msg.is_read) {
		if(state == EX_WAITING || state == EX_PRINTING) {
			state = EX_RUNNING;
		} else {
			break;
		}
		clear_delays();
		process();
	}
	return true;
}

void VnExecutor::clear_delays() {
	delay_until = 0;
	bg.trans_start = 0;
	bg.trans_duration = 0;
	for(int i = 0; i < MAX_SPRITES; i++) {
		sprites[i].trans_start = 0;
		sprites[i].trans_duration = 0;
	}
}

u8 VnExecutor::read8() {
	u8 x = sc->buf[cursor];
	cursor++;
	return x;
}

u16 VnExecutor::read16() {
	u16 x = *(u16*) (sc->buf + cursor);
	cursor += 2;
	return x;
}

u32 VnExecutor::read24() {
#ifdef IS_BIG_ENDIAN
	u32 x = *(u32*) (sc->buf + cursor) >> 8;
#else
	u32 x = *(u32*) (sc->buf + cursor) & 0xFFFFFF;
#endif
	cursor += 3;
	return x;
}

u32 VnExecutor::read32() {
	u32 x = *(u32*) (sc->buf + cursor);
	cursor += 4;
	return x;
}

u32 VnExecutor::readres() {
#ifdef IS_BIG_ENDIAN
	u32 x = *(u32*) (sc->buf + cursor) >> 8;
#else
	u32 x = *(u32*) (sc->buf + cursor) & 0xFFFFFF;
#endif
	cursor += 3;
	return x;
}

/*u64 VnExecutor::read64() {
	u64 x = *(u64*) (sc->buf + cursor);
	cursor += 8;
	return x;
}*/

const char* VnExecutor::read_str() {
	const char* str = (char*) (sc->buf + cursor);
	cursor += strlen(str) + 1;
	return str;
}

void VnExecutor::read_trans(Object& into) {
	u16 data = read16();
	into.trans_id = data >> 12;
	into.trans_start = sys_millis();
	into.trans_wait = data & 0x800;
	into.trans_duration = data & 0x7FF;
}
