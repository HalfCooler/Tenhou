#include "offline_lobby_view_model.hpp"
#include <map>

namespace tenhou {
    void OfflineLobbyViewModel::SetMode(const bool official, const bool service_backed) {
        official_ = official;
        service_backed_ = service_backed;
    }

    void OfflineLobbyViewModel::SyncServiceSnapshot(const LobbySnapshot &snapshot) {
        service_pending_ = snapshot.origin_pending;
        service_error_ = snapshot.origin_error;
        if (!snapshot.origin_lobby) return;
        const std::string previous_room = lobby_.room();
        lobby_ = *snapshot.origin_lobby;
        if (lobby_.room() != previous_room) move_declined_ = false;
    }

    void OfflineLobbyViewModel::SelectOuterTab(const int tab) {
        outer_tab_ = tab;
    }

    void OfflineLobbyViewModel::SelectTier(const RankedTier tier) {
        tier_ = tier;
    }

    void OfflineLobbyViewModel::SetProfileSelectors(const unsigned lifetime, const unsigned monthly) {
        lifetime_profile_ = lifetime;
        if (ValidMonthlyProfile(monthly)) monthly_profile_ = monthly;
    }

    std::string OfflineLobbyViewModel::RoomTitle() const {
        std::string title;
        if (lobby_.contest()) {
            title = lobby_.Contest() ? lobby_.Contest()->title : "大会室";
        } else if (lobby_.room() == kBroadcastLobby) {
            title = "广播室";
        } else if (!lobby_.ranked()) {
            title = "个室";
        } else {
            title = "段位战大厅";
        }
        return title + "  " + lobby_.room();
    }

    std::string OfflineLobbyViewModel::ServiceErrorSummary() const {
        static const std::map<std::string, const char *> kErrors{
            {"LOBBY_NOT_FOUND", "未找到指定大厅或大会室"},
            {"STALE_LOBBY", "大厅状态已过期，请重试"},
            {"ACCOUNT_REQUIRED", "该操作需要已注册账号"},
            {"INELIGIBLE", "当前账号不符合大会参赛资格"},
            {"REGISTER_FIRST", "请先报名参加该大会"},
            {"BEFORE_START", "大会尚未开始"},
            {"CLOSED", "大会已结束"},
            {"DRAINING", "服务器正在维护停服"},
        };
        const auto found = kErrors.find(service_error_);
        return found == kErrors.end() ? service_error_ : found->second;
    }

    std::string OfflineLobbyViewModel::StatusSummary() const {
        std::string status = official_ ? "天凤官方" : (service_backed_ ? "在线大厅" : "离线大厅");
        if (lobby_.pending()) return "正在切换大厅  " + *lobby_.pending();
        if (!service_pending_.empty()) return "等待服务器确认";
        if (!service_error_.empty()) return ServiceErrorSummary();
        if (move_declined_) return "未找到对应的大会室";
        if (!lobby_.contest()) {
            status += "  在线 " + std::to_string(lobby_.counts().totals[0]);
        }
        return status;
    }

    std::optional<OfflineLobbyCommand> OfflineLobbyViewModel::RequestMove(const std::string_view raw_input,
                                                                          const std::uint64_t now_ms) {
        const auto normalized = LobbyNumber(std::string(raw_input));
        if (!normalized) {
            move_declined_ = true;
            return std::nullopt;
        }
        move_declined_ = false;
        service_error_.clear();
        if (service_backed_) {
            service_pending_ = *normalized;
            return OfflineLobbyCommand{"change", *normalized};
        }
        if (!lobby_.RequestMove(*normalized)) {
            move_declined_ = true;
            return std::nullopt;
        }
        timing_.BeginMove(now_ms);
        return std::nullopt;
    }

    bool OfflineLobbyViewModel::CompleteOfflineMove(const std::uint64_t now_ms) {
        if (!lobby_.pending() || !timing_.MoveReady(now_ms)) return false;
        const std::string target = *lobby_.pending();
        const bool ok = lobby_.ConfirmMove(target);
        if (!ok) {
            lobby_.DeclineMove();
            move_declined_ = true;
        }
        return ok;
    }

    std::optional<OfflineLobbyCommand> OfflineLobbyViewModel::ToggleReservation(const unsigned slot) {
        const auto candidate = lobby_.Candidate(tier_, slot);
        if (!candidate.visible) return std::nullopt;
        if (service_backed_) {
            return OfflineLobbyCommand{candidate.reserved ? "cancel" : "join", std::to_string(candidate.type)};
        }
        lobby_.Reserve(tier_, slot);
        return std::nullopt;
    }

    std::optional<OfflineLobbyCommand> OfflineLobbyViewModel::CancelAllReservations() {
        if (!lobby_.reservations()) return std::nullopt;
        if (service_backed_) return OfflineLobbyCommand{"cancel", ""};
        lobby_.Cancel();
        return std::nullopt;
    }

    std::optional<OfflineLobbyCommand> OfflineLobbyViewModel::ContestPrimaryAction(const std::uint64_t now_seconds) {
        if (!lobby_.contest()) return std::nullopt;
        const auto status = lobby_.ContestAction(now_seconds);
        if (status == ContestStatus::Register || status == ContestStatus::Unregistered) {
            if (service_backed_) return OfflineLobbyCommand{"register", lobby_.room()};
            lobby_.RegisterContest(now_seconds);
            return std::nullopt;
        }
        if (status == ContestStatus::Open) {
            if (service_backed_) return OfflineLobbyCommand{"join", lobby_.room()};
            lobby_.ReserveContest(now_seconds);
            return std::nullopt;
        }
        if (status == ContestStatus::Reserved) {
            return CancelAllReservations();
        }
        return std::nullopt;
    }
} // namespace tenhou
