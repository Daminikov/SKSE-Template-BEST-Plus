#include "PCH.h"

#include "Logger.h"

namespace Log
{
	void Init()
	{
		auto dir = SKSE::log::log_directory();
		if (!dir) {
			return;
		}

		const auto path = *dir / (PRODUCT_NAME ".log");
		auto       sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
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