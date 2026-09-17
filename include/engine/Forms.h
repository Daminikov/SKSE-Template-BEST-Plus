#pragma once

// Form lookup helpers - the one genuinely useful piece of UselessFenixUtils' Json namespace
// (fenix31415, MIT), rewritten on top of CommonLibSSE-NG's own API.
//
// The point is the "Plugin.esp|0x1A2B" notation: a form is identified by plugin *name* plus local
// ID, so a config file or a JSON payload keeps working when the load order (and therefore the mod
// index) changes between runs.
//
//     auto* perk = Engine::Forms::Lookup("MyMod.esp|0x813");
namespace Engine::Forms
{
	// "MyMod.esp|0x813" -> { 0x01, 0x813 } resolved through the plugin's *current* index
	[[nodiscard]] inline std::optional<RE::FormID> ParseFormID(std::string_view a_text)
	{
		const auto bar = a_text.find('|');
		if (bar == std::string_view::npos) {
			try {
				return static_cast<RE::FormID>(std::stoul(std::string(a_text), nullptr, 16));
			} catch (const std::exception&) {
				return std::nullopt;
			}
		}

		const auto plugin = a_text.substr(0, bar);
		const auto local = a_text.substr(bar + 1);

		const auto* file = RE::TESDataHandler::GetSingleton()->LookupModByName(plugin);
		if (!file) {
			return std::nullopt;
		}

		try {
			const auto localID = static_cast<RE::FormID>(std::stoul(std::string(local), nullptr, 16));
			return (file->GetPartialIndex() << 24) | (localID & 0x00FFFFFF);
		} catch (const std::exception&) {
			return std::nullopt;
		}
	}

	// Resolves "Plugin.esp|0x1A2B" (or a plain "0x1A2B" for a full FormID) to a form.
	[[nodiscard]] inline RE::TESForm* Lookup(std::string_view a_text)
	{
		const auto formID = ParseFormID(a_text);
		if (!formID) {
			logger::warn("form lookup: cannot parse '{}'", a_text);
			return nullptr;
		}
		if (auto* form = RE::TESForm::LookupByID(*formID)) {
			return form;
		}
		logger::warn("form lookup: no form for '{}' (0x{:08X})", a_text, *formID);
		return nullptr;
	}

	template <class T>
	[[nodiscard]] T* Lookup(std::string_view a_text)
	{
		auto* form = Lookup(a_text);
		return form ? form->As<T>() : nullptr;
	}

	// "MyMod.esp" -> current compile index (0xFE | small index for ESLs), for hand-built FormIDs.
	[[nodiscard]] inline std::optional<std::uint32_t> ModIndex(std::string_view a_pluginName)
	{
		const auto* file = RE::TESDataHandler::GetSingleton()->LookupModByName(a_pluginName);
		if (!file) {
			return std::nullopt;
		}
		return file->GetPartialIndex();
	}
}