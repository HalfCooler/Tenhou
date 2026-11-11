// Match the original message-driven IPv4 API, including cancellable DNS.
// These legacy APIs remain an intentional, scoped stage-6 compatibility choice.
#define _WINSOCK_DEPRECATED_NO_WARNINGS
// Winsock must precede windows.h. All callbacks run on the owning UI thread.
#include <winsock2.h>
#include <ws2tcpip.h>
#include "network_client.hpp"
#include "replay_archive.hpp"
#include <windows.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <wincrypt.h>
#include <stdexcept>

namespace tenhou {
namespace {
constexpr UINT kDns = WM_APP + 31, kSocket = WM_APP + 32, kRead = WM_APP + 33;
constexpr wchar_t kNetworkWindow[] = L"TENHOU_REBUILD_NETWORK";
struct TransportFailure : std::runtime_error { using std::runtime_error::runtime_error; };
std::runtime_error SocketError(const char* action, int code = WSAGetLastError()) {
    return std::runtime_error(std::string(action) + " failed; Winsock error=" + std::to_string(code));
}
}
struct NetworkClient::Impl {
    std::string native_token;
    Observer changed;
    EventObserver accepted_event;
    std::function<void(const std::string&)> log;
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
    bool resuming = false;
    bool local_prepare = false;
    ULONGLONG action_start = 0;
    ULONGLONG native_request_start = 0;
    ULONGLONG recovery_start = 0;
    ULONGLONG phase_start = 0, last_receive = 0, last_heartbeat = 0, frame_start = 0, send_start = 0;

