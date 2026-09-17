#include "PCH.h"

#include "Configuration.h"
#include "Localization.h"
#include "PrismaUI.h"

#include "vendor/PrismaUI_API.h"

namespace
{
	PRISMA_UI_API::IVPrismaUI1* g_api = nullptr;
	PrismaView                  g_view = 0;
	bool                        g_visible = false;

	std::string OptionsJson()
	{
		const auto& settings = Config::Get();
		std::string json = "{";
		json += "\"hud\":";
		json += settings.showHudElement ? "true" : "false";
		json += ",\"sound\":";
		json += settings.enableSound ? "true" : "false";
		json += ",\"volume\":" + std::to_string(settings.volume);
		json += "}";
		return json;
	}

	void OnDomReady(PrismaView a_view)
	{
		logger::info("PrismaUI view ready (handle {})", a_view);
		if (g_api && a_view) {
			// hand the loaded strings to the web view; it re-applies them on every language change
			g_api->InteropCall(a_view, "applyTranslations", Loc::Json());
			g_api->InteropCall(a_view, "updateStatus", Loc::Get("$MyPlugin_View_Connected"));
		}
		Prisma::SyncOptions();
	}

	void OnJsMessage(const char* a_argument)
	{
		// free-form line sent from the view via window.sendDataToSKSE(...)
		logger::info("JS -> plugin: {}", a_argument ? a_argument : "(null)");
	}

	// window.setOption("hud", "1") / ("sound", "0") / ("volume", "0.42")
	void OnJsOption(const char* a_argument)
	{
		const std::string_view argument{ a_argument ? a_argument : "" };
		const auto             separator = argument.find('=');
		if (separator == std::string_view::npos) {
			logger::warn("view option ignored (expected name=value): {}", argument);
			return;
		}

		const std::string name{ argument.substr(0, separator) };
		const std::string value{ argument.substr(separator + 1) };
		auto&             settings = Config::Get();
		const bool        on = value == "1" || value == "true";

		if (name == "hud") {
			settings.showHudElement = on;
		} else if (name == "sound") {
			settings.enableSound = on;
		} else if (name == "volume") {
			try {
				settings.volume = std::clamp(std::stof(value), 0.0f, 1.0f);
			} catch (const std::exception&) {
				logger::warn("view option volume: not a number ({})", value);
				return;
			}
		} else {
			logger::warn("view option: unknown name {}", name);
			return;
		}

		Config::Save();
		logger::info("view option applied: {} = {} (hud {}, sound {}, volume {:.2f})",
			name, value, settings.showHudElement, settings.enableSound, settings.volume);

		// visible proof that the string came from translations/<language>.txt; the vanilla UI
		// sound is what the "Enable sound" option in the view switches on
		RE::SendHUDMessage::ShowHUDMessage(
			Loc::Get("$MyPlugin_Notice_OptionsSaved"),
			settings.enableSound ? "UIMenuOK" : nullptr);
		Prisma::SyncOptions();
	}
}

namespace Prisma
{
	bool Init()
	{
		g_api = static_cast<PRISMA_UI_API::IVPrismaUI1*>(
			PRISMA_UI_API::RequestPluginAPI(PRISMA_UI_API::InterfaceVersion::V1));
		if (!g_api) {
			logger::warn("PrismaUI is not installed - the web view stays disabled");
			return false;
		}

		g_view = g_api->CreateView(PRODUCT_NAME "/index.html", &OnDomReady);
		if (!g_view) {
			logger::error("PrismaUI: CreateView failed - expected PrismaUI/views/" PRODUCT_NAME "/index.html");
			return false;
		}

		g_api->RegisterJSListener(g_view, "sendDataToSKSE", &OnJsMessage);
		g_api->RegisterJSListener(g_view, "setOption", &OnJsOption);
		logger::info("PrismaUI view created (handle {})", g_view);
		return true;
	}

	bool Toggle()
	{
		if (!g_api || !g_view) {
			return false;
		}
		if (g_visible) {
			g_api->Unfocus(g_view);
			g_api->Hide(g_view);
			g_visible = false;
			logger::debug("PrismaUI view hidden");
		} else if (g_api->Focus(g_view)) {
			g_api->Show(g_view);
			g_visible = true;
			logger::debug("PrismaUI view shown");
			SyncOptions();
		}
		return g_visible;
	}

	bool IsAvailable()
	{
		return g_api != nullptr && g_view != 0;
	}

	void SendToView(const char* a_jsCode)
	{
		if (g_api && g_view && a_jsCode) {
			g_api->Invoke(g_view, a_jsCode);
		}
	}

	void Interop(const char* a_function, const char* a_argument)
	{
		if (g_api && g_view && a_function) {
			g_api->InteropCall(g_view, a_function, a_argument ? a_argument : "");
		}
	}

	void SyncOptions()
	{
		if (!g_api || !g_view) {
			return;
		}
		const std::string json = OptionsJson();
		g_api->InteropCall(g_view, "applyOptions", json.c_str());
	}
}