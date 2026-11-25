#include "action_captions.hpp"
#include "d2d_table_renderer.hpp"
#include "gdi_table_renderer.hpp"
#include "hand_projection.hpp"
#include "network_protocol.hpp"
#include "replay.hpp"
#include "replay_effects.hpp"
#include "table_render_frame.hpp"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

using namespace tenhou;

namespace {
    struct GdiSession {
        ULONG_PTR token = 0;
        GdiSession() {
            Gdiplus::GdiplusStartupInput input;
            if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok)
                throw std::runtime_error("Cannot start GDI+");
        }
        ~GdiSession() {
            if (token) Gdiplus::GdiplusShutdown(token);
        }
    };

    struct HostWindow {
        HWND hwnd = nullptr;
        HostWindow(int w, int h) {
            WNDCLASSEXW wc{sizeof(wc)};
            wc.lpfnWndProc = DefWindowProcW;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = L"TENHOU_RENDER_BENCH_HOST";
            RegisterClassExW(&wc);
            hwnd = CreateWindowExW(0, wc.lpszClassName, L"Render Bench", WS_POPUP,
                                   -32000, -32000, w, h, nullptr, nullptr, wc.hInstance, nullptr);
            if (!hwnd) throw std::runtime_error("Cannot create benchmark host window");
            ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        }
        void Resize(int w, int h) {
            SetWindowPos(hwnd, nullptr, -32000, -32000, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        ~HostWindow() {
            if (hwnd && IsWindow(hwnd)) DestroyWindow(hwnd);
        }
    };

    struct OffscreenDc {
        HDC dc = nullptr;
        HBITMAP bitmap = nullptr;
        HGDIOBJ old = nullptr;
        OffscreenDc(int w, int h) {
            dc = CreateCompatibleDC(nullptr);
            BITMAPINFO info{};
            info.bmiHeader = {sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
            void *bits = nullptr;
            bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
            old = SelectObject(dc, bitmap);
        }
        ~OffscreenDc() {
            if (old) SelectObject(dc, old);
            if (bitmap) DeleteObject(bitmap);
            if (dc) DeleteDC(dc);
        }
    };

    // Fixed mid-game 4-player Riichi scene with realistic wall, discards, melds, riichi sticks, and active turn.
    OnlineState BuildFixtureState(ReplayEvent &last_event_out, OnlineState &before_discard_out) {
        LobbySession session;
        session.official = session.interactive_games = true;
        session.Connected("ID00000000-00000000", 'M');
        auto feed = [&](const char *xml) {
            const auto msg = ParseNetworkMessage(xml, true);
            (void) session.Receive(msg);
        };
        feed("<HELO uname=\"BenchPlayer\" PF4=\"11,500,1700,0,10,10,10,10,1,100,25,15,30,20\" />");
        feed("<LN n=\"A\" j=\"A\" g=\"A\" />");
        feed("<GO type=\"9\" lobby=\"0\" />");
        feed("<UN n0=\"BenchPlayer\" n1=\"SouthBot\" n2=\"WestBot\" n3=\"NorthBot\" "
             "dan=\"11,10,12,13\" rate=\"1700,1650,1800,1900\" sx=\"M,F,M,M\" />");
        feed("<TAIKYOKU oya=\"0\" />");
        feed("<INIT seed=\"3,2,1,2,3,52\" ten=\"280,240,260,220\" oya=\"0\" "
             "hai=\"0,4,8,12,16,20,24,36,40,44,72,76,80\" />");
        // Sequence of ~24 turns producing realistic discards, a meld, and a riichi stick.
        const char *turns[] = {
            "<T84 />", "<D84 />",
            "<U />", "<e88 />",
            "<V />", "<f92 />",
            "<W />", "<g96 />",
            "<T100 />", "<D100 />",
            "<U />", "<e104 />",
            "<V />", "<f108 />",
            "<W />", "<g112 />",
            "<T116 />", "<D116 />",
            "<U />", "<e120 />",
            "<V />", "<f124 />",
            "<W />", "<g128 />",
            "<T132 />", "<D132 />",
            "<U />", "<e135 />",
            "<V />", "<f131 />",
            "<W />", "<g127 />",
            "<T123 />", "<D123 />",
            "<U />",
            "<REACH who=\"1\" step=\"1\" />",
            "<e119 />",
            "<REACH who=\"1\" step=\"2\" ten=\"280,230,260,220\" />",
            "<V />", "<f115 />",
            "<W />", "<g111 />",
            "<T107 />", "<D107 />",
            "<U />", "<e103 />",
            "<V />", "<f99 />",
            "<W />", "<g95 />",
            "<T91 />"
        };
        for (const char *t : turns) feed(t);
        before_discard_out = session.snapshot.game;
        feed("<D91 />");
        last_event_out.kind = ReplayEventKind::Discard;
        last_event_out.tag = "D91";
        last_event_out.who = 0;
        last_event_out.tile = 91;
        return session.snapshot.game;
    }

    struct BenchSummary {
        double p50_ms = 0;
        double p95_ms = 0;
        double p99_ms = 0;
        double mean_ms = 0;
        D2DTableRenderer::BenchStats stats{};
        std::size_t hud_hits = 0;
    };

    double Percentile(std::vector<double> sorted, double p) {
        if (sorted.empty()) return 0;
        const double idx = p * static_cast<double>(sorted.size() - 1);
        const auto lo = static_cast<std::size_t>(std::floor(idx));
        const auto hi = static_cast<std::size_t>(std::ceil(idx));
        if (lo == hi) return sorted[lo];
        return sorted[lo] + (sorted[hi] - sorted[lo]) * (idx - static_cast<double>(lo));
    }

    enum class Scenario { Static, HoverCountdown, Flight };

    const char *ScenarioName(Scenario s) {
        switch (s) {
            case Scenario::Static: return "static";
            case Scenario::HoverCountdown: return "hover_cd";
            case Scenario::Flight: return "flight";
        }
        return "?";
    }

    std::vector<TableActionButton> MakeButtons(int w, int h, int frame_idx, Scenario scenario) {
        const auto dest = FitNativeCanvasBounds({0, 0, w, h}, 1440);
        const int bw = 140, bh = 44;
        const int y = dest.bottom - 140;
        const int x0 = dest.right - 320;
        std::vector<TableActionButton> buttons;
        TableActionButton skip;
        skip.id = 5128;
        skip.viewport_rect = {x0, y, x0 + bw, y + bh};
        skip.style = TableActionButtonStyle::Sprite440;
        skip.enabled = true;
        skip.hot = (scenario == Scenario::HoverCountdown) && ((frame_idx / 4) % 2 == 1);
        skip.focused = true;
        buttons.push_back(skip);

        TableActionButton riichi;
        riichi.id = 5124;
        riichi.viewport_rect = {x0 + bw + 12, y, x0 + bw * 2 + 12, y + bh};
        riichi.style = TableActionButtonStyle::Sprite440;
        riichi.enabled = true;
        riichi.hot = (scenario == Scenario::HoverCountdown) && ((frame_idx / 4) % 2 == 0);
        buttons.push_back(riichi);
        return buttons;
    }

    BenchSummary RunScenario(ITableCanvasRenderer &renderer,
                             D2DTableRenderer *d2d_ptr,
                             HWND host_hwnd,
                             OffscreenDc *gdi_dc,
                             int out_w, int out_h,
                             Scenario scenario,
                             const OnlineState &before_discard,
                             const OnlineState &after_discard,
                             const ReplayEvent &discard_event,
                             int warmup_frames,
                             int measured_frames) {
        HandProjection hand;
        const auto &geometry = renderer.EnsureGeometry(after_discard.red_tiles);
        const ReplayState before_rep = hand.Project(before_discard);
        const ReplayState after_rep = hand.Project(after_discard);

        TablePresentation presentation;
        ActionCaptions captions;
        if (scenario == Scenario::Flight) {
            presentation.Begin(before_rep, after_rep, discard_event, 240, 1440,
                               [&](const TableTile &t) { return geometry.Origin(t, true); },
                               0.0, false, &geometry, true);
            captions.AcceptResource(473, 1);
        }

        const CanvasRect bounds{0, 0, out_w, out_h};
        std::vector<double> samples;
        samples.reserve(static_cast<std::size_t>(measured_frames));

        auto run_frame = [&](int idx) {
            TableInteraction inter;
            inter.automatic = {true, false, false, false};
            if (scenario == Scenario::HoverCountdown) {
                const auto &h0 = after_discard.players[0].known_hand;
                if (!h0.empty()) inter.hovered = h0[static_cast<std::size_t>(idx) % h0.size()];
                inter.playable = h0;
                inter.hovered_button = (idx % 5) - 1;
                inter.countdown = std::to_string(5 - ((idx / 3) % 5)) + "+10";
                inter.hint = "换三张  1 / 3";
            }
            const double anim_ms = scenario == Scenario::Flight ? static_cast<double>((idx * 12) % 220) : 0.0;
            if (scenario == Scenario::Flight && idx > 0 && (idx * 12) % 220 < 12) {
                presentation.Begin(before_rep, after_rep, discard_event, 240, 1440,
                                   [&](const TableTile &t) { return geometry.Origin(t, true); },
                                   0.0, false, &geometry, true);
            }
            auto frame = BuildOnlineTableRenderFrame(
                1440, bounds, after_discard, hand, geometry, presentation,
                nullptr, &inter, scenario == Scenario::Flight ? &captions : nullptr,
                false, false, true, true, anim_ms, 96);
            frame.buttons = MakeButtons(out_w, out_h, idx, scenario);

            const auto t0 = std::chrono::steady_clock::now();
            if (d2d_ptr && host_hwnd) {
                if (!renderer.RenderToWindow(host_hwnd, frame))
                    throw std::runtime_error("D2D RenderToWindow failed during benchmark");
            } else {
                renderer.RenderToDc(gdi_dc->dc, frame);
            }
            const auto t1 = std::chrono::steady_clock::now();
            return std::chrono::duration<double, std::milli>(t1 - t0).count();
        };

        for (int i = 0; i < warmup_frames; ++i) (void) run_frame(i);
        const std::size_t hits_before = renderer.HudCacheHits();
        if (d2d_ptr) d2d_ptr->ResetStats();

        for (int i = 0; i < measured_frames; ++i) {
            samples.push_back(run_frame(warmup_frames + i));
        }

        BenchSummary summary;
        summary.mean_ms = std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());
        std::sort(samples.begin(), samples.end());
        summary.p50_ms = Percentile(samples, 0.50);
        summary.p95_ms = Percentile(samples, 0.95);
        summary.p99_ms = Percentile(samples, 0.99);
        summary.hud_hits = renderer.HudCacheHits() - hits_before;
        if (d2d_ptr) summary.stats = d2d_ptr->Stats();
        return summary;
    }

    void PrintRow(const char *backend, int w, int h, Scenario s, const BenchSummary &r) {
        std::cout << std::left << std::setw(12) << backend
                  << std::setw(11) << (std::to_string(w) + "x" + std::to_string(h))
                  << std::setw(10) << ScenarioName(s)
                  << std::right << std::fixed << std::setprecision(3)
                  << "  p50=" << std::setw(7) << r.p50_ms << "ms"
                  << "  p95=" << std::setw(7) << r.p95_ms << "ms"
                  << "  p99=" << std::setw(7) << r.p99_ms << "ms"
                  << "  mean=" << std::setw(7) << r.mean_ms << "ms"
                  << "  up=" << std::setw(4) << r.stats.texture_uploads
                  << " (" << std::setw(7) << (r.stats.texture_upload_bytes / 1024) << " KB)"
                  << "  hud_reb=" << std::setw(3) << r.stats.hud_rebuilds
                  << "  int_reb=" << std::setw(3) << r.stats.interaction_rebuilds
                  << "  sdw_reb=" << std::setw(3) << r.stats.shadow_rebuilds
                  << "  base_hit=" << std::setw(3) << r.stats.static_base_hits
                  << '\n';
    }
} // namespace

int main(int argc, char **argv) {
    try {
        std::filesystem::path resources = "build/Tenhou.Client.Tests/resources";
        int frames = 120;
        bool hd_mode = false;
        int positional = 0;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--hd") { hd_mode = true; continue; }
            if (positional == 0) resources = arg;
            else if (positional == 1) frames = std::max(20, std::stoi(arg));
            ++positional;
        }
        const int warmup = 10;

        GdiSession gdi;
        ReplayEvent discard_event;
        OnlineState before_discard;
        const OnlineState after_discard = BuildFixtureState(discard_event, before_discard);
        ClientPreferences prefs;
        prefs.hd_artwork = hd_mode;
        const std::filesystem::path empty_config;

        const std::pair<int, int> resolutions[] = {{1440, 1080}, {2560, 1440}};
        const Scenario scenarios[] = {Scenario::Static, Scenario::HoverCountdown, Scenario::Flight};

        std::cout << "tenhou_render_bench (frames=" << frames << ", warmup=" << warmup << ", vsync=0, mode="
                  << (hd_mode ? "HD" : "Classic") << ")\n";
        std::cout << "---------------------------------------------------------------------------------------------------------------------------------\n";

        for (const auto &[w, h] : resolutions) {
            HostWindow host(w, h);
            OffscreenDc gdi_dc(w, h);

            if (!hd_mode) {
                // 1. GDI+ reference renderer
                GdiTableRenderer gdi_renderer;
                gdi_renderer.Configure(resources, 1440);
                gdi_renderer.SetPreferences(prefs, empty_config, true);
                for (Scenario sc : scenarios) {
                    const auto res = RunScenario(gdi_renderer, nullptr, nullptr, &gdi_dc,
                                                 w, h, sc, before_discard, after_discard, discard_event, warmup, frames);
                    PrintRow("GDI+", w, h, sc, res);
                }
            }

            // 2. Direct2D WARP
            {
                D2DTableRenderer warp(true);
                warp.SetSyncInterval(0);
                warp.Configure(resources, 1440);
                warp.SetPreferences(prefs, empty_config, true);
                for (Scenario sc : scenarios) {
                    const auto res = RunScenario(warp, &warp, host.hwnd, nullptr,
                                                 w, h, sc, before_discard, after_discard, discard_event, warmup, frames);
                    PrintRow(hd_mode ? "D2D-WARP-HD" : "D2D-WARP", w, h, sc, res);
                }
            }

            // 3. Direct2D Hardware (falls back to WARP if no GPU present)
            {
                D2DTableRenderer hw(false);
                hw.SetSyncInterval(0);
                hw.Configure(resources, 1440);
                hw.SetPreferences(prefs, empty_config, true);
                for (Scenario sc : scenarios) {
                    const auto res = RunScenario(hw, &hw, host.hwnd, nullptr,
                                                 w, h, sc, before_discard, after_discard, discard_event, warmup, frames);
                    const char *tag = hd_mode ? (hw.UsingHardware() ? "D2D-HW-HD" : "D2D-FB-HD")
                                              : (hw.UsingHardware() ? "D2D-HW" : "D2D-FALLBK");
                    PrintRow(tag, w, h, sc, res);
                }
            }
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "tenhou_render_bench error: " << e.what() << '\n';
        return 1;
    }
}
