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
                state.players[seat].rank = seat ? 0 : 11;
                state.players[seat].gender = seat ? '?' : 'M';
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
                    if (stage == 2) {
                        state.turn = 3; state.players[0].score += 1000; ++state.events;
                        state.players[3].connected = false;
                    }
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
                const auto &wind = first[0].bounds, &number = first[1].bounds, &unit = first[2].bounds;
                Require(std::abs(2 * (number.left + number.right) - wind.left - wind.right - unit.left - unit.right) <= 4 &&
                    std::abs(2 * (number.top + number.bottom) - wind.top - wind.bottom - unit.top - unit.bottom) <= 4,
                    "Round numeral is not centered between the wind and hand unit");
                scene.Paint(surface.dc, native, native * 3 / 4, surface.width, surface.height, {0, 0, surface.width, surface.height});
                const auto stats = artwork.Text().CacheStats();
                for (int frame = 0; frame < 10; ++frame) {
                    surface.Clear();
                    scene.Paint(surface.dc, native, native * 3 / 4, surface.width, surface.height, {0, 0, surface.width, surface.height});
                }
                const auto after = artwork.Text().CacheStats();
                Require(stats.layout_builds == after.layout_builds && stats.raster_builds == after.raster_builds,
                    "P3 static HUD rebuilds text layouts/rasters");

                for (int hands : {0, 8}) for (bool sichuan : {true, false}) {
                    ReplayState status; status.variant = status.initialized = true;
                    status.variant_rule = sichuan ? "sichuan-bloodbattle" : "guobiao";
                    status.total_hands = hands;
                    const int last = (hands ? hands : sichuan ? 8 : 16) - 1;
                    const auto label = artwork.Text().Layout(sichuan ? TextKey::info_sichuan : TextKey::info_guobiao).data;
                    const auto all_last = artwork.Text().Layout(TextKey::info_all_last).data;
                    for (int round : {last - 1, last}) {
                        status.round = round;
                        FinalScene status_scene(artwork.Text(), native == 1024);
                        artwork.AppendStatus(status_scene, status);
                        const auto runs = status_scene.Snapshot(native, native * 3 / 4, surface.width, surface.height);
                        Require(runs.size() == (round == last ? 2u : 1u) && runs.front().layout == label &&
                            (round < last || runs.back().layout == all_last), "Variant rule label or All Last text is incorrect");
                    }
                }
            }
        }
        void SettlementScores(const std::filesystem::path &resources, const std::filesystem::path &output) {
            for (int rule = 0; rule < 3; ++rule) {
                auto paid = State(rule); paid.round = 0; paid.furiten = false;
                SettlementReceipt a, b; a.who = 0; a.delta = {3000,-1000,-1000,-1000};
                b.who = 1; b.delta = {-2000,2000,0,0}; paid.settlement.receipts = {a,b};
                auto before = paid; before.settlement = {};
                for (int seat = 0; seat < 4; ++seat) before.players[seat].score -= a.delta[seat] + b.delta[seat];
                Surface actual(1197,897), expected(1197,897);
                OnlineTable table, reference; table.Configure(resources,1440); reference.Configure(resources,1440);
                table.Accept(paid, {}, false);
                table.Paint(actual.dc, {0,0,1197,897}, paid);
                reference.Paint(expected.dc, {0,0,1197,897}, before);
                Equal(actual, expected, "Win scores before settlement rule " + std::to_string(rule));
                table.PresentSettlement(); actual.Clear(); expected.Clear();
                auto after = paid; after.settlement = {};
                table.Paint(actual.dc, {0,0,1197,897}, paid);
                reference.Paint(expected.dc, {0,0,1197,897}, after);
                Equal(actual, expected, "Win scores after settlement rule " + std::to_string(rule));
                ReplayEvent init; init.kind = ReplayEventKind::Init;
                table.Accept(before, init, false); table.Accept(paid, {}, false);
                actual.Clear(); expected.Clear();
                table.Paint(actual.dc, {0,0,1197,897}, paid);
                reference.Paint(expected.dc, {0,0,1197,897}, before);
                Equal(actual, expected, "Next-hand score presentation reset");
            }
            auto state = State(1); state.round = 0; state.type = 2;
            auto &top = state.players[2]; top.hidden_count = 4;
            for (int kind: {14,10,4}) {
                ReplayMeld meld; meld.kind = MeldKind::OpenKan; meld.from = kind == 10 ? 0 : 1;
                meld.tiles = {kind*4,kind*4+1,kind*4+2,kind*4+3}; meld.called_tile = kind*4;
                top.melds.push_back(meld);
            }
            OnlineTable table; table.Configure(resources,1440); Surface image(1440,1080);
            table.Paint(image.dc, {0,0,1440,1080}, state);
            image.Save(output / "meld-overlap.png");
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
            if (resume) intro.Resume(state, 0); else intro.Begin(state, 0);
            if (!resume) Require(intro.seconds_ == 10, "Match ready countdown did not start at ten seconds");
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
        static SettlementState SettlementFixture(int rule) {
            SettlementState state;
            SettlementReceipt receipt;
            receipt.who = 0; receipt.from = 1;
            receipt.has_hand = true; receipt.winning_tile = 53;
            receipt.hand = {0,4,8,12,16,20,24,28,32,36,40,52,53,56};
            receipt.base = {25000,25000,25000,25000}; receipt.delta = {12000,-12000,0,0};
            if (rule == 0) {
                receipt.tag = "AGARI";
                receipt.has_yaku = receipt.has_ten = true;
                receipt.dora = {60}; receipt.ura = {64};
                receipt.yaku = {{0,1}, {1,1}, {2,1}, {7,1}, {8,1}, {9,1}, {10,1}, {11,1},
                    {13,1}, {20,1}, {52,123456789}, {48,1}, {999,1}};
                receipt.ten = {30,12000,3};
                receipt.ba = {2, 1};
            } else if (rule == 1) {
                receipt.generic = true; receipt.rule = "sichuan-bloodbattle"; receipt.from = 0;
                receipt.entries = {{"龙七对", 3}, {"根×2", 1}, {"自定义番型", 4, 2}};
                receipt.fan = 5; receipt.points = 1234567890;
            } else {
                receipt.generic = true; receipt.rule = "guobiao"; receipt.fan = 144;
                receipt.flowers = {136, 137};
                receipt.entries = {{"一色四步高"}, {"花牌×2", 2, 3}, {"大四喜"},
                    {"清一色"}, {"明暗杠"}, {"花牌", 2147483647, 2147483647}};
                receipt.entries.resize(14, {"花牌"});
                receipt.entries.push_back({"一个未登记的很长的国标番型名称", 18, 2});
            }
            state.receipts.push_back(std::move(receipt));
            SettlementReceipt draw; draw.tag = "RYUUKYOKU"; draw.reason = "kaze4";
            state.receipts.push_back(draw);
            state.has_final = true;
            state.final_scores = {37000,13000,25000,25000}; state.final_points = {32,-28,-2,-2};
            return state;
        }
        static void SettlementTextLayout(const std::filesystem::path &resources) {
            TextFontService font{ResourceManager(resources)};
            TextRenderer renderer(font);
            const auto find = [](const SettlementScene &scene, const std::string &role, const std::string &value) {
                return std::any_of(scene.text.begin(), scene.text.end(), [&](const SettlementTextItem &item) {
                    return item.role == role && item.text == value && item.visible;
                });
            };
            for (int canvas : {1024, 1440}) for (int rule = 0; rule < 3; ++rule) {
                const auto state = SettlementFixture(rule);
                SettlementLayout layout; layout.width = canvas;
                const auto all = static_cast<std::size_t>(-1);
                const auto hidden = BuildSettlementScene(state, 0, {}, 1, all, false, false, layout);
                const auto scene = BuildSettlementScene(state, 0, {}, all, all, true, true, layout);
                const auto name_role = rule ? "variant_pattern" : "yaku";
                const auto total_role = rule ? "variant_fan" : "total";
                const auto count = [&](const SettlementScene &s) {
                    return std::count_if(s.text.begin(), s.text.end(), [&](const SettlementTextItem &item) {
                        return item.role == name_role || item.role == "unknown_yaku";
                    });
                };
                Require(count(hidden) == 1 && count(scene) == (rule == 1 ? 3 : 14) &&
                    std::none_of(hidden.text.begin(), hidden.text.end(), [&](const SettlementTextItem &item) {
                        return item.role == total_role || item.role == "score_wind";
                    }), "P4 reveal stages changed");
                Require(std::all_of(scene.images.begin(), scene.images.end(), [](const SettlementImageItem &item) {
                    return item.id == 401 || item.id == 402;
                }), "P4 settlement requested a text image");
                if (rule == 0) {
                    Require(find(scene, "yaku", "国士無双13面") && find(scene, "unknown_yaku", "Unknown yaku #999") &&
                        find(scene, "han", "123456789 飜") && find(scene, "total", "30 符  123456801 飜  倍満  12000 点"),
                        "P4 riichi labels, large han or mixed-style total");
                } else if (rule == 1) {
                    Require(find(scene, name_role, "竜七対") && find(scene, name_role, "根×2") &&
                        find(scene, "variant_value", "2 飜") && find(scene, "variant_fan", "5 飜 1234567890 点"),
                        "P4 Sichuan labels, counts or supplied payout");
                } else {
                    Require(find(scene, name_role, "一色四歩高") && find(scene, name_role, "花牌×3") &&
                        find(scene, "variant_value", "32 飜") && find(scene, "variant_value", "6 飜") &&
                        find(scene, "variant_value", "4611686014132420609 飜") &&
                        find(scene, "variant_fan", "144 飜 12000 点"), "P4 Guobiao defaults, numeric range or payout fallback");
                    layout.pattern_page = 1;
                    const auto page = BuildSettlementScene(state, 0, {}, all, all, true, true, layout);
                    Require(count(page) == 1 && find(page, name_role, "一个未登记的很长的国标番型名称") &&
                        find(page, "variant_multiplier", "×2") && find(page, "variant_value", "36 飜") &&
                        find(page, "variant_page", "2 / 2"), "P4 fifteenth pattern or unknown name lost on page two");
                }
                for (const auto &item: scene.text) if (!item.spans.empty()) {
                    const auto context = TextRenderContext::Viewport(scene.width, scene.height, {0,0,1196,897}, canvas == 1024);
                    const auto runs = LayoutSettlementText(renderer, item, canvas == 1024);
                    Require(!runs.empty(), "P4 missing measured run");
                    for (const auto &run: runs) {
                        const auto bounds = TextRenderer::Bounds(context, run);
                        const float guard = 15; // Includes the renderer's device-pixel raster padding.
                        Require(!bounds.IsEmptyArea() && bounds.X >= item.rect.left * context.viewport_scale_x - guard &&
                            bounds.GetRight() <= item.rect.right * context.viewport_scale_x + guard &&
                            bounds.Y >= item.rect.top * context.viewport_scale_y - guard &&
                            bounds.GetBottom() <= item.rect.bottom * context.viewport_scale_y + guard,
                            "P4 measured text escaped its row");
                    }
                }
                if (rule == 0) {
                    const auto draw = BuildSettlementScene(state, 1, {}, all, all, true, true, layout);
                    const auto final = BuildSettlementScene(state, 2, {}, all, all, true, true, layout);
                    Require(find(draw, "reason", "四風連打") && find(final, "final_title", "終局"), "P4 titles lost on receipt switch");
                }
            }
        }
        static void SettlementFrame(HWND parent, const std::filesystem::path &resources, Surface &surface,
                                    int rule, int receipt, bool next_page) {
            SettlementView view(parent); view.Configure(resources, 1440);
            view.SetAnimationEnabled(false);
            view.Move({0,0,surface.width,surface.height});
            view.Set(SettlementFixture(rule), {"Player 1","Player 2","Player 3","Player 4"}, false);
            view.Show(true);
            const auto window = GetDlgItem(parent, 5400);
            const auto choices = GetDlgItem(window, 5401);
            SendMessageW(choices, CB_SETCURSEL, receipt, 0);
            SendMessageW(window, WM_COMMAND, MAKEWPARAM(5401, CBN_SELCHANGE), reinterpret_cast<LPARAM>(choices));
            if (next_page) Require(view.HandleKey(VK_NEXT), "P4 result window did not accept PageDown");
            view.Capture(surface.dc);
            Require(!IsWindowVisible(GetDlgItem(window, 5402)), "P4 production result fell back to resource-error details");
        }
        static void Run(HWND parent, const std::filesystem::path &resources) {
            const auto output = resources.parent_path() / "text-p3";
            std::filesystem::create_directories(output);
            const auto settlement_output = resources.parent_path() / "text-p4";
            std::filesystem::create_directories(settlement_output);
            SettlementTextLayout(resources);
            TextLayers(resources); Online(resources, output); SettlementScores(resources, output);
            {
                Surface surface(1197,897); auto game = State(1);
                OnlineTable table; table.Configure(resources,1440);
                SettlementState state; SettlementReceipt draw; draw.generic = true; draw.who = draw.from = -1;
                draw.rule = "sichuan-bloodbattle"; draw.reason = "查花猪、查大叫、退税";
                draw.base = {50000,50000,50000,50000}; draw.delta = {3000,-1000,-1000,-1000}; state.receipts = {draw};
                SettlementView view(parent); view.Configure(resources,1440); view.SetAnimationEnabled(false);
                view.Move({0,0,1197,897}); view.Set(state, {"Player 1","Player 2","Player 3","Player 4"}, false);
                view.SetBackgroundPainter([&](HDC dc, RECT bounds) { table.Paint(dc,bounds,game); });
                view.Show(true);
                const auto window = GetDlgItem(parent,5400); RECT button{};
                GetWindowRect(GetDlgItem(window,5404), &button);
                MapWindowPoints(nullptr,window,reinterpret_cast<POINT *>(&button),2);
                Require(button.top > 520 && button.bottom < 623, "Compact draw confirmation escaped its panel");
                view.Capture(surface.dc); surface.Save(output / "compact-draw.png");
            }
            const auto isolated = std::filesystem::temp_directory_path() / ("tenhou-text-p3-" + std::to_string(GetCurrentProcessId()));
            struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code ec; std::filesystem::remove_all(path, ec); } } cleanup{isolated};
            std::filesystem::copy(resources, isolated, std::filesystem::copy_options::recursive);
            const auto text_path = [&](unsigned id) { return isolated / "images" / "sprites" / (std::to_string(id) + ".png"); };
            for (const auto &text : kTextDefinitions) std::filesystem::remove(text_path(text.origin_id));
            // Exercise absent, valid and corrupt legacy overrides with fresh production renderers.
            std::filesystem::create_directories(text_path(440).parent_path());
            const auto valid_png = ResourceManager(resources).Read(AssetId::FlowerMei);
            const Bytes invalid_png{'P', '5'};
            for (const auto *legacy_png : {static_cast<const Bytes *>(nullptr), &valid_png, &invalid_png}) {
                if (legacy_png) for (const auto &text : kTextDefinitions)
                    WriteFile(text_path(text.origin_id), *legacy_png);
                DisplayArtwork display; display.Configure(isolated, 1440);
                VariantArtwork overrides; overrides.Configure(isolated);
                for (const auto &text : kTextDefinitions) {
                    Require(!overrides.GetSprite(text.origin_id), "Text OriginId still resolves to a PNG");
                    bool rejected = false;
                    try { display.Get(text.origin_id); } catch (const std::runtime_error &error) {
                        rejected = error.what() == "Unsupported display image ID " + std::to_string(text.origin_id);
                    }
                    Require(rejected, "Text OriginId remains accessible through the display pack");
                }
                Surface original(480, 13 * 56), changed(original.width, original.height);
                Buttons(parent, resources, original); Buttons(parent, isolated, changed); Equal(original, changed, "P3 text PNG independence: buttons");
                if (!legacy_png) original.Save(output / "buttons.png");
                for (bool resume : {false, true}) {
                    Surface source(1197, 897), copy(source.width, source.height);
                    Intro(parent, resources, source, resume); Intro(parent, isolated, copy, resume); Equal(source, copy, "P3 text PNG independence: intro");
                    if (!legacy_png) source.Save(output / (resume ? "intro-resume.png" : "intro-match.png"));
                }
                Surface source(1197, 897), copy(source.width, source.height);
                Replay(parent, resources, source); Replay(parent, isolated, copy); Equal(source, copy, "P3 text PNG independence: replay");
                if (!legacy_png) source.Save(output / "replay.png");
                for (int rule = 0; rule < 3; ++rule) {
                    OnlineTable normal, altered; normal.Configure(resources, 1440); altered.Configure(isolated, 1440);
                    source.Clear(); copy.Clear(); auto state = State(rule);
                    normal.Paint(source.dc, {0, 0, 1197, 897}, state); altered.Paint(copy.dc, {0, 0, 1197, 897}, state);
                    Equal(source, copy, "P3 text PNG independence: online");
                }
                struct ResultCase { int rule, receipt; bool next_page; const char *file; };
                for (const auto &sample : {ResultCase{0, 0, false, "riichi.png"}, {1, 0, false, "sichuan.png"},
                    {2, 0, false, "guobiao-1.png"}, {2, 0, true, "guobiao-2.png"}, {0, 1, false, "draw.png"},
                    {0, 2, false, "final.png"}}) {
                    source.Clear(); copy.Clear();
                    SettlementFrame(parent, resources, source, sample.rule, sample.receipt, sample.next_page);
                    SettlementFrame(parent, isolated, copy, sample.rule, sample.receipt, sample.next_page);
                    Equal(source, copy, "P4 text PNG independence: " + std::string(sample.file));
                    if (!legacy_png) source.Save(settlement_output / sample.file);
                }
            }
            // A real image still reaches the original resource error path.
            std::ofstream(text_path(440), std::ios::binary) << "invalid real image";
            DisplayArtwork broken; broken.Configure(isolated, 1440); bool rejected = false;
            try { broken.Get(440); } catch (const std::exception &) { rejected = true; }
            Require(rejected, "P3 real image corruption was ignored");
            std::cout << "P3 production text: three rules, cached/full/expose, SD text layout, buttons, intro, replay, PNG isolation\n";
        }
    };
    void RunTextTableChecks(HWND parent, const std::filesystem::path &resources) { TextTableCheck::Run(parent, resources); }
}
