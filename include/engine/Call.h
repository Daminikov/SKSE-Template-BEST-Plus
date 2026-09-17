#pragma once

// Engine call helpers - a trimmed-down take on UselessFenixUtils (fenix31415, MIT).
//
// Everything here goes through the Address Library (REL::ID), which is exactly what makes a plugin
// runtime-independent: the ID is resolved in the running game's versionlib (1.7.104 in this build).
// An ID that the current versionlib does not know resolves to a bogus address, so check with
// Engine::HasOffset() (or just never ship an untested call).
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

	// Read a static engine object by ID (a global pointer / float / singleton slot):
	//     static REL::Relocation<float*> kTime{ REL::ID(517597) };  ->  *kTime
	template <class T>
	[[nodiscard]] inline T& Static(std::uint64_t a_id)
	{
		REL::Relocation<T*> value{ REL::ID(a_id) };
		return *value;
	}

	// True when the running game's versionlib knows this ID (0 = unknown -> do not call).
	[[nodiscard]] inline bool HasOffset(std::uint64_t a_id)
	{
		return REL::ID(a_id).offset() != 0;
	}

	// Overwrite bytes at ID+offset (patching a function body, an instruction, a table entry).
	template <std::uint64_t ID, std::ptrdiff_t Offset = 0>
	void WriteBytes(std::string_view a_bytes)
	{
		REL::safe_write(REL::ID(ID).address() + Offset, a_bytes.data(), a_bytes.size());
	}

	// Same, from a byte array so it can live in constexpr data.
	template <std::uint64_t ID, std::ptrdiff_t Offset = 0, std::size_t N>
	void WriteBytes(const std::array<std::uint8_t, N>& a_bytes)
	{
		REL::safe_write(REL::ID(ID).address() + Offset, a_bytes.data(), a_bytes.size());
	}

	// Branch hook with a hand-built stub (xbyak). Only compiled when xbyak is available:
	// add "xbyak" to vcpkg.json and #include <xbyak/xbyak.h> before this header.
#if __has_include(<xbyak/xbyak.h>)
#	include <xbyak/xbyak.h>

	template <std::size_t BranchType, std::uint64_t ID, std::ptrdiff_t Offset = 0, bool Call = false>
	auto Hook(Xbyak::CodeGenerator* a_code)
	{
		const auto  address = REL::ID(ID).address();
		const auto  size = a_code->getSize();
		auto&       trampoline = SKSE::GetTrampoline();
		const auto  stub = trampoline.allocate(size);
		std::memcpy(stub, a_code->getCode(), size);
		if constexpr (Call) {
			return trampoline.write_call<BranchType>(address + Offset, reinterpret_cast<std::uintptr_t>(stub));
		} else {
			return trampoline.write_branch<BranchType>(address + Offset, reinterpret_cast<std::uintptr_t>(stub));
		}
	}
#endif
}