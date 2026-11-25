#include <winsock2.h>
#include <ws2tcpip.h>
#include "network_protocol.hpp"
#include "game_seats.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
    SOCKET socket = INVALID_SOCKET;
    try {
        const bool service = argc == 5 && std::string(argv[2]) == "--service";
        const bool service_return = argc == 4 && std::string(argv[2]) == "--service-return";
        if ((argc < 2 || argc > 3) && !service && !service_return)
            throw std::runtime_error("Usage: tenhou_protocol_check PORT [QUEUE_ID | --service ACCOUNT TYPE | --service-return ACCOUNT]");
        const unsigned queue_id = argc == 3 ? static_cast<unsigned>(std::stoul(argv[2])) : 101u;
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data)) return 1;
        socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in address{};
        address.sin_family = AF_INET; address.sin_port = htons(static_cast<u_short>(std::stoi(argv[1])));
        InetPtonA(AF_INET, "127.0.0.1", &address.sin_addr);
        if (connect(socket, reinterpret_cast<sockaddr *>(&address), sizeof(address))) throw std::runtime_error("Cannot connect");
        DWORD timeout = 15000;
        setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout));
        auto send = [&](const std::vector<std::string> &messages) {
            for (const auto &message: messages) {
                const auto bytes = message + '\0';
                std::size_t cursor = 0;
                while (cursor < bytes.size()) {
                    const int count = ::send(socket, bytes.data() + cursor, static_cast<int>(bytes.size() - cursor), 0);
                    if (count <= 0) throw std::runtime_error("Send failed");
                    cursor += count;
                }
            }
        };
        tenhou::LobbySession session;
        session.require_native = session.interactive_games = session.match_games = session.recover_games = true;
        session.origin_lobby = service || service_return;
        session.native_register = service;
        if (service || service_return) {
            session.native_password = "test-password";
            session.native_nickname = argv[3];
        }
        send(session.Connected(session.origin_lobby ? argv[3] : "NoName", 'M'));
        tenhou::NetworkFramer framer;
        bool requested = false, returning = false;
        unsigned actions = 0;
        std::string last;
        while (true) {
            char buffer[8192]; const int count = recv(socket, buffer, sizeof(buffer), 0);
            if (count <= 0) throw std::runtime_error("Receive failed after " + last);
            for (const auto &frame: framer.Feed(buffer, static_cast<std::size_t>(count))) {
                const auto message = tenhou::ParseNetworkMessage(frame); last = message.tag;
                try { send(session.Receive(message)); }
                catch (const std::exception &error) { throw std::runtime_error(last + ": " + error.what()); }
                const auto &s = session.snapshot;
                if (service && s.origin_lobby && s.phase == tenhou::LobbyPhase::Lobby && !requested) {
                    send(session.SubmitOriginLobby("join", argv[4]));
                    requested = true;
                }
                if (service && s.game.initialized) {
                    const auto type = static_cast<unsigned>(std::stoul(argv[4]));
                    if (s.match_type != type || s.game.type != (type & ~0xA0u) || !s.ranked ||
                        tenhou::SeatCount(s.game.seat_mask) != ((type & 16) ? 3 : 4) ||
                        s.game.players[0].name != argv[3] ||
                        std::any_of(s.game.players.begin(), s.game.players.end(), [](const auto &player) {
                            return player.name.rfind("人机", 0) == 0;
                        }) || s.game.players[0].known_hand.size() != 13 || s.origin_lobby->reservations())
                        throw std::runtime_error("Ranked handoff mismatch");
                    std::cout << "PASS native ranked handoff " << type << " for " << argv[3] << '\n';
                    closesocket(socket); WSACleanup(); return 0;
                }
                if (s.catalog_ready && s.phase == tenhou::LobbyPhase::Lobby && !requested) {
                    if (s.queues.empty() || s.rules.empty()) throw std::runtime_error("Empty catalog");
                    send(session.SubmitLobby(s.lobby_revision, "match", queue_id)); requested = true;
                }
                if (s.phase == tenhou::LobbyPhase::MatchComplete) {
                    if (s.recovering) continue;
                    if (service_return) {
                        send(session.SubmitLobby(s.lobby_revision, "return"));
                        returning = true;
                        continue;
                    }
                    send(session.SubmitLobby(s.lobby_revision, "return"));
                    std::cout << "PASS native client queue " << queue_id << " and full match: " << actions << " actions\n";
                    closesocket(socket); WSACleanup(); return 0;
                }
                if (returning && s.phase == tenhou::LobbyPhase::Lobby) {
                    if (!s.origin_lobby || s.lobby != "C00112233" || s.game.started || s.exit_table.size() ||
                        s.origin_lobby->reservations())
                        throw std::runtime_error("Completed table did not return to its contest");
                    std::cout << "PASS native completed-table recovery and contest return\n";
                    closesocket(socket); WSACleanup(); return 0;
                }
                if (s.phase != tenhou::LobbyPhase::Game || !s.actions.pending.empty() || s.actions.choices.empty()) continue;
                const auto &choices = s.actions.choices;
                auto rank = [](const tenhou::OnlineAction &a) {
                    return a.kind == "tsumo" || a.kind == "ron" ? 0 : a.kind == "skip" ? 1 : a.kind == "discard" ? 2 : 3;
                };
                const auto best = std::min_element(choices.begin(), choices.end(), [&](const auto &a, const auto &b) { return rank(a) < rank(b); });
                auto replies = session.SubmitAction(s.actions.revision, static_cast<std::size_t>(best - choices.begin()));
                for (auto &reply: replies) {
                    const auto tag = tenhou::ParseNetworkMessage(reply).tag;
                    if (tag == "D" || tag == "N" || tag == "REACH")
                        reply.insert(reply.find('>') - (reply[reply.find('>') - 1] == '/' ? 1 : 0), " window=\"" + std::to_string(s.native_window) + "\"");
                }
                send(replies); ++actions;
                if (actions > 10000) throw std::runtime_error("Too many actions");
            }
        }
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n'; if (socket != INVALID_SOCKET) closesocket(socket); WSACleanup(); return 1;
    }
}
