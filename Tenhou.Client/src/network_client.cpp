// Independent I/O thread TCP transport (getaddrinfo + non-blocking socket + WSAPoll).
// Winsock must precede windows.h. All protocol callbacks are marshaled onto the owning thread.
#include <winsock2.h>
#include <ws2tcpip.h>
#include "network_client.hpp"
#include "ini_store.hpp"
#include "monotonic_clock.hpp"
#include "replay_archive.hpp"
#include "win32_utf.hpp"
#include <windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cmath>
#include <deque>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace tenhou {
    namespace {
        constexpr UINT kWake = WM_APP + 31;
        constexpr wchar_t kNetworkWindow[] = L"TENHOU_REBUILD_NETWORK";
        constexpr std::size_t kMaxSendQueueBytes = 131072;

        struct TransportFailure : std::runtime_error {
            using std::runtime_error::runtime_error;
        };

        std::runtime_error SocketError(const char *action, const int code = WSAGetLastError()) {
            return std::runtime_error(std::string(action) + " failed; Winsock error=" + std::to_string(code));
        }

        // Process-wide Winsock lifetime so a detached blocking getaddrinfo call never races WSACleanup.
        struct WinsockRuntime {
            WinsockRuntime() {
                WSADATA data{};
                const int err = WSAStartup(MAKEWORD(2, 2), &data);
                if (err) throw SocketError("WSAStartup", err);
            }
            ~WinsockRuntime() { WSACleanup(); }
        };

        void EnsureWinsock() {
            static WinsockRuntime runtime;
        }

        bool IsLoopbackAddress(const sockaddr *addr) {
            if (!addr) return false;
            if (addr->sa_family == AF_INET) {
                const auto *in4 = reinterpret_cast<const sockaddr_in *>(addr);
                return (ntohl(in4->sin_addr.S_un.S_addr) >> 24) == 127;
            }
            if (addr->sa_family == AF_INET6) {
                const auto *in6 = reinterpret_cast<const sockaddr_in6 *>(addr);
                return IN6_IS_ADDR_LOOPBACK(&in6->sin6_addr) != 0;
            }
            return false;
        }

        SOCKET CreateWakeSocket() {
            SOCKET sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sock == INVALID_SOCKET) return INVALID_SOCKET;
            u_long nonblock = 1;
            ioctlsocket(sock, FIONBIO, &nonblock);
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.sin_port = 0;
            if (bind(sock, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR) {
                closesocket(sock);
                return INVALID_SOCKET;
            }
            int len = sizeof(addr);
            if (getsockname(sock, reinterpret_cast<sockaddr *>(&addr), &len) == SOCKET_ERROR ||
                ::connect(sock, reinterpret_cast<const sockaddr *>(&addr), len) == SOCKET_ERROR) {
                closesocket(sock);
                return INVALID_SOCKET;
            }
            return sock;
        }

        struct WorkerEvent {
            enum class Kind { Connecting, Connected, Bytes, SendDrained, Closed, TransportError, FatalError, Tick };
            Kind kind = Kind::Tick;
            std::uint64_t generation = 0;
            std::string data;
        };

        struct NotifyHub {
            std::mutex mutex;
            HWND window = nullptr;
            bool wake_posted = false;
            std::deque<WorkerEvent> events;

            void Post(WorkerEvent ev) {
                HWND target = nullptr;
                {
                    std::lock_guard lock(mutex);
                    if (!window) return;
                    if (ev.kind == WorkerEvent::Kind::Tick && !events.empty() &&
                        events.back().kind == WorkerEvent::Kind::Tick &&
                        events.back().generation == ev.generation) {
                        return;
                    }
                    events.push_back(std::move(ev));
                    if (!wake_posted) {
                        wake_posted = true;
                        target = window;
                    }
                }
                if (target) PostMessageW(target, kWake, 0, 0);
            }

            std::deque<WorkerEvent> Drain() {
                std::lock_guard lock(mutex);
                wake_posted = false;
                std::deque<WorkerEvent> out;
                out.swap(events);
                return out;
            }

            void DetachWindow() {
                std::lock_guard lock(mutex);
                window = nullptr;
                events.clear();
                wake_posted = false;
            }
        };

        struct Worker {
            std::uint64_t generation = 0;
            std::shared_ptr<NotifyHub> hub;
            std::atomic<bool> stop{false};
            std::atomic<bool> in_dns{false};

            std::mutex out_mutex;
            std::string outgoing;
            std::size_t sent_offset = 0;
            SOCKET wake_sock = INVALID_SOCKET;
            std::thread thread;

            Worker(const std::uint64_t gen, std::shared_ptr<NotifyHub> h)
                : generation(gen), hub(std::move(h)), wake_sock(CreateWakeSocket()) {}

            ~Worker() { Stop(); }

            void Wake() {
                SOCKET s = INVALID_SOCKET;
                {
                    std::lock_guard lock(out_mutex);
                    s = wake_sock;
                }
                if (s != INVALID_SOCKET) {
                    const char byte = 'w';
                    ::send(s, &byte, 1, 0);
                }
            }

            void Enqueue(const std::string &wire) {
                {
                    std::lock_guard lock(out_mutex);
                    if (outgoing.size() - sent_offset + wire.size() + 1 > kMaxSendQueueBytes)
                        throw std::runtime_error("Network send queue exceeds limit");
                    if (sent_offset) {
                        outgoing.erase(0, sent_offset);
                        sent_offset = 0;
                    }
                    outgoing += wire;
                    outgoing += '\0';
                }
                Wake();
            }

            void Stop() noexcept {
                stop.store(true, std::memory_order_release);
                Wake();
                if (thread.joinable()) {
                    if (in_dns.load(std::memory_order_acquire)) thread.detach();
                    else thread.join();
                }
                std::lock_guard lock(out_mutex);
                if (wake_sock != INVALID_SOCKET) {
                    closesocket(wake_sock);
                    wake_sock = INVALID_SOCKET;
                }
            }
        };

        void DrainWake(const SOCKET wake_sock) {
            if (wake_sock == INVALID_SOCKET) return;
            char sink[64];
            while (::recv(wake_sock, sink, sizeof(sink), 0) > 0) {}
        }

        void RunTimerOnlyWorker(std::shared_ptr<Worker> worker) {
            while (!worker->stop.load(std::memory_order_acquire)) {
                if (worker->wake_sock != INVALID_SOCKET) {
                    WSAPOLLFD pfd{};
                    pfd.fd = worker->wake_sock;
                    pfd.events = POLLIN;
                    WSAPoll(&pfd, 1, 50);
                    if (pfd.revents & POLLIN) DrainWake(worker->wake_sock);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                if (!worker->stop.load(std::memory_order_acquire)) {
                    worker->hub->Post({WorkerEvent::Kind::Tick, worker->generation, {}});
                }
            }
        }

        void RunTcpWorker(std::shared_ptr<Worker> worker, std::string host, const unsigned port, const bool require_loopback) {
            SOCKET sock = INVALID_SOCKET;
            auto cleanup = [&]() noexcept {
                if (sock != INVALID_SOCKET) {
                    shutdown(sock, SD_BOTH);
                    closesocket(sock);
                    sock = INVALID_SOCKET;
                }
            };
            try {
                // 1. Resolve host via getaddrinfo (supports IPv4 & IPv6).
                addrinfo hints{};
                hints.ai_family = AF_UNSPEC;
                hints.ai_socktype = SOCK_STREAM;
                hints.ai_protocol = IPPROTO_TCP;
                addrinfo *results = nullptr;
                const std::string port_str = std::to_string(port);

                worker->in_dns.store(true, std::memory_order_release);
                const int gai = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &results);
                worker->in_dns.store(false, std::memory_order_release);
                if (worker->stop.load(std::memory_order_acquire)) {
                    if (results) freeaddrinfo(results);
                    return;
                }
                if (gai != 0 || !results) {
                    if (results) freeaddrinfo(results);
                    throw TransportFailure("DNS failed; getaddrinfo error=" + std::to_string(gai));
                }

                // Prefer IPv4 when multiple records exist, matching legacy server expectations.
                addrinfo *chosen = nullptr;
                for (auto *it = results; it; it = it->ai_next) {
                    if (it->ai_family == AF_INET) {
                        chosen = it;
                        break;
                    }
                }
                if (!chosen) chosen = results;
                if (require_loopback && !IsLoopbackAddress(chosen->ai_addr)) {
                    freeaddrinfo(results);
                    throw std::runtime_error("Interactive DNS resolved outside loopback");
                }

                sock = ::socket(chosen->ai_family, chosen->ai_socktype, chosen->ai_protocol);
                if (sock == INVALID_SOCKET) {
                    const int err = WSAGetLastError();
                    freeaddrinfo(results);
                    throw SocketError("socket", err);
                }
                u_long nonblock = 1;
                ioctlsocket(sock, FIONBIO, &nonblock);
                const BOOL nodelay = TRUE;
                setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&nodelay), sizeof(nodelay));

                worker->hub->Post({WorkerEvent::Kind::Connecting, worker->generation, {}});
                const int cr = ::connect(sock, chosen->ai_addr, static_cast<int>(chosen->ai_addrlen));
                const int connect_err = cr == SOCKET_ERROR ? WSAGetLastError() : 0;
                freeaddrinfo(results);

                if (cr == SOCKET_ERROR && connect_err != WSAEWOULDBLOCK) {
                    throw TransportFailure(SocketError("connect", connect_err).what());
                }

                // 2. Wait for non-blocking TCP connect completion via WSAPoll.
                if (cr == SOCKET_ERROR) {
                    while (!worker->stop.load(std::memory_order_acquire)) {
                        WSAPOLLFD pfds[2]{};
                        ULONG count = 1;
                        pfds[0].fd = sock;
                        pfds[0].events = POLLOUT;
                        if (worker->wake_sock != INVALID_SOCKET) {
                            pfds[1].fd = worker->wake_sock;
                            pfds[1].events = POLLIN;
                            count = 2;
                        }
                        const int pr = WSAPoll(pfds, count, 50);
                        if (pr == SOCKET_ERROR) throw TransportFailure(SocketError("WSAPoll connect").what());
                        if (count == 2 && (pfds[1].revents & POLLIN)) DrainWake(worker->wake_sock);
                        if (worker->stop.load(std::memory_order_acquire)) {
                            cleanup();
                            return;
                        }
                        if (pfds[0].revents & (POLLOUT | POLLERR | POLLHUP)) {
                            int so_error = 0;
                            int opt_len = sizeof(so_error);
                            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&so_error), &opt_len) ==
                                SOCKET_ERROR) {
                                throw TransportFailure(SocketError("getsockopt SO_ERROR").what());
                            }
                            if (so_error != 0) throw TransportFailure(SocketError("connect", so_error).what());
                            break;
                        }
                        worker->hub->Post({WorkerEvent::Kind::Tick, worker->generation, {}});
                    }
                }
                if (worker->stop.load(std::memory_order_acquire)) {
                    cleanup();
                    return;
                }
                worker->hub->Post({WorkerEvent::Kind::Connected, worker->generation, {}});

                // 3. Steady-state non-blocking WSAPoll I/O loop.
                std::uint64_t last_tick = MonotonicMs();
                std::array<char, 8192> recv_buf{};
                while (!worker->stop.load(std::memory_order_acquire)) {
                    bool has_outgoing = false;
                    {
                        std::lock_guard lock(worker->out_mutex);
                        has_outgoing = worker->sent_offset < worker->outgoing.size();
                    }
                    WSAPOLLFD pfds[2]{};
                    ULONG count = 1;
                    pfds[0].fd = sock;
                    pfds[0].events = static_cast<SHORT>(POLLIN | (has_outgoing ? POLLOUT : 0));
                    if (worker->wake_sock != INVALID_SOCKET) {
                        pfds[1].fd = worker->wake_sock;
                        pfds[1].events = POLLIN;
                        count = 2;
                    }
                    const int pr = WSAPoll(pfds, count, 50);
                    if (pr == SOCKET_ERROR) throw TransportFailure(SocketError("WSAPoll").what());
                    if (count == 2 && (pfds[1].revents & POLLIN)) DrainWake(worker->wake_sock);
                    if (worker->stop.load(std::memory_order_acquire)) break;

                    // Flush outgoing bytes.
                    if (has_outgoing && (pfds[0].revents & POLLOUT || pr >= 0)) {
                        bool drained = false;
                        while (!worker->stop.load(std::memory_order_acquire)) {
                            std::string chunk;
                            {
                                std::lock_guard lock(worker->out_mutex);
                                if (worker->sent_offset >= worker->outgoing.size()) {
                                    worker->outgoing.clear();
                                    worker->sent_offset = 0;
                                    drained = true;
                                    break;
                                }
                                const std::size_t rem = worker->outgoing.size() - worker->sent_offset;
                                chunk.assign(worker->outgoing.data() + worker->sent_offset,
                                             std::min<std::size_t>(rem, 16384));
                            }
                            const int sent = ::send(sock, chunk.data(), static_cast<int>(chunk.size()), 0);
                            if (sent == SOCKET_ERROR) {
                                const int err = WSAGetLastError();
                                if (err == WSAEWOULDBLOCK) break;
                                throw TransportFailure(SocketError("send", err).what());
                            }
                            if (sent == 0) throw std::runtime_error("Network send made no progress");
                            {
                                std::lock_guard lock(worker->out_mutex);
                                worker->sent_offset += static_cast<std::size_t>(sent);
                                if (worker->sent_offset >= worker->outgoing.size()) {
                                    worker->outgoing.clear();
                                    worker->sent_offset = 0;
                                    drained = true;
                                    break;
                                }
                            }
                        }
                        if (drained) {
                            worker->hub->Post({WorkerEvent::Kind::SendDrained, worker->generation, {}});
                        }
                    }

                    // Drain incoming bytes.
                    if (pfds[0].revents & (POLLIN | POLLHUP | POLLERR)) {
                        std::string batch;
                        bool remote_closed = false;
                        for (int iter = 0; iter < 8; ++iter) {
                            const int n = ::recv(sock, recv_buf.data(), static_cast<int>(recv_buf.size()), 0);
                            if (n > 0) {
                                batch.append(recv_buf.data(), static_cast<std::size_t>(n));
                                if (n < static_cast<int>(recv_buf.size())) break;
                            } else if (n == 0) {
                                remote_closed = true;
                                break;
                            } else {
                                const int err = WSAGetLastError();
                                if (err == WSAEWOULDBLOCK) break;
                                if (!batch.empty()) {
                                    worker->hub->Post({WorkerEvent::Kind::Bytes, worker->generation, std::move(batch)});
                                    batch.clear();
                                }
                                throw TransportFailure(SocketError("recv", err).what());
                            }
                        }
                        if (!batch.empty()) {
                            worker->hub->Post({WorkerEvent::Kind::Bytes, worker->generation, std::move(batch)});
                        }
                        if (remote_closed) {
                            worker->hub->Post({WorkerEvent::Kind::Closed, worker->generation, {}});
                            break;
                        }
                    }

                    const auto now = MonotonicMs();
                    if (now - last_tick >= 50) {
                        last_tick = now;
                        worker->hub->Post({WorkerEvent::Kind::Tick, worker->generation, {}});
                    }
                }
            } catch (const TransportFailure &error) {
                if (!worker->stop.load(std::memory_order_acquire))
                    worker->hub->Post({WorkerEvent::Kind::TransportError, worker->generation, error.what()});
            } catch (const std::exception &error) {
                if (!worker->stop.load(std::memory_order_acquire))
                    worker->hub->Post({WorkerEvent::Kind::FatalError, worker->generation, error.what()});
            }
            cleanup();
        }
    } // namespace

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

        HWND notify_window = nullptr;
        std::shared_ptr<NotifyHub> hub;
        std::shared_ptr<Worker> worker;
        std::uint64_t generation = 0;
        bool has_pending_send = false;

        std::uint64_t revision_floor = 0;
        std::uint64_t server_error_sequence = 0;
        std::optional<LobbySnapshot> reconnect_view;
        std::uint64_t retry_at = 0, retry_until = 0;
        unsigned retry_delay = 1000;
        bool resuming = false;
        bool local_prepare = false;
        std::uint64_t action_start = 0;
        std::uint64_t native_request_start = 0;
        std::uint64_t recovery_start = 0;
        std::uint64_t phase_start = 0, last_receive = 0, last_heartbeat = 0, frame_start = 0, send_start = 0;

        Impl(Observer observer, std::function<void(const std::string &)> logger, EventObserver event)
            : changed(std::move(observer)), accepted_event(std::move(event)), log(std::move(logger)),
              hub(std::make_shared<NotifyHub>()) {
            EnsureWinsock();
            WNDCLASSEXW wc{sizeof(wc)};
            wc.lpfnWndProc = Procedure;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = kNetworkWindow;
            if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
                throw std::runtime_error("Cannot register network wake window");
            }
        }

        ~Impl() { Close(); }

        bool Active() const {
            const auto phase = session.snapshot.phase;
            return phase != LobbyPhase::Error && phase != LobbyPhase::Disconnected;
        }

        const LobbySnapshot &Snapshot() const { return reconnect_view ? *reconnect_view : session.snapshot; }

        void Notify() const { changed(Snapshot()); }

        void Phase(const LobbyPhase phase) {
            session.snapshot.phase = phase;
            phase_start = MonotonicMs();
            log(std::string("NETWORK state=") + LobbyPhaseName(phase));
            Notify();
        }

        void Close() noexcept {
            ++generation;
            if (worker) {
                worker->Stop();
                worker.reset();
            }
            if (hub) hub->DetachWindow();
            if (notify_window) {
                DestroyWindow(notify_window);
                notify_window = nullptr;
            }
            has_pending_send = false;
            framer = {};
            frame_start = 0;
            send_start = 0;
            local_prepare = false;
            action_start = 0;
            native_request_start = 0;
        }

        void OpenNotifyWindow() {
            notify_window = CreateWindowExW(0, kNetworkWindow, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                            GetModuleHandleW(nullptr), this);
            if (!notify_window) throw std::runtime_error("Cannot create network wake window");
            std::lock_guard lock(hub->mutex);
            hub->window = notify_window;
            hub->events.clear();
            hub->wake_posted = false;
        }

        bool ScheduleReconnect() {
            const auto now = MonotonicMs();
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
            try {
                OpenNotifyWindow();
                auto w = std::make_shared<Worker>(generation, hub);
                w->thread = std::thread(RunTimerOnlyWorker, w);
                worker = std::move(w);
            } catch (...) {
                return false;
            }
            Notify();
            return true;
        }

        void Error(const std::string &error, const bool transport = false, const int server_code = -1,
                   const std::string &server_text = {}, const bool server_rejected = false) {
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
                const auto type = ReadIniInt(options.config_file, "REBUILD", "ProfileMonthly",
                                             static_cast<int>(kDefaultMonthlyProfile));
                if (ValidMonthlyProfile(static_cast<unsigned>(type)))
                    session.preferred_monthly_profile = static_cast<unsigned>(type);
            }
            try {
                if (options.native_service && native_token.empty() && !options.session_file.empty() &&
                    std::filesystem::exists(options.session_file)) {
                    std::ifstream file(options.session_file, std::ios::binary);
                    native_token.assign(std::istreambuf_iterator<char>(file), {});
                    if (native_token.size() != 64 || !std::all_of(native_token.begin(), native_token.end(),
                        [](const char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }))
                        native_token.clear(); // A damaged recovery hint must not block login.
                }
                if (options.official && !resume && !options.session_file.empty()) {
                    const bool same_process = !official_game_id.empty();
                    official_game_id = ReadIniString(options.config_file, "OfficialRecovery",
                                                     PathToUtf8(options.session_file.filename()));
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
                if (options.matching_ms < 100 || options.matching_ms > 600000)
                    throw std::runtime_error("Invalid matchmaking timeout");
                // Validate before spawning the I/O thread.
                LobbySession validation;
                validation.require_native = options.native_service;
                (void) validation.Connected(options.identity, options.gender);
                if (options.host.empty() || options.host.size() > 253 || options.host.front() == '-' ||
                    !std::all_of(options.host.begin(), options.host.end(), [](const unsigned char c) {
                        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' ||
                               c == '.' || c == ':';
                    }) || options.port < 1 || options.port > 65535)
                    throw std::runtime_error("Invalid host or port (1..65535)");
                if (options.handshake_ms < 100 || options.handshake_ms > 60000 || options.idle_ms < 100 ||
                    options.idle_ms > 600000 || options.heartbeat_ms < 50 || options.heartbeat_ms > 60000)
                    throw std::runtime_error("Invalid connection timing");

                OpenNotifyWindow();
                last_receive = last_heartbeat = MonotonicMs();
                Phase(LobbyPhase::Resolving);

                const bool require_loopback = !options.native_service && !options.official &&
                                              (options.interactive_games || options.recover_games);
                auto w = std::make_shared<Worker>(generation, hub);
                w->thread = std::thread(RunTcpWorker, w, options.host, options.port, require_loopback);
                worker = std::move(w);
            } catch (const TransportFailure &error) { Error(error.what(), true); }
            catch (const std::exception &error) { Error(error.what()); }
        }

        static bool ValidGameId(const std::string &value) {
            return !value.empty() && value.size() < 80 && std::all_of(value.begin(), value.end(), [](const unsigned char c) {
                return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                       c == '_' || c == '-';
            });
        }

        void SaveOfficialGameId(std::string value) {
            if (!value.empty() && !ValidGameId(value)) return;
            official_game_id = std::move(value);
            if (options.session_file.empty()) return;
            const auto key = PathToUtf8(options.session_file.filename());
            try {
                if (official_game_id.empty()) WriteIniString(options.config_file, "OfficialRecovery", key, std::nullopt);
                else WriteIniString(options.config_file, "OfficialRecovery", key, official_game_id);
            } catch (...) {
                log("NETWORK unable to save official recovery hint");
            }
        }

        void AccumulatePrivateProfile(const NetworkMessage &message) const
        {
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
            const std::string_view key = (session.snapshot.game.type & 16) ? "PFPV3" : "PFPV4";
            const auto existing = ReadIniString(options.config_file, "STATUS", key);
            std::array<double, 11> total{};
            parse11(existing, total);
            for (std::size_t i = 0; i < total.size(); ++i) total[i] += delta[i];
            std::ostringstream out;
            out.imbue(std::locale::classic());
            out << std::fixed << std::setprecision(1) << total[0];
            for (std::size_t i = 1; i < total.size(); ++i) out << ',' << static_cast<long long>(std::llround(total[i]));
            WriteIniString(options.config_file, "STATUS", key, out.str());
        }

        void Reconnect(const ConnectionOptions &request) {
            if (Active() || !session.snapshot.reconnect_available)
                throw std::runtime_error("No recoverable disconnected game");
            if (!request.recover_games || request.host != options.host || request.port != options.port ||
                request.gender != options.gender || request.observe_games != options.observe_games ||
                request.interactive_games != options.interactive_games || request.match_games != options.match_games ||
                request.origin_lobby != options.origin_lobby)
                throw std::runtime_error("Reconnect requires the same server and game mode");
            Connect(request, true);
        }

        void Connected() {
            if (session.snapshot.phase != LobbyPhase::Connecting) return;
            const auto messages = session.Connected(options.identity, options.gender, revision_floor);
            recovery_start = resuming ? MonotonicMs() : 0;
            if (!options.auto_reconnect && !options.official) options.identity.clear();
            last_receive = last_heartbeat = MonotonicMs();
            Phase(LobbyPhase::AwaitHelo);
            // Origin tenhou.exe.c:62067 transmits <Z /> immediately on FD_CONNECT before <HELO .../>.
            if (options.official) Queue("<Z />");
            for (const auto &message : messages) Queue(message);
        }

        void Queue(const std::string &message) {
            if (!worker) throw TransportFailure("Cannot send without active connection");
            std::string wire = message;
            const auto tag = ParseNetworkMessage(message).tag;
            if (session.snapshot.native_service && (tag == "D" || tag == "N" || tag == "REACH"))
                wire.insert(wire.rfind('/'), " window=\"" + std::to_string(session.snapshot.native_window) + "\" ");
            if (!has_pending_send) {
                send_start = MonotonicMs();
                has_pending_send = true;
            }
            worker->Enqueue(wire);
            ++session.snapshot.sent;
            log("NETWORK send tag=" + tag);
        }

        void ProcessBytes(const std::string &bytes, const std::uint64_t ev_gen) {
            const bool was_pending = framer.pending();
            const auto messages = framer.Feed(bytes.data(), bytes.size());
            if (framer.pending()) {
                if (!was_pending || !messages.empty()) frame_start = MonotonicMs();
            } else frame_start = 0;

            bool close_after_owari = false;
            for (const auto &text : messages) {
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
                        auto temporary = options.session_file;
                        temporary += L".tmp";
                        {
                            std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
                            if (!file) throw std::runtime_error("Cannot open platform session temporary file");
                            file.write(native_token.data(), static_cast<std::streamsize>(native_token.size()));
                            file.flush();
                            if (!file) throw std::runtime_error("Cannot write platform session temporary file");
                        }
                        std::error_code ec;
                        std::filesystem::rename(temporary, options.session_file, ec);
                        if (ec) {
                            std::filesystem::remove(options.session_file, ec);
                            ec.clear();
                            std::filesystem::rename(temporary, options.session_file, ec);
                            if (ec) throw std::runtime_error("Cannot atomically save native guest session");
                        }
                    }
                }
                if (message.tag == "GO" || message.tag == "REINIT") local_prepare = false;
                if (message.tag == "PREPARE") local_prepare = true;
                if (message.tag == "START") local_prepare = false;
                const bool official_intro = options.official && options.presentation &&
                                            message.tag == "TAIKYOKU" && !replies.empty();
                if (official_intro) local_prepare = true;
                if (options.official && session.snapshot.recovering && !recovery_start)
                    recovery_start = MonotonicMs();
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
                if (ev_gen != generation || !worker) return;
                if (session.snapshot.actions.pending.empty() || session.snapshot.table_paused ||
                    (options.official && local_prepare)) action_start = 0;
                else if (!action_start) action_start = MonotonicMs();
                last_receive = MonotonicMs();
                log("NETWORK recv tag=" + message.tag);
                if (old_phase != session.snapshot.phase) Phase(session.snapshot.phase);
                if (!official_intro) for (const auto &reply : replies) Queue(reply);
                // Opt in only to the identified local rules service. The
                // original remote protocol and its GOK handshake stay intact.
                if (message.tag == "HELO" && message.attributes.count("uname") &&
                    message.attributes.at("uname") == "LOCAL-RULES" &&
                    options.interactive_games && options.presentation)
                    Queue("<PRESENTATION />");
                Notify();
            }
            if (close_after_owari) Close();
        }

        void SubmitAction(const std::uint64_t revision, const std::size_t index) {
            // Invalid/stale UI requests are rejected without tearing down a healthy connection.
            const auto replies = session.SubmitAction(revision, index);
            try {
                action_start = MonotonicMs();
                for (const auto &reply : replies) Queue(reply);
                Notify();
            } catch (const TransportFailure &error) { Error(error.what(), true); }
            catch (const std::exception &error) { Error(error.what()); }
        }

        void Tick() {
            const auto now = MonotonicMs();
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
            if (session.snapshot.native_pending.empty() && session.snapshot.origin_pending.empty())
                native_request_start = 0;
            else if (!native_request_start) native_request_start = now;
            else if (now - native_request_start >= options.handshake_ms)
                throw TransportFailure("Native room request confirmation timed out");
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
            const bool established = phase == LobbyPhase::Lobby || phase == LobbyPhase::Game ||
                                     phase == LobbyPhase::MatchComplete || phase == LobbyPhase::Matching;
            if (!established && now - phase_start >= recovery_limit)
                throw TransportFailure("Connection/handshake or recovery timed out");
            if (established && now - last_receive >= options.idle_ms)
                throw TransportFailure("Server receive timed out");
            const auto action_timeout = std::max<std::uint64_t>(
                options.handshake_ms,
                session.snapshot.game.local_action_wait_ms
                    ? session.snapshot.game.local_action_wait_ms + 10000ULL
                    : 0);
            if (action_start && now - action_start >= action_timeout)
                throw TransportFailure("Game action confirmation timed out");
            if (frame_start && now - frame_start >= options.handshake_ms)
                throw TransportFailure("Incomplete network message timed out");
            if (send_start && now - send_start >= options.handshake_ms)
                throw TransportFailure("Network send timed out");
            if ((phase == LobbyPhase::AwaitHelo || phase == LobbyPhase::AwaitLobby || phase == LobbyPhase::AwaitRecovery
                 || established) && now - last_heartbeat >= options.heartbeat_ms) {
                Queue("<Z />");
                if (options.official && (phase == LobbyPhase::Lobby || phase == LobbyPhase::Matching))
                    Queue(session.OfficialProfilePoll());
                last_heartbeat = now;
                Notify();
            }
        }

        void SubmitLobby(const std::uint64_t revision, const std::string &command, const unsigned value) {
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
                for (const auto &reply : replies) Queue(reply);
                Notify();
            } catch (const TransportFailure &error) { Error(error.what(), true); }
            catch (const std::exception &error) { Error(error.what()); }
        }

        void DrainEvents() {
            auto events = hub->Drain();
            for (const auto &ev : events) {
                if (ev.generation != generation) continue;
                try {
                    switch (ev.kind) {
                        case WorkerEvent::Kind::Connecting:
                            Phase(LobbyPhase::Connecting);
                            break;
                        case WorkerEvent::Kind::Connected:
                            Connected();
                            break;
                        case WorkerEvent::Kind::Bytes:
                            ProcessBytes(ev.data, ev.generation);
                            break;
                        case WorkerEvent::Kind::SendDrained:
                            has_pending_send = false;
                            send_start = 0;
                            break;
                        case WorkerEvent::Kind::Closed:
                            if (framer.pending()) throw TransportFailure("Connection closed with truncated message");
                            throw TransportFailure("Server closed connection");
                        case WorkerEvent::Kind::TransportError:
                            throw TransportFailure(ev.data);
                        case WorkerEvent::Kind::FatalError:
                            throw std::runtime_error(ev.data);
                        case WorkerEvent::Kind::Tick:
                            Tick();
                            break;
                    }
                } catch (const TransportFailure &error) {
                    Error(error.what(), true);
                } catch (const ServerRejection &error) {
                    Error(error.what(), false, error.code, error.wire_text, true);
                } catch (const std::exception &error) {
                    Error(error.what());
                }
            }
        }

        static LRESULT CALLBACK Procedure(const HWND window, const UINT message, const WPARAM wparam, const LPARAM lparam) {
            auto *self = reinterpret_cast<Impl *>(GetWindowLongPtrW(window, GWLP_USERDATA));
            if (message == WM_NCCREATE) {
                self = static_cast<Impl *>(reinterpret_cast<CREATESTRUCTW *>(lparam)->lpCreateParams);
                SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            if (self && message == kWake) {
                self->DrainEvents();
                return 0;
            }
            return DefWindowProcW(window, message, wparam, lparam);
        }
    };

    NetworkClient::NetworkClient(Observer changed, std::function<void(const std::string &)> log, EventObserver event)
        : impl_(std::make_unique<Impl>(std::move(changed), std::move(log), std::move(event))) {}

    NetworkClient::~NetworkClient() = default;

    void NetworkClient::Connect(const ConnectionOptions &options) const { impl_->Connect(options); }
    void NetworkClient::Disconnect() const { impl_->Disconnect(); }
    void NetworkClient::Reconnect(const ConnectionOptions &options) const { impl_->Reconnect(options); }

    void NetworkClient::SubmitAction(const std::uint64_t revision, const std::size_t index) const
    {
        impl_->SubmitAction(revision, index);
    }

    void NetworkClient::SubmitLobby(const std::uint64_t revision, const std::string &command, const unsigned value) const
    {
        impl_->SubmitLobby(revision, command, value);
    }

    void NetworkClient::SubmitOriginLobby(const std::string &command, const std::string &value) const
    {
        impl_->SendLobbyReplies(impl_->session.SubmitOriginLobby(command, value));
    }

    void NetworkClient::ConfigureRoom(const std::uint64_t revision, const std::map<std::string, std::string> &settings) const
    {
        const auto replies = impl_->session.ConfigureRoom(revision, settings);
        for (const auto &reply : replies) impl_->Queue(reply);
    }

    void NetworkClient::NativeRequest(const std::string &command, const unsigned value,
                                      const std::map<std::string, std::string> &settings) const
    {
        const auto replies = impl_->session.NativeRequest(command, value, settings);
        for (const auto &reply : replies) impl_->Queue(reply);
        impl_->Notify();
    }

    void NetworkClient::ConfirmMatchStart() const
    {
        if (impl_->local_prepare && impl_->Active()) {
            impl_->local_prepare = false;
            if (impl_->options.official) impl_->action_start = MonotonicMs();
            try { impl_->Queue(impl_->options.official ? "<NEXTREADY />" : "<STARTREADY />"); }
            catch (const TransportFailure &error) { impl_->Error(error.what(), true); }
        }
    }

    void NetworkClient::ExitGame(const int choice) const
    {
        const auto replies = impl_->session.ExitGame(choice);
        if (impl_->options.official && !replies.empty()) {
            impl_->local_prepare = false;
            impl_->action_start = 0;
            if (!impl_->official_game_id.empty()) impl_->SaveOfficialGameId({});
            impl_->SendLobbyReplies(replies);
        } else {
            for (const auto &reply : replies) impl_->Queue(reply);
            impl_->Notify();
        }
    }

    const LobbySnapshot &NetworkClient::snapshot() const { return impl_->Snapshot(); }
    bool NetworkClient::active() const { return impl_->Active(); }
} // namespace tenhou
