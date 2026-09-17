#pragma once

// Simple INI settings: Data\SKSE\Plugins\<PRODUCT_NAME>.ini
// Created with defaults on first run, rewritten when a value changes. Reading and writing go
// through SimpleIni (real sections, comments, UTF-8) via Settings::Ini; the hotkey is a
// "61" / "42+61" string of DirectInput scan codes.
// The last three values are mirrored in the PrismaUI web view (view/index.html): the view sends
// "name=value" strings, PrismaUI.cpp applies them here and saves.
namespace Config
{
	struct Settings
	{
		// preferences - the menu page (and the web view for the first two)
		bool        enableMenuPage = true;  // register a page in ApocryphaRealm Menu Framework
		bool        enablePrismaUI = true;  // create the PrismaUI web view
		std::string toggleKey = "61";       // "61" = F3, "42+61" = Shift+F3
		bool        logDebug = false;       // verbose logging
		float       exampleSlider = 0.50f;  // demo value shown on the menu page

		// options owned by the test window in the web view
		bool  showHudElement = true;
		bool  enableSound = false;
		float volume = 0.50f;
	};

	void      Load();        // read the INI (creating it with defaults if missing)
	void      Save();        // write current values back
	Settings& Get();
}