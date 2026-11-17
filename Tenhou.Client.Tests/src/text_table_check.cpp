#include "lobby_view.hpp"
#include "table_view.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace tenhou {
    namespace {
        void Require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
        struct Surface {
            HDC dc = CreateCompatibleDC(nullptr);
            HBITMAP bitmap = nullptr;
            HGDIOBJ old = nullptr;
            std::uint32_t *pixels = nullptr;
            int width, height;
            Surface(int w, int h) : width(w), height(h) {
                BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                info.bmiHeader.biWidth = w; info.bmiHeader.biHeight = -h;
                info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32;
                void *data = nullptr;
                bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &data, nullptr, 0);
                Require(dc && bitmap, "P3 surface allocation");
                old = SelectObject(dc, bitmap); pixels = static_cast<std::uint32_t *>(data);
                Clear();
            }
            ~Surface() { SelectObject(dc, old); DeleteObject(bitmap); DeleteDC(dc); }
            void Clear() { GdiFlush(); std::fill_n(pixels, std::size_t(width) * height, 0x19332d); }
            std::vector<std::uint32_t> Pixels() const {
                GdiFlush();
                std::vector<std::uint32_t> result(pixels, pixels + std::size_t(width) * height);
                for (auto &p : result) p &= 0xffffff;
                return result;
            }
            void Save(const std::filesystem::path &path) const {
                GdiFlush(); UINT count = 0, bytes = 0;
                Require(Gdiplus::GetImageEncodersSize(&count, &bytes) == Gdiplus::Ok, "P3 encoder size");
                std::vector<BYTE> data(bytes); auto *encoders = reinterpret_cast<Gdiplus::ImageCodecInfo *>(data.data());
                Require(Gdiplus::GetImageEncoders(count, bytes, encoders) == Gdiplus::Ok, "P3 encoders");
                for (UINT i = 0; i < count; ++i) if (wcscmp(encoders[i].MimeType, L"image/png") == 0) {
                    Gdiplus::Bitmap image(width, height, width * 4, PixelFormat32bppRGB, reinterpret_cast<BYTE *>(pixels));
                    Require(image.Save(path.c_str(), &encoders[i].Clsid) == Gdiplus::Ok, "P3 screenshot"); return;
                }
                throw std::runtime_error("P3 PNG encoder unavailable");
            }
        };
        void Equal(const Surface &a, const Surface &b, const std::string &label) {
            const auto x = a.Pixels(), y = b.Pixels();
            const auto mismatch = std::mismatch(x.begin(), x.end(), y.begin());
            if (mismatch.first != x.end()) {
                const auto at = mismatch.first - x.begin();
                throw std::runtime_error(label + " differs at " + std::to_string(at % a.width) + "," + std::to_string(at / a.width));
            }
        }
        OnlineState State(int rule) {
            OnlineState state; state.started = state.initialized = true; state.turn = 1;
            state.variant = rule != 0;
            state.variant_rule = rule == 1 ? "sichuan-bloodbattle" : rule == 2 ? "guobiao" : "";
            state.total_hands = rule == 1 ? 8 : 16; state.round = state.total_hands - 1;
            state.furiten = rule == 0; state.honba = 2; state.sticks = 1;
            state.players[0].known_hand = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48};
            for (int seat = 0; seat < 4; ++seat) {
                if (seat) state.players[seat].hidden_count = 13;
                state.players[seat].name = "Player " + std::to_string(seat + 1);
                state.players[seat].score = 25000 + seat * 100;
            }
            if (rule == 2) state.players[1].flowers = {136, 137};
            if (rule == 1) state.players[0].missing_suit = 2;
            state.wall.InitVariant(rule == 1 ? 108 : rule == 2 ? 144 : 136, 52, 0, 0);
            return state;
        }
        void Online(const std::filesystem::path &resources, const std::filesystem::path &output) {
            for (int rule = 0; rule < 3; ++rule) for (int width : {1024, 1197, 1440}) {
                OnlineTable cached, reference;
                cached.Configure(resources, 1440); reference.Configure(resources, 1440);
                reference.SetReferenceRendering(true);
                auto state = State(rule); TableInteraction interaction; ActionCaptions captions;
                for (int seat = 0; seat < 4; ++seat) captions.AcceptResource(seat == 3 ? 594 : 473 + seat, seat);
                for (int tick = 0; tick < 20; ++tick) captions.Tick();
                Surface actual(width + 26, width * 3 / 4 + 18), expected(actual.width, actual.height);
                for (int stage = 0; stage < 7; ++stage) {
                    if (stage == 1) { interaction.automatic = {false, true, true, true}; interaction.hovered_button = 2; }
                    if (stage == 2) { state.turn = 3; state.players[0].score += 1000; ++state.events; }
                    if (stage == 3) for (int tick = 0; tick < 62; ++tick) captions.Tick();
                    if (stage == 4) { captions.Clear(); state.furiten = false; interaction.hovered_button = -1; }
                    if (stage == 5) actual.Clear(); // Expose/restore from the retained composite.
                    const RECT bounds{13, 9, 13 + width, 9 + width * 3 / 4};
                    cached.Paint(actual.dc, bounds, state, true, nullptr, &interaction, 0, &captions);
                    reference.Paint(expected.dc, bounds, state, true, nullptr, &interaction, 0, &captions);
                    Equal(actual, expected, "P3 online rule " + std::to_string(rule) + " stage " + std::to_string(stage));
                    if (stage == 0 || stage == 4) actual.Save(output / ("table-" + std::to_string(rule) + "-" + std::to_string(width) + "-" + std::to_string(stage) + ".png"));
                }
            }
        }
        void TextLayers(const std::filesystem::path &resources) {
            // Native SD layout does not require the optional original SD image packs.
            for (int native : {1024, 1440}) {
                DisplayArtwork artwork; artwork.Configure(resources, native);
                auto state = OnlineTableState(State(0));
                FinalScene scene(artwork.Text(), native == 1024);
                artwork.AppendHud(scene, state); artwork.AppendChrome(scene, state.type); artwork.AppendStatus(scene, state);
                Surface surface(1197, 897);
                const auto first = scene.Snapshot(native, native * 3 / 4, surface.width, surface.height);
                Require(first.size() == 17, "P3 HUD/Info/option/furiten text command coverage");
                scene.Paint(surface.dc, native, native * 3 / 4, surface.width, surface.height, {0, 0, surface.width, surface.height});
                const auto stats = artwork.Text().CacheStats();
                for (int frame = 0; frame < 10; ++frame) {
                    surface.Clear();
                    scene.Paint(surface.dc, native, native * 3 / 4, surface.width, surface.height, {0, 0, surface.width, surface.height});
                }
                const auto after = artwork.Text().CacheStats();
                Require(stats.layout_builds == after.layout_builds && stats.raster_builds == after.raster_builds,
                    "P3 static HUD rebuilds text layouts/rasters");
            }
        }
    }

    // Friend access is limited to driving the production painters without input automation.
    struct TextTableCheck {
        static void Buttons(HWND parent, const std::filesystem::path &resources, Surface &surface) {
            LobbyView view(parent, resources / "p3-test.ini", [](const std::string &) {});
            view.ConfigureArtwork(resources, 1440);
            int row = 0;
            for (unsigned control : {5121u,5122u,5123u,5124u,5125u,5126u,5127u,5128u,5151u,5170u,5171u,5172u,5173u}) {
                for (UINT state : {0u, unsigned(ODS_SELECTED), unsigned(ODS_DISABLED)}) {
                    const int saved = SaveDC(surface.dc);
                    SetViewportOrgEx(surface.dc, static_cast<int>(state == ODS_SELECTED ? 160 : state == ODS_DISABLED ? 320 : 0), row * 56, nullptr);
                    DRAWITEMSTRUCT item{}; item.CtlID = control; item.hDC = surface.dc;
                    item.hwndItem = view.controls_.at(control); item.rcItem = {0, 0, 140, 50}; item.itemState = state;
                    view.PaintAction(item);
                    RestoreDC(surface.dc, saved);
                    Require(view.action_artwork_error_.empty(), "P3 production button renderer failed");
                }
                ++row;
            }
        }
        static void Intro(HWND parent, const std::filesystem::path &resources, Surface &surface, bool resume) {
            MatchIntroView intro(parent, [] {}); intro.Configure(resources, 1440);
            intro.Move({0, 0, surface.width, surface.height});
            const auto state = OnlineTableState(State(2));
            if (resume) intro.Resume(state, 0, "1,2,3,4"); else intro.Begin(state, 0, "1,2,3,4");
            intro.Capture(surface.dc);
            Require(intro.artwork_.Text().CacheStats().raster_builds >= 9, "P3 intro did not draw title/round/winds/Info");
        }
        static void Replay(HWND parent, const std::filesystem::path &resources, Surface &surface) {
            TableView view(parent, [](const std::string &) {}, resources / "p3-replay.ini");
            view.ConfigureArtwork(resources, 1440);
            // Keep the production replay PaintTable path and inject only a public state fixture.
            view.session_ = std::make_unique<ReplaySession>(ReplayLog::Parse("<mjloggm ver=\"2.3\"><INIT seed=\"0,0,0,1,1,52\" ten=\"250,250,250,250\" oya=\"0\" hai0=\"0,4,8,12,16,20,24,28,32,36,40,44,48\" hai1=\"1,5,9,13,17,21,25,29,33,37,41,45,49\" hai2=\"2,6,10,14,18,22,26,30,34,38,42,46,50\" hai3=\"3,7,11,15,19,23,27,31,35,39,43,47,51\"/></mjloggm>"));
            view.session_->Step();
            view.captions_.AcceptResource(478, 0);
            for (int tick = 0; tick < 20; ++tick) view.captions_.Tick();
            view.PaintTable(surface.dc, {0, 0, surface.width, surface.height});
            Require(view.artwork_error_.empty(), "P3 replay table renderer failed");
        }
        static void Run(HWND parent, const std::filesystem::path &resources) {
            const auto output = resources.parent_path() / "text-p3";
            std::filesystem::create_directories(output);
            TextLayers(resources); Online(resources, output);
            const auto isolated = std::filesystem::temp_directory_path() / ("tenhou-text-p3-" + std::to_string(GetCurrentProcessId()));
            struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code ec; std::filesystem::remove_all(path, ec); } } cleanup{isolated};
            std::filesystem::copy(resources, isolated, std::filesystem::copy_options::recursive);
            const auto text_path = [&](unsigned id) { return isolated / "images" / "sprites" / (std::to_string(id) + ".png"); };
            for (const auto &text : kTextDefinitions) std::filesystem::remove(text_path(text.origin_id));
            // The missing and corrupt PNG variants each use fresh production renderers.
            for (int pass = 0; pass < 2; ++pass) {
                if (pass) for (const auto &text : kTextDefinitions) {
                    const auto path = text_path(text.origin_id); std::filesystem::create_directories(path.parent_path());
                    std::ofstream(path, std::ios::binary) << "deliberately invalid old text PNG";
                }
                Surface original(480, 13 * 56), changed(original.width, original.height);
                Buttons(parent, resources, original); Buttons(parent, isolated, changed); Equal(original, changed, "P3 text PNG independence: buttons");
                if (pass) original.Save(output / "buttons.png");
                for (bool resume : {false, true}) {
                    Surface source(1197, 897), copy(source.width, source.height);
                    Intro(parent, resources, source, resume); Intro(parent, isolated, copy, resume); Equal(source, copy, "P3 text PNG independence: intro");
                    if (pass) source.Save(output / (resume ? "intro-resume.png" : "intro-match.png"));
                }
                Surface source(1197, 897), copy(source.width, source.height);
                Replay(parent, resources, source); Replay(parent, isolated, copy); Equal(source, copy, "P3 text PNG independence: replay");
                if (pass) source.Save(output / "replay.png");
                for (int rule = 0; rule < 3; ++rule) {
                    OnlineTable normal, altered; normal.Configure(resources, 1440); altered.Configure(isolated, 1440);
                    source.Clear(); copy.Clear(); auto state = State(rule);
                    normal.Paint(source.dc, {0, 0, 1197, 897}, state); altered.Paint(copy.dc, {0, 0, 1197, 897}, state);
                    Equal(source, copy, "P3 text PNG independence: online");
                }
            }
            // A real image still reaches the original resource error path.
            std::filesystem::create_directories(text_path(440).parent_path());
            std::ofstream(text_path(440), std::ios::binary) << "invalid real image";
            DisplayArtwork broken; broken.Configure(isolated, 1440); bool rejected = false;
            try { broken.Get(440); } catch (const std::exception &) { rejected = true; }
            Require(rejected, "P3 real image corruption was ignored");
            std::cout << "P3 production text: three rules, cached/full/expose, SD text layout, buttons, intro, replay, PNG isolation\n";
        }
    };
    void RunTextTableChecks(HWND parent, const std::filesystem::path &resources) { TextTableCheck::Run(parent, resources); }
}
