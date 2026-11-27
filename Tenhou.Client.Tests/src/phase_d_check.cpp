#include "matchmaking_view_model.hpp"
#include "offline_lobby_view_model.hpp"
#include "resource_manager.hpp"
#include "resource_pack.hpp"
#include "room_settings_view_model.hpp"
#include "settings_schema.hpp"
#include "text_rasterizer.hpp"
#include "welcome_view_model.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    void Check(const bool condition, const char *message) {
        if (!condition) throw std::runtime_error(message);
    }

    tenhou::Bytes ReadAllBytes(const std::filesystem::path &path) {
        std::ifstream input(path, std::ios::binary);
        Check(static_cast<bool>(input), ("Cannot open file: " + path.string()).c_str());
        return tenhou::Bytes(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }

    // Portable mock verifying ITextRasterizer can be implemented with zero Win32/GDI+ headers.
    class MockTextRasterizer final : public tenhou::ITextRasterizer {
    public:
        tenhou::PortableTextLayout LayoutPortable(const tenhou::TextKey key) override {
            const auto *def = tenhou::FindText(key);
            return LayoutPortable(def ? def->label : std::string_view{},
                                  def ? def->style : tenhou::TextStyleKey::action_caption);
        }

        tenhou::PortableTextLayout LayoutPortable(const std::string_view utf8_text, tenhou::TextStyleKey) override {
            ++stats_.layouts;
            ++stats_.layout_builds;
            tenhou::PortableTextLayout layout;
            const float width = static_cast<float>(utf8_text.size()) * 10.0f;
            const float height = 20.0f;
            layout.metrics.advance = width;
            layout.metrics.line_height = height;
            layout.metrics.ink = {0.0f, 0.0f, width, height};
            layout.metrics.effects = {-1.0f, -1.0f, width + 2.0f, height + 2.0f};
            return layout;
        }

        tenhou::TextRectF MeasureBoundsPortable(const tenhou::PortableTextContext &context,
                                                const tenhou::PortableTextRun &run) const override {
            const auto &e = run.layout.metrics.effects;
            return {
                static_cast<float>(context.origin_x + run.anchor.x + e.x * context.viewport_scale_x),
                static_cast<float>(context.origin_y + run.anchor.y + e.y * context.viewport_scale_y),
                static_cast<float>(e.width * context.viewport_scale_x),
                static_cast<float>(e.height * context.viewport_scale_y),
            };
        }

        tenhou::PortableTextRaster RasterizePortable(const tenhou::PortableTextContext &context,
                                                     const tenhou::PortableTextRun &run) override {
            ++stats_.rasters;
            ++stats_.raster_builds;
            const auto rect = MeasureBoundsPortable(context, run);
            auto pixels = std::make_shared<tenhou::TextRasterPixels>();
            pixels->width = std::max(1, static_cast<int>(rect.width));
            pixels->height = std::max(1, static_cast<int>(rect.height));
            pixels->bgra.assign(static_cast<std::size_t>(pixels->width * pixels->height * 4), 0xffu);
            stats_.raster_bytes += pixels->bgra.size();
            return {{static_cast<int>(rect.x), static_cast<int>(rect.y), pixels->width, pixels->height}, pixels};
        }

        tenhou::TextCacheStats CacheStats() const override { return stats_; }

        void ClearCaches() override {
            stats_.layouts = 0;
            stats_.rasters = 0;
            stats_.raster_bytes = 0;
        }

    private:
        tenhou::TextCacheStats stats_{};
    };

    void TestPortableSha256() {
        const auto hash_of = [](std::string_view text) {
            return tenhou::Sha256(tenhou::Bytes(text.begin(), text.end()));
        };
        // NIST FIPS 180-4 test vectors.
        Check(hash_of("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
              "SHA-256 empty string vector failed");
        Check(hash_of("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
              "SHA-256 'abc' vector failed");
        Check(hash_of("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
                  "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
              "SHA-256 448-bit vector failed");
        Check(hash_of("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
                      "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu") ==
                  "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1",
              "SHA-256 896-bit vector failed");
    }

    void TestEmbeddedAssets(const std::filesystem::path &assets_root) {
        const struct ExpectedAsset {
            tenhou::AssetId id;
            const char *relative_path;
        } expected[]{
            {tenhou::AssetId::Dialog104, "dialogs/104.bin"},
            {tenhou::AssetId::Dialog105, "dialogs/105.bin"},
            {tenhou::AssetId::Dialog106, "dialogs/106.bin"},
            {tenhou::AssetId::Dialog107, "dialogs/107.bin"},
            {tenhou::AssetId::Dialog108, "dialogs/108.bin"},
            {tenhou::AssetId::Dialog109, "dialogs/109.bin"},
            {tenhou::AssetId::Dialog110, "dialogs/110.bin"},
            {tenhou::AssetId::Dialog114, "dialogs/114.bin"},
            {tenhou::AssetId::TileShadows, "shadows/shadow-hd.dat"},
        };

        for (const auto &entry: expected) {
            const auto embedded = tenhou::ResourceManager::Embedded(entry.id);
            Check(embedded.data != nullptr && embedded.size > 0, "Embedded asset missing");
            const auto on_disk = ReadAllBytes(assets_root / entry.relative_path);
            Check(embedded.size == on_disk.size(), "Embedded asset size mismatch");
            const tenhou::Bytes embedded_bytes(embedded.data, embedded.data + embedded.size);
            Check(tenhou::Sha256(embedded_bytes) == tenhou::Sha256(on_disk), "Embedded asset SHA-256 mismatch");
        }
    }

    void TestPortableTextRasterizer() {
        MockTextRasterizer mock;
        tenhou::ITextRasterizer &rasterizer = mock;
        tenhou::PortableTextContext ctx;
        ctx.viewport_scale_x = 1.5;
        ctx.viewport_scale_y = 1.5;
        tenhou::PortableTextRun run;
        run.layout = rasterizer.LayoutPortable("Tenhou", tenhou::TextStyleKey::action_caption);
        const auto bounds = rasterizer.MeasureBoundsPortable(ctx, run);
        Check(bounds.width > 0.0f && bounds.height > 0.0f, "Portable MeasureBounds failed");
        const auto raster = rasterizer.RasterizePortable(ctx, run);
        Check(raster.pixels && raster.pixels->width > 0 && !raster.pixels->bgra.empty(),
              "Portable Rasterize failed");
        Check(rasterizer.CacheStats().layouts == 1 && rasterizer.CacheStats().rasters == 1,
              "Portable CacheStats failed");
        rasterizer.ClearCaches();
        Check(rasterizer.CacheStats().layouts == 0 && rasterizer.CacheStats().rasters == 0 &&
                  rasterizer.CacheStats().raster_bytes == 0,
              "Portable ClearCaches failed");
    }

    void TestSettingsSchemaAndStore(const std::filesystem::path &temp_dir) {
        const auto &pages = tenhou::SettingsSchemaPages();
        Check(pages.size() == 7, "Expected 7 declarative settings pages");
        const std::array<tenhou::SettingsPageId, 7> order{
            tenhou::SettingsPageId::Tile, tenhou::SettingsPageId::Table, tenhou::SettingsPageId::Sound,
            tenhou::SettingsPageId::Music, tenhou::SettingsPageId::Replay, tenhou::SettingsPageId::Screen,
            tenhou::SettingsPageId::Other};
        for (std::size_t i = 0; i < order.size(); ++i) {
            Check(pages[i].id == order[i], "Settings page order mismatch");
            Check(tenhou::FindSettingsPage(order[i]) == &pages[i], "FindSettingsPage lookup mismatch");
            Check(pages[i].field_count > 0 && pages[i].fields != nullptr, "Settings page fields missing");
        }
        Check(tenhou::SettingsTilePresets().size() == 5, "Tile preset count mismatch");
        Check(tenhou::SettingsBackgroundPresets().size() == 3, "Background preset count mismatch");
        Check(tenhou::SettingsScreenSizePresets().size() == 5, "Screen size preset count mismatch");
        Check(tenhou::SettingsSoundModes().size() == 3, "Sound mode count mismatch");

        const auto ini = temp_dir / "settings_test.ini";
        std::filesystem::remove(ini);
        tenhou::SettingsStore store(ini);

        store.SaveTileAppearance({-5, 300, 38}, 99);
        auto prefs = store.LoadPreferences();
        Check(prefs.tile_rgb == std::array<int, 3>{0, 256, 38} && prefs.tile_shadow == 10,
              "Tile RGB/shadow clamping failed");

        store.ApplyTilePreset(1); // DarkGreen
        prefs = store.LoadPreferences();
        Check(prefs.tile_rgb == std::array<int, 3>{0, 38, 0} && prefs.tile_shadow == 6,
              "ApplyTilePreset failed");

        store.SetCenterPanelHidden(true);
        Check((store.LoadPreferences().flags & 0x2000u) != 0, "Center panel flag set failed");
        store.SetCenterPanelHidden(false);
        Check((store.LoadPreferences().flags & 0x2000u) == 0, "Center panel flag clear failed");

        store.ApplyBackgroundPreset(2); // BG1
        prefs = store.LoadPreferences();
        Check(prefs.background_source == "./BG1" && prefs.background_lhsv[4] == "0",
              "ApplyBackgroundPreset failed");

        store.SaveSoundMode(9);
        Check(store.LoadSoundMode() == 2, "Sound mode clamp failed");
        store.SaveSoundVolume(75);
        Check(store.LoadPreferences().se_volume == 75, "Sound volume persistence failed");

        store.SaveMusicMasterVolume(150);
        Check(store.LoadMusicMasterVolume() == 150, "Music master volume persistence failed");
        store.SaveMusicTrackVolume("BGM0", 120);
        Check(store.LoadMusicTrackVolume("BGM0") == 120, "Music track volume persistence failed");

        store.SaveReplayDirectory("C:/Replays/Tenhou");
        Check(store.LoadReplayDirectory() == "C:/Replays/Tenhou", "Replay dir save failed");
        store.SaveReplayDirectory(std::nullopt);
        Check(store.LoadReplayDirectory().empty(), "Replay dir clear failed");

        Check(store.LoadScreenSize() == "default", "Default screen size mismatch");
        Check(store.SaveScreenSize("1920x1440"), "SaveScreenSize should report changed");
        Check(!store.SaveScreenSize("1920x1440"), "SaveScreenSize duplicate should report unchanged");
        Check(store.LoadScreenSize() == "1920x1440", "LoadScreenSize round-trip failed");
        store.SaveFullscreenSize(std::pair<int, int>{2560, 1440});
        Check(store.LoadFullscreenSize() == "2560,1440", "Fullscreen size save failed");
        store.SaveFullscreenSize(std::nullopt);
        Check(store.LoadFullscreenSize().empty(), "Fullscreen size clear failed");

        store.SaveHiddenId(true);
        Check(store.LoadPreferences().hidden_id, "HiddenId set failed");
        store.SaveOtherFlag(0x423, true);
        store.SaveOtherFlag(0x428, false); // inverted bit 0x10000
        prefs = store.LoadPreferences();
        Check(prefs.Checked(0x423) && !prefs.Checked(0x428), "Other FLAGS0 round-trip failed");
    }

    void TestWelcomeViewModel(const std::filesystem::path &temp_dir) {
        const auto ini = temp_dir / "welcome_test.ini";
        std::filesystem::remove(ini);

        tenhou::WelcomeViewModel vm;
        vm.SetServer(false);
        vm.SetGender('F');
        vm.SetIdentity("Alice01");
        vm.SetOfficialIdentity("ID1234ABCD-5678efgh");
        vm.SetHost(" 192.168.1.10 ");
        vm.SetPortText("10081");
        vm.SetPassword("super-secret");
        vm.SavePreferences(ini);

        tenhou::WelcomeViewModel loaded;
        loaded.LoadPreferences(ini);
        Check(!loaded.official_server() && loaded.gender() == 'F', "Welcome prefs server/gender failed");
        Check(loaded.EffectiveIdentity() == "Alice01", "Welcome prefs identity failed");
        Check(loaded.TrimmedOfficialIdentity() == "ID1234ABCD-5678efgh", "Welcome prefs official ID failed");
        Check(loaded.TrimmedHost() == "192.168.1.10" && loaded.ParsedPort() == 10081, "Welcome host/port failed");
        Check(loaded.password().empty(), "Password must never be persisted");

        loaded.SetHiddenId(true);
        Check(loaded.DisplayIdentity() == tenhou::WelcomeViewModel::kMaskedIdentity, "Hidden ID display mask failed");
        loaded.SetIdentity(std::string(tenhou::WelcomeViewModel::kMaskedIdentity));
        Check(loaded.EffectiveIdentity() == "Alice01", "Setting mask literal must preserve true identity");

        Check(loaded.HasValidOfficialId(), "Valid 19-char official ID rejected");
        loaded.SetOfficialIdentity("BAD-ID");
        Check(!loaded.HasValidOfficialId(), "Malformed official ID accepted");
        loaded.SetServer(true);
        auto invalid_conn = loaded.BuildConnection(false, ini, true);
        Check(!invalid_conn.options.has_value() && !invalid_conn.error_message.empty(),
              "Invalid official connection should fail validation");

        loaded.SetOfficialIdentity("NoName");
        auto official_conn = loaded.BuildConnection(false, ini, false);
        Check(official_conn.options.has_value() && official_conn.options->official &&
                  official_conn.options->host == "b.tenhou.net" && official_conn.options->port == 10080,
              "Official connection build failed");

        loaded.SetServer(false);
        loaded.SetPortText("99999");
        Check(loaded.ParsedPort() == 0, "Out-of-range port accepted");
        auto bad_port = loaded.BuildConnection(false, ini, false);
        Check(!bad_port.options.has_value(), "Bad port connection should fail validation");
    }

    void TestRoomSettingsViewModel() {
        tenhou::ServerRule r1{"riichi", "立直麻将", "riichi-v1", {}};
        for (int i = 0; i < 10; ++i) {
            tenhou::RuleField f;
            f.key = "k" + std::to_string(i);
            f.label = "字段" + std::to_string(i);
            f.initial = "25000";
            f.minimum = "10000";
            f.maximum = "50000";
            f.step = 1000;
            r1.fields.push_back(std::move(f));
        }
        tenhou::ServerRule r2{"custom", "自定义玩法", "mahjong-v1",
                              {{"mode", "局数", "east", "", "", 1, {{"east", "东风"}, {"south", "半庄"}}}}};
        tenhou::ServerRule r3{"unsupported", "不支持", "poker-v1", {}};

        std::map<std::string, tenhou::ServerRule> rules{{"riichi", r1}, {"custom", r2}, {"unsupported", r3}};
        tenhou::RoomSettingsViewModel vm;
        Check(vm.Initialize(rules, {{"rule", "riichi"}, {"stale_key", "999"}}), "RoomSettings initialize failed");
        Check(vm.RuleCount() == 2, "Unsupported presentation was not filtered");
        Check(vm.values().count("stale_key") == 0, "Stale key was not purged");
        Check(!vm.CanPrevPage() && vm.CanNextPage() && vm.PageBegin() == 0 && vm.PageEnd() == 8,
              "Pagination page 0 mismatch");
        Check(vm.NextPage() && vm.PageBegin() == 8 && vm.PageEnd() == 10 && !vm.CanNextPage(),
              "Pagination page 1 mismatch");
        Check(!vm.SetTextField(0, "25500") && !vm.error().empty(), "Step validation should reject 25500");
        Check(vm.SetTextField(0, "30000") && vm.CurrentFieldValue(0) == "30000", "Valid numeric field failed");

        const std::size_t other_idx = vm.selected_rule() == 0 ? 1 : 0;
        Check(vm.SelectRule(other_idx) && vm.page() == 0 && vm.values().size() == 1,
              "SelectRule must reset values and page");
        Check(vm.SetChoiceField(0, 1) && vm.CurrentFieldValue(0) == "south", "Choice field selection failed");
    }

    void TestMatchmakingViewModel() {
        tenhou::MatchmakingViewModel vm;
        tenhou::LobbySnapshot snap;
        snap.native_service = true;
        snap.catalog_ready = true;
        snap.phase = tenhou::LobbyPhase::Lobby;
        snap.queues.push_back({42u, 1u, "四人东风", "riichi"});
        snap.rules["riichi"] = {"riichi", "四人立直", "riichi-v1", {}};
        vm.SetSnapshot(snap);

        auto ui = vm.UiState();
        Check(!ui.custom_mode && ui.can_practice && !ui.in_room, "Initial practice UI state mismatch");
        auto cmd = vm.PracticeSelectedQueue();
        Check(cmd && cmd->action == "match" && cmd->value == 42u, "Practice queue command mismatch");

        vm.SetCustomMode(true);
        Check(!vm.JoinRoomNumber("999").has_value() && !vm.status().empty(), "Invalid 3-digit room accepted");
        auto join = vm.JoinRoomNumber("4321");
        Check(join && join->action == "join" && join->value == 4321u, "Valid join room command failed");

        snap.lobby = "L4321";
        snap.room_settings = {{"owner", "1"}, {"rule", "riichi"}};
        vm.SetSnapshot(snap);
        ui = vm.UiState();
        Check(ui.in_room && ui.is_owner && ui.can_edit_settings && ui.can_close_room && ui.rule_summary == "四人立直",
              "Owner room UI state mismatch");

        snap.room_settings["owner"] = "0";
        vm.SetSnapshot(snap);
        ui = vm.UiState();
        Check(ui.in_room && !ui.is_owner && !ui.can_edit_settings && !ui.can_close_room && ui.can_leave_room,
              "Guest room UI state mismatch");

        snap.native_error = "FULL";
        vm.SetSnapshot(snap);
        Check(vm.status() == "房间已满", "Localized native_error mismatch");
    }

    void TestOfflineLobbyViewModel() {
        tenhou::OfflineLobbyViewModel vm;
        Check(vm.RoomTitle().find("L0000") != std::string::npos, "Default room title mismatch");

        // Offline move with 250ms timing.
        Check(!vm.RequestMove("1234", 1000).has_value(), "Offline move must not emit network command");
        Check(!vm.CompleteOfflineMove(1200), "Offline move completed before 250ms delay");
        Check(vm.CompleteOfflineMove(1250) && vm.lobby().room() == "L1234", "Offline move did not complete at 250ms");

        // Service-backed commands.
        vm.SetMode(false, true);
        auto move_cmd = vm.RequestMove("8141", 2000);
        Check(move_cmd && move_cmd->kind == "change" && move_cmd->argument == "L8141",
              "Service move command mismatch");

        tenhou::LobbySnapshot snap;
        snap.origin_lobby = tenhou::OfflineLobby(tenhou::FixtureProfile());
        snap.origin_lobby->Enter("L8141");
        snap.origin_error = "ACCOUNT_REQUIRED";
        vm.SyncServiceSnapshot(snap);
        Check(vm.lobby().room() == "L8141", "Service snapshot room sync failed");
        Check(vm.ServiceErrorSummary() == "该操作需要已注册账号", "Service error summary mismatch");

        // Contest register -> join -> cancel cycle (offline mode).
        vm.SetMode(false, false);
        const std::uint64_t now = 1'700'000'000ULL;
        tenhou::OfflineContest contest{"测试大会", 4, 9, 10, false, false, false, now - 60, now + 3600};
        vm.lobby().Enter("C00112233");
        vm.lobby().SetContest("C00112233", contest);
        Check(vm.lobby().ContestAction(now) == tenhou::ContestStatus::Register, "Contest initial status mismatch");
        vm.ContestPrimaryAction(now);
        Check(vm.lobby().ContestAction(now) == tenhou::ContestStatus::Open, "Contest should be open after register");
        vm.ContestPrimaryAction(now);
        Check(vm.lobby().ContestAction(now) == tenhou::ContestStatus::Reserved, "Contest should be reserved after join");
        vm.ContestPrimaryAction(now);
        Check(vm.lobby().reservations() == 0, "Contest cancel did not clear reservations");
    }
} // namespace

int main(const int argc, char **argv) {
    try {
        Check(argc >= 2, "Usage: tenhou_phase_d_check <assets-dir>");
        const std::filesystem::path assets_root = std::filesystem::u8path(argv[1]);
        const auto temp_dir = std::filesystem::temp_directory_path() / "tenhou_phase_d_check";
        std::filesystem::create_directories(temp_dir);

        TestPortableSha256();
        TestEmbeddedAssets(assets_root);
        TestPortableTextRasterizer();
        TestSettingsSchemaAndStore(temp_dir);
        TestWelcomeViewModel(temp_dir);
        TestRoomSettingsViewModel();
        TestMatchmakingViewModel();
        TestOfflineLobbyViewModel();

        std::error_code ec;
        std::filesystem::remove_all(temp_dir, ec);
        std::cout << "PHASE_D_CHECK OK\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "PHASE_D_CHECK FAIL: " << error.what() << "\n";
        return 1;
    }
}
