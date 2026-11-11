#pragma once
#include "network_protocol.hpp"
#include <functional>
#include <memory>
#include <filesystem>

namespace tenhou {
struct ConnectionOptions {
    std::string host = "127.0.0.1", identity = "NoName";
    unsigned port = 10080;
    char gender = 'M';
    unsigned handshake_ms = 10000, idle_ms = 60000, heartbeat_ms = 15000;
    bool observe_games = false, interactive_games = false;
    bool recover_games = false;
    bool match_games = false;
    bool presentation = false;
    unsigned matching_ms = 120000;
    bool native_service = false;
    std::string password,nickname;
    bool register_account=false;
    std::filesystem::path config_file;
    std::filesystem::path session_file;
};
class NetworkClient {
public:
    using Observer = std::function<void(const LobbySnapshot&)>;
    using EventObserver = std::function<void(const NetworkMessage&, const LobbySnapshot&)>;
    NetworkClient(Observer changed, std::function<void(const std::string&)> log, EventObserver event = {});
    ~NetworkClient();
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;
    void Connect(const ConnectionOptions& options);
    void Disconnect();
    void Reconnect(const ConnectionOptions& options);
    void SubmitAction(std::uint64_t revision, std::size_t index);
    void SubmitLobby(std::uint64_t revision, const std::string& command, unsigned value = 0);
    void ConfirmMatchStart();
    void ExitGame(int choice = -1);
    void ConfigureRoom(std::uint64_t revision, const std::map<std::string,std::string>& settings);
    void NativeRequest(const std::string& command, unsigned value = 0, const std::map<std::string,std::string>& settings = {});
    const LobbySnapshot& snapshot() const;
    bool active() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace tenhou
