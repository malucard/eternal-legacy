#pragma once

void input_map_setup(bool swap_a_b);
// strength may be 0-1, but to avoid spamming button down events, values >=0.5 actuate a single down event,
// while lower values are mapped to 0-1 and make button up events whether they're increasing or decreasing
// TODO: make this better and not cut range off, maybe change the "bool pressed" system everywhere to allow a third event
// note: values should maybe stay mapped to 0-2, because having the button press at 1 is useful for button color transitions
// oh! maybe integrate it in a new unified event system! the rigid separation of _input, _cursor, _resize, _close, etc is
// kinda clunky! TODO
void input_map_submit(int ctrl, float strength, bool repeating);
