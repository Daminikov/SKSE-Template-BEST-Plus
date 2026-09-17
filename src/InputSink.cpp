#include "PCH.h"

#include "Configuration.h"
#include "InputSink.h"
#include "PrismaUI.h"

namespace
{
	class ToggleKeySink final : public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override
		{
			if (!a_event) {
				return RE::BSEventNotifyControl::kContinue;
			}

			const auto toggleKey = static_cast<std::uint32_t>(Config::Get().toggleKeyScanCode);
			for (auto* event = *a_event; event; event = event->next) {
				if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
					continue;
				}

				const auto* button = static_cast<RE::ButtonEvent*>(event);
				if (button->GetDevice() != RE::INPUT_DEVICE::kKeyboard) {
					continue;
				}
				if (button->GetIDCode() != toggleKey || !button->IsDown()) {
					continue;
				}

				if (Config::Get().enablePrismaUI) {
					if (!Prisma::IsAvailable()) {
						Prisma::Init();
					}
					Prisma::Toggle();
				}
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	ToggleKeySink g_sink;
	bool          g_installed = false;
}

namespace InputSink
{
	void Install()
	{
		if (g_installed) {
			return;
		}
		auto* manager = RE::BSInputDeviceManager::GetSingleton();
		if (!manager) {
			logger::warn("input sink: BSInputDeviceManager is not ready yet");
			return;
		}
		manager->AddEventSink(&g_sink);
		g_installed = true;
		logger::info("input sink installed (toggle scan code 0x{:02X})", Config::Get().toggleKeyScanCode);
	}
}