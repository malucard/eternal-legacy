#include "n7.hpp"
#include <audio.hpp>
#include <toml.hpp>
#include <curl/curl.h>

namespace n7 {

enum {
	MODS_BROWSE,
	MODS_STARRED,
	MODS_INSTALLED,
	MODS_VIEW
};

struct ModEntry {
	std::string link;
	std::u32string name;
	std::u32string author;
	std::u32string desc;
	SemVer ver;
	std::vector<std::u32string> tags;
	std::vector<std::tuple<std::string, std::u32string, std::vector<std::string>>> components;
	std::optional<TextureRef> icon;
	std::string folder;
	std::optional<std::string> icon_path;
};

static struct ModsMenu {
	TextureRef bg, label;
	TextureRef btn_o, btn_x, btn_square, btn_triangle, btn_leftright;
	std::vector<ModEntry*> loaded_entries;
	int section = MODS_BROWSE;
	int selected = 0;
	ModEntry* viewing;
}* mods_menu = nullptr;

template<typename T>
struct CArrayIter: std::input_iterator_tag {
	T* data;
	CArrayIter operator++() {
		data++;
		return *this;
	}
	T operator*() {
		return *data;
	}
	bool operator==(const CArrayIter& o) {
		return data == o.data;
	}
};

u8* download(const std::string& url, size_t* size) {
	CURLcode res;
	CURL* curl = curl_easy_init();
	if(curl) {
		uintptr_t buf[3];
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_HEADER, 1);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
		curl_easy_perform(curl);
		curl_off_t len;
		curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &len);
		buf[0] = (uintptr_t) malloc(len);
		buf[1] = 0;
		buf[2] = (uintptr_t) len;
		log_info("downloading %s, %lu bytes", url.c_str(), (unsigned long) len);
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 0);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback) [](char* ptr, size_t size, size_t n, void* buf) {
			u8*& bufp = ((u8**) buf)[0];
			uintptr_t& bufsiz = ((uintptr_t*) buf)[1];
			uintptr_t& bufcap = ((uintptr_t*) buf)[2];
			if(bufsiz + size * n > bufcap) {
				bufcap = bufsiz + size * n;
				bufp = (u8*) realloc(bufp, bufcap);
			}
			memcpy(bufp + bufsiz, ptr, size * n);
			bufsiz += size * n;
			return size * n;
		});
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if(res) {
			free((u8*) buf[0]);
			log_err("error downloading %s", url.c_str());
			platform_throw();
		} else {
			log_info("downloaded %s, %lu bytes, res %d", url.c_str(), buf[1], res);
		}
		*size = buf[1];
		return (u8*) buf[0];
	}
	log_err("libcurl error");
	platform_throw();
}

std::string download_string(const std::string& url) {
	size_t size;
	u8* v = download(url, &size);
	return std::string(v, v + size);
}

void load_mod_entries() {
	std::string folder = "file:///home/malucart/code/eternal/out/res/n7/mods_repo/";
	std::istringstream is(download_string(folder + "index.toml"));
	auto repo = toml::parse(is);
	auto items = toml::find<toml::table>(repo, "repo");
	for(auto pair: items) {
		std::istringstream mis(download_string(folder + pair.second.as_string().str + "/index.toml"));
		auto mod = toml::parse(mis);
		auto meta = toml::find<toml::table>(mod, "meta");
		auto components = toml::find<toml::table>(mod, "components");
		std::u32string name = strcvt(meta["name"].as_string().str);
		std::u32string author = strcvt(meta["author"].as_string().str);
		std::u32string desc = strcvt(meta["desc"].as_string().str);
		auto icon_path_iter = meta.find("icon");
		ModEntry* entry = new ModEntry();
		entry->name = name;
		entry->author = author;
		entry->desc = desc;
		entry->folder = folder + pair.second.as_string().str;
		if(icon_path_iter != meta.end()) {
			entry->icon_path = folder + pair.second.as_string().str + '/' + icon_path_iter->second.as_string().str;
		}
		for(auto cpair: components) {
			std::vector<std::string> deps;
			std::vector<toml::value> arr = cpair.second.as_array();
			for(int i = 1; i < arr.size(); i++) {
				deps.push_back(arr[i].as_string());
			}
			entry->components.push_back(std::make_tuple(cpair.first, strcvt(cpair.second.as_array()[0].as_string()), deps));
		}
		mods_menu->loaded_entries.push_back(entry);
	}

	/*mods_menu->loaded_entries.push_back(new ModEntry {
		.link = ""s,
		.name = U"Okuhiko7"s,
		.author = U"okkuman69"s,
		.desc = U"Removes everything in the main story except for Okuhiko."s,
		.ver = {.major = 1, .minor = 0, .patch = 0},
		.icon = g_load_texture(FileDataStream("res/n7/packs/okuhiko7/icon.png"))
	});*/
}

