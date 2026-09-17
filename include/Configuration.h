#pragma once

// Simple INI settings: Data\SKSE\Plugins\<PRODUCT_NAME>.ini
// The file is created with defaults on first run and rewritten only when a value changes.
namespace Config
{
	struct Settings
	{
		bool  enableMenuPage = true;    // register a page in ApocryphaRealm Menu Framework
		bool  enablePrismaUI = true;    // create the PrismaUI web view
		int   toggleKeyScanCode = 0x3D; // DirectInput scan code of the view toggle (0x3D = F3)
		bool  logDebug = false;         // verbose logging
		float exampleSlider = 0.50f;    // demo value shown on the menu page
	};

	void      Load();        // read the INI (creating it with defaults if missing)
	void      Save();        // write current values back
	Settings& Get();
}