#include "settlement_view.hpp"
#include "replay_effects.hpp"
#include "lobby_view.hpp"
#include "tile_shadow.hpp"
#include "wall_scene.hpp"
#include "variant_patterns.hpp"
#include <commctrl.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <set>

using namespace tenhou;
namespace tenhou {
    // Exercise the production receive/display boundary with packet bursts and a stalled UI timer.
    struct LobbyPresentationCheck {
        static void ProtocolOpening(HWND parent, const std::filesystem::path &resources, const std::string &rule) {
            LobbyView view(parent, resources / "presentation-test.ini", [](const std::string &) {});
            view.ConfigureArtwork(resources, 1440); view.sound_enabled_ = false; view.Show(true);
            LobbySession session; session.interactive_games = true;
            session.snapshot.native_service = true; session.snapshot.phase = LobbyPhase::Lobby;
            auto receive = [&](const NetworkMessage &message) {
                session.Receive(message);
                const auto &snapshot = session.snapshot;
                const_cast<LobbySnapshot &>(view.client_->snapshot()) = snapshot;
                view.OnGameEvent(message, snapshot); view.Update(snapshot);
            };
            auto require = [](bool ok, const char *message) { if (!ok) throw std::runtime_error(message); };
            receive({"VGO", {{"rule", rule}, {"table", "opening-check"}}});
            const bool sichuan = rule == "sichuan-bloodbattle";
            std::map<std::string, std::string> fields{
                {"phase", sichuan ? "exchange" : "turn"}, {"seat", "0"}, {"dealer", "0"}, {"turn", "0"},
                {"hand_number", "1"}, {"total_hands", "8"}, {"seq", "1"}, {"event_seq", "1"},
                {"events", "1:start:0:-1:-1"}, {"wall_total", sichuan ? "108" : "144"},
                {"wall", sichuan ? "56" : "92"}, {"wall_front", "52"}, {"window", "1"},
                {"scores", "50000,50000,50000,50000"}, {"hand0", "0,4,8,12,16,20,24,28,32,36,40,44,48"},
                {"hidden0", "13"}, {"hidden1", "13"}, {"hidden2", "13"}, {"hidden3", "13"},
                {"actions", sichuan ? "exchange:0,4,8;exchange:4,8,12" : "discard:0;discard:4"}};
            for (int hand = 1; hand <= 2; ++hand) {
                fields["hand_number"] = std::to_string(hand); fields["seq"] = fields["event_seq"] = std::to_string(hand);
                fields["events"] = std::to_string(hand) + ":start:0:-1:-1";
                receive({"PREPARE", {}}); receive({"VSTATE", fields});
                require(view.opening_.Active() && view.variant_hand_ == hand, "Protocol opening did not install the new hand");
                require(view.match_intro_->Active() == (hand == 1), "First match introduction was skipped or repeated between hands");
                require(!IsWindowVisible(view.controls_.at(5173)), "Exchange confirmation appeared before dealing/selection");
                if (hand == 1) {
                    const auto steps = view.opening_.steps();
                    SendMessageW(view.window_, WM_TIMER, 2, 0);
                    require(view.opening_.steps() == steps && view.match_intro_->Active(),
                        "Deal started before the match introduction was confirmed");
                }
                receive({"READYACK", {{"table", "opening-check"}, {"kind", "STARTREADY"}}});
                receive({"START", {}});
                if (hand == 1) {
                    require(view.match_intro_->Active(), "Server START flashed away the local match introduction");
                    view.match_intro_->Finish();
                }
                require(session.snapshot.actions.choices.size() == 2 && !view.start_barrier_,
                    "Ready acknowledgement erased the initial/next-hand actions");
                if (sichuan) require(view.selection_.Active() && view.selection_.indices.size() == 2,
                    "Ready acknowledgement invalidated the exchange selection");
                for (int tick = 0; tick < 400 && view.opening_.Active(); ++tick) SendMessageW(view.window_, WM_TIMER, 2, 0);
                require(!view.opening_.Active() && !IsWindowVisible(view.controls_.at(5173)),
                    "Exchange confirmation appeared without three selected tiles");
                if (sichuan) {
                    for (int tile: {0, 4, 8}) view.selection_.Pick(view.Snapshot().actions, tile);
                    view.Arrange();
                    require(IsWindowVisible(view.controls_.at(5173)) && IsWindowEnabled(view.controls_.at(5173)),
                        "A legal three-tile exchange has no confirmation button");
                }
            }
            view.Disconnect();
        }
        static void Run(HWND parent, const std::filesystem::path &resources) {
            LobbyView view(parent, resources / "presentation-test.ini", [](const std::string &) {});
            view.ConfigureArtwork(resources, 1440); view.sound_enabled_ = false; view.Show(true);
            LobbySnapshot snapshot; snapshot.variant_active = true; snapshot.phase = LobbyPhase::Game;
            snapshot.actions.enabled = true;
            auto &state = snapshot.variant; state.initialized = true; state.rule = "sichuan-bloodbattle";
            state.local = state.dealer = state.turn = 0; state.hand_number = 1; state.seq = state.event_seq = 1;
            state.phase = VariantPhase::Exchange; state.wall_front = 52; state.wall = 56;
            state.events = {{1, "start", 0, -1, -1}};
            for (auto &player: state.players) player.hidden = 13;
            state.players[0].hand = {0,4,8,12,16,20,24,28,32,36,40,44,48};
            auto receive = [&](const std::string &tag) {
                snapshot.game = VariantTableState(state, {});
                const_cast<LobbySnapshot &>(view.client_->snapshot()) = snapshot;
                view.OnGameEvent({tag, {}}, snapshot); view.Update(snapshot);
            };
            auto require = [](bool ok, const char *message) { if (!ok) throw std::runtime_error(message); };
            receive("VGO"); receive("PREPARE"); receive("VSTATE");
            require(view.opening_.Active() && view.start_barrier_ && view.displayed_.game.events == 1,
                "Variant initial snapshot deadlocked behind PREPARE");
            require(view.match_intro_->Active(), "Variant first hand skipped the match introduction");
            view.match_intro_->Finish();
            receive("START"); require(!view.start_barrier_, "Variant START did not release opening");
            view.opening_.Cancel();
            state.phase = VariantPhase::Turn; state.turn = 1; state.wall_front = 53; state.wall = 55;
            state.players[1].hidden = 14; state.seq = state.event_seq = 2;
            state.events.push_back({2, "draw", 1, -1, -1}); receive("VSTATE");
            require(view.displayed_.game.events == 2 && view.table_.Animating(), "Variant draw did not animate");
            state.phase = VariantPhase::Claims; state.last_actor = 1; state.last_tile = 52;
            state.players[1].hidden = 13; state.players[1].river = {52}; state.seq = state.event_seq = 3;
            state.events.push_back({3, "discard", 1, 52, -1}); receive("VSTATE");
            require(view.displayed_.game.events == 2 && view.presentation_queue_.size() == 1,
                "Packet burst replaced the visible opponent draw");
            view.table_.CancelAnimation();
            for (int i = 0; i < 41; ++i) view.PresentationTick();
            require(view.displayed_.game.events == 2, "Opponent draw lost its think wait");
            view.PresentationTick(); require(view.displayed_.game.events == 3 && view.table_.Animating(),
                "Queued discard never animated after draw wait");
            snapshot.table_paused = true; receive("PAUSED");
            const auto wait = view.pause_ticks_; view.PresentationTick();
            require(view.pause_ticks_ == wait, "Paused table advanced its event queue");
            snapshot.table_paused = false; snapshot.recovering = true; receive("VSTATE");
            require(view.presentation_queue_.empty() && !view.table_.Animating() && !view.captions_.Active(),
                "Recovery replayed queued motion or declarations");
            snapshot.recovering = false; receive("RESTORED");
            ++state.hand_number; ++state.seq; ++state.event_seq; state.phase = VariantPhase::Exchange;
            state.events.push_back({state.event_seq, "start", 0, -1, -1});
            receive("PREPARE"); receive("VSTATE");
            require(view.displayed_.variant.hand_number == 2 && view.opening_.Active(),
                "Next-hand PREPARE deadlocked behind the previous displayed hand");
            receive("START"); view.opening_.Cancel();
            state.rule = "guobiao"; state.wall_total = 144; state.wall_front = 53; state.wall_tail = 2; state.wall = 89;
            state.phase = VariantPhase::Turn; ++state.hand_number; ++state.seq; ++state.event_seq;
            state.players[0].flowers = {136,140}; state.events.push_back({state.event_seq, "start", 0, -1, -1});
            receive("PREPARE"); receive("VSTATE"); receive("START");
            require(view.opening_flowers_[0], "Fresh guobiao deal lost its automatic flower caption");
            for (int i = 0; i < 400 && view.opening_.Active(); ++i) SendMessageW(view.window_, WM_TIMER, 2, 0);
            require(!view.opening_.Active() && view.captions_.Frames().size() == 1 && view.captions_.Frames()[0].resource == 480,
                "Opening flowers never appeared at the end of the deal");
            snapshot.recovering = true; receive("VSTATE");
            require(!view.captions_.Active() && !view.opening_flowers_[0], "Recovery repeated the opening flower effect");
            view.Disconnect();
        }
    };
    struct SettlementPresentationCheck {
        static void Run(HWND parent) {
            auto require = [](bool ok, const char *message) { if (!ok) throw std::runtime_error(message); };
            SettlementView result(parent); result.SetAnimationEnabled(false); result.SetSoundEnabled(false);
            SettlementState state; SettlementReceipt win; win.generic = true; win.who = win.from = 0;
            win.entries = {{"清一色"}}; auto second = win; second.who = second.from = 1;
            auto draw = win; draw.who = draw.from = -1; state.receipts = {win, second, draw};
            int advanced = 0; result.SetConfirmHandler([&] { ++advanced; });
            result.Set(state, {}, false); result.SetContinuation(true); result.SetRoundFinished(true); result.Show(true);
            const auto expire = [&] {
                require(result.confirm_deadline_ != 0 && result.confirm_seconds_ == 5,
                    "A variant receipt has no fresh five-second OK countdown");
                result.confirm_deadline_ = GetTickCount64(); result.ConfirmTick();
            };
            result.SetConfirmEnabled(false);
            require(!result.confirm_deadline_, "Disabled confirmation retained a running countdown");
            result.SetConfirmEnabled(true);
            for (std::size_t i = 0; i < state.receipts.size(); ++i) {
                require(result.selected_ == i && advanced == 0, "Countdown skipped a player or submitted next hand early");
                expire();
            }
            require(advanced == 1, "Last receipt did not automatically continue the hand");
            result.Show(false); require(!result.confirm_deadline_, "Hidden results retained the OK countdown");
            // The final match hand and every page of a long fan list also advance.
            state.receipts = {win}; state.receipts[0].entries.resize(kSettlementPatternsPerPage + 1, {"花牌"});
            result.SetContinuation(false); result.Set(state, {}, false); result.Show(true);
            expire(); require(result.pattern_page_ == 1 && advanced == 1, "Fan page did not auto-advance");
            expire(); require(advanced == 2, "Final hand without continuation had no automatic OK");
            result.Show(false); result.SetRoundFinished(false); result.Show(true);
            require(!result.confirm_deadline_, "History browsing started a live-hand countdown");
            result.Show(false);
        }
    };
}
namespace {
    void Check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
    void Geometry(const std::filesystem::path &resources) {
        TableArtwork art; art.Load(resources, 1440);
        ReplayState state; state.initialized = state.variant = true;
        for (int seat: {1, 3}) {
            auto &player = state.players[seat]; player.hand.assign(13, -1);
            player.retired = true; player.winning_tile = 4;
        }
        const auto scene = BuildTableScene(state);
        for (const auto &winning: scene) if (std::string(winning.role) == "winning") {
            const auto &face = art.Get(winning); const auto shift = art.Offset(winning);
            RECT win{face.x + shift.x, face.y + shift.y,
                face.x + shift.x + face.image.width, face.y + shift.y + face.image.height};
            for (const auto &back: scene) if (back.direction == winning.direction && std::string(back.role) == "retired") {
                const auto &image = art.Get(back); const auto offset = art.Offset(back);
                RECT hand{image.x + offset.x, image.y + offset.y,
                    image.x + offset.x + image.image.width, image.y + offset.y + image.image.height}, intersection{};
                Check(!IntersectRect(&intersection, &win, &hand), "Side winner overlaps the retired hand");
            }
        }
    }
    void Settlement(HWND parent) {
        SettlementState state; SettlementReceipt receipt;
        receipt.generic = true; receipt.who = receipt.from = 0; receipt.fan = 3;
        receipt.entries = {{"清一色"}, {"根×1"}}; state.receipts = {receipt};
        SettlementView result(parent); std::vector<unsigned> sounds;
        result.SetSoundPlayer([&](unsigned id) { sounds.push_back(id); });
        result.SetSoundEnabled(true);
        result.Set(state, {}, false); result.Show(true);
        const auto window = GetDlgItem(parent, 5400);
        Check(result.Animating() && sounds == std::vector<unsigned>{101}, "Generic settlement has no opening sound or wait");
        for (int i = 0; i < 29; ++i) SendMessageW(window, WM_TIMER, 1, 0);
        Check(sounds.size() == 1, "First pattern skipped the opening wait");
        SendMessageW(window, WM_TIMER, 1, 0);
        Check(sounds == std::vector<unsigned>({101, 109}), "First generic pattern is silent");
        for (int i = 0; i < 55; ++i) SendMessageW(window, WM_TIMER, 1, 0);
        Check(sounds == std::vector<unsigned>({101, 109, 109}), "Second generic pattern is silent");
        for (int i = 0; i < 12; ++i) SendMessageW(window, WM_TIMER, 1, 0);
        Check(!result.Animating(), "Generic settlement never finished");
        result.Show(false); result.SetSoundEnabled(false); sounds.clear(); result.Show(true);
        for (int i = 0; i < 120; ++i) SendMessageW(window, WM_TIMER, 1, 0);
        Check(sounds.empty() && !result.Animating(), "Muted settlement played audio or blocked completion");
        result.Show(false); result.SetSoundEnabled(true); result.Show(true); sounds.clear();
        result.SetPaused(true); SendMessageW(window, WM_TIMER, 1, 0);
        Check(sounds.empty(), "Paused settlement advanced");
        result.SetPaused(false); result.Show(false);
        SendMessageW(window, WM_TIMER, 1, 0); Check(sounds.empty(), "Hidden settlement replayed audio");
        // Browsing live receipts must not skip earlier winners when the hand ends.
        result.SetAnimationEnabled(false); result.SetSoundEnabled(false); result.SetContinuation(true);
        SettlementState ordered;
        SettlementReceipt first; first.generic = true; first.who = first.from = 0;
        auto second = first; second.who = second.from = 1;
        auto draw = first; draw.who = draw.from = -1;
        ordered.receipts = {first, second, draw}; result.Set(ordered, {}, false);
        const auto choices = GetDlgItem(window, 5401);
        SendMessageW(choices, CB_SETCURSEL, 1, 0);
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(5401, CBN_SELCHANGE), reinterpret_cast<LPARAM>(choices));
        result.SetRoundFinished(true); result.Show(true);
        Check(SendMessageW(choices, CB_GETCURSEL, 0, 0) == 0, "Hand-end results did not rewind to the first winner");
        int advanced = 0; result.SetConfirmHandler([&] { ++advanced; });
        for (int i = 0; i < 2; ++i) {
            SendMessageW(window, WM_COMMAND, 5404, 0);
            Check(SendMessageW(choices, CB_GETCURSEL, 0, 0) == i + 1 && advanced == 0,
                "Next hand was submitted before all winners and the draw receipt");
        }
        SendMessageW(window, WM_COMMAND, 5404, 0);
        Check(advanced == 1, "Last combined draw receipt did not release confirmation");
        result.Show(false); result.SetConfirmHandler({});
    }

    void Guobiao(HWND parent, const std::filesystem::path &resources) {
        Check(VariantActionButtonSprite(5173, true) == 595 && VariantActionButtonSprite(5173, false) == 599,
            "Exchange confirmation PNGs disagree with Sprites705.csv");
        for (unsigned i = 0; i < 3; ++i)
            Check(VariantActionButtonSprite(5170 + i, true) == 596 + i && VariantActionButtonSprite(5170 + i, false) == 600 + i,
                "Missing-suit button PNGs disagree with Sprites705.csv");
        ReplayState status; status.variant = status.initialized = true; status.variant_rule = "sichuan-bloodbattle";
        status.round = 6;
        Check(VariantStatusSprites(status) == std::vector<unsigned>({704}), "Early Sichuan All Last");
        status.round = 7;
        Check(VariantStatusSprites(status) == std::vector<unsigned>({704,578}), "Missing eighth-hand All Last");
        status.variant_rule = "guobiao"; status.round = 14;
        Check(VariantStatusSprites(status) == std::vector<unsigned>({705}), "Early Guobiao All Last");
        status.round = 15;
        Check(VariantStatusSprites(status) == std::vector<unsigned>({705,578}), "Missing north-four All Last");
        status.total_hands = 8; status.round = 7;
        Check(VariantStatusSprites(status) == std::vector<unsigned>({705,578}), "Configured short Guobiao match ignores its final hand");
        TableArtwork art; art.Load(resources, 1440);
        for (const auto &rule: {"sichuan-bloodbattle", "guobiao"}) {
            const int rounds = std::string(rule) == "guobiao" ? 16 : 8;
            ReplayState hud_state; hud_state.variant = true; hud_state.variant_rule = rule;
            for (int round = 0; round < rounds; ++round) {
                hud_state.round = round;
                const auto hud = BuildOriginalHud(hud_state, 1440);
                Check(hud.sprites[0].id == 407u + round / 4 && hud.sprites[1].id == 403u + round % 4 &&
                    hud.sprites[2].id == 411, "Variant round title does not reuse riichi wind/number/hand artwork");
            }
            ReplayState opening_state; opening_state.variant = opening_state.initialized = true;
            opening_state.wall.InitVariant(rounds == 8 ? 108 : 144, 52, 0, 0);
            const std::vector<int> order{48,0,44,4,40,8,36,12,32,16,28,20,24};
            for (auto &player: opening_state.players) player.hand.assign(13, -1);
            opening_state.players[0].hand = order; std::sort(opening_state.players[0].hand.begin(), opening_state.players[0].hand.end());
            opening_state.players[0].deal_hand = order;
            OpeningPresentation opening; opening.Begin(opening_state, true, &art);
            opening.Tick(opening.wall_tiles() / 4 + 1);
            Check(opening.Frame().players[0].hand == std::vector<int>(order.begin(), order.begin() + 4),
                "Variant first deal packet arrived already sorted");
            while (opening.Active()) opening.Tick();
            auto scene = opening.Scene(art); std::vector<std::pair<int,int>> slots;
            for (const auto &tile: scene) if (tile.tile.direction == 0 && std::string(tile.tile.role) == "hand")
                slots.emplace_back(tile.tile.slot, tile.tile.tile);
            std::sort(slots.begin(), slots.end());
            for (std::size_t i = 0; i < slots.size(); ++i)
                Check(slots[i].second == static_cast<int>(i) * 4, "Variant opening did not sort after the full deal");
            Check(slots.size() == 13, "Variant sort omitted dealt entities");
        }
        ReplayState table; table.initialized = table.variant = true;
        table.wall.InitVariant(144, 0, 0, 0);
        for (int seat = 0; seat < 4; ++seat) {
            auto &player = table.players[seat]; player.hand.assign(10, -1);
            if (!seat) player.hand = {4,8,12,16,20,24,28,32,36,40};
            player.flowers = {136 + seat, 140 + seat};
            ReplayMeld meld; meld.kind = MeldKind::Pon; meld.from = (seat + 1) % 4;
            meld.tiles = {0,1,2}; meld.called_tile = 2; player.melds = {meld};
        }
        const auto table_scene = BuildTableScene(table);
        Check(std::count_if(table_scene.begin(), table_scene.end(), [](const TableTile &t) {
            return std::string(t.role) == "flower"; }) == 4, "Flowers are not public corner counters");
        for (const auto &tile: table_scene) if (std::string(tile.role) == "flower") {
            Check(tile.tile == 136 + tile.direction && tile.slot == 0, "Counter did not retain the first flower");
            for (const auto &meld: table_scene) if (meld.direction == tile.direction && std::string(meld.role) == "meld")
                Check(meld.slot % 20 > 0, "Flower occupies a meld placeholder");
        }
        for (int seat = 0; seat < 4; ++seat) {
            std::set<Bytes> faces;
            for (int entity = 136; entity < 144; ++entity) faces.insert(art.Get({'n', 0, entity, seat}).image.bgra);
            Check(faces.size() == 8, "Flower faces alias each other in the artwork cache");
        }
        std::vector<PositionedTile> wall;
        const auto full = BuildWallScene(table, 144);
        Check(full.size() == 72, "144-tile wall has fewer than 72 stacks");
        for (const auto &tile: full) {
            const auto &im = art.Get(tile); const auto offset = art.Offset(tile);
            RECT rect{im.x + offset.x, im.y + offset.y, im.x + offset.x + im.image.width, im.y + offset.y + im.image.height};
            Check(rect.left >= 0 && rect.top >= 0 && rect.right <= 1440 && rect.bottom <= 1080, "Eighteenth stack leaves the canvas");
            for (const auto &other: table_scene) {
                const auto &image = art.Get(other); const auto shift = art.Offset(other);
                RECT target{image.x + shift.x, image.y + shift.y, image.x + shift.x + image.image.width, image.y + shift.y + image.image.height}, intersection{};
                bool opaque_overlap = false;
                if (IntersectRect(&intersection, &rect, &target))
                    for (LONG y = intersection.top; y < intersection.bottom; ++y)
                        for (LONG x = intersection.left; x < intersection.right; ++x) {
                            const auto a = (std::size_t(y - rect.top) * im.image.width + x - rect.left) * 4 + 3;
                            const auto b = (std::size_t(y - target.top) * image.image.width + x - target.left) * 4 + 3;
                            if (im.image.bgra[a] > 32 && image.image.bgra[b] > 32) opaque_overlap = true;
                        }
                if (opaque_overlap) throw std::runtime_error("Full wall direction=" + std::to_string(tile.direction) +
                    " stack=" + std::to_string(tile.wall_stack) + " overlaps " + other.role + " direction=" + std::to_string(other.direction) +
                    " slot=" + std::to_string(other.slot) + " area=" + std::to_string(intersection.right - intersection.left) + "x" + std::to_string(intersection.bottom - intersection.top));
            }
            auto command = tile;
            if (tile.wall_stack == 17) command = WallTile(table.wall, tile.direction, 17, false);
            const auto &image = art.Get(command); const auto shift = art.Offset(command);
            wall.push_back({command, image.x + shift.x, image.y + shift.y});
        }
        const auto shadows = BuildTileShadows(wall, art, ShadowPack::Embedded());
        Check(shadows.size() == 72, "Eighteenth lower tile lost its shadow to the seventeenth stack");
        for (const auto &shadow: shadows) Check(shadow.ground != nullptr, "Extended wall lacks a ground shadow");
        ReplayState before; before.initialized = before.variant = true; before.wall.InitVariant(144, 52, 0, 0);
        before.players[0].hand = {0,4,8,12,16,20,24,28,32,36,40,44,48};
        auto after = before; after.wall.InitVariant(144, 53, 3, 0); after.players[0].hand.push_back(52); after.players[0].drawn_tile = 52;
        ReplayEvent draw; draw.kind = ReplayEventKind::Draw; draw.who = 0;
        const auto origin = [&](const TableTile &tile) {
            const auto &image = art.Get(tile); const auto shift = art.Offset(tile);
            return std::pair<int,int>{image.x + shift.x, image.y + shift.y};
        };
        const auto transition = BuildReplayTransition(before, after, draw, 12, 1440, origin);
        const auto position = before.wall.Position(141); // Two tail flowers followed by the ordinary replacement.
        const auto expected = origin(WallTile(before.wall, position.first, position.second / 2, position.second % 2 == 0));
        const auto moving = std::find_if(transition.tiles.begin(), transition.tiles.end(), [](const AnimatedTile &tile) { return tile.tile.tile == 52; });
        Check(moving != transition.tiles.end() && moving->from_x == expected.first && moving->from_y == expected.second,
            "Consecutive flowers draw the replacement from the first tail flower instead of the final tile");
        std::size_t patterns = 0;
        for (const auto &pattern: kVariantPatterns) if (std::string(pattern.rule) == "guobiao") {
            ++patterns; Check(FindPatternArtwork("guobiao", pattern.name).has_value(), "Unmapped guobiao fan");
        }
        Check(patterns == 82 && !FindPatternArtwork("guobiao", "../secret"), "Pattern catalogue is incomplete or accepts paths");
        const auto test_directory = std::filesystem::temp_directory_path() / (L"tenhou-variant-art-" + std::to_wstring(GetCurrentProcessId()));
        struct Cleanup { std::filesystem::path directory; ~Cleanup() { std::error_code error; std::filesystem::remove_all(directory, error); } } cleanup{test_directory};
        // Exercise authored PNG overrides independently of which user artwork is installed.
        for (const auto id: {AssetId::Sprite594, AssetId::GuobiaoFan00, AssetId::Sprite599}) {
            const auto path = test_directory / ResourceManager::Info(id).path;
            std::filesystem::create_directories(path.parent_path());
            std::filesystem::copy_file(ResourceManager(resources).Path(AssetId::FlowerMei), path, std::filesystem::copy_options::overwrite_existing);
        }
        VariantArtwork custom; custom.Configure(test_directory);
        Check(custom.GetSprite(594) && custom.Get(AssetId::GuobiaoFan00) && !custom.Get(AssetId::GuobiaoFan01),
            "Authored PNG lookup, Unicode paths or optional fallback failed");
        DisplayArtwork display; display.Configure(test_directory, 1440);
        Check(display.Get(594).bgra == custom.GetSprite(594)->bgra && display.Get(622).bgra == custom.Get(AssetId::GuobiaoFan00)->bgra,
            "Display sprites and fan names use different numbered PNGs");
        DisplayArtwork sd; sd.Configure(test_directory, 1024);
        Check(sd.Get(594).width == static_cast<int>(display.Get(594).width / 1.4), "HD PNG was not scaled for the SD canvas");
        SettlementState state; SettlementReceipt receipt;
        receipt.generic = true; receipt.rule = "guobiao"; receipt.who = receipt.from = 0; receipt.fan = 24;
        for (int i = 0; i < 30; ++i) receipt.entries.push_back(ParseSettlementEntry("花牌×2"));
        state.receipts = {receipt};
        for (std::size_t page = 0; page < 3; ++page) {
            SettlementLayout layout; layout.pattern_page = page;
            const auto scene = BuildSettlementScene(state, 0, {}, static_cast<std::size_t>(-1), 0, true, true, layout);
            Check(std::count_if(scene.text.begin(), scene.text.end(), [](const SettlementTextItem &text) {
                return text.role == "variant_pattern"; }) == (page == 2 ? 2 : 14), "Pattern paging omitted or duplicated entries");
            Check(std::any_of(scene.text.begin(), scene.text.end(), [](const SettlementTextItem &text) {
                return text.role == "variant_fan" && text.text == "24 飜"; }), "Guobiao uses the wrong fan unit");
        }
        SettlementLayout images; images.variant_image_size = [](AssetId) { return SIZE{225,34}; };
        const auto image_scene = BuildSettlementScene(state, 0, {}, static_cast<std::size_t>(-1), 0, true, true, images);
        Check(image_scene.pattern_images.size() == 14, "Numeric pattern PNGs are not connected");
        for (const auto &item: image_scene.pattern_images)
            Check(item.rect.left >= image_scene.panel.left && item.rect.right <= image_scene.panel.right, "Long pattern escaped the settlement panel");
        SettlementReceipt sichuan; sichuan.generic = true; sichuan.rule = "sichuan-bloodbattle";
        sichuan.who = sichuan.from = 0; sichuan.fan = 2; sichuan.points = 2000;
        sichuan.entries = {{"底和", 1, 1}, {"根", 1, 1}};
        const auto sc = BuildSettlementScene(SettlementState{{}, 15, {sichuan}}, 0, {});
        Check(std::count_if(sc.text.begin(), sc.text.end(), [](const SettlementTextItem &t) {
            return t.role == "variant_value" && t.text == "1 飜"; }) == 2, "Sichuan per-pattern values are absent");
        Check(std::any_of(sc.text.begin(), sc.text.end(), [](const SettlementTextItem &t) {
            return t.role == "variant_fan" && t.text == "2 飜 2000 点"; }), "Sichuan total lost the displayed payment");
        sichuan.who = sichuan.from = -1; sichuan.entries.clear(); sichuan.reason = "查花猪、查大叫、退税";
        const auto drawn = BuildSettlementScene(SettlementState{{}, 15, {sichuan}}, 0, {});
        Check(std::count_if(drawn.images.begin(), drawn.images.end(), [](const SettlementImageItem &im) { return im.id == 461; }) == 1 &&
            drawn.score_panels.size() == 4, "Variant draw does not share the riichi title and four payment panels");
        Check(SettlementText(state, 0, {}).find("花牌×2") != std::string::npos, "Text details lost pattern repetitions");
        SettlementView result(parent); result.SetAnimationEnabled(false); result.Set(state, {}, false); result.Show(true);
        const auto window = GetDlgItem(parent, 5400);
        Check(!IsWindowEnabled(GetDlgItem(window, 5405)) && IsWindowEnabled(GetDlgItem(window, 5406)), "Paging controls have the wrong initial state");
        Check(result.HandleKey(VK_NEXT) && result.HandleKey(VK_NEXT), "PageDown is not routed to pattern paging");
        Check(IsWindowEnabled(GetDlgItem(window, 5405)) && !IsWindowEnabled(GetDlgItem(window, 5406)), "Last pattern page is inaccessible");
        result.Show(false);
    }
}
int main(int argc, char **argv) {
    HWND parent = nullptr;
    ULONG_PTR gdiplus = 0;
    try {
        Check(argc == 2, "Usage: tenhou_presentation_check RESOURCES");
        Gdiplus::GdiplusStartupInput input;
        Check(Gdiplus::GdiplusStartup(&gdiplus, &input, nullptr) == Gdiplus::Ok, "Cannot start GDI+");
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES}; InitCommonControlsEx(&controls);
        Geometry(argv[1]);
        parent = CreateWindowExW(0, L"STATIC", L"Presentation checks", WS_POPUP,
            -32000, -32000, 1200, 850, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        Check(parent != nullptr, "Cannot create test parent"); ShowWindow(parent, SW_SHOWNOACTIVATE);
        Settlement(parent); SettlementPresentationCheck::Run(parent); Guobiao(parent, argv[1]);
        LobbyPresentationCheck::ProtocolOpening(parent, argv[1], "sichuan-bloodbattle");
        LobbyPresentationCheck::ProtocolOpening(parent, argv[1], "guobiao");
        LobbyPresentationCheck::Run(parent, argv[1]); DestroyWindow(parent); parent = nullptr;
        Gdiplus::GdiplusShutdown(gdiplus);
        std::cout << "PASS guobiao flowers, 18-stack geometry/shadows, replacement origins, 82 fan mappings, PNG overrides/paging; shared presentation and recovery\n";
        return 0;
    } catch (const std::exception &error) {
        if (parent) DestroyWindow(parent);
        if (gdiplus) Gdiplus::GdiplusShutdown(gdiplus);
        std::cerr << error.what() << '\n'; return 1;
    }
}
