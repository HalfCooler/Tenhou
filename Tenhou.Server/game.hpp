#pragma once
#include "common.hpp"
#include <deque>
#include <random>

namespace authority {
    struct Action {
        std::string kind;
        std::vector<int> tiles;
        bool operator==(const Action &a) const { return kind == a.kind && tiles == a.tiles; }
    };

    struct Player {
        std::vector<int> hand, river, nuki;
        std::vector<Meld> melds;
        std::set<int> called, forbidden;
        std::map<int, int> pao;
        bool riichi = false, double_reach = false, ippatsu = false, temp_furiten = false, riichi_furiten = false;
        int drawn = -1, draws = 0;

        [[nodiscard]] bool closed() const {
            return std::none_of(melds.begin(), melds.end(), [](const Meld &m) { return m.opened(); });
        }
    };

    struct Event {
        std::string tag;
        Attrs attrs;
        std::map<int, int> hints;
    };

    class Game {
    public:
        explicit Game(Config config = {}, std::uint64_t seed = 0);

        void start(const std::vector<int> &wall = {});

        [[nodiscard]] std::vector<Action> options(int seat) const;

        void submit(int seat, const Action &action);

        [[nodiscard]] Value value(int seat, int tile, bool tsumo = false, bool chankan = false, bool closed_rob = false) const;

        void invariants() const;

        [[nodiscard]] Action bot(int seat) const;

        [[nodiscard]] std::vector<int> dora(bool ura = false) const;

        [[nodiscard]] int n() const { return cfg.n(); }

        Config cfg;
        std::array<Player, 4> players;
        std::array<int, 4> scores{};
        int round = 0, honba = 0, sticks = 0, dealer = 0, turn = 0, hand_number = 0,
            last_actor = -1, last_tile = -1, pending_riichi = -1, pending_dora = 0,
            dora_count = 1, replacements = 0, next_round = 0, next_honba = 0;
        bool rinshan = false, interrupted = false, kan_discard = false, kan_flip = false;
        std::string phase = "new", claim_kind = "discard";
        std::deque<int> live;
        std::vector<int> dead, sealed;
        std::map<int, std::vector<Action> > claims;
        std::map<int, Action> responses;
        std::vector<Event> events;

    private:
        friend struct ProbeAccess;
        std::mt19937_64 rng;

        void emit(std::string tag, Attrs attrs = {}, std::map<int, int> hints = {});

        void draw(int seat, bool replacement = false, bool nuki = false);

        void discard(int seat, int tile, bool reach);

        void prepare_claims();

        void resolve_claims();

        void interrupt();

        void call(int seat, const Action &action);

        void kan(int seat, const Action &action);

        void finish_kan();

        void flip();

        void win(const std::vector<int> &winners, int source, bool tsumo);

        void draw_end(const std::string &reason = "");

        void end(bool stays, bool draw, std::size_t first);

        [[nodiscard]] std::array<int, 4> transfers(int seat, int source, const Value &value, bool tsumo) const;

        [[nodiscard]] std::map<int, int> hints() const;
    };

    std::string project(const Event &event, int viewer, bool open_hands);

    Action decode(const Message &m, const std::vector<Action> &options, int declared = -1);
}
