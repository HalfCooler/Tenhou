#include <winsock2.h>
#include <ws2tcpip.h>
#include "network_protocol.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
    SOCKET socket = INVALID_SOCKET;
    try {
        if (argc < 2 || argc > 3) throw std::runtime_error("Usage: tenhou_protocol_check PORT [QUEUE_ID]");
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
        send(session.Connected("NoName", 'M'));
        tenhou::NetworkFramer framer;
        bool requested = false;
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
                if (s.catalog_ready && s.phase == tenhou::LobbyPhase::Lobby && !requested) {
                    if (s.queues.empty() || s.rules.empty()) throw std::runtime_error("Empty catalog");
                    send(session.SubmitLobby(s.lobby_revision, "match", queue_id)); requested = true;
                }
                if (s.phase == tenhou::LobbyPhase::MatchComplete) {
                    send(session.SubmitLobby(s.lobby_revision, "return"));
                    std::cout << "PASS native client queue " << queue_id << " and full match: " << actions << " actions\n";
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
