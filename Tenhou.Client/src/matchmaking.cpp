#include "matchmaking.hpp"
#include <algorithm>

namespace tenhou {
    bool MatchmakingDraft::CreateRoom(const ServerRule &rule) {
        if (request_ || room_ || intent_ != RoomIntent::None) return false;
        // Only the service allocates room numbers. This is an unnumbered form.
        room_ = 0;
        intent_ = RoomIntent::Create;
        settings_ = {{"rule", rule.id}};
        for (const auto &field: rule.fields) settings_[field.key] = field.initial;
        return true;
    }

    bool MatchmakingDraft::JoinRoom(std::string_view number) {
        if (request_ || room_ || number.size() != 4 || number[0] < '1' || number[0] > '9' ||
            !std::all_of(number.begin(), number.end(), [](char c) { return c >= '0' && c <= '9'; }))
            return false;
        room_ = 0;
        for (char c: number) room_ = room_ * 10 + static_cast<unsigned>(c - '0');
        intent_ = RoomIntent::Join;
        settings_.clear();
        return true;
    }

    bool MatchmakingDraft::SetSettings(std::map<std::string, std::string> settings) {
        if (request_ || intent_ != RoomIntent::Create) return false;
        // Preserve a complete room draft; detailed domain validation remains in the editor/server.
        for (const auto &entry: settings_) if (!settings.count(entry.first)) return false;
        settings_ = std::move(settings);
        return true;
    }

    bool MatchmakingDraft::Prepare(unsigned type) {
        if (request_) return false;
        // Selection IDs and display flags are both supplied by the server.
        if (intent_ != RoomIntent::None) type = 0;
        request_ = MatchRequest{type, room_, intent_, settings_};
        return true;
    }

    void MatchmakingDraft::LeaveRoom() {
        if (!request_) {
            room_ = 0;
            intent_ = RoomIntent::None;
            settings_.clear();
        }
    }

    void MatchmakingDraft::Reset() {
        Cancel();
        LeaveRoom();
    }

    void MatchmakingDraft::Confirm(unsigned room, bool owner, std::map<std::string, std::string> settings) {
        request_.reset();
        room_ = room;
        intent_ = room ? (owner ? RoomIntent::Create : RoomIntent::Join) : RoomIntent::None;
        settings_ = std::move(settings);
    }
}
