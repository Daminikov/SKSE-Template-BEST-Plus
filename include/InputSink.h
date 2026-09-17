#pragma once

// Minimal SKSE input sink - used for the PrismaUI toggle hotkey (Config::Settings::toggleKeyScanCode).
namespace InputSink
{
	void Install();   // idempotent; requires the input device manager to exist (kInputLoaded / kDataLoaded)
}