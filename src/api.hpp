#pragma once

extern std::string sys_gfx_vendor;
extern std::string sys_gfx_version;
extern std::string sys_gfx_renderer;

void api_init(int argc, char* argv[]);
void api_load();
void api_resize(int rw, int rh);
void api_frame();
void api_input(int key, bool pressing, bool repeating, float strength);
void api_input(int key, bool pressing, bool repeating);
void api_cursor(float x, float y, bool dragging);
void api_close();
