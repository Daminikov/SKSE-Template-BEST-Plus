#include "PCH.h"

#include "Configuration.h"
#include "InputSink.h"
#include "Localization.h"
#include "Logger.h"
#include "Menu.h"
#include "PrismaUI.h"

namespace
{
	// Demonstrates the engine helpers (include/engine/*) - runs only with verbose logging on, so a
	// normal install stays quiet. Everything here is safe: no engine call by raw ID, only wrappers,
	// maths and a form lookup by "plugin|local id".
	void LogEngineUtilities()
	{
		const auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		// engine/Format.h - RE types print straight into spdlog (NG formats NiPoint3 itself,
		// we added the quaternion/Havok types)
		logger::debug("engine: player at {} facing {:.1f} deg", player->GetPosition(), player->GetAngleZ());

		// engine/Math.h - pure maths, no ID lookups
		logger::debug("engine: clamp01(1.7)={:.2f}, lerp(0.25, 10..20)={:.2f}",
			Engine::Clamp01(1.7f), Engine::Lerp<10.0f, 20.0f>(0.25f));

		// engine/Forms.h - "Plugin.esp|0xLOCALID" survives load-order changes
		if (auto* playerRef = Engine::Forms::Lookup("Skyrim.esm|0x00000014")) {
			const char* name = playerRef->GetName();
			logger::debug("engine: form lookup Skyrim.esm|0x14 -> {} (0x{:08X})",
				name ? name : "<unnamed>", playerRef->GetFormID());
		}

		// engine/Format.h hashing - compare hot strings by hash instead of strcmp
		constexpr auto kStagger = "staggerStart"_hl;
		logger::debug("engine: hash of \"staggerStart\" = 0x{:08X} (matches the literal: {})",
			kStagger, kStagger == Engine::HashLowercase("staggerStart", 12));

		// Settings.h - the hotkey string from the INI parsed into scan codes
		const auto hotkey = Settings::ParseHotkey(Config::Get().toggleKey);
		logger::debug("engine: hotkey '{}' -> key 0x{:02X}, shift 0x{:02X}, ctrl 0x{:02X}, alt 0x{:02X}",
			Config::Get().toggleKey, hotkey.key, hotkey.shift, hotkey.ctrl, hotkey.alt);
	}

	void OnSkskMessage(SKSE::MessagingInterface::Message* a_message)
	{
		switch (a_message->type) {
		case SKSE::MessagingInterface::kDataLoaded:
		{
			Config::Load();
			Loc::Init();
			Log::SetLevel(Config::Get().logDebug ? spdlog::level::debug : spdlog::level::info);

			if (Config::Get().enableMenuPage) {
				Menu::Init();
			}
			if (Config::Get().enablePrismaUI) {
				Prisma::Init();
			}
			InputSink::Install();

			if (Config::Get().logDebug) {
				LogEngineUtilities();
			}
			break;
		}
		case SKSE::MessagingInterface::kPostLoadGame:
			// a save is in memory now: the engine demo finally shows real player data
			// instead of the pre-load default position
			if (Config::Get().logDebug) {
				LogEngineUtilities();
			}
			break;
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