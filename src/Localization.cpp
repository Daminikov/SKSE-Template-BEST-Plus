#include "PCH.h"

#include <Windows.h>

#include <fstream>
#include <sstream>

#include "Localization.h"
#include "Menu.h"

namespace
{
	constexpr const char* kTranslationDir = "Data\\Interface\\Translations\\";
	constexpr const char* kFallbackLanguage = "english";

	std::unordered_map<std::string, std::string> g_strings;
	std::unordered_map<std::string, std::string> g_fallback;
	std::unordered_set<std::string>              g_missing;
	std::string                                  g_language = kFallbackLanguage;
	std::string                                  g_json = "{}";

	std::filesystem::path FileForLanguage(const std::string& a_language)
	{
		return std::filesystem::path(std::string(kTranslationDir) + PRODUCT_NAME "_" + a_language + ".txt");
	}

	std::string Utf16ToUtf8(std::wstring_view a_text)
	{
		if (a_text.empty()) {
			return {};
		}
		const auto size = ::WideCharToMultiByte(CP_UTF8, 0, a_text.data(), static_cast<int>(a_text.size()), nullptr, 0, nullptr, nullptr);
		std::string out(static_cast<std::size_t>(size), '\0');
		::WideCharToMultiByte(CP_UTF8, 0, a_text.data(), static_cast<int>(a_text.size()), out.data(), size, nullptr, nullptr);
		return out;
	}

	// Reads a "$Key<TAB>text" file. UTF-16 LE (Skyrim's own format) is decoded; UTF-8 is accepted too.
	std::size_t ReadTranslationFile(const std::filesystem::path& a_path, std::unordered_map<std::string, std::string>& a_out)
	{
		std::ifstream file(a_path, std::ios::binary);
		if (!file) {
			return 0;
		}

		std::string raw{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
		std::string text;
		if (raw.size() >= 2 && static_cast<unsigned char>(raw[0]) == 0xFF && static_cast<unsigned char>(raw[1]) == 0xFE) {
			std::wstring wide((raw.size() - 2) / 2, L'\0');
			std::memcpy(wide.data(), raw.data() + 2, wide.size() * sizeof(wchar_t));
			text = Utf16ToUtf8(wide);
		} else {
			if (raw.size() >= 3 && static_cast<unsigned char>(raw[0]) == 0xEF) {
				raw.erase(0, 3);
			}
			text = std::move(raw);
		}

		std::size_t count = 0;
		std::size_t pos = 0;
		while (pos <= text.size()) {
			auto end = text.find('\n', pos);
			if (end == std::string::npos) {
				end = text.size();
			}
			std::string line = text.substr(pos, end - pos);
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			if (!line.empty() && line.front() == '$') {
				if (const auto tab = line.find('\t'); tab != std::string::npos) {
					a_out[line.substr(0, tab)] = line.substr(tab + 1);
					++count;
				}
			}
			if (end == text.size()) {
				break;
			}
			pos = end + 1;
		}
		return count;
	}

	std::string EscapeJson(const std::string& a_text)
	{
		std::string out;
		out.reserve(a_text.size() + 8);
		for (const char c : a_text) {
			switch (c) {
			case '"': out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '\n': out += "\\n"; break;
			case '\r': out += "\\r"; break;
			case '\t': out += "\\t"; break;
			default:
				if (static_cast<unsigned char>(c) < 0x20) {
					char buffer[8]{};
					std::snprintf(buffer, sizeof(buffer), "\\u%04X", c);
					out += buffer;
				} else {
					out += c;
				}
			}
		}
		return out;
	}

	void BuildJson()
	{
		std::string json = "{";
		bool        first = true;
		for (const auto& [key, value] : g_strings) {
			if (!first) {
				json += ',';
			}
			first = false;
			json += '"';
			json += EscapeJson(key);
			json += "\":\"";
			json += EscapeJson(value);
			json += '"';
		}
		json += '}';
		g_json = std::move(json);
	}

	void LoadLanguage(const std::string& a_language)
	{
		g_strings.clear();
		g_fallback.clear();
		g_missing.clear();

		// english is always loaded first so a half-finished translation still shows something
		ReadTranslationFile(FileForLanguage(kFallbackLanguage), g_fallback);
		std::size_t count = 0;
		if (a_language == kFallbackLanguage) {
			count = ReadTranslationFile(FileForLanguage(a_language), g_strings);
		} else {
			count = ReadTranslationFile(FileForLanguage(a_language), g_strings);
			for (const auto& [key, value] : g_fallback) {
				g_strings.try_emplace(key, value);
			}
		}

		BuildJson();
		logger::info("localization: {} ({} string(s) from {}_{}.txt, {} english fallback string(s))",
			a_language, count, PRODUCT_NAME, a_language, g_fallback.size());
	}
}

namespace Loc
{
	void Init()
	{
		g_language = Menu::Language();
		LoadLanguage(g_language);
	}

	bool Refresh()
	{
		const std::string current = Menu::Language();
		if (current == g_language) {
			return false;
		}
		g_language = current;
		LoadLanguage(g_language);
		logger::info("localization: language changed to {}", g_language);
		return true;
	}

	const char* Get(const char* a_key)
	{
		if (!a_key || !*a_key) {
			return "";
		}
		if (const auto it = g_strings.find(a_key); it != g_strings.end()) {
			return it->second.c_str();
		}
		if (g_missing.insert(a_key).second) {
			logger::warn("localization: missing key {} for language {}", a_key, g_language);
		}
		return a_key;
	}

	const char* Language()
	{
		return g_language.c_str();
	}

	const char* Json()
	{
		return g_json.c_str();
	}
}