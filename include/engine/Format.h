#pragma once

// Vector/quaternion printing for logs, plus compile-time string hashing.
// Trimmed-down take on UselessFenixUtils (fenix31415, MIT).
//
// With these formatters a plain spdlog call just works:
//     logger::debug("actor at {} facing {}", refr->GetPosition(), refr->GetAngle());
#include <fmt/format.h>

// NOTE: CommonLibSSE-NG already ships fmt::formatter for RE::NiPoint3 (with an 'f'/'e'
// presentation), RE::NiColor(A), BSFixedString and the enum-ish types - do not redefine those.
// What is missing there, and is added here: quaternions and the Havok maths types.
template <>
struct fmt::formatter<RE::NiQuaternion> : fmt::formatter<std::string_view>
{
	auto format(const RE::NiQuaternion& a_value, format_context& a_ctx) const
	{
		char buffer[96]{};
		std::snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f, %.3f)", a_value.x, a_value.y, a_value.z, a_value.w);
		return fmt::format_to(a_ctx.out(), "{}", buffer);
	}
};

template <>
struct fmt::formatter<RE::hkVector4> : fmt::formatter<std::string_view>
{
	auto format(const RE::hkVector4& a_value, format_context& a_ctx) const
	{
		char buffer[80]{};
		std::snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f)",
			a_value.quad.m128_f32[0], a_value.quad.m128_f32[1], a_value.quad.m128_f32[2]);
		return fmt::format_to(a_ctx.out(), "{}", buffer);
	}
};

template <>
struct fmt::formatter<RE::hkQuaternion> : fmt::formatter<std::string_view>
{
	auto format(const RE::hkQuaternion& a_value, format_context& a_ctx) const
	{
		char buffer[96]{};
		std::snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f, %.3f)",
			a_value.vec.quad.m128_f32[0], a_value.vec.quad.m128_f32[1],
			a_value.vec.quad.m128_f32[2], a_value.vec.quad.m128_f32[3]);
		return fmt::format_to(a_ctx.out(), "{}", buffer);
	}
};

template <>
struct fmt::formatter<RE::hkQsTransform> : fmt::formatter<std::string_view>
{
	auto format(const RE::hkQsTransform& a_value, format_context& a_ctx) const
	{
		char buffer[80]{};
		std::snprintf(buffer, sizeof(buffer), "(%.3f, %.3f, %.3f)",
			a_value.translation.quad.m128_f32[0], a_value.translation.quad.m128_f32[1], a_value.translation.quad.m128_f32[2]);
		return fmt::format_to(a_ctx.out(), "{}", buffer);
	}
};

// ---------------------------------------------------------------------------------------------
// Compile-time string hashing (djb2, the same algorithm Skyrim uses for EditorIDs and event
// names). Cheap switch/compare instead of strcmp in hot paths:
//
//     switch (hash_lowercase(name, strlen(name))) {
//     case "IdleStop"_hl: ... break;
//     }
// ---------------------------------------------------------------------------------------------
namespace Engine
{
	constexpr std::uint32_t Hash(const char* a_data, std::size_t a_size) noexcept
	{
		std::uint32_t hash = 5381;
		for (const char* c = a_data; c < a_data + a_size; ++c) {
			hash = ((hash << 5) + hash) + static_cast<unsigned char>(*c);
		}
		return hash;
	}

	constexpr char ToLower(char a_char)
	{
		return (a_char >= 'A' && a_char <= 'Z') ? static_cast<char>(a_char + 32) : a_char;
	}

	constexpr std::uint32_t HashLowercase(const char* a_data, std::size_t a_size) noexcept
	{
		std::uint32_t hash = 5381;
		for (const char* c = a_data; c < a_data + a_size; ++c) {
			hash = ((hash << 5) + hash) + static_cast<unsigned char>(ToLower(*c));
		}
		return hash;
	}
}

constexpr std::uint32_t operator""_h(const char* a_string, std::size_t a_size) noexcept
{
	return Engine::Hash(a_string, a_size);
}

constexpr std::uint32_t operator""_hl(const char* a_string, std::size_t a_size) noexcept
{
	return Engine::HashLowercase(a_string, a_size);
}