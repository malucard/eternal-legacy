#include "input.hpp"
#include <api.hpp>

int mappings[CTRL_COUNT];

void input_map_setup(bool swap_a_b) {
	mappings[CTRL_UP] = INPUT_UP;
	mappings[CTRL_DOWN] = INPUT_DOWN;
	mappings[CTRL_LEFT] = INPUT_LEFT;
	mappings[CTRL_RIGHT] = INPUT_RIGHT;
	mappings[CTRL_STICK_L_UP] = INPUT_UP;
	mappings[CTRL_STICK_L_DOWN] = INPUT_DOWN;
	mappings[CTRL_STICK_L_LEFT] = INPUT_LEFT;
	mappings[CTRL_STICK_L_RIGHT] = INPUT_RIGHT;
	mappings[CTRL_STICK_R_UP] = INPUT_SCROLL_UP;
	mappings[CTRL_STICK_R_DOWN] = INPUT_SCROLL_DOWN;
	mappings[CTRL_STICK_R_LEFT] = INPUT_SCROLL_LEFT;
	mappings[CTRL_STICK_R_RIGHT] = INPUT_SCROLL_RIGHT;
	mappings[CTRL_A] = swap_a_b? INPUT_BACK: INPUT_OK;
	mappings[CTRL_B] = swap_a_b? INPUT_OK: INPUT_BACK;
	mappings[CTRL_X] = INPUT_MENU;
	mappings[CTRL_Y] = INPUT_MORE;
	mappings[CTRL_SELECT] = INPUT_SYS_MENU;
	mappings[CTRL_START] = INPUT_SKIP_INSTANT;
	mappings[CTRL_L] = INPUT_AUTO;
	mappings[CTRL_R] = INPUT_SKIP;
	mappings[CTRL_L1] = INPUT_HIDE;
	mappings[CTRL_R1] = INPUT_AUTO;
	mappings[CTRL_L2] = INPUT_OK;
	mappings[CTRL_R2] = INPUT_SKIP;
	mappings[CTRL_L3] = INPUT_OK;
	mappings[CTRL_R3] = INPUT_SKIP;
	mappings[CTRL_L4] = INPUT_QUICK_LOAD;
	mappings[CTRL_R4] = INPUT_QUICK_SAVE;
	mappings[CTRL_L5] = INPUT_REWIND;
	mappings[CTRL_R5] = 0;
}

// don't require the backends to do this if they don't need it
static int a = (input_map_setup(false), 0);

float strengths[CTRL_COUNT];

void input_map_submit(int ctrl, float strength, bool repeating) {
	float prev = strengths[ctrl];
	strengths[ctrl] = strength;
	if(strength < 0.f) {
		switch(ctrl) {
			case CTRL_STICK_L_RIGHT: ctrl = CTRL_STICK_L_LEFT; strength = -strength; break;
			case CTRL_STICK_R_RIGHT: ctrl = CTRL_STICK_R_LEFT; strength = -strength; break;
			case CTRL_R2: ctrl = CTRL_L2; strength = -strength; break;
		}
	}
	if(strength >= 0.5f) {
		if(prev < 0.5f) {
			api_input(mappings[ctrl], true, repeating, strength);
		}
	} else if(strength != prev) {
		api_input(mappings[ctrl], false, repeating, strength);
	}
}
