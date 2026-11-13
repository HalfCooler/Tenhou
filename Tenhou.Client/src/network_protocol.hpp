#pragma once
#include "online_actions.hpp"
#include "matchmaking.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace tenhou {
    constexpr std::size_t kNetworkMessageLimit = 65536;

    struct NetworkMessage {
        std::string tag;
        std::map<std::string, std::string> attributes;
    };

    NetworkMessage ParseNetworkMessage(const std::string &xml);

    std::string AuthenticationResponse(const std::string &challenge);

    std::vector<int> DecodeLobbyNumbers(const std::string &encoded);

    std::wstring NetworkWide(const std::string &utf8);

    std::string NetworkUtf8(const std::wstring &wide);

    std::string DecodeNetworkText(const std::string &encoded);

    std::string NetworkJson(const std::string &value);

    class NetworkFramer {
    public:
        std::vector<std::string> Feed(const char *bytes, std::size_t count);

        bool pending() const { return !pending_.empty(); }

    private:
        std::string pending_;
    };

    enum class LobbyPhase {
        Disconnected, Resolving, Connecting, AwaitHelo, AwaitLobby, Lobby, AwaitRecovery, Game,
        Matching, Cancelling, ChangingLobby, MatchComplete, AwaitReturn, Error
    };

    const char *LobbyPhaseName(LobbyPhase phase);

    struct LobbySnapshot {
        bool variant_active = false;
        std::map<std::string, std::string> variant_state;
        bool catalog_ready = false;
        std::map<std::string, ServerRule> rules;
        std::vector<ServerQueue> queues;
        bool native_service = false, queue_confirmed = false, table_paused = false;
        std::string native_token, native_pending, native_error;
        std::map<std::string, std::string> profile;
        bool ranked = false;
        std::string replay_status;
        unsigned native_request = 0;
        std::uint64_t native_window = 0;
        std::string exit_table;
        bool exit_allowed = false, exit_pending = false, exit_voting = false, exit_ballot_pending = false;
        unsigned exit_vote_id = 0, exit_vote_ms = 0, exit_total = 0, exit_yes = 0, exit_no = 0;
        int exit_choice = -1;
        bool matchmaking = false;
        std::uint64_t lobby_revision = 0;
        unsigned match_type = 0;
        std::string target_lobby, match_result;
        SettlementState last_settlement;
        std::array<std::string, 4> last_result_names;
        int last_result_dealer = 0, last_result_honba = -1, last_result_sticks = -1;
        bool last_result_red = true;
        bool reconnect_available = false, recovering = false;
        LobbyPhase phase = LobbyPhase::Disconnected;
        std::string identity, lobby, error, last_tag;
        std::vector<int> players, waiting, games;
        std::map<std::string, std::string> server;
        std::map<std::string, std::string> room_settings;
        std::vector<std::string> messages;
        std::uint64_t received = 0, sent = 0;
        OnlineState game;
        ActionSnapshot actions;
    };

    class LobbySession {
    public:
        std::vector<std::string> Connected(const std::string &identity, char gender, std::uint64_t revision = 0);

        std::vector<std::string> Receive(const NetworkMessage &message);

        LobbySnapshot snapshot;
        bool observe_games = false, interactive_games = false;
        bool recover_games = false, resume_connection = false;
        bool match_games = false;
        bool require_native = false, presentation = false;
        std::string native_token;
        std::string native_password, native_nickname;
        bool native_register = false;
        char native_gender = 'M';

        std::vector<std::string> NativeRequest(const std::string &command, unsigned value,
                                               const std::map<std::string, std::string> &settings = {});

        std::vector<std::string> SubmitLobby(std::uint64_t revision, const std::string &command, unsigned value = 0);

        std::vector<std::string> SubmitAction(std::uint64_t revision, std::size_t index);

        std::vector<std::string> ExitGame(int choice = -1);

        std::vector<std::string> ConfigureRoom(std::uint64_t revision,
                                               const std::map<std::string, std::string> &settings);

    private:
        std::vector<std::string> Apply(const NetworkMessage &message);

        OnlineGame game_;
        OnlineActions actions_;
        bool rejoin_received_ = false, recovery_go_ = false;
        unsigned room_ = 0;
    };
} // namespace tenhou
