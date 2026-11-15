#include "settlement_view.hpp"
#include "replay_effects.hpp"
#include "lobby_view.hpp"
#include <commctrl.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace tenhou;
namespace tenhou {
    // Exercise the production receive/display boundary with packet bursts and a stalled UI timer.
    struct LobbyPresentationCheck {
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
            receive("START"); view.Disconnect();
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
        Settlement(parent); LobbyPresentationCheck::Run(parent, argv[1]); DestroyWindow(parent); parent = nullptr;
        Gdiplus::GdiplusShutdown(gdiplus);
        std::cout << "PASS side winning-tile geometry and settlement opening/pattern sounds, waits, mute, pause and hide\n";
        return 0;
    } catch (const std::exception &error) {
        if (parent) DestroyWindow(parent);
        if (gdiplus) Gdiplus::GdiplusShutdown(gdiplus);
        std::cerr << error.what() << '\n'; return 1;
    }
}
