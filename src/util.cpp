#include "util.hpp"
#include <codecvt>
#include <locale>
#include <chrono>

bool starts_with(const std::string& s, const std::string& o, int at) {
	return s.size() >= o.size() + at && !memcmp(s.c_str() + at, o.c_str(), o.size());
}

bool starts_with(const std::u32string& s, const std::u32string& o, int at) {
	return s.size() >= o.size() + at && !memcmp(s.c_str() + at, o.c_str(), o.size());
}

bool ends_with(const std::string& s, const std::string& o, int at) {
	return s.size() >= o.size() + at && !memcmp(s.c_str() + s.size() - o.size() - at, o.c_str(), o.size());
}

std::u32string strcvt(std::string s) {
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
	return converter.from_bytes(s);
}

std::string ustr_to_str(std::u32string s) {
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
	return converter.to_bytes(s);
}

#if !defined(PLATFORM_PSP) && !defined(PLATFORM_PC)
i64 init_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

u32 sys_millis() {
	return (std::chrono::high_resolution_clock::now().time_since_epoch().count() - init_time) / 1000000;// + ((1L << 32) - 10000); // to simulate an overflow in 5 seconds
}

float sys_secs() {
	return (std::chrono::high_resolution_clock::now().time_since_epoch().count() - init_time) / 1000 / 1000000.f;
}
#endif
