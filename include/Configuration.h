#pragma once

// Simple INI settings: Data\SKSE\Plugins\<PRODUCT_NAME>.ini
// Created with defaults on first run, rewritten when a value changes.
// The last three are mirrored in the PrismaUI web view (view/index.html): the view sends
// "name=value" strings, PrismaUI.cpp applies them here and saves.
namespace Config
{
	struct Settings
	{
		// preferences - menu page (and the web view for the first two)
		bool  enableMenuPage = true;    // register a page in ApocryphaRealm Menu Framework
		bool  enablePrismaUI = true;    // create the PrismaUI web view
		int   toggleKeyScanCode = 0x3D; // DirectInput scan code of the view toggle (0x3D = F3)
		bool  logDebug = false;         // verbose logging
		float exampleSlider = 0.50f;    // demo value shown on the menu page

		// options owned by the test window in the web view
		bool  showHudElement = true;
		bool  enableSound = false;
		float volume = 0.50f;
	};

	void      Load();        // read the INI (creating it with defaults if missing)
	void      Save();        // write current values back
	Settings& Get();
}