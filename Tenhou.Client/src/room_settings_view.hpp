#pragma once
#include <windows.h>
#include <map>
#include <optional>
#include <string>
#include "matchmaking.hpp"

namespace tenhou {
    std::optional<std::map<std::string, std::string> > EditRoomSettings(
        HWND owner, const std::map<std::string, std::string> &settings, const std::map<std::string, ServerRule> &rules);
}
