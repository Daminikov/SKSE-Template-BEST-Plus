#pragma once

// ------------------------------------------------------------------------------
//  Standard library FIRST.
//  CommonLibSSE-NG v8 generates __<Target>Plugin.cpp containing only
//  #include "REL/Relocation.h" and #include "SKSE/SKSE.h", while REL/Version.h
//  uses std::uint16_t / std::array / std::less / std::strong_ordering and "sv"
//  literals without including anything itself. This PCH is what feeds them.
// ------------------------------------------------------------------------------
#include <array>
#include <chrono>
#include <compare>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

namespace logger = SKSE::log;
using namespace std::literals;