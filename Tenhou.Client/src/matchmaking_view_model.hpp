#pragma once
#include "matchmaking.hpp"
#include "network_protocol.hpp"
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace tenhou {
    struct MatchmakingCommand {
        std::string action; // "match", "cancel", "logout", "create", "join", "settings", "leave", "close"
        unsigned value = 0;
        std::map<std::string, std::string> settings;
    };

    struct MatchmakingUiState {
        bool custom_mode = false;
        bool busy = false;
        bool in_room = false;
        bool is_owner = false;
        bool can_practice = false;
        bool can_create_room = false;
        bool can_join_room = false;
        bool can_edit_settings = false;
        bool can_leave_room = false;
        bool can_close_room = false;
        bool can_start_room_match = false;
        bool can_cancel = false;
        std::string queue_status;
        std::string rule_summary;
        std::string error_status;
    };

    // Pure C++ ViewModel for Matchmaking / Friend Room lobby (zero Win32/HWND dependencies).
    class MatchmakingViewModel {
    public:
        void Reset();
        void SetSnapshot(const LobbySnapshot &snapshot);
        void SetCustomMode(bool custom);
        void SetEditing(bool editing);
        void SetSelectedQueueIndex(std::size_t index);

        bool custom_mode() const { return custom_; }
        bool editing() const { return editing_; }
        std::size_t selected_queue_index() const { return selected_queue_; }
        const MatchmakingDraft &draft() const { return draft_; }
        const LobbySnapshot &snapshot() const { return snapshot_; }
        const std::string &status() const { return status_; }

        MatchmakingUiState UiState() const;

        // Pure command constructors (returns nullopt and sets status() on validation error).
        std::optional<MatchmakingCommand> PracticeSelectedQueue();
        std::optional<MatchmakingCommand> JoinRoomNumber(std::string_view number);
        std::optional<MatchmakingCommand> StartRoomMatch();
        std::optional<MatchmakingCommand> CancelQueue() const;
        std::optional<MatchmakingCommand> LeaveRoom() const;
        std::optional<MatchmakingCommand> CloseRoom() const;
        std::optional<MatchmakingCommand> Logout() const;
        std::optional<MatchmakingCommand> ApplyRoomSettings(bool creating,
                                                            std::map<std::string, std::string> settings);

    private:
        MatchmakingDraft draft_;
        LobbySnapshot snapshot_;
        bool custom_ = false;
        bool editing_ = false;
        std::size_t selected_queue_ = 0;
        std::string status_;
    };
} // namespace tenhou
