#pragma once

// Engine call helpers - a trimmed-down take on UselessFenixUtils (fenix31415, MIT).
//
// Why this exists: CommonLibSSE-NG wraps a lot of the game, but not everything. Functions the
// headers do not expose are still reachable through the Address Library: every engine function has
// an ID, and `REL::Relocation<Fn>{ REL::ID(id) }` turns it into a callable pointer. That is the
// whole trick behind UselessFenixUtils' `_generic_foo_`, and it is how you call PushActorAway, the
// PlaceAtMe VM function or CombatUtilities when no wrapper exists.
//
// Where IDs come from: a source known to work on the current runtime - another mod's source, an ID
// dump, the versionlib viewer. UselessFenixUtils itself is a catalogue; its Impl namespace has
// PushActorAway = 38858, PlaceAtMe (VM) = 55672, Actor::GetActorValueModifier = 37524,
// BGSImpactManager::GetSingleton = 515123, and so on.
//
// IMPORTANT - an ID cannot be tested at runtime, it is either right or fatal:
// `REL::ID(id).offset()` calls stl::report_and_fail() when the running game's versionlib has no
// such ID: the game aborts with a message box naming the ID. It does NOT return 0, so "probing" an
// ID kills the session. Therefore:
//   * never probe an ID before use;
//   * keep every ID-based call behind a config switch (Config::Settings) so a bad call can be
//     turned off without a rebuild, and test it on the live game;
//   * if the game dies with "Failed to find the id within the address library: NNN", that number is
//     the culprit - replace that one call or drop it;
//   * a wrapper from CommonLibSSE-NG is always preferable to a raw ID: wrappers are checked by the
//     whole ecosystem, IDs are only as good as their source.
namespace Engine
{
	// Call an engine function by address-library ID:
	//
	//     float GetHeadingAngle(RE::TESObjectREFR* a, const RE::NiPoint3& p, bool abs)
	//     {
	//         return Engine::Call<36444, decltype(GetHeadingAngle)>::eval(a, p, abs);
	//     }
	template <std::uint64_t ID, typename T>
	class Call;

	template <std::uint64_t ID, typename R, typename... Args>
	class Call<ID, R(Args...)>
	{
	public:
		static R eval(Args... a_args)
		{
			REL::Relocation<R(Args...)> function{ REL::ID(ID) };
			return function(std::forward<Args>(a_args)...);
		}
	};

	// Read a global the headers do not expose (a time float, a singleton slot):
	//
	//     static REL::Relocation<float*> kGameTime{ REL::ID(517597) };
	//     const float now = *kGameTime;
	template <class T>
	[[nodiscard]] inline T& Static(std::uint64_t a_id)
	{
		REL::Relocation<T*> value{ REL::ID(a_id) };
		return *value;
	}

	// Overwrite bytes at ID+offset - nulling a check, changing a constant, patching a branch:
	//
	//     Engine::WriteBytes<40314, 0x1A>(std::array<std::uint8_t, 5>{ 0x90, 0x90, 0x90, 0x90, 0x90 });
	template <std::uint64_t ID, std::ptrdiff_t Offset = 0>
	void WriteBytes(std::string_view a_bytes)
	{
		REL::safe_write(REL::ID(ID).address() + Offset, a_bytes.data(), a_bytes.size());
	}

	template <std::uint64_t ID, std::ptrdiff_t Offset = 0, std::size_t N>
	void WriteBytes(const std::array<std::uint8_t, N>& a_bytes)
	{
		REL::safe_write(REL::ID(ID).address() + Offset, a_bytes.data(), a_bytes.size());
	}

	// Branch hook with a hand-built stub (xbyak). Prefer CommonLibSSE-NG's own hook_vtable /
	// hook_call when a wrapper exists; reach for this when the target is known only by ID.
	// Enabled by adding "xbyak" to vcpkg.json and #include <xbyak/xbyak.h> before this header.
#if __has_include(<xbyak/xbyak.h>)
#	include <xbyak/xbyak.h>

	template <std::size_t BranchType, std::uint64_t ID, std::ptrdiff_t Offset = 0, bool Call = false>
	auto Hook(Xbyak::CodeGenerator* a_code)
	{
		const auto address = REL::ID(ID).address();
		const auto size = a_code->getSize();
		auto&      trampoline = SKSE::GetTrampoline();
		const auto stub = trampoline.allocate(size);
		std::memcpy(stub, a_code->getCode(), size);
		if constexpr (Call) {
			return trampoline.write_call<BranchType>(address + Offset, reinterpret_cast<std::uintptr_t>(stub));
		} else {
			return trampoline.write_branch<BranchType>(address + Offset, reinterpret_cast<std::uintptr_t>(stub));
		}
	}
#endif
}