    Impl(Observer observer, std::function<void(const std::string&)> logger, EventObserver event)
        : changed(std::move(observer)), accepted_event(std::move(event)), log(std::move(logger)) {
        WSADATA data{};
        const int error = WSAStartup(MAKEWORD(2, 2), &data);
        if (error) throw SocketError("WSAStartup", error);
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc = Procedure; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = kNetworkWindow;
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            WSACleanup(); throw std::runtime_error("Cannot register network message window");
        }
    }
    ~Impl() { Close(); WSACleanup(); }
    bool Active() const {
        const auto phase = session.snapshot.phase;
        return phase != LobbyPhase::Error && phase != LobbyPhase::Disconnected;
    }
    void Notify() { changed(session.snapshot); }
    void Phase(LobbyPhase phase) {
        session.snapshot.phase = phase; phase_start = GetTickCount64();
        log(std::string("NETWORK state=") + LobbyPhaseName(phase)); Notify();
    }
    void Close() noexcept {
        if (dns) { WSACancelAsyncRequest(dns); dns = nullptr; }
        if (socket != INVALID_SOCKET) {
            WSAAsyncSelect(socket, window, 0, 0);
            shutdown(socket, SD_BOTH); closesocket(socket); socket = INVALID_SOCKET;
        }
        if (window) { KillTimer(window, 1); DestroyWindow(window); window = nullptr; }
        outgoing.clear(); sent_offset = 0; framer = {}; frame_start = 0; send_start = 0; action_start = 0;
        native_request_start=0;
    }
    void Error(const std::string& error, bool transport = false) {
        const bool available = transport && options.recover_games &&
            ((session.snapshot.game.initialized && !session.snapshot.game.complete) || resuming);
        revision_floor = std::max({revision_floor, session.snapshot.actions.revision, session.snapshot.lobby_revision}) + 1;
        Close(); session = {}; session.snapshot.error = error;
        session.snapshot.reconnect_available = available; session.snapshot.recovering = false;
        log("NETWORK error=" + error); Phase(LobbyPhase::Error);
    }
    void Disconnect() {
        revision_floor = std::max({revision_floor, session.snapshot.actions.revision, session.snapshot.lobby_revision}) + 1;
        resuming = false;
        Close(); session = {}; Phase(LobbyPhase::Disconnected);
    }
    void Connect(const ConnectionOptions& request, bool resume = false) {
        if(request.host!=options.host || request.port!=options.port || request.identity!=options.identity) native_token.clear();
        revision_floor = std::max({revision_floor, session.snapshot.actions.revision, session.snapshot.lobby_revision}) + 1;
        resuming = resume;
        recovery_start = 0;
        Close(); archive.Reset(); session = {}; options = request; session.observe_games = options.observe_games; session.interactive_games = options.interactive_games;
        session.recover_games = options.recover_games; session.resume_connection = resume;
        session.match_games = options.match_games;
        session.require_native=options.native_service; session.presentation=options.presentation; session.native_gender=options.gender;
        session.native_password=options.password; session.native_nickname=options.nickname; session.native_register=options.register_account && !resume;
        try {
        if(options.native_service && native_token.empty() && !options.session_file.empty() && std::filesystem::exists(options.session_file)) {
            std::ifstream file(options.session_file,std::ios::binary); std::vector<BYTE> bytes((std::istreambuf_iterator<char>(file)),{});
            if(bytes.size()>4096) throw std::runtime_error("Invalid native session file");
            DATA_BLOB in{static_cast<DWORD>(bytes.size()),bytes.data()},out{};
            if(!CryptUnprotectData(&in,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out)) throw std::runtime_error("Cannot decrypt native session file");
            native_token.assign(reinterpret_cast<char*>(out.pbData),out.cbData); LocalFree(out.pbData);
        }
        session.native_token=native_token;
        session.snapshot.recovering = resume;
            if ((options.interactive_games || options.recover_games) && options.host != "localhost" && options.host != "127.0.0.1")
                throw std::runtime_error("Game actions are limited to localhost or 127.0.0.1 in this stage");
            if (options.recover_games && !options.observe_games && !options.interactive_games)
                throw std::runtime_error("Local recovery requires a game mode");
            if (options.match_games && (!options.interactive_games || options.observe_games))
                throw std::runtime_error("Matchmaking requires local interactive mode without read-only observation");
            if (options.matching_ms < 100 || options.matching_ms > 600000) throw std::runtime_error("Invalid matchmaking timeout");
            // Validate before creating a DNS request or socket.
            LobbySession validation; validation.require_native=options.native_service; (void)validation.Connected(options.identity, options.gender);
            if (options.host.empty() || options.host.size() > 253 || options.host.front() == '-' ||
                !std::all_of(options.host.begin(), options.host.end(), [](unsigned char c) {
                    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '.';
                }) || options.port < 1 || options.port > 65535) throw std::runtime_error("Invalid IPv4 host or port (1..65535)");
            if (options.handshake_ms < 100 || options.handshake_ms > 60000 || options.idle_ms < 100 || options.idle_ms > 600000 ||
                options.heartbeat_ms < 50 || options.heartbeat_ms > 60000) throw std::runtime_error("Invalid connection timing");
            window = CreateWindowExW(0, kNetworkWindow, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), this);
            if (!window || !SetTimer(window, 1, 50, nullptr)) throw std::runtime_error("Cannot create network event window/timer");
            last_receive = last_heartbeat = GetTickCount64();
            Phase(LobbyPhase::Resolving);
            IN_ADDR address{};
            if (InetPtonA(AF_INET, options.host.c_str(), &address) == 1) BeginTcp(address);
            else {
                dns = WSAAsyncGetHostByName(window, kDns, options.host.c_str(), host.data(), static_cast<int>(host.size()));
                if (!dns) throw SocketError("DNS request");
            }
        } catch (const std::exception& error) { Error(error.what()); }
    }
    void Reconnect(const ConnectionOptions& request) {
        if (Active() || !session.snapshot.reconnect_available) throw std::runtime_error("No recoverable disconnected game");
        if (!request.recover_games || request.host != options.host || request.port != options.port ||
            request.gender != options.gender || request.observe_games != options.observe_games || request.interactive_games != options.interactive_games || request.match_games != options.match_games)
            throw std::runtime_error("Reconnect requires the same server and game mode");
        Connect(request, true);
    }
    void BeginTcp(IN_ADDR address) {
        if ((options.interactive_games || options.recover_games) && (ntohl(address.S_un.S_addr) >> 24) != 127)
            throw std::runtime_error("Interactive DNS resolved outside loopback");
        socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket == INVALID_SOCKET) throw SocketError("socket");
        if (WSAAsyncSelect(socket, window, kSocket, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR) throw SocketError("WSAAsyncSelect");
        sockaddr_in endpoint{}; endpoint.sin_family = AF_INET;
        endpoint.sin_addr = address; endpoint.sin_port = htons(static_cast<u_short>(options.port));
        Phase(LobbyPhase::Connecting);
        const int result = ::connect(socket, reinterpret_cast<sockaddr*>(&endpoint), sizeof(endpoint));
        if (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) throw SocketError("connect");
        if (result == 0) Connected();
    }
    void Connected() {
        if (session.snapshot.phase != LobbyPhase::Connecting) return;
        const auto messages = session.Connected(options.identity, options.gender, revision_floor);
        recovery_start = resuming ? GetTickCount64() : 0;
        options.identity.clear(); // Do not retain the input ID beyond the handshake.
        last_receive = last_heartbeat = GetTickCount64(); Phase(LobbyPhase::AwaitHelo);
        for (const auto& message : messages) Queue(message);
    }
    void Queue(const std::string& message) {
        std::string wire=message;
        const auto tag=ParseNetworkMessage(message).tag;
        if(session.snapshot.native_service && (tag=="D" || tag=="N" || tag=="REACH"))
            wire.insert(wire.rfind('/')," window=\""+std::to_string(session.snapshot.native_window)+"\" ");
        if (outgoing.size() - sent_offset + wire.size() + 1 > 131072) throw std::runtime_error("Network send queue exceeds limit");
        if (sent_offset) { outgoing.erase(0, sent_offset); sent_offset = 0; }
        if (outgoing.empty()) send_start = GetTickCount64();
        outgoing += wire; outgoing += '\0';
        ++session.snapshot.sent;
        log("NETWORK send tag=" + ParseNetworkMessage(message).tag);
        Flush();
    }
    void Flush() {
        while (sent_offset < outgoing.size()) {
            const int count = send(socket, outgoing.data() + sent_offset, static_cast<int>(outgoing.size() - sent_offset), 0);
            if (count == SOCKET_ERROR) { if (WSAGetLastError() == WSAEWOULDBLOCK) return; throw TransportFailure(SocketError("send").what()); }
            if (!count) throw std::runtime_error("Network send made no progress");
            sent_offset += static_cast<std::size_t>(count);
        }
        outgoing.clear(); sent_offset = 0; send_start = 0;
    }
    void Read() {
        // Limit work per message dispatch so a busy server cannot monopolize the UI.
        std::array<char, 4096> bytes{};
        for (int batch = 0; batch < 16; ++batch) {
            const int count = recv(socket, bytes.data(), static_cast<int>(bytes.size()), 0);
            if (count == SOCKET_ERROR) { if (WSAGetLastError() == WSAEWOULDBLOCK) return; throw TransportFailure(SocketError("recv").what()); }
            if (!count) {
                if (framer.pending()) throw std::runtime_error("Connection closed with truncated message");
                throw TransportFailure("Server closed connection");
            }
            const bool was_pending = framer.pending();
            const auto messages = framer.Feed(bytes.data(), static_cast<std::size_t>(count));
            if (framer.pending()) { if (!was_pending || !messages.empty()) frame_start = GetTickCount64(); }
            else frame_start = 0;
            for (const auto& text : messages) {
                const auto message = ParseNetworkMessage(text);
                const auto old_phase = session.snapshot.phase;
                const auto replies = session.Receive(message);
                if(message.tag.rfind("REPLAY",0)==0) {
                    try { archive.Receive(message); session.snapshot.replay_status=archive.status(); }
                    catch(const std::exception& e) { session.snapshot.replay_status=std::string("牌谱保存失败：")+e.what(); }
                    log("REPLAY STATUS "+session.snapshot.replay_status);
                }
                if((message.tag=="PROF" || message.tag=="RESTORED") && archive.Request(session.snapshot,options.config_file)) {
                    session.snapshot.replay_status=archive.status(); Queue("<GETREPLAY table=\""+session.snapshot.exit_table+"\"/>");
                }
                if(message.tag=="SESSION" && session.snapshot.native_service) {
                    native_token=session.snapshot.native_token;
                    if(!options.session_file.empty()) {
                        DATA_BLOB in{static_cast<DWORD>(native_token.size()),reinterpret_cast<BYTE*>(native_token.data())},out{};
                        if(!CryptProtectData(&in,L"Tenhou native guest session",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out)) throw std::runtime_error("Cannot protect native guest session");
                        std::filesystem::create_directories(options.session_file.parent_path());
                        const auto temporary=options.session_file.wstring()+L".tmp";
                        HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
                        if(file==INVALID_HANDLE_VALUE) { LocalFree(out.pbData); throw std::runtime_error("Cannot open native guest session temporary file"); }
                        DWORD written=0; const bool saved=WriteFile(file,out.pbData,out.cbData,&written,nullptr) && written==out.cbData && FlushFileBuffers(file);
                        CloseHandle(file); LocalFree(out.pbData);
                        if(!saved || !MoveFileExW(temporary.c_str(),options.session_file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Cannot atomically save native guest session");
                    }
                }
                if (message.tag=="GO" || message.tag=="REINIT") local_prepare=false;
                if (message.tag=="PREPARE") local_prepare=true;
                if (message.tag=="START") local_prepare=false;
                if (accepted_event) accepted_event(message, session.snapshot);
                if (message.tag == "REINIT" || message.tag=="RESTORED") { resuming = false; recovery_start=0; }
                if (session.snapshot.actions.pending.empty() || session.snapshot.table_paused) action_start = 0;
                else if (!action_start) action_start = GetTickCount64();
                last_receive = GetTickCount64();
                log("NETWORK recv tag=" + message.tag);
                if (old_phase != session.snapshot.phase) Phase(session.snapshot.phase);
                for (const auto& reply : replies) Queue(reply);
                // Opt in only to the identified local rules service. The
                // original remote protocol and its GOK handshake stay intact.
                if (message.tag=="HELO" && message.attributes.count("uname") && message.attributes.at("uname")=="LOCAL-RULES" && options.interactive_games && options.presentation)
                    Queue("<PRESENTATION />");
                Notify();
            }
        }
        PostMessageW(window, kRead, 0, 0);
    }
    void SubmitAction(std::uint64_t revision, std::size_t index) {
        // Invalid/stale UI requests are rejected without tearing down a healthy connection.
        const auto replies = session.SubmitAction(revision, index);
        try {
            action_start = GetTickCount64();
            for (const auto& reply : replies) Queue(reply);
            Notify();
        } catch (const TransportFailure& error) { Error(error.what(), true); }
        catch (const std::exception& error) { Error(error.what()); }
    }
    void Tick() {
        if (!Active()) return;
        const auto now = GetTickCount64(); const auto phase = session.snapshot.phase;
        if(session.snapshot.native_pending.empty()) native_request_start=0;
        else if(!native_request_start) native_request_start=now;
        else if(now-native_request_start>=options.handshake_ms) throw TransportFailure("Native room request confirmation timed out");
        if (resuming && recovery_start && now - recovery_start >= options.handshake_ms)
            throw std::runtime_error("Game recovery snapshot timed out");
        if (phase == LobbyPhase::Matching && now - phase_start >= options.matching_ms)
            throw std::runtime_error("Matchmaking timed out; connect explicitly to retry");
        const bool established = phase == LobbyPhase::Lobby || phase == LobbyPhase::Game || phase == LobbyPhase::MatchComplete || phase == LobbyPhase::Matching;
        if (!established && now - phase_start >= options.handshake_ms) throw std::runtime_error("Connection/handshake or recovery timed out");
        if (established && now - last_receive >= options.idle_ms) throw TransportFailure("Server receive timed out");
        const auto action_timeout=std::max<ULONGLONG>(options.handshake_ms,session.snapshot.game.local_action_wait_ms ? session.snapshot.game.local_action_wait_ms+10000ULL : 0);
        if (action_start && now - action_start >= action_timeout) throw TransportFailure("Game action confirmation timed out");
        if (frame_start && now - frame_start >= options.handshake_ms) throw std::runtime_error("Incomplete network message timed out");
        if (send_start && now - send_start >= options.handshake_ms) throw std::runtime_error("Network send timed out");
        if ((phase == LobbyPhase::AwaitHelo || phase == LobbyPhase::AwaitLobby || phase == LobbyPhase::AwaitRecovery || established) && now - last_heartbeat >= options.heartbeat_ms) {
            Queue("<Z />"); last_heartbeat = now; Notify();
        }
    }
    void SubmitLobby(std::uint64_t revision, const std::string& command, unsigned value) {
        const auto replies = session.SubmitLobby(revision, command, value);
        try {
            Phase(session.snapshot.phase);
            for (const auto& reply : replies) Queue(reply);
            Notify();
        } catch (const TransportFailure& error) { Error(error.what(), true); }
        catch (const std::exception& error) { Error(error.what()); }
    }
    static LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        auto* self = reinterpret_cast<Impl*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<Impl*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (!self) return DefWindowProcW(window, message, wparam, lparam);
        try {
            if (message == kDns && reinterpret_cast<HANDLE>(wparam) == self->dns) {
                self->dns = nullptr;
                if (WSAGETASYNCERROR(lparam)) throw SocketError("DNS", WSAGETASYNCERROR(lparam));
                const auto* entry = reinterpret_cast<const hostent*>(self->host.data());
                if (entry->h_addrtype != AF_INET || entry->h_length != sizeof(IN_ADDR) || !entry->h_addr_list || !entry->h_addr_list[0])
                    throw std::runtime_error("DNS returned no IPv4 address");
                IN_ADDR address{}; memcpy(&address, entry->h_addr_list[0], sizeof(address)); self->BeginTcp(address); return 0;
            }
            if (message == kSocket && static_cast<SOCKET>(wparam) == self->socket) {
                const int event = WSAGETSELECTEVENT(lparam), error = WSAGETSELECTERROR(lparam);
                if (error) throw TransportFailure(SocketError("socket event", error).what());
                if (event == FD_CONNECT) self->Connected();
                else if (event == FD_READ || event == FD_CLOSE) self->Read();
                else if (event == FD_WRITE) self->Flush();
                return 0;
            }
            if (message == kRead && self->socket != INVALID_SOCKET) { self->Read(); return 0; }
            if (message == WM_TIMER) { self->Tick(); return 0; }
        } catch (const TransportFailure& error) { self->Error(error.what(), true); return 0; }
        catch (const std::exception& error) { self->Error(error.what()); return 0; }
        return DefWindowProcW(window, message, wparam, lparam);
    }
};
NetworkClient::NetworkClient(Observer changed, std::function<void(const std::string&)> log, EventObserver event)
    : impl_(std::make_unique<Impl>(std::move(changed), std::move(log), std::move(event))) {}
