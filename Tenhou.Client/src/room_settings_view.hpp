#pragma once
#include <windows.h>
#include <map>
#include <optional>
#include <string>

namespace tenhou {
    std::optional<std::map<std::string, std::string> > EditRoomSettings(
        HWND owner, const std::map<std::string, std::string> &settings);
}
