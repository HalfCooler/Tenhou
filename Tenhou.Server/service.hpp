#pragma once
#include <winsock2.h>
#include <windows.h>
#include "game.hpp"
#include "accounts.hpp"
#include <filesystem>
#include <fstream>
#include <memory>

namespace authority {
    struct Settings {
        std::string listen = "127.0.0.1";
        int port = 10080, humans = 1, bot_delay = 900, max_connections = 60, max_sessions = 4096, max_tables = 64,
                shutdown_ms = 5000;
        std::filesystem::path state, logs, replays;
    };

    struct Session {
        std::string token, gender = "M", table;
        int room = 0;
        SOCKET socket = INVALID_SOCKET;
        bool queued = false, presentation = false, exited = false;
        int rule = 0;
        std::uint64_t order = 0, last_request = 0;
        std::string request_text, request_reply, account;
        ULONGLONG queued_at = 0;
    };

    struct Room {
        Config config;
        std::string owner;
        unsigned revision = 1;
        bool closed = false;
    };

    struct Table {
        std::string id, stage = "gok";
        Config config;
        std::uint64_t seed = 0;
        std::vector<std::string> humans;
        std::unique_ptr<Game> game;
        std::vector<std::string> commands;
        std::set<std::string> ready;
        std::size_t cursor = 0;
        bool paused = false, general = false, ranked = false, settled = false;
        ULONGLONG due = 0, paused_at = 0;
        std::array<ULONGLONG, 4> deadlines{}, started{};
        std::array<int, 4> reserve{}, declared{{-1, -1, -1, -1}};
        std::array<std::string, 4> offers;
        std::array<std::size_t, 4> windows{};
        std::array<int, 4> elapsed{};
        ULONGLONG next_event = 0;
        unsigned exit_revision = 0;
        ULONGLONG vote_due = 0, vote_until = 0;
        std::array<int, 4> exit_votes{{-1, -1, -1, -1}};
    };

    struct Connection {
        SOCKET socket = INVALID_SOCKET;
        std::string input, output, token, login_name, replay_data, replay_id;
        bool helo = false, closing = false;
        std::size_t replay_offset = 0;
        std::size_t sent = 0;
        ULONGLONG created = 0, last = 0, frame_start = 0, rate_at = 0, out_at = 0;
        int rate = 0;
    };

    class Service {
    public:
        explicit Service(Settings settings);

        ~Service();

        int run(HANDLE stop);

    private:
        Settings cfg;
        SOCKET listener = INVALID_SOCKET;
        std::map<SOCKET, Connection> connections;
        std::map<std::string, Session> sessions;
        std::map<int, Room> rooms;
        std::map<std::string, Table> tables;
        std::map<std::string, Account> accounts;
        std::uint64_t order = 0;
        bool draining = false;
        ULONGLONG drain_until = 0;
        std::ofstream log;

        void record(const std::string &event, int value = 0);

        static void send(Connection &c, const std::string &text);

        void send(const Session &p, const std::string &text);

        void close(SOCKET socket);

        void handle(Connection &c, const Message &message, const std::string &raw);

        void command(Session &p, Connection &c, const Message &m, const std::string &raw);

        void lobby(Session &p, const std::string &rid = "");

        void room_snapshot(const Session &p, const std::string &rid = "");

        void save() const;

        void load();

        [[nodiscard]] std::string state() const;

        void match();

        void tick();

        void table_tick(Table &t);

        void flush(Table &t, bool opening = false);

        void identity(Table &t, const Session &p);

        void restore(Table &t, Session &p);

        void pause(Table &t);

        void unpause(Table &t);

        void clock(Table &t, Session &p);

        static void apply(Table &t, const std::string &command);

        void start(Table &t);

        void replay(Table &t);

        void profile(Session &p);

        void settle(Table &t);

        std::string replay_xml(const Table &t) const;

        void exit_snapshot(Table &t, Session &p);

        void resolve_exit(Table &t);

        void end_exit(Table &t);
    };
}