NetworkClient::~NetworkClient() = default;
void NetworkClient::Connect(const ConnectionOptions& options) { impl_->Connect(options); }
void NetworkClient::Disconnect() { impl_->Disconnect(); }
void NetworkClient::Reconnect(const ConnectionOptions& options) { impl_->Reconnect(options); }
void NetworkClient::SubmitAction(std::uint64_t revision, std::size_t index) { impl_->SubmitAction(revision, index); }
void NetworkClient::SubmitLobby(std::uint64_t revision, const std::string& command, unsigned value) { impl_->SubmitLobby(revision, command, value); }
void NetworkClient::ConfigureRoom(std::uint64_t revision, const std::map<std::string,std::string>& settings) {
    const auto replies=impl_->session.ConfigureRoom(revision,settings);
    for (const auto& reply : replies) impl_->Queue(reply);
}
void NetworkClient::NativeRequest(const std::string& command,unsigned value,const std::map<std::string,std::string>& settings) {
    const auto replies=impl_->session.NativeRequest(command,value,settings);
    for(const auto& reply:replies) impl_->Queue(reply);
    impl_->Notify();
}
void NetworkClient::ConfirmMatchStart() { if (impl_->local_prepare && impl_->Active()) { impl_->local_prepare=false; impl_->Queue("<STARTREADY />"); } }
void NetworkClient::ExitGame(int choice) {
    for(const auto& reply:impl_->session.ExitGame(choice)) impl_->Queue(reply);
    impl_->Notify();
}
const LobbySnapshot& NetworkClient::snapshot() const { return impl_->session.snapshot; }
bool NetworkClient::active() const { return impl_->Active(); }
} // namespace tenhou
