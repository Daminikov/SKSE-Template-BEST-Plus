#include "PCH.h"

#include <cstdio>
#include <optional>

#include "Configuration.h"
#include "Settings.h"

namespace
{
	constexpr const char* kIniPath = "Data\\SKSE\\Plugins\\" PRODUCT_NAME ".ini";
	constexpr const char* kSection = "General";

	Config::Settings g_settings;
	Settings::Ini    g_ini;

	// Values live in the INI as percentages - easier for a human to edit than 0.42.
	constexpr const char* kMenuPage = "bEnableMenuPage";
	constexpr const char* kPrismaUI = "bEnablePrismaUI";
	constexpr const char* kToggleKey = "sToggleKey";
	constexpr const char* kLogDebug = "bLogDebug";
	constexpr const char* kSlider = "iExampleSliderPercent";
	constexpr const char* kHud = "bShowHudElement";
	constexpr const char* kSound = "bEnableSound";
	constexpr const char* kVolume = "iVolumePercent";

	template <class T>
	T Or(const std::optional<T>& a_value, T a_default)
	{
		return a_value.value_or(a_default);
	}
}

namespace Config
{
	void Load()
	{
		const bool exists = std::filesystem::exists(kIniPath);

		if (!exists) {
			Save();
			logger::info("configuration created: {}", kIniPath);
			return;
		}

		if (!g_ini.Load(kIniPath)) {
			logger::error("configuration: cannot read {}, using defaults", kIniPath);
			return;
		}

		bool value{};

		if (g_ini.GetBool(kSection, kMenuPage, value)) {
			g_settings.enableMenuPage = value;
		}
		if (g_ini.GetBool(kSection, kPrismaUI, value)) {
			g_settings.enablePrismaUI = value;
		}
		if (g_ini.GetBool(kSection, kLogDebug, value)) {
			g_settings.logDebug = value;
		}
		if (g_ini.GetBool(kSection, kHud, value)) {
			g_settings.showHudElement = value;
		}
		if (g_ini.GetBool(kSection, kSound, value)) {
			g_settings.enableSound = value;
		}

		std::string text;
		bool        upgraded = false;
		if (g_ini.GetString(kSection, kToggleKey, text)) {
			g_settings.toggleKey = text;
		} else if (std::int32_t legacy{}; g_ini.GetInt(kSection, "iToggleKeyScanCode", legacy)) {
			// pre-SimpleIni installs stored the code as a number - keep them working
			g_settings.toggleKey = std::to_string(legacy);
			upgraded = true;
		}

		std::int32_t number{};
		if (g_ini.GetInt(kSection, kSlider, number)) {
			g_settings.exampleSlider = static_cast<float>(number) / 100.0f;
		}
		if (g_ini.GetInt(kSection, kVolume, number)) {
			g_settings.volume = static_cast<float>(number) / 100.0f;
		}

		logger::info("configuration loaded: {} (menu {}, prismaUI {}, key '{}', hud {}, sound {}, volume {:.2f})",
			kIniPath, g_settings.enableMenuPage, g_settings.enablePrismaUI, g_settings.toggleKey,
			g_settings.showHudElement, g_settings.enableSound, g_settings.volume);

		// an old-format file is rewritten once, so it stops carrying the retired numeric key
		if (upgraded) {
			Save();
			logger::info("configuration upgraded to the current key format (sToggleKey)");
		}
	}

	void Save()
	{
		g_ini.SetSectionComment(kSection, "; " PRODUCT_NAME " settings - rewritten by the plugin when a value changes");

		g_ini.SetBool(kSection, kMenuPage, g_settings.enableMenuPage);
		g_ini.SetBool(kSection, kPrismaUI, g_settings.enablePrismaUI);
		g_ini.SetString(kSection, kToggleKey, g_settings.toggleKey.c_str(),
			"; DirectInput scan codes joined with '+', e.g. 61 = F3, 42+61 = Shift+F3");
		g_ini.SetBool(kSection, kLogDebug, g_settings.logDebug);
		g_ini.SetInt(kSection, kSlider, static_cast<std::int32_t>(g_settings.exampleSlider * 100.0f));
		g_ini.SetBool(kSection, kHud, g_settings.showHudElement);
		g_ini.SetBool(kSection, kSound, g_settings.enableSound);
		g_ini.SetInt(kSection, kVolume, static_cast<std::int32_t>(g_settings.volume * 100.0f));

		if (!g_ini.Save()) {
			logger::error("configuration: cannot write {}", kIniPath);
		}
	}

	Settings& Get()
	{
		return g_settings;
	}
}