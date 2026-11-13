#pragma once
#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tenhou {
    struct RuleChoice {
        std::string value, label;
    };
    struct RuleField {
        std::string key, label, initial, minimum, maximum;
        unsigned step = 1;
        std::vector<RuleChoice> choices;
    };
    struct ServerRule {
        std::string id, name, presentation;
        std::vector<RuleField> fields;
    };
    struct ServerQueue {
        unsigned id = 0, type = 0;
        std::string name, rule;
    };

    enum class RoomIntent { None, Create, Join };

    struct MatchRequest {
        unsigned type = 0, room = 0;
        RoomIntent intent = RoomIntent::None;
        std::map<std::string, std::string> settings;
    };

    // Form state only. Membership and queue confirmation belong to LobbySession.
    class MatchmakingDraft {
    public:
        bool CreateRoom(const ServerRule &rule);

        bool JoinRoom(std::string_view number);

        bool SetSettings(std::map<std::string, std::string> settings);

        bool Prepare(unsigned type);

        void Cancel() { request_.reset(); }

        void LeaveRoom();

        void Reset();

        void Confirm(unsigned room, bool owner, std::map<std::string, std::string> settings);

        unsigned room() const { return room_; }
        RoomIntent intent() const { return intent_; }
        const std::map<std::string, std::string> &settings() const { return settings_; }
        const std::optional<MatchRequest> &request() const { return request_; }

    private:
        unsigned room_ = 0;
        RoomIntent intent_ = RoomIntent::None;
        std::map<std::string, std::string> settings_;
        std::optional<MatchRequest> request_;
    };
}
