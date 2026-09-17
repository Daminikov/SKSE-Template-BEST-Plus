#pragma once

// Per-mod log file: Documents\My Games\Skyrim Special Edition\SKSE\<PRODUCT_NAME>.log
// (the same place SKSE and every other plugin writes). Set up once from SKSEPluginLoad
// and then just use logger::info(...) anywhere.
namespace Log
{
	void Init();
	void SetLevel(spdlog::level::level_enum a_level);
}