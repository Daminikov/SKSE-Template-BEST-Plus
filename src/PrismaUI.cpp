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

	void OnDomReady(PrismaView a_view)
	{
		logger::info("PrismaUI view ready (handle {})", a_view);
		if (g_api && a_view) {
			// hand the loaded strings to the web view; it re-applies them on every language change
			g_api->InteropCall(a_view, "applyTranslations", Loc::Json());
			g_api->InteropCall(a_view, "updateStatus", Loc::Get("$MyPlugin_View_Connected"));
		}
	}

	void OnJsMessage(const char* a_argument)
	{
		// message sent from view/index.html via window.sendDataToSKSE(...)
		logger::info("JS -> plugin: {}", a_argument ? a_argument : "(null)");
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
}