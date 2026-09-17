#pragma once

// Settings plumbing: an INI reader and a hotkey parser.
// A trimmed-down take on UselessFenixUtils' SettingsBase (fenix31415, MIT), which does the same
// job with SimpleIni + a "key+key+key" hotkey string - the two parts of that file actually worth
// keeping. SimpleIni is header-only, MIT, and gives us real sections, comments and UTF-8, which
// GetPrivateProfileString does not.
#include <SimpleIni.h>

namespace Settings
{
	// Readers return false when the key is absent, so a caller can tell "missing" from "false".
	class Ini
	{
	public:
		bool Load(std::string_view a_path)
		{
			_path = std::string(a_path);
			_ini.SetUnicode(true);
			return SI_OK == _ini.LoadFile(_path.c_str());
		}

		bool Save() const
		{
			return SI_OK == _ini.SaveFile(_path.c_str());
		}

		[[nodiscard]] bool GetBool(const char* a_section, const char* a_key, bool& a_out) const
		{
			if (!_ini.GetValue(a_section, a_key)) {
				return false;
			}
			a_out = _ini.GetBoolValue(a_section, a_key);
			return true;
		}

		[[nodiscard]] bool GetInt(const char* a_section, const char* a_key, std::int32_t& a_out) const
		{
			if (!_ini.GetValue(a_section, a_key)) {
				return false;
			}
			a_out = static_cast<std::int32_t>(_ini.GetLongValue(a_section, a_key));
			return true;
		}

		[[nodiscard]] bool GetFloat(const char* a_section, const char* a_key, float& a_out) const
		{
			if (!_ini.GetValue(a_section, a_key)) {
				return false;
			}
			a_out = static_cast<float>(_ini.GetDoubleValue(a_section, a_key));
			return true;
		}

		[[nodiscard]] bool GetString(const char* a_section, const char* a_key, std::string& a_out) const
		{
			const char* value = _ini.GetValue(a_section, a_key);
			if (!value) {
				return false;
			}
			a_out = value;
			return true;
		}

		void SetBool(const char* a_section, const char* a_key, bool a_value, const char* a_comment = nullptr)
		{
			_ini.SetBoolValue(a_section, a_key, a_value, a_comment);
		}

		void SetInt(const char* a_section, const char* a_key, std::int32_t a_value, const char* a_comment = nullptr)
		{
			_ini.SetLongValue(a_section, a_key, a_value, a_comment);
		}

		void SetFloat(const char* a_section, const char* a_key, double a_value, const char* a_comment = nullptr)
		{
			_ini.SetDoubleValue(a_section, a_key, a_value, a_comment);
		}

		void SetString(const char* a_section, const char* a_key, const char* a_value, const char* a_comment = nullptr)
		{
			_ini.SetValue(a_section, a_key, a_value, a_comment);
		}

		// SimpleIni has no SetComment in this version: a section comment is written by setting the
		// section with a null key and value.
		void SetSectionComment(const char* a_section, const char* a_comment)
		{
			_ini.SetValue(a_section, nullptr, nullptr, a_comment);
		}

		[[nodiscard]] bool Has(const char* a_section, const char* a_key) const { return _ini.GetValue(a_section, a_key) != nullptr; }

	private:
		CSimpleIniA _ini;
		std::string _path;
	};

	// "61" (F3), "42+61" (Shift+F3) - DirectInput scan codes joined with '+'. Modifier keys are
	// recognised by their own code so the order in the string does not matter.
	//
	// Deliberately no IsPressed() here: polling RE::BSInputDeviceManager::GetKeyboard() pulls
	// NG's BSWin32KeyboardDevice objects out of the static library, and those reference virtuals
	// of BSInputDevice that only the game itself defines -> undefined externals at link time.
	// Track the modifiers from the input event stream instead (see InputSink.cpp).
	struct Hotkey
	{
		std::uint32_t key = 0;
		std::uint32_t shift = 0;
		std::uint32_t ctrl = 0;
		std::uint32_t alt = 0;

		[[nodiscard]] bool IsValid() const { return key != 0; }
		[[nodiscard]] bool NeedsModifiers() const { return shift != 0 || ctrl != 0 || alt != 0; }
	};

	[[nodiscard]] inline Hotkey ParseHotkey(std::string_view a_keys)
	{
		using Key = RE::BSKeyboardDevice::Keys::Key;

		Hotkey      hotkey;
		std::size_t position = 0;
		while (position <= a_keys.size()) {
			auto end = a_keys.find('+', position);
			if (end == std::string_view::npos) {
				end = a_keys.size();
			}

			std::string token{ a_keys.substr(position, end - position) };
			token.erase(0, token.find_first_not_of(" \t"));
			token.erase(token.find_last_not_of(" \t") + 1);

			if (!token.empty()) {
				try {
					const auto code = static_cast<std::uint32_t>(std::stoul(token));
					switch (code) {
					case Key::kLeftShift:
					case Key::kRightShift:
						hotkey.shift = code;
						break;
					case Key::kLeftControl:
					case Key::kRightControl:
						hotkey.ctrl = code;
						break;
					case Key::kLeftAlt:
					case Key::kRightAlt:
						hotkey.alt = code;
						break;
					default:
						hotkey.key = code;
						break;
					}
				} catch (const std::exception&) {
					logger::warn("hotkey: '{}' is not a scan code, ignored", token);
				}
			}

			if (end == a_keys.size()) {
				break;
			}
			position = end + 1;
		}
		return hotkey;
	}
}