#pragma once
#include <string>
#include <functional>

struct GuiElement {
	int positioning;
	std::function<void(GuiElement*, int x, int y)> draw;
};

struct GuiLayout {
	std::vector<GuiElement> children;
};
