#pragma once

// Small math helpers used by gameplay mods, all pure computation (no ID lookups, no crash risk).
// Trimmed-down take on UselessFenixUtils (fenix31415, MIT): the geometry functions that are safe
// to keep, without the archery/projectile machinery that only makes sense inside its own mod.
namespace Engine
{
	[[nodiscard]] constexpr float Clamp(float a_value, float a_min, float a_max)
	{
		return a_value < a_min ? a_min : (a_value > a_max ? a_max : a_value);
	}

	[[nodiscard]] constexpr float Clamp01(float a_value)
	{
		return Clamp(a_value, 0.0f, 1.0f);
	}

	// lerp from A to B by t (t is not clamped: extrapolation is sometimes wanted)
	[[nodiscard]] constexpr float Lerp(float a_t, float a_a, float a_b)
	{
		return a_a + (a_b - a_a) * a_t;
	}

	template <float Min, float Max>
	[[nodiscard]] constexpr float Lerp(float a_t)
	{
		return Min + (Max - Min) * a_t;
	}

	// Angles: x = pitch (-pi/2 up .. pi/2 down), z = yaw (0 => +Y), y is unused by the game.
	[[nodiscard]] inline RE::NiPoint3 AnglesToDirection(const RE::NiPoint3& a_angles)
	{
		const float sinX = std::sin(a_angles.x);
		const float cosX = std::cos(a_angles.x);
		const float sinZ = std::sin(a_angles.z);
		const float cosZ = std::cos(a_angles.z);
		return RE::NiPoint3{ cosX * sinZ, cosX * cosZ, -sinX };
	}

	// A point at distance a_radius along a_angles
	[[nodiscard]] inline RE::NiPoint3 Rotate(float a_radius, const RE::NiPoint3& a_angles)
	{
		return AnglesToDirection(a_angles) * a_radius;
	}

	// Rodriguez rotation of a_point around an arbitrary axis (axis is normalized internally)
	[[nodiscard]] inline RE::NiPoint3 Rotate(const RE::NiPoint3& a_point, float a_angle, const RE::NiPoint3& a_axis)
	{
		const RE::NiPoint3 axis{ a_axis.x / a_axis.Length(), a_axis.y / a_axis.Length(), a_axis.z / a_axis.Length() };
		const float        cosA = std::cos(a_angle);
		const float        sinA = std::sin(a_angle);
		return a_point * cosA + axis.Cross(a_point) * sinA + axis * (axis.Dot(a_point) * (1.0f - cosA));
	}

	// Signed/absolute angle (degrees) from a_refr to a_position, ignoring cells - the stock
	// GetHeadingAngle without an engine call.
	[[nodiscard]] inline float HeadingAngle(RE::TESObjectREFR* a_refr, const RE::NiPoint3& a_position, bool a_absolute = false)
	{
		const float theta = RE::NiFastATan2(a_position.x - a_refr->GetPositionX(), a_position.y - a_refr->GetPositionY());
		float       heading = theta * (180.0f / RE::NI_PI) - a_refr->GetAngleZ();
		if (heading < -180.0f) {
			heading += 360.0f;
		}
		if (heading > 180.0f) {
			heading -= 360.0f;
		}
		return a_absolute ? std::abs(heading) : heading;
	}
}