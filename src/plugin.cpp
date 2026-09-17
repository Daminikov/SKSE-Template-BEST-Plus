#include "PCH.h"

#include "Configuration.h"
#include "InputSink.h"
#include "Logger.h"
#include "Menu.h"
#include "PrismaUI.h"

namespace
{
	void OnSkskMessage(SKSE::MessagingInterface::Message* a_message)
	{
		switch (a_message->type) {
		case SKSE::MessagingInterface::kDataLoaded:
		{
			Config::Load();
			Log::SetLevel(Config::Get().logDebug ? spdlog::level::debug : spdlog::level::info);

			if (Config::Get().enableMenuPage) {
				Menu::Init();
			}
			if (Config::Get().enablePrismaUI) {
				Prisma::Init();
			}
			InputSink::Install();
			break;
		}
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);

	Log::Init();
	logger::info("{} {} | game {} | CommonLibSSE-NG", BEAUTIFUL_NAME, MOD_VERSION,
		a_skse->RuntimeVersion().string());

	auto* messaging = SKSE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(OnSkskMessage)) {
		logger::critical("SKSE messaging interface is unavailable");
		return false;
	}

	return true;
}