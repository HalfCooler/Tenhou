#pragma once
#include "network_protocol.hpp"
#include "offline_lobby.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace tenhou {
    struct OfflineLobbyCommand {
        std::string kind; // "change", "join", "cancel", "register", "pxr"
        std::string argument;
    };

    // Pure C++ ViewModel for OfflineLobbyView / Origin-style service lobby
    // (zero Win32/HWND dependencies).
    class OfflineLobbyViewModel {
    public:
        explicit OfflineLobbyViewModel(OfflineLobbyProfile profile = FixtureProfile())
            : lobby_(std::move(profile)) {}

        void SetMode(bool official, bool service_backed);
        void SyncServiceSnapshot(const LobbySnapshot &snapshot);

        void SelectOuterTab(int tab);
        void SelectTier(RankedTier tier);
        void SetProfileSelectors(unsigned lifetime, unsigned monthly);

        int outer_tab() const { return outer_tab_; }
        RankedTier tier() const { return tier_; }
        unsigned lifetime_profile() const { return lifetime_profile_; }
        unsigned monthly_profile() const { return monthly_profile_; }
        bool official() const { return official_; }
        bool service_backed() const { return service_backed_; }
        bool move_declined() const { return move_declined_; }
        const std::string &service_pending() const { return service_pending_; }
        const std::string &service_error() const { return service_error_; }

        OfflineLobby &lobby() { return lobby_; }
        const OfflineLobby &lobby() const { return lobby_; }
        OfflineLobbyTiming &timing() { return timing_; }
        const OfflineLobbyTiming &timing() const { return timing_; }

        // User-facing pure UTF-8 title and status summary derived from lobby & service state.
        std::string RoomTitle() const;
        std::string StatusSummary() const;
        std::string ServiceErrorSummary() const;

        // Room move request (validates L0000..L9999 / C00000000..C99999999 / bare digits).
        std::optional<OfflineLobbyCommand> RequestMove(std::string_view raw_input, std::uint64_t now_ms);
        bool CompleteOfflineMove(std::uint64_t now_ms);
        std::optional<OfflineLobbyCommand> ToggleReservation(unsigned slot);
        std::optional<OfflineLobbyCommand> CancelAllReservations();
        std::optional<OfflineLobbyCommand> ContestPrimaryAction(std::uint64_t now_seconds);

    private:
        OfflineLobby lobby_;
        OfflineLobbyTiming timing_;
        int outer_tab_ = 0;
        RankedTier tier_ = RankedTier::General;
        unsigned lifetime_profile_ = kRankedFourProfile;
        unsigned monthly_profile_ = kDefaultMonthlyProfile;
        bool official_ = false;
        bool service_backed_ = false;
        bool move_declined_ = false;
        std::string service_pending_;
        std::string service_error_;
    };
} // namespace tenhou
