#pragma once
#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace tenhou {
// FUN_0043f590 / 0043ae93: general-room rule IDs, without rank bits.
struct MatchRule { unsigned type; const wchar_t* name; };
const std::array<MatchRule,12>& GeneralMatchRules();
enum class RoomIntent { None, Create, Join };
struct MatchRequest {
    unsigned type = 0, room = 0;
    RoomIntent intent = RoomIntent::None;
    std::map<std::string,std::string> settings;
};
// Form state only. Membership and queue confirmation belong to LobbySession.
class MatchmakingDraft {
public:
    bool CreateRoom();
    bool JoinRoom(std::string_view number);
    bool SetSettings(std::map<std::string,std::string> settings);
    bool Prepare(unsigned type);
    void Cancel() { request_.reset(); }
    void LeaveRoom();
    void Reset();
    void Confirm(unsigned room,bool owner,std::map<std::string,std::string> settings);
    unsigned room() const { return room_; }
    RoomIntent intent() const { return intent_; }
    const std::map<std::string,std::string>& settings() const { return settings_; }
    const std::optional<MatchRequest>& request() const { return request_; }
private:
    unsigned room_ = 0;
    RoomIntent intent_ = RoomIntent::None;
    std::map<std::string,std::string> settings_;
    std::optional<MatchRequest> request_;
};
}
