#include "service.hpp"
#include "storage.hpp"
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>
#include <numeric>
#include <cmath>

namespace authority {
    namespace {
        std::string room_name(int room) {
            auto s = std::to_string(room);
            return "L" + std::string(4 - s.size(), '0') + s;
        }

        std::string attr(const Message &m, const char *k, const std::string &fallback = "") {
            auto i = m.attrs.find(k);
            return i == m.attrs.end() ? fallback : i->second;
        }

        std::string offer(const Game &g, int s) {
            std::string r;
            for (const auto &a: g.options(s)) r += a.kind + ":" + list(a.tiles) + ";";
            return r;
        }

        ULONGLONG utc_ms() {
            FILETIME ft{};
            GetSystemTimeAsFileTime(&ft);
            ULARGE_INTEGER value{};
            value.LowPart = ft.dwLowDateTime;
            value.HighPart = ft.dwHighDateTime;
            return value.QuadPart / 10000;
        }
    }

    Service::Service(Settings settings) : cfg(std::move(settings)) {
        std::filesystem::create_directories(cfg.logs);
        log.open(cfg.logs / "server.jsonl", std::ios::app);
        if (!log) throw std::runtime_error("Cannot open structured log");
        load();
        listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listener == INVALID_SOCKET) throw std::runtime_error("Socket creation failed");
        BOOL exclusive = TRUE;
        setsockopt(listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char *>(&exclusive),
                   sizeof(exclusive));
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<u_short>(cfg.port));
        if (InetPtonA(AF_INET, cfg.listen.c_str(), &address.sin_addr) != 1 || (ntohl(address.sin_addr.s_addr) >> 24) !=
            127) throw std::runtime_error("Stage 26 supports explicit IPv4 loopback listening only");
        if (bind(listener, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0 || listen(listener, SOMAXCONN)
            != 0) throw std::runtime_error("Cannot bind configured endpoint");
        u_long nonblocking = 1;
        ioctlsocket(listener, FIONBIO, &nonblocking);
        record("ready", cfg.port);
    }

    Service::~Service() {
        for (auto &e: connections) closesocket(e.first);
        if (listener != INVALID_SOCKET) closesocket(listener);
    }

    void Service::record(const std::string &event, int value) {
        log << "{\"event\":\"" << event << "\",\"value\":" << value << ",\"tick_ms\":" << GetTickCount64() << "}\n";
        log.flush();
    }

    void Service::send(Connection &c, const std::string &text) {
        if (c.sent) {
            c.output.erase(0, c.sent);
            c.sent = 0;
        }
        if (c.output.size() + text.size() + 1 > 512 * 1024) throw std::runtime_error("Slow peer output limit");
        if (c.output.empty()) c.out_at = GetTickCount64();
        c.output += text;
        c.output += '\0';
    }

    void Service::send(Session &p, const std::string &text) {
        auto c = connections.find(p.socket);
        if (c != connections.end()) send(c->second, text);
    }

    void Service::close(SOCKET socket_) {
        auto i = connections.find(socket_);
        if (i == connections.end()) return;
        auto p = sessions.find(i->second.token);
        if (p != sessions.end() && p->second.socket == socket_) {
            p->second.socket = INVALID_SOCKET;
            p->second.queued = false;
            if (!p->second.table.empty()) {
                auto t = tables.find(p->second.table);
                if (t != tables.end()) {
                    auto seat = std::find(t->second.humans.begin(), t->second.humans.end(), p->first) - t->second.humans
                                .begin();
                    t->second.declared[seat] = -1;
                    pause(t->second);
                }
            }
        }
        closesocket(socket_);
        connections.erase(i);
        save();
        record("disconnect");
    }

    void Service::lobby(Session &p, const std::string &rid) {
        Attrs a{{"lobby", room_name(p.room)}};
        if (!rid.empty()) a["rid"] = rid;
        send(p, xml("CS", a));
        if (p.room) room_snapshot(p, rid);
        profile(p);
    }

    void Service::profile(Session &p) {
        Attrs a{{"account", p.account}, {"nickname", p.account.empty() ? "NoName" : accounts.at(p.account).nickname}};
        if (!p.account.empty()) {
            const auto &account = accounts.at(p.account);
            a["played"] = std::to_string(account.played);
            for (int i = 0; i < 2; ++i) {
                const auto key = std::to_string(i == 0 ? 4 : 3);
                const auto &r = account.ranks[i];
                a["grade" + key] = rank_name(r.level);
                a["points" + key] = std::to_string(r.points);
                a["target" + key] = std::to_string(rank_target(r.level));
                a["rating" + key] = std::to_string(r.rating);
                a["games" + key] = std::to_string(r.games);
            }
        }
        send(p, xml("PROFILE", a));
    }

    void Service::room_snapshot(Session &p, const std::string &rid) {
        const auto &r = rooms.at(p.room);
        auto a = r.config.wire();
        a["room"] = std::to_string(p.room);
        a["revision"] = std::to_string(r.revision);
        a["owner"] = r.owner == p.token ? "1" : "0";
        if (!rid.empty()) a["rid"] = rid;
        send(p, xml("ROOM", a));
    }

    std::string Service::state() const {
        std::string r = xml("STATE", {{"version", "27"}, {"order", std::to_string(order)}}) + "\n";
        for (const auto &e: accounts) {
            const auto &a = e.second;
            r += xml("ACCOUNT", {
                         {"id", a.id}, {"nickname", a.nickname}, {"salt", a.salt}, {"hash", a.password_hash},
                         {"played", std::to_string(a.played)}
                     }) + "\n";
            for (int i = 0; i < 2; ++i) {
                const auto &rank = a.ranks[i];
                r += xml("RANK", {
                             {"id", a.id}, {"kind", std::to_string(i)}, {"level", std::to_string(rank.level)},
                             {"points", std::to_string(rank.points)}, {"rating", std::to_string(rank.rating)},
                             {"games", std::to_string(rank.games)}, {"places", list(rank.places)}
                         }) + "\n";
            }
        }
        for (const auto &e: sessions) {
            const auto &p = e.second;
            r += xml("SESSION", {
                         {"token", p.token}, {"gender", p.gender}, {"room", std::to_string(p.room)}, {"table", p.table},
                         {"presentation", std::to_string(p.presentation)},
                         {"rid", std::to_string(p.last_request)}, {"request", p.request_text},
                         {"reply", p.request_reply}, {"account", p.account}
                     }) + "\n";
        }
        for (const auto &e: rooms) {
            auto a = e.second.config.wire();
            a["number"] = std::to_string(e.first);
            a["owner"] = e.second.owner;
            a["revision"] = std::to_string(e.second.revision);
            a["closed"] = std::to_string(e.second.closed);
            r += xml("ROOM", a) + "\n";
        }
        for (const auto &e: tables) {
            const auto &t = e.second;
            if (t.stage == "aborted") continue;
            auto a = t.config.wire();
            a["id"] = t.id;
            a["seed"] = std::to_string(t.seed);
            a["stage"] = t.stage;
            a["reserve"] = list(t.reserve);
            a["remaining"] = "";
            a["exit_revision"] = std::to_string(t.exit_revision);
            a["vote_until"] = std::to_string(t.vote_until);
            a["exit_votes"] = list(t.exit_votes);
            auto now = t.paused ? t.paused_at : GetTickCount64();
            for (int s = 0; s < 4; ++s) {
                if (s) a["remaining"] += ',';
                a["remaining"] += std::to_string(t.deadlines[s]
                                                     ? static_cast<int>(t.deadlines[s] > now ? t.deadlines[s] - now : 0)
                                                     : -1);
            }
            a["general"] = std::to_string(t.general);
            a["type"] = std::to_string(t.config.type());
            a["ranked"] = std::to_string(t.ranked);
            a["settled"] = std::to_string(t.settled);
            auto elapsed = t.elapsed;
            for (int s = 0; s < 4; ++s) if (t.started[s]) elapsed[s] += static_cast<int>(now - t.started[s]);
            a["elapsed"] = list(elapsed);
            a["due"] = std::to_string(t.due && t.due > now ? t.due - now : 0);
            for (std::size_t s = 0; s < t.humans.size(); ++s) a["p" + std::to_string(s)] = t.humans[s];
            r += xml("TABLE", a) + "\n";
            for (const auto &c: t.commands) r += xml("COMMAND", {{"text", c}}) + "\n";
            for (const auto &ready: t.ready) r += xml("READY", {{"token", ready}}) + "\n";
        }
        return r + "<END/>\n";
    }

    void Service::save() { write_state(cfg.state, state()); }

    void Service::load() {
        auto raw = read_state(cfg.state);
        if (raw.empty()) return;
        std::istringstream input(raw);
        std::string line;
        Table *current = nullptr;
        bool ended = false, header = false;
        while (std::getline(input, line)) {
            auto m = parse(line);
            auto a = m.attrs;
            if (ended || (!header && m.tag != "STATE")) throw std::runtime_error("Invalid state record order");
            if (m.tag == "STATE") {
                if (header || (a.at("version") != "26" && a.at("version") != "27")) throw std::runtime_error(
                    "Unsupported/duplicate state header");
                header = true;
                order = std::stoull(a.at("order"));
            } else if (m.tag == "ACCOUNT") {
                Account account;
                account.id = account_id(a.at("id"));
                account.nickname = a.at("nickname");
                account.salt = a.at("salt");
                account.password_hash = a.at("hash");
                account.played = number(a.at("played"), 0);
                if (account.salt.size() != 64 || account.password_hash.size() != 64 || !accounts.
                    emplace(account.id, account).second) throw std::runtime_error("Invalid account record");
            } else if (m.tag == "RANK") {
                auto &rank = accounts.at(a.at("id")).ranks[number(a.at("kind"), 0, 1)];
                rank.level = number(a.at("level"), 0, 18);
                rank.points = number(a.at("points"), 0, 3600);
                rank.games = number(a.at("games"), 0);
                rank.rating = std::stod(a.at("rating"));
                auto places = integers(a.at("places"));
                if (!std::isfinite(rank.rating) || places.size() != 4) throw std::runtime_error("Invalid rank record");
                std::copy(places.begin(), places.end(), rank.places.begin());
            } else if (m.tag == "SESSION") {
                Session p;
                p.token = a.at("token");
                p.gender = a.at("gender");
                p.room = number(a.at("room"), 0, 9999);
                p.table = a.at("table");
                p.presentation = a.at("presentation") == "1";
                p.last_request = std::stoull(a.at("rid"));
                p.request_text = a.at("request");
                p.request_reply = a.at("reply");
                p.account = attr(m, "account");
                if (!p.account.empty() && !accounts.count(p.account)) throw std::runtime_error(
                    "Missing session account");
                if (!sessions.emplace(p.token, std::move(p)).second) throw std::runtime_error("Duplicate session");
            } else if (m.tag == "ROOM") {
                int id = number(a.at("number"), 1000, 9999);
                Room r;
                r.owner = a.at("owner");
                r.revision = number(a.at("revision"), 1);
                r.closed = a.at("closed") == "1";
                for (const char *k: {"number", "owner", "revision", "closed"}) a.erase(k);
                r.config = Config::from(a);
                if (!rooms.emplace(id, std::move(r)).second) throw std::runtime_error("Duplicate room record");
            } else if (m.tag == "TABLE") {
                Table t;
                t.id = a.at("id");
                t.seed = std::stoull(a.at("seed"));
                t.stage = a.at("stage");
                t.paused = true;
                t.paused_at = GetTickCount64();
                t.general = a.at("general") == "1";
                int type = number(a.at("type"), 1, 127);
                t.due = t.paused_at + number(a.at("due"), 0, 60000);
                t.ranked = attr(m, "ranked") == "1";
                t.settled = attr(m, "settled") == "1";
                a.erase("ranked");
                a.erase("settled");
                if (a.count("exit_revision")) {
                    t.exit_revision = number(a.at("exit_revision"), 0);
                    t.vote_until = std::stoull(a.at("vote_until"));
                    auto votes = integers(a.at("exit_votes"));
                    if (votes.size() != 4) throw std::runtime_error("Invalid exit votes");
                    for (int s = 0; s < 4; ++s) {
                        if (votes[s] < -1 || votes[s] > 1) throw std::runtime_error("Invalid ballot");
                        t.exit_votes[s] = votes[s];
                    }
                    if (t.vote_until) {
                        auto now = utc_ms();
                        t.vote_due = GetTickCount64() + std::min<ULONGLONG>(
                                         30000, t.vote_until > now ? t.vote_until - now : 0);
                    }
                    for (const char *k: {"exit_revision", "vote_until", "exit_votes"}) a.erase(k);
                }
                auto reserve = integers(a.at("reserve")), remaining = integers(a.at("remaining"));
                if (reserve.size() != 4 || remaining.size() != 4) throw std::runtime_error("Invalid clocks");
                auto elapsed = integers(a.at("elapsed"));
                if (elapsed.size() != 4) throw std::runtime_error("Invalid clock elapsed");
                for (int s = 0; s < 4; ++s) {
                    t.reserve[s] = reserve[s];
                    t.deadlines[s] = remaining[s] < 0 ? 0 : t.paused_at + remaining[s];
                    t.elapsed[s] = elapsed[s];
                    t.started[s] = remaining[s] < 0 ? 0 : t.paused_at;
                }
                for (int s = 0; s < 4; ++s) {
                    auto key = "p" + std::to_string(s);
                    if (a.count(key)) t.humans.push_back(a.at(key));
                    a.erase(key);
                }
                for (const char *k: {
                         "id", "seed", "stage", "reserve", "remaining", "general", "type", "due", "elapsed"
                     }) a.erase(k);
                t.config = t.general ? Config::general(type) : Config::from(a);
                t.game = std::make_unique<Game>(t.config, t.seed);
                current = &tables.emplace(t.id, std::move(t)).first->second;
            } else if (m.tag == "COMMAND") {
                if (!current) throw std::runtime_error("Orphan command");
                apply(*current, a.at("text"));
            } else if (m.tag == "READY") {
                if (!current) throw std::runtime_error("Orphan readiness");
                current->ready.insert(a.at("token"));
            } else if (m.tag == "END") ended = true;
            else throw std::runtime_error("Unknown persisted record");
        }
        if (!header || !ended || sessions.size() > static_cast<std::size_t>(cfg.max_sessions) || tables.size() >
            static_cast<std::size_t>(cfg.max_tables)) throw std::runtime_error("Incomplete/oversized state");
        for (const auto &e: rooms) if (!sessions.count(e.second.owner)) throw std::runtime_error("Missing room owner");
        for (const auto &e: sessions) {
            const auto &p = e.second;
            if (p.token.size() != 64 || !std::all_of(p.token.begin(), p.token.end(), [](char c) {
                return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
            }) || (p.gender != "M" && p.gender != "F")) throw std::runtime_error("Invalid persisted session");
            if (p.room && (!rooms.count(p.room) || rooms.at(p.room).closed)) throw std::runtime_error(
                "Invalid persisted membership");
        }
        for (auto &e: tables) {
            auto &t = e.second;
            t.cursor = t.game->events.size();
            for (std::size_t s = 0; s < t.humans.size(); ++s) {
                if (!sessions.count(t.humans[s])) throw std::runtime_error("Missing table session");
                t.offers[s] = offer(*t.game, static_cast<int>(s));
                t.windows[s] = t.offers[s].empty() ? 0 : t.game->events.size();
            }
        }
        for (const auto &e: sessions) if (!e.second.table.empty() && !tables.count(e.second.table)) throw
                std::runtime_error("Missing table");
        record("restored", static_cast<int>(tables.size()));
    }

    void Service::handle(Connection &c, const Message &m, const std::string &raw) {
        if (m.tag == "HEALTH" && m.attrs.empty()) {
            send(c, xml("HEALTH", {
                            {"version", "26"}, {"ready", draining ? "0" : "1"},
                            {"connections", std::to_string(connections.size())},
                            {"sessions", std::to_string(sessions.size())}, {"rooms", std::to_string(rooms.size())},
                            {"tables", std::to_string(tables.size())}
                        }));
            return;
        }
        if (m.tag == "Z" && m.attrs.empty()) {
            send(c, "<Z/>");
            return;
        }
        if (m.tag == "HELO" && !c.helo && c.token.empty()) {
            if (draining) throw std::runtime_error("Service draining");
            c.login_name = attr(m, "name");
            if (c.login_name != "NoName") {
                try { c.login_name = account_id(c.login_name); } catch (const std::exception &e) {
                    send(c, xml("ERR", {{"code", "401"}, {"text", e.what()}}));
                    c.closing = true;
                    return;
                }
            }
            if (attr(m, "sx") != "M" && attr(m, "sx") != "F") throw std::runtime_error("Invalid gender");
            c.helo = true;
            send(c, xml("HELO", {
                            {"uname", "LOCAL-NATIVE"}, {"version", "26"}, {"caps", "rooms,queue,recovery,presentation"}
                        }));
            return;
        }
        if (m.tag == "CAPS" && c.helo && c.token.empty()) {
            if (attr(m, "version") != "26") throw std::runtime_error("Protocol version mismatch");
            auto token = attr(m, "token");
            if (c.login_name == "NoName" && attr(m, "register") == "1") {
                send(c, xml("ERR", {{"code", "401"}, {"text", "请填写要注册的账号，不能使用 NoName"}}));
                c.closing = true;
                return;
            }
            if (c.login_name != "NoName") {
                auto reject = [&](const std::string &reason) {
                    send(c, xml("ERR", {{"code", "401"}, {"text", reason}}));
                    c.closing = true;
                };
                auto account = accounts.find(c.login_name);
                if (attr(m, "register") == "1") {
                    if (account != accounts.end()) {
                        reject("账号已存在");
                        return;
                    }
                    if (accounts.size() >= 10000) {
                        reject("账号容量已满");
                        return;
                    }
                    if (sessions.size() >= static_cast<std::size_t>(cfg.max_sessions)) {
                        reject("会话容量已满");
                        return;
                    }
                    try {
                        auto created = new_account(c.login_name, attr(m, "nickname"), attr(m, "password"));
                        account = accounts.emplace(created.id, std::move(created)).first;
                    } catch (const std::exception &e) {
                        reject(e.what());
                        return;
                    }
                } else if (account == accounts.end() || !password_matches(account->second, attr(m, "password"))) {
                    reject("账号或密码错误");
                    return;
                }
                token.clear();
                for (const auto &e: sessions) if (e.second.account == c.login_name) {
                    token = e.first;
                    break;
                }
                if (token.empty()) {
                    if (sessions.size() >= static_cast<std::size_t>(cfg.max_sessions)) {
                        reject("会话容量已满");
                        return;
                    }
                    token = secret();
                    Session p;
                    p.token = token;
                    p.account = c.login_name;
                    p.gender = attr(m, "sx", "M");
                    sessions.emplace(token, std::move(p));
                }
            } else if (!token.empty() && sessions.count(token) && !sessions.at(token).account.empty()) {
                send(c, "<ERR code=\"401\"/>");
                c.closing = true;
                return;
            }
            if (token.empty()) {
                if (sessions.size() >= static_cast<std::size_t>(cfg.max_sessions)) throw std::runtime_error(
                    "Session capacity");
                token = secret();
                Session p;
                p.token = token;
                p.gender = attr(m, "sx", "M");
                if (p.gender != "M" && p.gender != "F") throw std::runtime_error("Invalid gender");
                sessions.emplace(token, std::move(p));
            }
            auto i = sessions.find(token);
            if (i == sessions.end()) {
                send(c, "<ERR code=\"401\"/>");
                c.closing = true;
                return;
            }
            auto &p = i->second;
            if (p.socket != INVALID_SOCKET) {
                send(c, xml("ERR", {{"code", "409"}, {"text", "该账号已在其他窗口登录"}}));
                c.closing = true;
                return;
            }
            p.socket = c.socket;
            c.token = token;
            p.presentation = attr(m, "presentation") == "1";
            p.exited = false;
            save();
            send(c, xml("SESSION", {{"token", token}, {"rid", std::to_string(p.last_request)}}));
            lobby(p);
            if (!p.table.empty()) {
                auto &t = tables.at(p.table);
                restore(t, p);
                unpause(t);
            }
            record("authenticated");
            return;
        }
        if (c.token.empty()) throw std::runtime_error("Authentication required");
        command(sessions.at(c.token), c, m, raw);
    }

    void Service::command(Session &p, Connection &c, const Message &m, const std::string &raw) {
        const auto &tag = m.tag;
        auto a = m.attrs;
        const auto rid = attr(m, "rid");
        const bool request = tag == "ROOMCREATE" || tag == "ROOMJOIN" || tag == "ROOMLEAVE" || tag == "ROOMCLOSE" || tag
                             == "SETROOM";
        if (request) {
            std::vector<std::string> changed_members;
            const auto id = number(rid, 1);
            if (static_cast<std::uint64_t>(id) == p.last_request) {
                if (raw != p.request_text) throw std::runtime_error("Request id reused");
                send(c, p.request_reply);
                lobby(p, rid);
                return;
            }
            if (static_cast<std::uint64_t>(id) < p.last_request) throw std::runtime_error("Stale request id");
            if (!p.table.empty() || p.queued) {
                send(c, xml("REJECT", {{"rid", rid}, {"code", "BUSY"}}));
                return;
            }
            auto reject = [&](const std::string &code) {
                auto reply = xml("REJECT", {{"rid", rid}, {"code", code}});
                p.last_request = id;
                p.request_text = raw;
                p.request_reply = reply;
                save();
                send(c, reply);
            };
            if (tag == "ROOMCREATE") {
                if (draining) {
                    reject("DRAINING");
                    return;
                }
                if (p.room) {
                    reject("LEAVE_FIRST");
                    return;
                }
                a.erase("rid");
                Config config = Config::from(a);
                int chosen = 0;
                const int start = 1000 + static_cast<int>(std::stoul(secret().substr(0, 6), nullptr, 16) % 9000);
                for (int j = 0; j < 9000; ++j) {
                    int n = 1000 + (start - 1000 + j) % 9000;
                    auto r = rooms.find(n);
                    if (r == rooms.end() || r->second.closed) {
                        chosen = n;
                        break;
                    }
                }
                if (!chosen) {
                    reject("ROOM_CAPACITY");
                    return;
                }
                rooms[chosen] = {config, p.token, 1, false};
                p.room = chosen;
            } else if (tag == "ROOMJOIN") {
                if (p.room) {
                    reject("LEAVE_FIRST");
                    return;
                }
                int target = number(a.at("room"), 1000, 9999);
                auto i = rooms.find(target);
                if (i == rooms.end()) {
                    reject("NOT_FOUND");
                    return;
                }
                if (i->second.closed) {
                    reject("CLOSED");
                    return;
                }
                int count = 0;
                for (const auto &e: sessions) if (e.second.room == target) {
                    ++count;
                    if (!e.second.table.empty()) {
                        reject("IN_PROGRESS");
                        return;
                    }
                }
                if (count >= i->second.config.n()) {
                    reject("FULL");
                    return;
                }
                p.room = target;
            } else if (tag == "ROOMLEAVE") { p.room = 0; } else {
                if (!p.room || !rooms.count(p.room)) {
                    reject("NOT_IN_ROOM");
                    return;
                }
                auto &room = rooms.at(p.room);
                if (room.owner != p.token) {
                    reject("OWNER_REQUIRED");
                    return;
                }
                for (const auto &e: sessions) if (
                    e.second.room == p.room && (e.second.queued || !e.second.table.empty())) {
                    reject("BUSY");
                    return;
                }
                if (tag == "ROOMCLOSE") {
                    auto closed = p.room;
                    room.closed = true;
                    ++room.revision;
                    for (auto &e: sessions) if (e.second.room == closed) {
                        changed_members.push_back(e.first);
                        e.second.room = 0;
                    }
                } else {
                    auto revision = number(a.at("revision"), 1);
                    if (static_cast<unsigned>(revision) != room.revision) {
                        reject("REVISION_CONFLICT");
                        room_snapshot(p);
                        return;
                    }
                    a.erase("rid");
                    a.erase("revision");
                    room.config = Config::from(a);
                    ++room.revision;
                }
            }
            p.last_request = id;
            p.request_text = raw;
            p.request_reply = xml("ACCEPT", {{"rid", rid}, {"room", std::to_string(p.room)}});
            save();
            send(c, p.request_reply);
            lobby(p, rid);
            for (const auto &token: changed_members) if (token != p.token) lobby(sessions.at(token));
            if (tag == "SETROOM") for (auto &e: sessions) if (e.second.room == p.room && e.first != p.token)
                room_snapshot(e.second);
            return;
        }
        if (tag == "PRESENTATION" && a.empty()) {
            p.presentation = true;
            return;
        }
        if (tag == "GETREPLAY") {
            if (p.account.empty() || p.table.empty() || attr(m, "table") != p.table || tables.at(p.table).game->phase !=
                "complete") {
                send(p, "<REPLAYERROR text=\"只有注册玩家可以保存本人已完成的对局\"/>");
                return;
            }
            auto data = replay_xml(tables.at(p.table));
            if (data.size() > 8 * 1024 * 1024 || !c.replay_data.empty()) {
                send(p, "<REPLAYERROR text=\"牌谱超过大小限制或正在传输\"/>");
                return;
            }
            c.replay_data = std::move(data);
            c.replay_id = p.table;
            c.replay_offset = 0;
            send(p, xml("REPLAYBEGIN", {{"table", c.replay_id}, {"bytes", std::to_string(c.replay_data.size())}}));
            return;
        }
        if (tag == "CHAT") {
            const auto text = attr(m, "text");
            if (text.rfind("%2Flobby%20", 0) != 0) throw std::runtime_error("Chat unsupported");
            int room = number(text.substr(11), 0, 9999);
            if (!p.table.empty()) return; // A committed GO wins a late cancellation barrier.
            if (room != p.room) throw std::runtime_error("Use confirmed room membership");
            lobby(p);
            return;
        }
        if (tag == "JOIN") {
            if (a.empty()) {
                if (p.table.empty()) {
                    p.queued = false;
                    save();
                }
                return;
            }
            if (draining) {
                send(p, "<QUEUEREJECT code=\"DRAINING\"/>");
                return;
            }
            if (!p.table.empty() || p.queued || a.size() != (p.room ? 2u : 1u)) throw std::runtime_error(
                "Invalid/repeated JOIN");
            auto fields = integers(a.at("t"));
            if (fields.size() != 2 || fields[0] != p.room) throw std::runtime_error("Room mismatch");
            if (p.room) {
                if (static_cast<unsigned>(number(a.at("revision"), 1)) != rooms.at(p.room).revision) {
                    send(p, "<QUEUEREJECT code=\"REVISION_CONFLICT\"/>");
                    room_snapshot(p);
                    return;
                }
                for (const auto &e: sessions) if (e.second.room == p.room && !e.second.table.empty()) {
                    send(p, "<QUEUEREJECT code=\"IN_PROGRESS\"/>");
                    return;
                }
            }
            Config config = p.room ? rooms.at(p.room).config : Config::general(fields[1]);
            if (p.room && config.type() != fields[1]) throw std::runtime_error("Room rule mismatch");
            p.rule = fields[1];
            p.queued = true;
            p.queued_at = GetTickCount64();
            p.order = ++order;
            save();
            send(p, xml("QUEUED", {{"room", std::to_string(p.room)}, {"type", std::to_string(p.rule)}}));
            match();
            return;
        }
        if (tag == "BYE" && a.empty()) {
            if (p.table.empty()) throw std::runtime_error("No table to return from");
            auto &t = tables.at(p.table);
            if (t.game->phase != "complete") throw std::runtime_error("Table is not complete");
            auto id = p.table;
            p.table.clear();
            bool used = false;
            for (const auto &e: sessions) used = used || e.second.table == id;
            if (!used) tables.erase(id);
            save();
            if (!draining) match();
            return;
        }
        // Explicit table/generation guards keep late clicks out of the next match.
        if (tag == "EXIT" || tag == "EXITVOTE") {
            if (p.table.empty() || attr(m, "table") != p.table) return;
            auto &t = tables.at(p.table);
            if (t.stage == "complete" || t.stage == "aborted" || t.ranked || (t.general && t.humans.size() > 1)) return;
            if (t.vote_due && GetTickCount64() >= t.vote_due) {
                resolve_exit(t);
                return;
            }
            if (tag == "EXIT") {
                if (a.size() != 1) throw std::runtime_error("Invalid EXIT");
                if (t.humans.size() == 1) {
                    end_exit(t);
                    return;
                }
                if (!t.vote_due) {
                    ++t.exit_revision;
                    t.exit_votes.fill(-1);
                    t.vote_due = GetTickCount64() + 30000;
                    t.vote_until = utc_ms() + 30000;
                    pause(t);
                    save();
                    record("exit_vote_started");
                }
            } else {
                if (a.size() != 3) throw std::runtime_error("Invalid EXITVOTE");
                if (!t.vote_due || number(attr(m, "id"), 1) != static_cast<int>(t.exit_revision)) return;
                auto seat = std::find(t.humans.begin(), t.humans.end(), p.token) - t.humans.begin();
                int yes = number(attr(m, "yes"), 0, 1);
                if (t.exit_votes[seat] < 0) {
                    t.exit_votes[seat] = yes;
                    save();
                }
                if (std::all_of(t.exit_votes.begin(), t.exit_votes.begin() + t.humans.size(),
                                [](int v) { return v >= 0; })) {
                    resolve_exit(t);
                    return;
                }
            }
            for (const auto &token: t.humans) exit_snapshot(t, sessions.at(token));
            return;
        }
        // Packets queued before the abort receipt belong to the removed table.
        // This transient guard ends on the next explicit match (or TCP reconnect).
        if (p.table.empty() && p.exited && (tag == "D" || tag == "N" || tag == "REACH" || tag == "GOK" || tag ==
                                            "STARTREADY" || tag == "NEXTREADY")) return;
        if (p.table.empty()) throw std::runtime_error("No active table");
        auto &t = tables.at(p.table);
        int seat = static_cast<int>(std::find(t.humans.begin(), t.humans.end(), p.token) - t.humans.begin());
        if (tag == "GOK" && a.empty() && t.stage == "gok") {
            t.ready.insert(p.token);
            save();
            return;
        }
        if (tag == "STARTREADY" && a.empty() && t.stage == "prepare") {
            t.ready.insert(p.token);
            save();
            return;
        }
        if (tag == "NEXTREADY" && a.empty() && t.stage == "next") {
            t.ready.insert(p.token);
            save();
            return;
        }
        if (t.paused) {
            restore(t, p);
            return;
        }
        if (t.stage != "play") throw std::runtime_error("Table not accepting actions");
        auto window = number(attr(m, "window"), 1);
        if (static_cast<std::size_t>(window) != t.windows[seat]) {
            t.declared[seat] = -1;
            restore(t, p);
            record("stale_action");
            return;
        }
        a.erase("window");
        Message action_message{m.tag, a};
        auto opts = t.game->options(seat);
        if (tag == "REACH" && a.size() == 1 && a.count("hai") && t.declared[seat] < 0) {
            int tile = number(a.at("hai"), 0, 135);
            if (std::find(opts.begin(), opts.end(), Action{"riichi", {tile}}) == opts.end()) throw std::runtime_error(
                "Illegal REACH");
            t.declared[seat] = tile;
            return;
        }
        auto action = decode(action_message, opts, t.declared[seat]);
        if (t.declared[seat] >= 0 && action.kind != "riichi") throw std::runtime_error(
            "REACH requires selected discard");
        t.declared[seat] = -1;
        if (t.started[seat]) {
            auto elapsed = t.elapsed[seat] + static_cast<int>(GetTickCount64() - t.started[seat]);
            int base = number(t.config.time.substr(0, t.config.time.find('+'))) * 1000;
            t.reserve[seat] = std::max(0, t.reserve[seat] - std::max(0, elapsed - base));
        }
        apply(t, xml("ACT", {{"seat", std::to_string(seat)}, {"kind", action.kind}, {"tiles", list(action.tiles)}}));
        t.deadlines[seat] = t.started[seat] = 0;
        t.offers[seat].clear();
        save();
        flush(t);
    }

    void Service::apply(Table &t, const std::string &text) {
        if (t.commands.size() >= 100000) throw std::runtime_error("Table command capacity");
        auto m = parse(text);
        if (m.tag == "START") t.game->start();
        else if (m.tag == "ACT") t.game->submit(number(m.attrs.at("seat"), 0, 3),
                                                {m.attrs.at("kind"), integers(m.attrs.at("tiles"))});
        else throw std::runtime_error("Invalid replay command");
        t.commands.push_back(text);
    }

    void Service::identity(Table &t, Session &p) {
        int viewer = static_cast<int>(std::find(t.humans.begin(), t.humans.end(), p.token) - t.humans.begin());
        Attrs names;
        std::string genders;
        for (int j = 0; j < 4; ++j) {
            int s = (j + viewer) % 4;
            names["n" + std::to_string(j)] = s >= t.config.n()
                                                 ? ""
                                                 : s < static_cast<int>(t.humans.size())
                                                       ? (sessions.at(t.humans[s]).account.empty()
                                                              ? "NoName"
                                                              : accounts.at(sessions.at(t.humans[s]).account).nickname)
                                                       : "人机" + std::to_string(s + 1);
            genders += s < static_cast<int>(t.humans.size()) ? sessions.at(t.humans[s]).gender : "M";
        }
        names["sx"] = genders;
        std::string ranks, ratings;
        for (int j = 0; j < 4; ++j) {
            if (j) {
                ranks += ',';
                ratings += ',';
            }
            int s = (j + viewer) % 4;
            const bool registered = s < static_cast<int>(t.humans.size()) && !sessions.at(t.humans[s]).account.empty();
            const Rank rank = registered
                                  ? accounts.at(sessions.at(t.humans[s]).account).ranks[t.config.n() == 3 ? 1 : 0]
                                  : Rank{};
            ranks += std::to_string(rank.level);
            ratings += std::to_string(rank.rating);
        }
        names["dan"] = ranks;
        names["rate"] = ratings;
        send(p, xml("UN", names));
        auto plus = t.config.time.find('+');
        send(p, xml("GO", {
                        {"type", std::to_string(t.config.type())}, {"table", t.id}, {"ranked", t.ranked ? "1" : "0"},
                        {"exit", (!t.ranked && (!t.general || t.humans.size() == 1)) ? "1" : "0"},
                        {"open_hands", std::to_string(t.config.open_hands)}, {"red_tiles", list(t.config.red_tiles())},
                        {"room_length", t.config.length},
                        {
                            "local_time",
                            std::to_string(
                                (number(t.config.time.substr(0, plus)) + number(t.config.time.substr(plus + 1))) * 1000)
                        }
                    }));
    }

    void Service::match() {
        std::map<std::pair<int, int>, std::vector<Session *> > queues;
        for (auto &e: sessions) if (e.second.queued && e.second.socket != INVALID_SOCKET) queues[{
            e.second.room, e.second.rule
        }].push_back(&e.second);
        for (auto &q: queues) {
            auto &queue = q.second;
            std::sort(queue.begin(), queue.end(), [](auto a, auto b) { return a->order < b->order; });
            Config config = q.first.first ? rooms.at(q.first.first).config : Config::general(q.first.second);
            while (!queue.empty() && tables.size() < static_cast<std::size_t>(cfg.max_tables)) {
                if (q.first.first && queue.size() < static_cast<std::size_t>(config.n()) && GetTickCount64() - queue.
                    front()->queued_at < 10000) break;
                const int humans = q.first.first ? std::min(config.n(), static_cast<int>(queue.size())) : 1;
                Table t;
                t.id = secret().substr(0, 24);
                t.seed = std::stoull(secret().substr(0, 16), nullptr, 16);
                t.config = config;
                t.general = q.first.first == 0;
                t.game = std::make_unique<Game>(config, t.seed);
                t.due = GetTickCount64() + 10000;
                for (int s = 0; s < humans; ++s) {
                    auto &p = *queue[s];
                    t.humans.push_back(p.token);
                    p.table = t.id;
                    p.queued = false;
                    p.exited = false;
                }
                t.ranked = !t.general && humans == config.n() && std::all_of(
                               t.humans.begin(), t.humans.end(), [&](const std::string &token) {
                                   return !sessions.at(token).account.empty();
                               });
                queue.erase(queue.begin(), queue.begin() + humans);
                auto &stored = tables.emplace(t.id, std::move(t)).first->second;
                save();
                for (const auto &token: stored.humans) identity(stored, sessions.at(token));
                record("table_created", config.n());
            }
        }
    }

    void Service::start(Table &t) {
        t.ready.clear();
        t.deadlines = {};
        t.started = {};
        t.offers = {};
        t.elapsed = {};
        auto plus = t.config.time.find('+');
        t.reserve.fill(number(t.config.time.substr(plus + 1)) * 1000);
        apply(t, "<START/>");
        bool presentation = false;
        for (const auto &token: t.humans) presentation = presentation || sessions.at(token).presentation;
        t.stage = presentation ? "prepare" : "play";
        t.due = GetTickCount64() + (presentation ? 10000 : 0);
        save();
        if (presentation) for (const auto &token: t.humans) if (sessions.at(token).presentation) send(
            sessions.at(token), "<PREPARE/>");
        flush(t, presentation);
    }

    void Service::flush(Table &t, bool opening) {
        if (t.game->phase == "complete" && !t.settled) settle(t);
        const bool paced = std::any_of(t.humans.begin(), t.humans.end(), [&](const std::string &token) {
            return sessions.at(token).presentation;
        });
        while (t.cursor < t.game->events.size()) {
            const auto &e = t.game->events[t.cursor];
            const bool movement = e.tag == "N" || (
                                      !e.tag.empty() && std::string("TUVWtuvwDEFGdefg").find(e.tag[0]) !=
                                      std::string::npos && e.tag.size() > 1 && isdigit(
                                          static_cast<unsigned char>(e.tag[1])));
            if (paced && movement && GetTickCount64() < t.next_event) return;
            ++t.cursor;
            for (std::size_t s = 0; s < t.humans.size(); ++s) {
                auto outgoing = e;
                outgoing.attrs["window"] = std::to_string(t.game->events.size());
                send(sessions.at(t.humans[s]), project(outgoing, static_cast<int>(s), t.config.open_hands));
            }
            if (paced && movement) t.next_event = GetTickCount64() + (e.tag == "N" ? 400 : 280);
            if (opening && e.tag == "INIT") break;
        }
        for (std::size_t s = 0; s < t.humans.size(); ++s) t.windows[s] = t.game->options(static_cast<int>(s)).empty()
                                                                             ? 0
                                                                             : t.game->events.size();
    }

    void Service::pause(Table &t) {
        if (t.paused) return;
        t.paused = true;
        t.paused_at = GetTickCount64();
        for (const auto &token: t.humans) send(sessions.at(token), "<PAUSED show=\"1\"/>");
        record("table_paused");
    }

    void Service::unpause(Table &t) {
        if (t.vote_due || t.stage == "aborted") return;
        for (const auto &token: t.humans) if (sessions.at(token).socket == INVALID_SOCKET) return;
        if (!t.paused) return;
        auto now = GetTickCount64();
        auto delay = now - t.paused_at;
        if (t.due) t.due += delay;
        if (t.next_event) t.next_event += delay;
        for (int s = 0; s < 4; ++s) {
            if (t.deadlines[s]) t.deadlines[s] += delay;
            if (t.started[s]) t.started[s] += delay;
        }
        // restore() reissues readiness requests. An already expired barrier must
        // allow that response to arrive instead of pausing again in this tick.
        if (t.stage == "gok" || t.stage == "prepare") t.due = now + 10000;
        t.paused = false;
        for (const auto &token: t.humans) {
            send(sessions.at(token), "<PAUSED show=\"0\"/>");
            clock(t, sessions.at(token));
        }
        save();
        record("table_resumed");
    }

    void Service::clock(Table &t, Session &p) {
        int s = static_cast<int>(std::find(t.humans.begin(), t.humans.end(), p.token) - t.humans.begin());
        if (t.paused || !t.deadlines[s] || t.game->options(s).empty()) return;
        auto now = GetTickCount64();
        int remaining = t.deadlines[s] > now ? static_cast<int>(t.deadlines[s] - now) : 0;
        int base = number(t.config.time.substr(0, t.config.time.find('+'))) * 1000;
        int spent = t.elapsed[s] + (t.started[s] ? static_cast<int>(now - t.started[s]) : 0);
        send(p, xml("CLOCK",
                    {{"ms", std::to_string(remaining)}, {"base_ms", std::to_string(std::max(0, base - spent))}}));
    }

    void Service::restore(Table &t, Session &p) {
        send(p, "<RESTORE/>");
        identity(t, p);
        int viewer = static_cast<int>(std::find(t.humans.begin(), t.humans.end(), p.token) - t.humans.begin());
        std::size_t begin = 0;
        for (std::size_t i = 0; i < t.game->events.size(); ++i) if (t.game->events[i].tag == "INIT") begin = i;
        for (auto i = begin; i < t.cursor; ++i) {
            auto outgoing = t.game->events[i];
            outgoing.attrs["window"] = std::to_string(t.game->events.size());
            send(p, project(outgoing, viewer, t.config.open_hands));
        }
        send(p, "<RESTORED/>");
        if (t.paused) send(p, "<PAUSED show=\"1\"/>");
        else clock(t, p);
        if (t.stage == "gok") send(p, "<READYREQUEST kind=\"gok\"/>");
        if (t.stage == "prepare" && p.presentation) send(p, "<READYREQUEST kind=\"start\"/>");
        if (t.vote_due) exit_snapshot(t, p);
    }

    void Service::exit_snapshot(Table &t, Session &p) {
        auto seat = std::find(t.humans.begin(), t.humans.end(), p.token) - t.humans.begin();
        auto now = GetTickCount64();
        int yes = 0, no = 0;
        for (std::size_t s = 0; s < t.humans.size(); ++s) {
            yes += t.exit_votes[s] == 1;
            no += t.exit_votes[s] == 0;
        }
        send(p, xml("EXITVOTE", {
                        {"table", t.id}, {"show", "1"}, {"id", std::to_string(t.exit_revision)},
                        {"ms", std::to_string(t.vote_due > now ? t.vote_due - now : 0)},
                        {"total", std::to_string(t.humans.size())}, {"yes", std::to_string(yes)},
                        {"no", std::to_string(no)}, {"choice", std::to_string(t.exit_votes[seat])}
                    }));
    }

    void Service::end_exit(Table &t) {
        t.stage = "aborted";
        t.vote_due = t.vote_until = 0;
        for (const auto &token: t.humans) {
            auto &p = sessions.at(token);
            p.table.clear();
            p.queued = false;
            p.exited = true;
        }
        save(); // Persist removal before publishing the return to the lobby.
        for (const auto &token: t.humans) {
            auto &p = sessions.at(token);
            send(p, xml("TABLEEND", {{"table", t.id}, {"reason", "exit"}}));
            lobby(p);
        }
        record("table_exited");
    }

    void Service::resolve_exit(Table &t) {
        int yes = 0;
        for (std::size_t s = 0; s < t.humans.size(); ++s) yes += t.exit_votes[s] == 1;
        const bool approved = yes * 2 > static_cast<int>(t.humans.size());
        if (approved) {
            end_exit(t);
            return;
        }
        t.vote_due = t.vote_until = 0;
        save();
        for (const auto &token: t.humans) send(sessions.at(token),
                                               xml("EXITVOTE", {
                                                       {"table", t.id}, {"show", "0"},
                                                       {"result", approved ? "exit" : "continue"}
                                                   }));
        unpause(t);
        record("exit_vote_rejected");
    }

    void Service::settle(Table &t) {
        std::vector<int> seats;
        for (int s = 0; s < t.config.n(); ++s) seats.push_back(s);
        std::stable_sort(seats.begin(), seats.end(),
                         [&](int a, int b) { return t.game->scores[a] > t.game->scores[b]; });
        const auto kind = t.config.n() == 3 ? 1 : 0;
        double average = 0;
        if (t.ranked) {
            for (const auto &token: t.humans) average += accounts.at(sessions.at(token).account).ranks[kind].rating;
            average /= t.config.n();
        }
        for (std::size_t s = 0; s < t.humans.size(); ++s) {
            auto &p = sessions.at(t.humans[s]);
            if (p.account.empty()) continue;
            auto &account = accounts.at(p.account);
            ++account.played;
            if (t.ranked) update_rank(account.ranks[kind],
                                      static_cast<int>(
                                          std::find(seats.begin(), seats.end(), static_cast<int>(s)) - seats.begin()),
                                      t.config.n(), t.config.length == "south", average);
        }
        t.settled = true;
        save();
        for (const auto &token: t.humans) profile(sessions.at(token));
        record(t.ranked ? "ranked_complete" : "unranked_complete");
    }

    std::string Service::replay_xml(const Table &t) const {
        Attrs go{{"type", std::to_string(t.config.type())}, {"red_tiles", list(t.config.red_tiles())}};
        Attrs names;
        for (int s = 0; s < 4; ++s) names["n" + std::to_string(s)] = s >= t.config.n()
                                                                         ? ""
                                                                         : s < static_cast<int>(t.humans.size())
                                                                               ? (sessions.at(t.humans[s]).account.
                                                                                   empty()
                                                                                       ? "NoName"
                                                                                       : accounts.at(
                                                                                           sessions.at(t.humans[s]).
                                                                                           account).nickname)
                                                                               : "人机" + std::to_string(s + 1);
        std::string result = "<mjloggm ver=\"2.3\">\n" + xml("GO", go) + "\n" + xml("UN", names) + "\n";
        for (const auto &e: t.game->events) if (e.tag != "PROF") result += xml(e.tag, e.attrs) + "\n";
        return result + "</mjloggm>\n";
    }

    void Service::replay(Table &t) {
        // Guests have no replay/statistics archive. Registered participants may
        // save their completed non-ranked games as well as ranked games.
        if (std::none_of(t.humans.begin(), t.humans.end(), [&](const std::string &token) {
            return !sessions.at(token).account.empty();
        })) return;
        std::filesystem::create_directories(cfg.replays);
        auto path = cfg.replays / (t.id + ".xml");
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("Replay write failed");
        out << replay_xml(t);
        out.flush();
        if (!out) throw std::runtime_error("Replay flush failed");
        auto attrs = t.config.wire();
        attrs["seed"] = std::to_string(t.seed);
        attrs["general"] = std::to_string(t.general);
        attrs["type"] = std::to_string(t.config.type());
        std::string diagnostic = xml("DIAGNOSTIC", attrs) + "\n";
        for (const auto &command: t.commands) diagnostic += command + "\n";
        write_state(cfg.replays / (t.id + ".diagnostic.dat"), diagnostic);
    }

    void Service::table_tick(Table &t) {
        if (t.vote_due && GetTickCount64() >= t.vote_due) resolve_exit(t);
        if (t.paused || t.stage == "complete" || t.stage == "aborted") return;
        const auto now = GetTickCount64();
        if (t.stage == "gok") {
            if (t.ready.size() == t.humans.size()) start(t);
            else if (now >= t.due) throw std::runtime_error("GOK timeout");
            return;
        }
        if (t.stage == "prepare") {
            bool ready = true;
            for (const auto &token: t.humans) if (sessions.at(token).presentation && !t.ready.count(token))
                ready = false;
            if (ready) {
                t.stage = "deal";
                t.due = now + 3200;
                save();
                for (const auto &token: t.humans) send(sessions.at(token), "<START/>");
            } else if (now >= t.due) throw std::runtime_error("STARTREADY timeout");
            return;
        }
        if (t.stage == "deal") {
            if (now < t.due) return;
            t.stage = "play";
            t.due = 0;
        }
        if (t.stage == "next") {
            if (t.ready.size() == t.humans.size()) start(t);
            return;
        }
        flush(t);
        if (t.cursor < t.game->events.size()) return;
        if (t.game->phase == "complete") {
            t.stage = "complete";
            save();
            flush(t);
            replay(t);
            record("table_complete");
            return;
        }
        if (t.game->phase == "ended") {
            t.stage = "next";
            t.ready.clear();
            save();
            flush(t);
            return;
        }
        bool anyhuman = false;
        for (std::size_t s = 0; s < t.humans.size(); ++s) {
            auto opts = t.game->options(static_cast<int>(s));
            auto current = offer(*t.game, static_cast<int>(s));
            if (current.empty()) {
                t.offers[s].clear();
                t.deadlines[s] = t.started[s] = 0;
                continue;
            }
            anyhuman = true;
            if (t.offers[s] != current) {
                t.offers[s] = current;
                t.started[s] = now;
                t.elapsed[s] = 0;
                auto plus = t.config.time.find('+');
                int base = number(t.config.time.substr(0, plus)) * 1000;
                t.deadlines[s] = now + base + t.reserve[s];
                save();
                send(sessions.at(t.humans[s]), xml("CLOCK", {
                                                       {"ms", std::to_string(base + t.reserve[s])},
                                                       {"base_ms", std::to_string(base)}
                                                   }));
            }
            if (now >= t.deadlines[s]) {
                Action a = t.game->phase == "claims"
                               ? Action{"skip"}
                               : *std::find_if(opts.begin(), opts.end(), [](const Action &a) {
                                   return a.kind == "discard";
                               });
                t.reserve[s] = 0;
                apply(t, xml("ACT", {{"seat", std::to_string(s)}, {"kind", a.kind}, {"tiles", list(a.tiles)}}));
                t.offers[s].clear();
                t.deadlines[s] = t.started[s] = 0;
                save();
                flush(t);
                return;
            }
        }
        for (int s = static_cast<int>(t.humans.size()); s < t.config.n(); ++s)
            if (!t.game->options(s).empty()) {
                if (!t.due) {
                    bool paced = std::any_of(t.humans.begin(), t.humans.end(), [&](const std::string &token) {
                        return sessions.at(token).presentation;
                    });
                    t.due = now + (paced ? std::max(900, cfg.bot_delay) : cfg.bot_delay);
                    return;
                }
                if (now < t.due) return;
                auto a = t.game->bot(s);
                apply(t, xml("ACT", {{"seat", std::to_string(s)}, {"kind", a.kind}, {"tiles", list(a.tiles)}}));
                t.due = 0;
                save();
                flush(t);
                return;
            }
        if (!anyhuman && t.game->phase != "ended" && t.game->phase != "complete") throw std::runtime_error(
            "Unserviceable table state");
    }

    void Service::tick() {
        for (auto &e: tables) try { table_tick(e.second); } catch (const StorageFailure &) { throw; } catch (const
            std::exception &) {
            auto &t = e.second;
            pause(t);
            for (const auto &token: t.humans) send(sessions.at(token), "<TABLEERROR code=\"TABLE_STOPPED\"/>");
            save();
            record("table_error");
        }
        bool removed = false;
        for (auto i = tables.begin(); i != tables.end();) {
            if (i->second.stage == "aborted") {
                i = tables.erase(i);
                removed = true;
            } else ++i;
        }
        if (removed) save();
        if (!draining) match();
    }

    int Service::run(HANDLE stop) {
        std::cout << "Native authority ready " << cfg.listen << ':' << cfg.port << std::endl;
        ULONGLONG accept_at = GetTickCount64(), checkpoint_at = accept_at;
        unsigned accept_count = 0;
        for (;;) {
            if (!draining && WaitForSingleObject(stop, 0) == WAIT_OBJECT_0) {
                draining = true;
                drain_until = GetTickCount64() + cfg.shutdown_ms;
                record("draining");
                for (auto &e: sessions) {
                    e.second.queued = false;
                    send(e.second, "<DRAINING/>");
                }
                save();
            }
            if (draining && GetTickCount64()>=drain_until)
            {
                for (auto &e: tables) pause(e.second);
                save();
                record("stopped");
                return 0;
            }
            fd_set readers, writers;
            FD_ZERO(&readers);
            FD_ZERO(&writers);
            if (!draining) FD_SET(listener, &readers);
            for (auto &e: connections) {
                FD_SET(e.first, &readers);
                if (e.second.sent < e.second.output.size()) FD_SET(e.first, &writers);
            }
            timeval timeout{0, 20000};
            if (select(0, &readers, &writers, nullptr, &timeout) == SOCKET_ERROR) throw std::runtime_error(
                "Socket select failed");
            if (FD_ISSET(listener, &readers)) {
                auto socket_ = accept(listener, nullptr, nullptr);
                if (socket_ != INVALID_SOCKET) {
                    if (GetTickCount64() - accept_at >= 1000) {
                        accept_at = GetTickCount64();
                        accept_count = 0;
                    }
                    if (++accept_count > 64 || connections.size() >= static_cast<std::size_t>(cfg.max_connections)) {
                        closesocket(socket_);
                        record("connection_limit");
                    } else {
                        u_long mode = 1;
                        ioctlsocket(socket_, FIONBIO, &mode);
                        Connection c;
                        c.socket = socket_;
                        c.created = c.last = c.rate_at = GetTickCount64();
                        connections.emplace(socket_, std::move(c));
                    }
                }
            }
            std::vector<SOCKET> closed;
            for (auto &e: connections) {
                auto &c = e.second;
                try {
                    if (!c.replay_data.empty() && c.output.size() - c.sent < 128 * 1024) {
                        for (int batch = 0; batch < 8 && c.replay_offset < c.replay_data.size(); ++batch) {
                            const auto count = std::min<std::size_t>(1500, c.replay_data.size() - c.replay_offset);
                            std::string hex;
                            hex.reserve(count * 2);
                            for (std::size_t j = 0; j < count; ++j) {
                                auto b = static_cast<unsigned char>(c.replay_data[c.replay_offset + j]);
                                hex += "0123456789abcdef"[b >> 4];
                                hex += "0123456789abcdef"[b & 15];
                            }
                            send(c, xml("REPLAYCHUNK", {{"offset", std::to_string(c.replay_offset)}, {"data", hex}}));
                            c.replay_offset += count;
                        }
                        if (c.replay_offset == c.replay_data.size()) {
                            send(c, xml("REPLAYEND", {{"table", c.replay_id}}));
                            c.replay_data.clear();
                            c.replay_id.clear();
                        }
                    }
                    if (!c.closing && FD_ISSET(c.socket, &readers)) {
                        char bytes[8192];
                        int count = recv(c.socket, bytes, sizeof(bytes), 0);
                        if (count == 0) {
                            closed.push_back(c.socket);
                            continue;
                        }
                        if (count < 0) {
                            if (WSAGetLastError() != WSAEWOULDBLOCK) closed.push_back(c.socket);
                            continue;
                        }
                        c.last = GetTickCount64();
                        if (c.input.empty()) c.frame_start = c.last;
                        c.input.append(bytes, count);
                        std::size_t end;
                        int batch = 0;
                        while ((end = c.input.find('\0')) != std::string::npos) {
                            if (end > 4096 || ++batch > 128) throw std::runtime_error("Frame/work limit");
                            auto raw = c.input.substr(0, end);
                            c.input.erase(0, end + 1);
                            auto now = GetTickCount64();
                            if (now - c.rate_at >= 1000) {
                                c.rate_at = now;
                                c.rate = 0;
                            }
                            if (++c.rate > 240) throw std::runtime_error("Command rate exceeded");
                            handle(c, parse(raw), raw);
                            if (c.closing) break;
                        }
                        if (c.input.size() > 4096) throw std::runtime_error("Frame size exceeded");
                        if (c.input.empty()) c.frame_start = 0;
                    }
                    if (FD_ISSET(c.socket, &writers)) {
                        int count = ::send(c.socket, c.output.data() + c.sent,
                                           static_cast<int>(c.output.size() - c.sent), 0);
                        if (count > 0) {
                            c.sent += count;
                            if (c.sent == c.output.size()) {
                                c.output.clear();
                                c.sent = 0;
                                c.out_at = 0;
                            }
                        } else if (WSAGetLastError() != WSAEWOULDBLOCK) closed.push_back(c.socket);
                    }
                    auto now = GetTickCount64();
                    if ((c.token.empty() && now - c.created >= 10000) || now - c.last >= 60000 || (
                            c.frame_start && now - c.frame_start >= 10000) || (c.out_at && now - c.out_at >= 10000) || (
                            c.closing && c.output.empty())) closed.push_back(c.socket);
                } catch (const StorageFailure &) { throw; } catch (const std::exception &) {
                    record("protocol_error");
                    c.closing = true;
                    c.input.clear();
                    send(c, "<ERR code=\"400\"/>");
                }
            }
            for (auto s: closed) close(s);
            tick();
            if (GetTickCount64() - checkpoint_at >= 1000) {
                bool active = false;
                for (const auto &e: tables) active = active || (!e.second.paused && e.second.stage != "complete");
                if (active) save();
                checkpoint_at = GetTickCount64();
            }
        }
    }
}
