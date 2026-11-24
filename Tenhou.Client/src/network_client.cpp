// Message-driven IPv4 transport with cancellable DNS for the desktop client.
#define _WINSOCK_DEPRECATED_NO_WARNINGS
// Winsock must precede windows.h. All callbacks run on the owning UI thread.
#include <winsock2.h>
#include <ws2tcpip.h>
#include "network_client.hpp"
#include "replay_archive.hpp"
#include <windows.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    namespace {
        constexpr UINT kDns = WM_APP + 31, kSocket = WM_APP + 32, kRead = WM_APP + 33;
        constexpr wchar_t kNetworkWindow[] = L"TENHOU_REBUILD_NETWORK";

        struct TransportFailure : std::runtime_error {
            using std::runtime_error::runtime_error;
        };

        std::runtime_error SocketError(const char *action, int code = WSAGetLastError()) {
            return std::runtime_error(std::string(action) + " failed; Winsock error=" + std::to_string(code));
        }
    }

    struct NetworkClient::Impl {
        std::string native_token;
        std::string official_game_id;
        Observer changed;
        EventObserver accepted_event;
        std::function<void(const std::string &)> log;
        LobbySession session;
        ConnectionOptions options;
        NetworkFramer framer;
        ReplayArchive archive;
        HWND window = nullptr;
        SOCKET socket = INVALID_SOCKET;
        HANDLE dns = nullptr;
        std::array<char, MAXGETHOSTSTRUCT> host{};
        std::string outgoing;
        std::size_t sent_offset = 0;
        std::uint64_t revision_floor = 0;
        std::uint64_t server_error_sequence = 0;
        std::optional<LobbySnapshot> reconnect_view;
        ULONGLONG retry_at = 0, retry_until = 0;
        unsigned retry_delay = 1000;
        bool resuming = false;
        bool local_prepare = false;
        ULONGLONG action_start = 0;
        ULONGLONG native_request_start = 0;
        ULONGLONG recovery_start = 0;
        ULONGLONG phase_start = 0, last_receive = 0, last_heartbeat = 0, frame_start = 0, send_start = 0;

        Impl(Observer observer, std::function<void(const std::string &)> logger, EventObserver event)
            : changed(std::move(observer)), accepted_event(std::move(event)), log(std::move(logger)) {
            WSADATA data{};
            const int error = WSAStartup(MAKEWORD(2, 2), &data);
            if (error) throw SocketError("WSAStartup", error);
            WNDCLASSEXW wc{sizeof(wc)};
            wc.lpfnWndProc = Procedure;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = kNetworkWindow;
            if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
                WSACleanup();
                throw std::runtime_error("Cannot register network message window");
            }
        }

        ~Impl() {
            Close();
            WSACleanup();
        }

        bool Active() const {
            const auto phase = session.snapshot.phase;
            return phase != LobbyPhase::Error && phase != LobbyPhase::Disconnected;
        }

        const LobbySnapshot &Snapshot() const { return reconnect_view ? *reconnect_view : session.snapshot; }

        void Notify() { changed(Snapshot()); }

        void Phase(LobbyPhase phase) {
            session.snapshot.phase = phase;
            phase_start = GetTickCount64();
            log(std::string("NETWORK state=") + LobbyPhaseName(phase));
            Notify();
        }

        void Close() noexcept {
            if (dns) {
                WSACancelAsyncRequest(dns);
                dns = nullptr;
            }
            if (socket != INVALID_SOCKET) {
                WSAAsyncSelect(socket, window, 0, 0);
                shutdown(socket, SD_BOTH);
                closesocket(socket);
                socket = INVALID_SOCKET;
            }
            if (window) {
                KillTimer(window, 1);
                DestroyWindow(window);
                window = nullptr;
            }
            outgoing.clear();
            sent_offset = 0;
            framer = {};
            frame_start = 0;
            send_start = 0;
            local_prepare = false;
            action_start = 0;
            native_request_start = 0;
        }

        void OpenEventWindow() {
            window = CreateWindowExW(0, kNetworkWindow, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                     GetModuleHandleW(nullptr), this);
            if (!window || !SetTimer(window, 1, 50, nullptr))
                throw std::runtime_error("Cannot create network event window/timer");
        }

        bool ScheduleReconnect() {
            const auto now = GetTickCount64();
            if (!reconnect_view) {
                if ((!session.snapshot.game.started && !session.snapshot.variant_active) ||
                    session.snapshot.game.complete) return false;
                reconnect_view = session.snapshot;
                reconnect_view->recovering = reconnect_view->silent_recovery = true;
                reconnect_view->actions.choices.clear();
                reconnect_view->actions.pending.clear();
                ++reconnect_view->actions.revision;
                retry_delay = 1000;
                retry_until = now + 60000;
            }
            if (now >= retry_until) return false;
            Close();
            retry_at = now + retry_delay;
            retry_delay = std::min(2 * retry_delay, 5000u);
            try { OpenEventWindow(); }
            catch (...) { return false; }
            Notify();
            return true;
        }

        void Error(const std::string &error, bool transport = false, int server_code = -1,
                   const std::string &server_text = {}, bool server_rejected = false) {
            log("NETWORK error=" + error);
            if (transport && options.auto_reconnect && options.recover_games && ScheduleReconnect()) return;
            const bool available = transport && options.recover_games &&
                                   (((session.snapshot.game.initialized || session.snapshot.variant_active) && !session.snapshot.game.complete) ||
                                    session.snapshot.origin_lobby.has_value() || resuming);
            revision_floor = std::max({
                                 revision_floor, session.snapshot.actions.revision, session.snapshot.lobby_revision
                             }) + 1;
            Close();
            reconnect_view.reset();
            retry_at = 0;
            session = {};
            session.snapshot.error = error;
            session.snapshot.reconnect_available = available;
            session.snapshot.recovering = false;
            if (server_rejected) {
                session.snapshot.server_error_receipt = ++server_error_sequence;
                session.snapshot.server_error_code = server_code;
                session.snapshot.server_error_text = server_text;
                session.snapshot.server_error_fatal = true;
            }
            Phase(LobbyPhase::Error);
        }

        void Disconnect() {
            revision_floor = std::max({
                                 revision_floor, session.snapshot.actions.revision, session.snapshot.lobby_revision
                             }) + 1;
            resuming = false;
            reconnect_view.reset();
            retry_at = 0;
            Close();
            session = {};
            Phase(LobbyPhase::Disconnected);
        }

        void Connect(const ConnectionOptions &request, bool resume = false) {
            if (request.host != options.host || request.port != options.port || request.identity != options.identity) {
                native_token.clear();
                official_game_id.clear();
            }
            revision_floor = std::max({
                                 revision_floor, session.snapshot.actions.revision, session.snapshot.lobby_revision
                             }) + 1;
            resuming = resume;
            recovery_start = 0;
            Close();
            archive.Reset();
            session = {};
            options = request;
            session.observe_games = options.observe_games;
            session.interactive_games = options.interactive_games;
            session.recover_games = options.recover_games;
            session.resume_connection = resume;
            session.match_games = options.match_games;
            session.require_native = options.native_service;
            session.official = options.official;
            session.snapshot.official = options.official;
            session.silent_recovery = resume;
            session.origin_lobby = options.origin_lobby;
            session.presentation = options.presentation;
            session.native_gender = options.gender;
            session.native_password = options.password;
            session.native_nickname = options.nickname;
            session.native_register = options.register_account && !resume;
            if (options.official) {
                const auto type = GetPrivateProfileIntW(L"REBUILD", L"ProfileMonthly",
                    kDefaultMonthlyProfile, options.config_file.c_str());
                if (ValidMonthlyProfile(type)) session.preferred_monthly_profile = type;
            }
            try {
                if (options.native_service && native_token.empty() && !options.session_file.empty() &&
                    std::filesystem::exists(options.session_file)) {
                    std::ifstream file(options.session_file, std::ios::binary);
                    native_token.assign(std::istreambuf_iterator<char>(file), {});
                    if (native_token.size() != 64 || !std::all_of(native_token.begin(), native_token.end(),
                        [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }))
                        native_token.clear(); // A damaged recovery hint must not block login.
                }
                if (options.official && !resume && !options.session_file.empty()) {
                    const bool same_process = !official_game_id.empty();
                    wchar_t value[81]{};
                    GetPrivateProfileStringW(L"OfficialRecovery", options.session_file.filename().c_str(), L"",
                                             value, ARRAYSIZE(value), options.config_file.c_str());
                    official_game_id = NetworkUtf8(value);
                    if (!ValidGameId(official_game_id)) official_game_id.clear();
                    resuming = resume = !official_game_id.empty();
                    session.resume_connection = resume;
                    session.silent_recovery = resume && same_process;
                }
                session.official_game_id = official_game_id;
                session.native_token = native_token;
                session.snapshot.recovering = resume;
                if (options.recover_games && !options.observe_games && !options.interactive_games)
                    throw std::runtime_error("Local recovery requires a game mode");
                if (options.match_games && (!options.interactive_games || options.observe_games))
                    throw std::runtime_error(
                        "Matchmaking requires local interactive mode without read-only observation");
                if (options.matching_ms < 100 || options.matching_ms > 600000) throw std::runtime_error(
                    "Invalid matchmaking timeout");
                // Validate before creating a DNS request or socket.
                LobbySession validation;
                validation.require_native = options.native_service;
                (void) validation.Connected(options.identity, options.gender);
                if (options.host.empty() || options.host.size() > 253 || options.host.front() == '-' ||
                    !std::all_of(options.host.begin(), options.host.end(), [](unsigned char c) {
                        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' ||
                               c == '.';
                    }) || options.port < 1 || options.port > 65535)
                    throw std::runtime_error("Invalid IPv4 host or port (1..65535)");
                if (options.handshake_ms < 100 || options.handshake_ms > 60000 || options.idle_ms < 100 || options.
                    idle_ms > 600000 ||
                    options.heartbeat_ms < 50 || options.heartbeat_ms > 60000)
                    throw std::runtime_error("Invalid connection timing");
                OpenEventWindow();
                last_receive = last_heartbeat = GetTickCount64();
                Phase(LobbyPhase::Resolving);
                IN_ADDR address{};
                if (InetPtonA(AF_INET, options.host.c_str(), &address) == 1) BeginTcp(address);
                else {
                    dns = WSAAsyncGetHostByName(window, kDns, options.host.c_str(), host.data(),
                                                static_cast<int>(host.size()));
                    if (!dns) throw TransportFailure(SocketError("DNS request").what());
                }
            } catch (const TransportFailure &error) { Error(error.what(), true); }
            catch (const std::exception &error) { Error(error.what()); }
        }

        static bool ValidGameId(const std::string &value) {
            return !value.empty() && value.size() < 80 && std::all_of(value.begin(), value.end(), [](unsigned char c) {
                return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                       c == '_' || c == '-';
            });
        }

        void SaveOfficialGameId(std::string value) {
            if (!value.empty() && !ValidGameId(value)) return;
            official_game_id = std::move(value);
            if (options.session_file.empty()) return;
            const auto key = options.session_file.filename().wstring();
            if (!WritePrivateProfileStringW(L"OfficialRecovery", key.c_str(),
                    official_game_id.empty() ? nullptr : NetworkWide(official_game_id).c_str(), options.config_file.c_str()))
                log("NETWORK unable to save official recovery hint");
        }

        void AccumulatePrivateProfile(const NetworkMessage &message) {
            // Origin tenhou.exe.c:92620-92706 accumulates non-zero lobby PROF add=11 fields into [STATUS] PFPV3/PFPV4.
            if (options.config_file.empty()) return;
            const auto lobby_it = message.attributes.find("lobby");
            const auto add_it = message.attributes.find("add");
            if (lobby_it == message.attributes.end() || add_it == message.attributes.end()) return;
            int lobby_num = 0;
            const auto parsed_lobby = std::from_chars(
                lobby_it->second.data(), lobby_it->second.data() + lobby_it->second.size(), lobby_num);
            if (parsed_lobby.ec != std::errc{} || parsed_lobby.ptr != lobby_it->second.data() + lobby_it->second.size() ||
                lobby_num == 0)
                return;
            auto parse11 = [](const std::string &text, std::array<double, 11> &out) {
                out.fill(0.0);
                std::istringstream in(text);
                in.imbue(std::locale::classic());
                std::string token;
                std::size_t i = 0;
                while (i < out.size() && std::getline(in, token, ',')) {
                    double v = 0.0;
                    const auto res = std::from_chars(token.data(), token.data() + token.size(), v);
                    if (res.ec == std::errc{} && res.ptr == token.data() + token.size() && std::isfinite(v))
                        out[i] = v;
                    ++i;
                }
                return i;
            };
            std::array<double, 11> delta{};
            if (parse11(add_it->second, delta) == 0) return;
            const wchar_t *key = (session.snapshot.game.type & 16) ? L"PFPV3" : L"PFPV4";
            wchar_t existing[256]{};
            GetPrivateProfileStringW(L"STATUS", key, L"", existing, ARRAYSIZE(existing), options.config_file.c_str());
            std::array<double, 11> total{};
            parse11(NetworkUtf8(existing), total);
            for (std::size_t i = 0; i < total.size(); ++i) total[i] += delta[i];
            std::ostringstream out;
            out.imbue(std::locale::classic());
            out << std::fixed << std::setprecision(1) << total[0];
            for (std::size_t i = 1; i < total.size(); ++i) out << ',' << static_cast<long long>(std::llround(total[i]));
            WritePrivateProfileStringW(L"STATUS", key, NetworkWide(out.str()).c_str(), options.config_file.c_str());
        }

        void Reconnect(const ConnectionOptions &request) {
            if (Active() || !session.snapshot.reconnect_available) throw std::runtime_error(
                "No recoverable disconnected game");
            if (!request.recover_games || request.host != options.host || request.port != options.port ||
                request.gender != options.gender || request.observe_games != options.observe_games || request.
                interactive_games != options.interactive_games || request.match_games != options.match_games ||
                request.origin_lobby != options.origin_lobby)
                throw std::runtime_error("Reconnect requires the same server and game mode");
            Connect(request, true);
        }

        void BeginTcp(IN_ADDR address) {
            if (!options.native_service && !options.official && (options.interactive_games || options.recover_games) &&
                (ntohl(address.S_un.S_addr) >> 24) != 127)
                throw std::runtime_error("Interactive DNS resolved outside loopback");
            socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (socket == INVALID_SOCKET) throw SocketError("socket");
            if (WSAAsyncSelect(socket, window, kSocket, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
                throw SocketError("WSAAsyncSelect");
            sockaddr_in endpoint{};
            endpoint.sin_family = AF_INET;
            endpoint.sin_addr = address;
            endpoint.sin_port = htons(static_cast<u_short>(options.port));
            Phase(LobbyPhase::Connecting);
            const int result = ::connect(socket, reinterpret_cast<sockaddr *>(&endpoint), sizeof(endpoint));
            if (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
                throw TransportFailure(SocketError("connect").what());
            if (result == 0) Connected();
        }

        void Connected() {
            if (session.snapshot.phase != LobbyPhase::Connecting) return;
            const auto messages = session.Connected(options.identity, options.gender, revision_floor);
            recovery_start = resuming ? GetTickCount64() : 0;
            if (!options.auto_reconnect && !options.official) options.identity.clear();
            last_receive = last_heartbeat = GetTickCount64();
            Phase(LobbyPhase::AwaitHelo);
            // Origin tenhou.exe.c:62067 transmits <Z /> immediately on FD_CONNECT before <HELO .../>.
            if (options.official) Queue("<Z />");
            for (const auto &message: messages) Queue(message);
        }

        void Queue(const std::string &message) {
            std::string wire = message;
            const auto tag = ParseNetworkMessage(message).tag;
            if (session.snapshot.native_service && (tag == "D" || tag == "N" || tag == "REACH"))
                wire.insert(wire.rfind('/'), " window=\"" + std::to_string(session.snapshot.native_window) + "\" ");
            if (outgoing.size() - sent_offset + wire.size() + 1 > 131072) throw std::runtime_error(
                "Network send queue exceeds limit");
            if (sent_offset) {
                outgoing.erase(0, sent_offset);
                sent_offset = 0;
            }
            if (outgoing.empty()) send_start = GetTickCount64();
            outgoing += wire;
            outgoing += '\0';
            ++session.snapshot.sent;
            log("NETWORK send tag=" + ParseNetworkMessage(message).tag);
            Flush();
        }

        void Flush() {
            while (sent_offset < outgoing.size()) {
                const int count = send(socket, outgoing.data() + sent_offset,
                                       static_cast<int>(outgoing.size() - sent_offset), 0);
                if (count == SOCKET_ERROR) {
                    if (WSAGetLastError() == WSAEWOULDBLOCK) return;
                    throw TransportFailure(SocketError("send").what());
                }
                if (!count) throw std::runtime_error("Network send made no progress");
                sent_offset += static_cast<std::size_t>(count);
            }
            outgoing.clear();
            sent_offset = 0;
            send_start = 0;
        }

        void Read() {
            // Limit work per message dispatch so a busy server cannot monopolize the UI.
            std::array < char, 4096 > bytes{};
            for (int batch = 0; batch < 16; ++batch) {
                const int count = recv(socket, bytes.data(), static_cast<int>(bytes.size()), 0);
                if (count == SOCKET_ERROR) {
                    if (WSAGetLastError() == WSAEWOULDBLOCK) return;
                    throw TransportFailure(SocketError("recv").what());
                }
                if (!count) {
                    if (framer.pending()) throw TransportFailure("Connection closed with truncated message");
                    throw TransportFailure("Server closed connection");
                }
                const bool was_pending = framer.pending();
                const auto messages = framer.Feed(bytes.data(), static_cast<std::size_t>(count));
                if (framer.pending()) {
                    if (!was_pending || !messages.empty()) frame_start = GetTickCount64();
                } else frame_start = 0;
                bool close_after_owari = false;
                for (const auto &text: messages) {
                    const auto message = ParseNetworkMessage(text, options.official);
                    const auto old_phase = session.snapshot.phase;
                    const auto old_err = session.snapshot.server_error_receipt;
                    const auto replies = session.Receive(message);
                    if (session.snapshot.server_error_receipt != old_err)
                        session.snapshot.server_error_receipt = ++server_error_sequence;
                    if (options.official && message.tag == "HELO") {
                        if (!official_game_id.empty()) SaveOfficialGameId({});
                        resuming = false;
                        recovery_start = 0;
                        reconnect_view.reset();
                    }
                    if (options.official && message.tag == "GO" && session.snapshot.game.started) {
                        if (const auto id = message.attributes.find("gpid"); id != message.attributes.end())
                            SaveOfficialGameId(id->second);
                    }
                    if (options.official && message.tag == "PROF") AccumulatePrivateProfile(message);
                    if (options.official && session.snapshot.game.complete) {
                        if (!official_game_id.empty()) SaveOfficialGameId({});
                        // Origin tenhou.exe.c:97928-97932 closes WC_CLIENT immediately on owari;
                        // END opens a fresh TCP connection to return to the lobby.
                        close_after_owari = true;
                    }
                    if (message.tag.rfind("REPLAY", 0) == 0) {
                        try {
                            archive.Receive(message);
                            session.snapshot.replay_status = archive.status();
                        } catch (const std::exception &e) {
                            session.snapshot.replay_status = std::string("牌谱保存失败：") + e.what();
                        }
                        log("REPLAY STATUS " + session.snapshot.replay_status);
                    }
                    if ((message.tag == "PROF" || message.tag == "RESTORED") && archive.Request(
                            session.snapshot, options.config_file)) {
                        session.snapshot.replay_status = archive.status();
                        Queue("<GETREPLAY table=\"" + session.snapshot.exit_table + "\"/>");
                    }
                    if (message.tag == "SESSION" && session.snapshot.native_service) {
                        native_token = session.snapshot.native_token;
                        options.password.clear();
                        if (!options.session_file.empty()) {
                            std::filesystem::create_directories(options.session_file.parent_path());
                            const auto temporary = options.session_file.wstring() + L".tmp";
                            HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                                      FILE_ATTRIBUTE_NORMAL, nullptr);
                            if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open platform session temporary file");
                            DWORD written = 0;
                            const bool saved = WriteFile(file, native_token.data(), static_cast<DWORD>(native_token.size()), &written, nullptr)
                                               && written == native_token.size() && FlushFileBuffers(file);
                            CloseHandle(file);
                            if (!saved || !MoveFileExW(temporary.c_str(), options.session_file.c_str(),
                                                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) throw
                                    std::runtime_error("Cannot atomically save native guest session");
                        }
                    }
                    if (message.tag == "GO" || message.tag == "REINIT") local_prepare = false;
                    if (message.tag == "PREPARE") local_prepare = true;
                    if (message.tag == "START") local_prepare = false;
                    const bool official_intro = options.official && options.presentation &&
                        message.tag == "TAIKYOKU" && !replies.empty();
                    if (official_intro) local_prepare = true;
                    if (options.official && session.snapshot.recovering && !recovery_start)
                        recovery_start = GetTickCount64();
                    if (!session.snapshot.recovering && ((message.tag == "REINIT" && !session.snapshot.native_service) ||
                        message.tag == "RESTORED" ||
                        (options.official && message.tag == "INIT") ||
                        (message.tag == "CHAT" && session.snapshot.origin_lobby) ||
                        (message.tag == "LN" && session.snapshot.phase == LobbyPhase::Lobby))) {
                        resuming = false;
                        recovery_start = 0;
                        reconnect_view.reset();
                    }
                    if (accepted_event && !reconnect_view) accepted_event(message, session.snapshot);
                    if (socket == INVALID_SOCKET) return;
                    if (session.snapshot.actions.pending.empty() || session.snapshot.table_paused ||
                        (options.official && local_prepare)) action_start = 0;
                    else if (!action_start) action_start = GetTickCount64();
                    last_receive = GetTickCount64();
                    log("NETWORK recv tag=" + message.tag);
                    if (old_phase != session.snapshot.phase) Phase(session.snapshot.phase);
                    if (!official_intro) for (const auto &reply: replies) Queue(reply);
                    // Opt in only to the identified local rules service. The
                    // original remote protocol and its GOK handshake stay intact.
                    if (message.tag == "HELO" && message.attributes.count("uname") && message.attributes.at("uname") ==
                        "LOCAL-RULES" && options.interactive_games && options.presentation)
                        Queue("<PRESENTATION />");
                    Notify();
                }
                if (close_after_owari) {
                    Close();
                    return;
                }
            }
            PostMessageW(window, kRead, 0, 0);
        }

        void SubmitAction(std::uint64_t revision, std::size_t index) {
            // Invalid/stale UI requests are rejected without tearing down a healthy connection.
            const auto replies = session.SubmitAction(revision, index);
            try {
                action_start = GetTickCount64();
                for (const auto &reply: replies) Queue(reply);
                Notify();
            } catch (const TransportFailure &error) { Error(error.what(), true); } catch (const std::exception &error) {
                Error(error.what());
            }
        }

        void Tick() {
            const auto now = GetTickCount64();
            if (reconnect_view && now >= retry_until) {
                Error("Automatic game recovery timed out", true);
                return;
            }
            if (retry_at) {
                if (now >= retry_at) {
                    retry_at = 0;
                    const auto request = options;
                    Connect(request, true);
                }
                return;
            }
            if (!Active()) return;
            const auto phase = session.snapshot.phase;
            if (session.snapshot.native_pending.empty() && session.snapshot.origin_pending.empty()) native_request_start = 0;
            else if (!native_request_start) native_request_start = now;
            else if (now - native_request_start >= options.handshake_ms) throw TransportFailure(
                "Native room request confirmation timed out");
            const bool official_saikai_wait = options.official && session.snapshot.game.started &&
                                              phase == LobbyPhase::AwaitRecovery;
            const auto recovery_limit = official_saikai_wait ? options.idle_ms : options.handshake_ms;
            if (recovery_start && now - recovery_start >= recovery_limit) {
                if (options.official && phase == LobbyPhase::Lobby && !session.snapshot.game.started) {
                    SaveOfficialGameId({});
                    session.FinishOfficialRecovery();
                    reconnect_view.reset();
                    resuming = false;
                    recovery_start = 0;
                    Notify();
                } else throw TransportFailure("Game recovery snapshot timed out");
            }
            if (!options.origin_lobby && phase == LobbyPhase::Matching && now - phase_start >= options.matching_ms)
                throw std::runtime_error("Matchmaking timed out; connect explicitly to retry");
            const bool established = phase == LobbyPhase::Lobby || phase == LobbyPhase::Game || phase ==
                                     LobbyPhase::MatchComplete || phase == LobbyPhase::Matching;
            if (!established && now - phase_start >= recovery_limit) throw TransportFailure(
                "Connection/handshake or recovery timed out");
            if (established && now - last_receive >= options.idle_ms) throw
                    TransportFailure("Server receive timed out");
            const auto action_timeout = std::max<ULONGLONG>(options.handshake_ms,
                                                            session.snapshot.game.local_action_wait_ms
                                                                ? session.snapshot.game.local_action_wait_ms + 10000ULL
                                                                : 0);
            if (action_start && now - action_start >= action_timeout) throw TransportFailure(
                "Game action confirmation timed out");
            if (frame_start && now - frame_start >= options.handshake_ms) throw TransportFailure(
                "Incomplete network message timed out");
            if (send_start && now - send_start >= options.handshake_ms) throw TransportFailure(
                "Network send timed out");
            if ((phase == LobbyPhase::AwaitHelo || phase == LobbyPhase::AwaitLobby || phase == LobbyPhase::AwaitRecovery
                 || established) && now - last_heartbeat >= options.heartbeat_ms) {
                Queue("<Z />");
                if (options.official && (phase == LobbyPhase::Lobby || phase == LobbyPhase::Matching))
                    Queue(session.OfficialProfilePoll());
                last_heartbeat = now;
                Notify();
            }
        }

        void SubmitLobby(std::uint64_t revision, const std::string &command, unsigned value) {
            if (reconnect_view) return;
            const auto phase = session.snapshot.phase;
            if (options.official && command == "return" && phase == LobbyPhase::MatchComplete) {
                (void) session.SubmitLobby(revision, command, value);
                if (!official_game_id.empty()) SaveOfficialGameId({});
                // Origin tenhou.exe.c:62821-62826 & 62067-62075: after owari closed WC_CLIENT,
                // END posts CONNECT to open a fresh TCP socket and sends <Z /> + <HELO .../>.
                const auto request = options;
                Connect(request, false);
                return;
            }
            const auto replies = session.SubmitLobby(revision, command, value);
            if (replies.empty() && phase != session.snapshot.phase) Phase(session.snapshot.phase);
            SendLobbyReplies(replies);
        }

        void SendLobbyReplies(const std::vector<std::string> &replies) {
            if (replies.empty()) return;
            try {
                Phase(session.snapshot.phase);
                for (const auto &reply: replies) Queue(reply);
                Notify();
            } catch (const TransportFailure &error) { Error(error.what(), true); } catch (const std::exception &error) {
                Error(error.what());
            }
        }

        static LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
            auto *self = reinterpret_cast<Impl *>(GetWindowLongPtrW(window, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<Impl *>(reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams);
                SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (!self) return DefWindowProcW(window, message, wparam, lparam);
            try {
                if (message == kDns && reinterpret_cast<HANDLE>(wparam) == self->dns) {
                    self->dns = nullptr;
                    if (WSAGETASYNCERROR(lparam))
                        throw TransportFailure(SocketError("DNS", WSAGETASYNCERROR(lparam)).what());
                    const auto *entry = reinterpret_cast<const hostent *>(self->host.data());
                    if (entry->h_addrtype != AF_INET || entry->h_length != sizeof(IN_ADDR) || !entry->h_addr_list || !
                        entry->h_addr_list[0])
                        throw std::runtime_error("DNS returned no IPv4 address");
                    IN_ADDR address{};
                    memcpy(&address, entry->h_addr_list[0], sizeof(address));
                    self->BeginTcp(address);
                    return 0;
                }
                if (message == kSocket && static_cast<SOCKET>(wparam) == self->socket) {
                    const int event = WSAGETSELECTEVENT(lparam), error = WSAGETSELECTERROR(lparam);
                    if (error) throw TransportFailure(SocketError("socket event", error).what());
                    if (event == FD_CONNECT) self->Connected();
                    else if (event == FD_READ || event == FD_CLOSE) self->Read();
                    else if (event == FD_WRITE) self->Flush();
                    return 0;
                }
                if (message == kRead && self->socket != INVALID_SOCKET) {
                    self->Read();
                    return 0;
                }
                if (message == WM_TIMER) {
                    self->Tick();
                    return 0;
                }
            } catch (const TransportFailure &error) {
                self->Error(error.what(), true);
                return 0;
            } catch (const ServerRejection &error) {
                self->Error(error.what(), false, error.code, error.wire_text, true);
                return 0;
            } catch (const std::exception &error) {
                self->Error(error.what());
                return 0;
            }
            return DefWindowProcW(window, message, wparam, lparam);
        }
    };

    NetworkClient::NetworkClient(Observer changed, std::function<void(const std::string &)> log, EventObserver event)
        : impl_(std::make_unique<Impl>(std::move(changed), std::move(log), std::move(event))) {
    }

    NetworkClient::~NetworkClient() = default;

    void NetworkClient::Connect(const ConnectionOptions &options) { impl_->Connect(options); }
    void NetworkClient::Disconnect() { impl_->Disconnect(); }
    void NetworkClient::Reconnect(const ConnectionOptions &options) { impl_->Reconnect(options); }

    void NetworkClient::SubmitAction(std::uint64_t revision, std::size_t index) {
        impl_->SubmitAction(revision, index);
    }

    void NetworkClient::SubmitLobby(std::uint64_t revision, const std::string &command, unsigned value) {
        impl_->SubmitLobby(revision, command, value);
    }

    void NetworkClient::SubmitOriginLobby(const std::string &command, const std::wstring &value) {
        impl_->SendLobbyReplies(impl_->session.SubmitOriginLobby(command, value));
    }

    void NetworkClient::ConfigureRoom(std::uint64_t revision, const std::map<std::string, std::string> &settings) {
        const auto replies = impl_->session.ConfigureRoom(revision, settings);
        for (const auto &reply: replies) impl_->Queue(reply);
    }

    void NetworkClient::NativeRequest(const std::string &command, unsigned value,
                                      const std::map<std::string, std::string> &settings) {
        const auto replies = impl_->session.NativeRequest(command, value, settings);
        for (const auto &reply: replies) impl_->Queue(reply);
        impl_->Notify();
    }

    void NetworkClient::ConfirmMatchStart() {
        if (impl_->local_prepare && impl_->Active()) {
            impl_->local_prepare = false;
            if (impl_->options.official) impl_->action_start = GetTickCount64();
            try { impl_->Queue(impl_->options.official ? "<NEXTREADY />" : "<STARTREADY />"); }
            catch (const TransportFailure &error) { impl_->Error(error.what(), true); }
        }
    }

    void NetworkClient::ExitGame(int choice) {
        const auto replies = impl_->session.ExitGame(choice);
        if (impl_->options.official && !replies.empty()) {
            impl_->local_prepare = false;
            impl_->action_start = 0;
            if (!impl_->official_game_id.empty()) impl_->SaveOfficialGameId({});
            impl_->SendLobbyReplies(replies);
        } else {
            for (const auto &reply: replies) impl_->Queue(reply);
            impl_->Notify();
        }
    }

    const LobbySnapshot &NetworkClient::snapshot() const { return impl_->Snapshot(); }
    bool NetworkClient::active() const { return impl_->Active(); }
} // namespace tenhou
