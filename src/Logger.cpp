#include "PCH.h"

#include <spdlog/sinks/rotating_file_sink.h>

#include "Logger.h"

namespace Log
{
	void Init()
	{
		auto dir = SKSE::log::log_directory();
		if (!dir) {
			return;
		}

		// Rotating instead of truncating: the previous log survives the next launch, which matters
		// when the interesting lines came from the run before the one that just crashed.
		const auto path = *dir / (PRODUCT_NAME ".log");
		auto       sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(path.string(), 2 * 1024 * 1024, 3);
		auto       log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
	}

	void SetLevel(spdlog::level::level_enum a_level)
	{
		auto log = spdlog::default_logger();
		if (!log) {
			return;
		}
		log->set_level(a_level);
		log->flush_on(a_level);
	}
}