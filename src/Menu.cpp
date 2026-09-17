#include "PCH.h"

#include <Windows.h>

#include <cstdio>

#include "Configuration.h"
#include "Localization.h"
#include "Logger.h"
#include "Menu.h"
#include "PrismaUI.h"

#include "vendor/AMF/API.h"
#include "vendor/MenuFramework/SKSEMenuFramework.h"

namespace
{
	HMODULE g_framework = nullptr;

	using FnGetVersionString = decltype(&AMF_GetVersionString);
	using FnGetLanguage = decltype(&AMF_GetLanguage);
	using FnOpenMenu = decltype(&AMF_OpenMenu);
	using FnSetPageVisible = decltype(&AMF_SetPageVisible);
	using FnRegisterPage = decltype(&AMF_RegisterPage);

	FnGetVersionString g_getVersionString = nullptr;
	FnGetLanguage      g_getLanguage = nullptr;
	FnOpenMenu         g_openMenu = nullptr;
	FnSetPageVisible   g_setPageVisible = nullptr;
	FnRegisterPage     g_registerPage = nullptr;

	bool g_pageRegistered = false;

	// Section shown in the framework's mod list, and the tab inside it.
	constexpr const char* kSectionName = BEAUTIFUL_NAME;
	constexpr const char* kPageName = "Settings";

	template <class T>
	T Resolve(HMODULE a_module, const char* a_name)
	{
		return a_module ? reinterpret_cast<T>(::GetProcAddress(a_module, a_name)) : nullptr;
	}

	HMODULE FindFrameworkModule()
	{
		// AMF ships as "!ApocryphaMenuFramework.dll" in this load order (the '!' sorts it early);
		// the plain name and the stock SMF name are probed too so the same code works either way.
		const wchar_t* names[] = { L"ApocryphaMenuFramework", L"!ApocryphaMenuFramework", L"SKSEMenuFramework" };
		for (const auto* name : names) {
			if (auto module = ::GetModuleHandleW(name)) {
				return module;
			}
		}
		return nullptr;
	}

	bool FrameworkPresent()
	{
		// The stock consumer header probes for Data/SKSE/Plugins/SKSEMenuFramework.dll; AMF answers
		// that file probe (and the module-name lookup), which is why this returns true on AMF.
		return SKSEMenuFramework::IsInstalled();
	}
}

namespace Menu
{
	bool Init()
	{
		g_framework = FindFrameworkModule();
		if (g_framework) {
			g_getVersionString = Resolve<FnGetVersionString>(g_framework, "AMF_GetVersionString");
			g_getLanguage = Resolve<FnGetLanguage>(g_framework, "AMF_GetLanguage");
			g_openMenu = Resolve<FnOpenMenu>(g_framework, "AMF_OpenMenu");
			g_setPageVisible = Resolve<FnSetPageVisible>(g_framework, "AMF_SetPageVisible");
			g_registerPage = Resolve<FnRegisterPage>(g_framework, "AMF_RegisterPage");
		}

		if (!FrameworkPresent()) {
			logger::warn("no menu framework installed - the settings page stays unregistered");
			return false;
		}
		if (g_pageRegistered) {
			return true;
		}

		// Preferred: the framework's own C API. The stock SMF-compatible call below prepends an
		// internal section key, so calling AddSectionItem without SetSection() first sends
		// "/<mod>/<page>" and a framework reading the section as a name rejects it.
		if (g_registerPage && g_registerPage(kSectionName, kPageName, &Menu::Render)) {
			g_pageRegistered = true;
			logger::info("menu page registered: {}/{} (native AMF, framework: {} {})",
				kSectionName, kPageName, FrameworkName(), FrameworkVersion());
			return true;
		}

		SKSEMenuFramework::SetSection(kSectionName);
		SKSEMenuFramework::AddSectionItem(kPageName, &Menu::Render);
		g_pageRegistered = true;
		logger::info("menu page registered: {}/{} (SMF-compat, framework: {} {})",
			kSectionName, kPageName, FrameworkName(), FrameworkVersion());
		return true;
	}

	void Render()
	{
		// the framework's language can change while the game runs - reload when it does
		if (Loc::Refresh()) {
			Prisma::Interop("applyTranslations", Loc::Json());
		}

		auto& settings = Config::Get();

		ImGuiMCP::Text("%s %s", BEAUTIFUL_NAME, MOD_VERSION);
		ImGuiMCP::TextDisabled("%s: %s %s", Loc::Get("$MyPlugin_Settings_Framework"), FrameworkName(), FrameworkVersion());
		ImGuiMCP::Separator();

		if (ImGuiMCP::Checkbox(Loc::Get("$MyPlugin_Settings_EnablePrismaUI"), &settings.enablePrismaUI)) {
			Config::Save();
		}
		if (ImGuiMCP::Checkbox(Loc::Get("$MyPlugin_Settings_RegisterPage"), &settings.enableMenuPage)) {
			Config::Save();
		}
		if (ImGuiMCP::Checkbox(Loc::Get("$MyPlugin_Settings_DebugLogging"), &settings.logDebug)) {
			Log::SetLevel(settings.logDebug ? spdlog::level::debug : spdlog::level::info);
			Config::Save();
		}
		if (ImGuiMCP::SliderFloat(Loc::Get("$MyPlugin_Settings_ExampleSlider"), &settings.exampleSlider, 0.0f, 1.0f)) {
			Config::Save();
		}

		ImGuiMCP::Separator();
		ImGuiMCP::TextDisabled("%s: %s", Loc::Get("$MyPlugin_Settings_Language"), Loc::Language());
		// the translated string is an ARGUMENT here, never the format string: a translation must
		// not be able to smuggle a printf conversion into the framework's variadic text call
		ImGuiMCP::TextDisabled("%s: 0x%02X", Loc::Get("$MyPlugin_Settings_HotkeyHint"), settings.toggleKeyScanCode);
	}

	const char* FrameworkName()
	{
		if (g_framework) {
			return "ApocryphaRealm Menu Framework";
		}
		return FrameworkPresent() ? "SKSE Menu Framework" : "none";
	}

	const char* FrameworkVersion()
	{
		if (g_getVersionString) {
			if (const auto* version = g_getVersionString()) {
				return version;
			}
		}
		static char buffer[16]{};
		std::snprintf(buffer, sizeof(buffer), "%.2f", SKSEMenuFramework::GetMenuFrameworkVersion());
		return buffer;
	}

	const char* Language()
	{
		if (g_getLanguage) {
			if (const auto* language = g_getLanguage()) {
				return language;
			}
		}
		return "english";
	}

	bool Open()
	{
		if (g_openMenu) {
			return g_openMenu(BEAUTIFUL_NAME);
		}
		return false;
	}
}