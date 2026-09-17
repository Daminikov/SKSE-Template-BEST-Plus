#pragma once

// Menu integration.
//
// We intentionally talk to the *ApocryphaRealm Menu Framework* (AMF), not to SKSE Menu
// Framework: AMF is an original framework that publishes the same consumer surface and
// answers the stock module/file probes, so the public SKSE Menu Framework consumer header
// (vendored in include/vendor/MenuFramework) registers and draws against AMF unchanged.
// The AMF-native C API (include/vendor/AMF/API.h) is used on top of that for the extras:
// version probe, language of the framework's own UI, opening the menu on our page.
namespace Menu
{
	bool        Init();          // resolve the framework + register our page; false when no framework is installed
	void        Render();        // page body, runs inside the framework's ImGui frame
	const char* FrameworkName(); // "ApocryphaRealm Menu Framework" / "SKSE Menu Framework" / "none"
	const char* FrameworkVersion();
	const char* Language();      // AMF_GetLanguage(), "" when unavailable
	bool        Open();          // AMF_OpenMenu(<our mod name>)
}