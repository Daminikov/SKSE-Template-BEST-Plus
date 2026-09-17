#include "PCH.h"

#include <Windows.h>

#include <cstdio>

#include "Configuration.h"

namespace
{
	constexpr const char* kIniPath = "Data\\SKSE\\Plugins\\" PRODUCT_NAME ".ini";
	constexpr const char* kSection = "General";

	Config::Settings g_settings;

	int ReadInt(const char* a_key, int a_default)
	{
		return static_cast<int>(::GetPrivateProfileIntA(kSection, a_key, a_default, kIniPath));
	}

	bool ReadBool(const char* a_key, bool a_default)
	{
		return ReadInt(a_key, a_default ? 1 : 0) != 0;
	}

	void WriteInt(const char* a_key, int a_value)
	{
		char buffer[32]{};
		std::snprintf(buffer, sizeof(buffer), "%d", a_value);
		::WritePrivateProfileStringA(kSection, a_key, buffer, kIniPath);
	}
}

namespace Config
{
	void Load()
	{
		std::error_code ec;
		const bool        exists = std::filesystem::exists(kIniPath, ec);

		g_settings.enableMenuPage = ReadBool("bEnableMenuPage", true);
		g_settings.enablePrismaUI = ReadBool("bEnablePrismaUI", true);
		g_settings.toggleKeyScanCode = ReadInt("iToggleKeyScanCode", 0x3D);
		g_settings.logDebug = ReadBool("bLogDebug", false);

		// floats live in the INI as percentages - easier for a human to edit
		g_settings.exampleSlider = static_cast<float>(ReadInt("iExampleSliderPercent", 50)) / 100.0f;

		g_settings.showHudElement = ReadBool("bShowHudElement", true);
		g_settings.enableSound = ReadBool("bEnableSound", false);
		g_settings.volume = static_cast<float>(ReadInt("iVolumePercent", 50)) / 100.0f;

		if (!exists) {
			Save();
			logger::info("configuration created: {}", kIniPath);
		} else {
			logger::info("configuration loaded: {} (menu {}, prismaUI {}, hud {}, sound {}, volume {:.2f})",
				kIniPath, g_settings.enableMenuPage, g_settings.enablePrismaUI,
				g_settings.showHudElement, g_settings.enableSound, g_settings.volume);
		}
	}

	void Save()
	{
		WriteInt("bEnableMenuPage", g_settings.enableMenuPage ? 1 : 0);
		WriteInt("bEnablePrismaUI", g_settings.enablePrismaUI ? 1 : 0);
		WriteInt("iToggleKeyScanCode", g_settings.toggleKeyScanCode);
		WriteInt("bLogDebug", g_settings.logDebug ? 1 : 0);
		WriteInt("iExampleSliderPercent", static_cast<int>(g_settings.exampleSlider * 100.0f));
		WriteInt("bShowHudElement", g_settings.showHudElement ? 1 : 0);
		WriteInt("bEnableSound", g_settings.enableSound ? 1 : 0);
		WriteInt("iVolumePercent", static_cast<int>(g_settings.volume * 100.0f));
	}

	Settings& Get()
	{
		return g_settings;
	}
}