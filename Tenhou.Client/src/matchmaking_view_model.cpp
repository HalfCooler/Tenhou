#include "matchmaking_view_model.hpp"
#include <algorithm>
#include <map>

namespace tenhou {
    namespace {
        unsigned ParseRoomNumber(const std::string &lobby) {
            if (lobby.size() != 5 || lobby[0] != 'L') return 0;
            if (!std::all_of(lobby.begin() + 1, lobby.end(),
                             [](char c) { return c >= '0' && c <= '9'; })) {
                return 0;
            }
            return static_cast<unsigned>(std::stoul(lobby.substr(1)));
        }

        std::string LocalizeNativeError(const std::string &code) {
            static const std::map<std::string, const char *> kErrors{
                {"NOT_FOUND", "房间不存在"},
                {"CLOSED", "房间已关闭"},
                {"IN_PROGRESS", "房间正在对局"},
                {"FULL", "房间已满"},
                {"OWNER_REQUIRED", "只有房主可以修改设置"},
                {"REVISION_CONFLICT", "房间设置已更新，请重新打开"},
                {"BUSY", "排队或对局期间不能修改"},
                {"ROOM_CAPACITY", "房间号已用完"},
                {"DRAINING", "服务正在停止，暂停接受匹配"},
            };
            const auto found = kErrors.find(code);
            return found == kErrors.end() ? code : found->second;
        }
    } // namespace

    void MatchmakingViewModel::Reset() {
        draft_.Reset();
        custom_ = false;
        editing_ = false;
        selected_queue_ = 0;
        status_.clear();
    }

    void MatchmakingViewModel::SetSnapshot(const LobbySnapshot &snapshot) {
        snapshot_ = snapshot;
        const unsigned room = ParseRoomNumber(snapshot_.lobby);
        if (room && !snapshot_.room_settings.empty()) {
            const bool owner = snapshot_.room_settings.count("owner") &&
                               snapshot_.room_settings.at("owner") == "1";
            draft_.Confirm(room, owner, snapshot_.room_settings);
        } else if (!room) {
            draft_.Confirm(0, false, {});
        }
        if (selected_queue_ >= snapshot_.queues.size()) selected_queue_ = 0;

        if (!snapshot_.native_error.empty()) {
            status_ = LocalizeNativeError(snapshot_.native_error);
        } else if (snapshot_.phase == LobbyPhase::Error) {
            status_ = "连接失败：" + snapshot_.error;
        } else {
            status_.clear();
        }
    }

    void MatchmakingViewModel::SetCustomMode(bool custom) {
        custom_ = custom;
        status_.clear();
    }

    void MatchmakingViewModel::SetEditing(bool editing) { editing_ = editing; }

    void MatchmakingViewModel::SetSelectedQueueIndex(std::size_t index) {
        selected_queue_ = index;
    }

    MatchmakingUiState MatchmakingViewModel::UiState() const {
        MatchmakingUiState ui;
        ui.custom_mode = custom_;
        const bool matching = snapshot_.phase == LobbyPhase::Matching || snapshot_.phase == LobbyPhase::Cancelling;
        const bool pending = matching || !snapshot_.native_pending.empty();
        const bool ready = snapshot_.native_service && snapshot_.catalog_ready && snapshot_.phase == LobbyPhase::Lobby;
        const bool interactive = !pending && ready && !editing_;
        ui.busy = pending || editing_;
        ui.in_room = draft_.room() != 0;
        ui.is_owner = ui.in_room && draft_.intent() == RoomIntent::Create;
        ui.can_practice = !custom_ && interactive && !snapshot_.queues.empty();
        ui.can_create_room = custom_ && interactive && !ui.in_room;
        ui.can_join_room = custom_ && interactive && !ui.in_room;
        ui.can_edit_settings = custom_ && interactive && ui.is_owner;
        ui.can_leave_room = custom_ && interactive && ui.in_room;
        ui.can_close_room = custom_ && interactive && ui.is_owner;
        ui.can_start_room_match = custom_ && interactive && ui.in_room;
        ui.can_cancel = snapshot_.phase == LobbyPhase::Matching;
        ui.error_status = status_;

        if (matching) {
            if (snapshot_.phase == LobbyPhase::Cancelling) {
                ui.queue_status = "等待取消确认";
            } else if (snapshot_.queue_confirmed) {
                ui.queue_status = draft_.room() ? "已排队，等待真人\n10 秒后人机补席" : "正在准备人机对局";
            } else {
                ui.queue_status = "等待排队确认";
            }
        } else {
            ui.queue_status = custom_ ? "创建或加入好友房间，\n由房主选择玩法和规则。"
                                      : "玩法列表来自服务器。\n选择后与人机练习。";
        }

        if (custom_ && ui.is_owner) {
            const auto rule_it = draft_.settings().find("rule");
            if (rule_it != draft_.settings().end()) {
                const auto found = snapshot_.rules.find(rule_it->second);
                ui.rule_summary = found == snapshot_.rules.end() ? "未知规则" : found->second.name;
            }
        }
        return ui;
    }

    std::optional<MatchmakingCommand> MatchmakingViewModel::PracticeSelectedQueue() {
        if (draft_.room()) {
            status_ = "请先离开大厅战房间";
            return std::nullopt;
        }
        if (snapshot_.queues.empty() || selected_queue_ >= snapshot_.queues.size()) {
            status_ = "当前没有可用的玩法队列";
            return std::nullopt;
        }
        status_.clear();
        return MatchmakingCommand{"match", snapshot_.queues[selected_queue_].id, {}};
    }

    std::optional<MatchmakingCommand> MatchmakingViewModel::JoinRoomNumber(std::string_view number) {
        MatchmakingDraft probe;
        if (!probe.JoinRoom(number)) {
            status_ = "请输入 1000～9999 的四位房间号";
            return std::nullopt;
        }
        status_.clear();
        return MatchmakingCommand{"join", probe.room(), {}};
    }

    std::optional<MatchmakingCommand> MatchmakingViewModel::StartRoomMatch() {
        if (snapshot_.room_settings.empty()) {
            status_ = "请先确认房间规则设置";
            return std::nullopt;
        }
        status_.clear();
        return MatchmakingCommand{"match", 0, {}};
    }

    std::optional<MatchmakingCommand> MatchmakingViewModel::CancelQueue() const {
        return MatchmakingCommand{"cancel", 0, {}};
    }

    std::optional<MatchmakingCommand> MatchmakingViewModel::LeaveRoom() const {
        return MatchmakingCommand{"leave", 0, {}};
    }

    std::optional<MatchmakingCommand> MatchmakingViewModel::CloseRoom() const {
        return MatchmakingCommand{"close", 0, {}};
    }

    std::optional<MatchmakingCommand> MatchmakingViewModel::Logout() const {
        return MatchmakingCommand{"logout", 0, {}};
    }

    std::optional<MatchmakingCommand> MatchmakingViewModel::ApplyRoomSettings(
        bool creating, std::map<std::string, std::string> settings) {
        if (settings.empty()) return std::nullopt;
        status_.clear();
        return MatchmakingCommand{creating ? "create" : "settings", 0, std::move(settings)};
    }
} // namespace tenhou