void mods_menu_draw() {
	int sw = g_width(), sh = g_height();
	if(!mods_menu) {
		mods_menu = new ModsMenu();
		mods_menu->bg = g_load_texture(FileDataStream("res/n7/smenubg.png"));
		mods_menu->label = g_load_texture(FileDataStream("res/n7/modsbg.png"));
		mods_menu->btn_o = g_load_texture(FileDataStream("res/buttons/o.png"), TEX_NEAREST);
		mods_menu->btn_x = g_load_texture(FileDataStream("res/buttons/x.png"), TEX_NEAREST);
		mods_menu->btn_square = g_load_texture(FileDataStream("res/buttons/square.png"), TEX_NEAREST);
		mods_menu->btn_triangle = g_load_texture(FileDataStream("res/buttons/triangle.png"), TEX_NEAREST);
		mods_menu->btn_leftright = g_load_texture(FileDataStream("res/buttons/leftright.png"), TEX_NEAREST);
		load_mod_entries();
	}
	g_draw_quad(mods_menu->bg, 0, 0, g_width(), -1, G_CENTER, G_CENTER);
	g_draw_quad(mods_menu->label, 0, 0, g_width(), -1, G_TOP_LEFT, G_TOP_LEFT);
	if(mods_menu->section == MODS_BROWSE) {
		int y = 72_600p;
		for(int i = 0; i < mods_menu->loaded_entries.size(); i++) {
			ModEntry* mod = mods_menu->loaded_entries[i];
			if(mods_menu->selected == i) {
				draw_beam_2(0, y, sw, 40_600p);
				/*if(gui_input.event == GUI_EVENT_OK) {
					mods_menu->section = MODS_VIEW;
					mods_menu->viewing = mod;
					gui_input.event = GUI_EVENT_NONE;
				}
			} else if(gui_input.event == GUI_EVENT_MOUSE) {
				int gy = gui_scale(gui_state.y);
				if(gui_input.mouse_y >= gy && gui_input.mouse_y < gy + gui_scale(40)) {
					gui_input.selected = -i;
					gui_input.event = GUI_EVENT_NONE;
				}*/
			}
			y += g_draw_text(mod->name, 0, y, sw, 24_600p).h;
			y += g_draw_text(mod->desc, 0, y, sw, 16_600p).h;
			/*g_draw_text(mod->name, 0, y, 800_600p, 24_600p);
			g_push_color(gui_theme.shadow_color);
			g_draw_text(U"by "s + mod->author, 0, y, 800_600p, 16_600p, G_TOP_RIGHT, G_TOP_RIGHT);
			g_draw_text(mod->desc, 0, y + 24_600p, 800_600p, 16_600p, G_TOP_RIGHT, G_TOP_RIGHT);
			g_pop_color();
			g_push_color(gui_theme.text_color);
			g_draw_text(mod->desc, 0, y + 24_600p, 800_600p, 16_600p, G_TOP_RIGHT, G_TOP_RIGHT);
			y += 40_600p;*/
		}
	} else if(mods_menu->section == MODS_VIEW) {
		int tw = sw - 400_600p;
		int y = 72_600p;
		y += g_draw_text(mods_menu->viewing->name, 400_600p, y, tw, 24_600p).h;
		y += g_draw_text(mods_menu->viewing->author, 0, y, tw, 18_600p, G_TOP_RIGHT, G_TOP_RIGHT).h;
		y += g_draw_text(mods_menu->viewing->desc, 400_600p, y, tw, 24_600p).h;
		if(!mods_menu->viewing->icon && mods_menu->viewing->icon_path) {
			size_t size;
			u8* v = download(mods_menu->viewing->icon_path.value(), &size);
			try {
				mods_menu->viewing->icon.emplace(g_load_texture(BufDataStream(v, size)));
			} catch(...) {}
		}
		g_draw_quad(mods_menu->viewing->icon.value(), 32_600p, 72_600p, 336_600p, 336_600p);
		tw = 400_600p;
		y = 424_600p + g_draw_text(U"Install"s, 18_600p, y, 400_600p, sh - y).h;
		for(int i = 0; i < mods_menu->viewing->components.size(); i++) {
			auto& t = mods_menu->viewing->components[i];
			std::u32string txt = std::get<1>(t) + U" (not installed)"s;
			y += g_draw_text(txt, 18_600p, y, 400_600p, sh - y).h;
			//gui_button(-i, std::get<1>(t) + U" (not installed)"s, 18, []() {});
		}
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
	/*g_draw_quad(mods_menu->btn_x, 16_600p, -16_600p, 32_600p, 32_600p, G_BOTTOM_LEFT, G_BOTTOM_LEFT);
	g_push_color(0.f, 0.f, 0.f, 1.f);
	g_draw_text("Back"s, 50_600p, -42_600p, 12903, 24_600p, G_TOP_LEFT, G_BOTTOM_LEFT);
	g_pop_color();
	int tw = g_draw_text("Back"s, 48_600p, -44_600p, 12903, 24_600p, G_TOP_LEFT, G_BOTTOM_LEFT);
	g_draw_quad(mods_menu->btn_triangle, 16_600p + tw, -16_600p, 32_600p, 32_600p, G_BOTTOM_LEFT, G_BOTTOM_LEFT);
	g_draw_quad(mods_menu->btn_o, 16_600p + tw, -16_600p, 32_600p, 32_600p, G_BOTTOM_LEFT, G_BOTTOM_LEFT);
	g_push_color(0.f, 0.f, 0.f, 1.f);
	g_draw_text("Install"s, 50_600p + tw, -42_600p, 12903, 24_600p, G_TOP_LEFT, G_BOTTOM_LEFT);
	g_pop_color();
	tw += g_draw_text("Install"s, 48_600p + tw, -44_600p, 12903, 24_600p, G_TOP_LEFT, G_BOTTOM_LEFT);*/
}

bool mods_menu_input(int key, int action) {
	if(action) {
		switch(mods_menu->section) {
			case MODS_BROWSE:
				switch(key) {
					case INPUT_OK:
						ModEntry* mod = mods_menu->loaded_entries[mods_menu->selected];
						mods_menu->section = MODS_VIEW;
						mods_menu->viewing = mod;
						return true;
				}
				break;
		}
	}
	return false;
}

void mods_menu_cursor(int x, int y, bool dragging) {

}

void mods_menu_close() {
	delete mods_menu;
	mods_menu = nullptr;
}

}
