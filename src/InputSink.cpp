#include "PCH.h"

#include "Configuration.h"
#include "InputSink.h"
#include "PrismaUI.h"
#include "Settings.h"

namespace
{
	// Modifier state is tracked from the event stream - never polled off the keyboard device.
	// RE::BSInputDeviceManager::GetKeyboard() returns a device class whose base virtuals live in
	// the game, not in CommonLibSSE-NG's static library: touching it breaks the link with
	// "unresolved external symbol RE::BSInputDevice::..." errors.
	struct HeldModifiers
	{
		bool shift = false;
		bool ctrl = false;
		bool alt = false;

		void Update(std::uint32_t a_scanCode, bool a_down)
		{
			using Key = RE::BSKeyboardDevice::Keys::Key;

			switch (a_scanCode) {
			case Key::kLeftShift:
			case Key::kRightShift:
				shift = a_down;
				break;
			case Key::kLeftControl:
			case Key::kRightControl:
				ctrl = a_down;
				break;
			case Key::kLeftAlt:
			case Key::kRightAlt:
				alt = a_down;
				break;
			default:
				break;
			}
		}

		[[nodiscard]] bool Satisfies(const Settings::Hotkey& a_hotkey) const
		{
			return (a_hotkey.shift == 0 || shift) &&
			       (a_hotkey.ctrl == 0 || ctrl) &&
			       (a_hotkey.alt == 0 || alt);
		}
	};

	class ToggleKeySink final : public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override
		{
			if (!a_event) {
				return RE::BSEventNotifyControl::kContinue;
			}

			const Settings::Hotkey hotkey = Settings::ParseHotkey(Config::Get().toggleKey);
			for (auto* event = *a_event; event; event = event->next) {
				if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
					continue;
				}

				const auto* button = static_cast<RE::ButtonEvent*>(event);
				if (button->GetDevice() != RE::INPUT_DEVICE::kKeyboard) {
					continue;
				}

				const auto scanCode = button->GetIDCode();
				const bool down = button->IsDown();

				_held.Update(scanCode, down);

				if (scanCode != hotkey.key || !down || !_held.Satisfies(hotkey)) {
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

	private:
		HeldModifiers _held;
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

		const auto hotkey = Settings::ParseHotkey(Config::Get().toggleKey);
		logger::info("input sink installed (toggle key '{}' -> scan code 0x{:02X}, shift {}, ctrl {}, alt {})",
			Config::Get().toggleKey, hotkey.key, hotkey.shift, hotkey.ctrl, hotkey.alt);
	}
}