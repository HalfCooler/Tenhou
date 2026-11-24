#include "network_protocol.hpp"
#include "online_table.hpp"
#include "settlement_view.hpp"
#include <objidl.h>
#include <gdiplus.h>
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 3 || argc > 5) return 1;
    try {
        std::ifstream input(argv[1]);
        tenhou::LobbySnapshot snapshot;
        snapshot.variant_active = true; snapshot.phase = tenhou::LobbyPhase::Game;
        std::string line;
        while (std::getline(input, line)) {
            const auto message = tenhou::ParseNetworkMessage(line);
            for (const auto &item: message.attributes) snapshot.variant_state[item.first] = item.second;
        }
        const auto &s = snapshot.variant_state;
        if (s.empty()) return 1;
        HWND parent = CreateWindowExW(0, L"STATIC", L"Offscreen UI check", WS_OVERLAPPEDWINDOW,
            0, 0, 1200, 850, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        Gdiplus::GdiplusStartupInput gdiplus_input;
        ULONG_PTR gdiplus_token = 0;
        if (Gdiplus::GdiplusStartup(&gdiplus_token, &gdiplus_input, nullptr) != Gdiplus::Ok) throw std::runtime_error("Cannot start GDI+");
        {
            BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth = 1200;
            info.bmiHeader.biHeight = -850; info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
            void *pixels = nullptr; HDC dc = CreateCompatibleDC(nullptr);
            HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0); const auto old = SelectObject(dc, bitmap);
            {
                const auto state = tenhou::ParseVariantState(s);
                const auto game = tenhou::VariantTableState(state, s);
                tenhou::OnlineTable table;
                const auto resources = argc >= 4 ? std::filesystem::path(argv[3]) : std::filesystem::path(argv[0]).parent_path() / "resources";
                table.Configure(resources, 1440);
                tenhou::TableInteraction interaction;
                if (state.phase == tenhou::VariantPhase::Exchange) interaction.hint = L"换三张  0 / 3";
                else if (state.phase == tenhou::VariantPhase::Missing) interaction.hint = L"请选择定缺花色：万、筒、索";
                table.Paint(dc, {0, 0, 1200, 850}, game, true, nullptr, &interaction);
                if (argc == 5 && std::string(argv[4]).rfind("--settlement", 0) == 0) {
                    if (game.settlement.receipts.empty()) throw std::runtime_error("No settlement supplied");
                    tenhou::SettlementView result(parent);
                    result.Configure(resources, 1440); result.Move({0, 0, 1200, 850}); result.SetContext(game.dealer, -1, -1);
                    std::array<std::string, 4> names;
                    for (int i = 0; i < 4; ++i) names[i] = game.players[i].name;
                    result.SetBackgroundPainter([&](HDC target, RECT bounds) { table.Paint(target, bounds, game, false); });
                    result.Set(game.settlement, names, false);
                    result.SetAnimationEnabled(false); result.Show(true);
                    if (std::string(argv[4]).rfind("--settlement-page=", 0) == 0) {
                        const int page = std::stoi(std::string(argv[4]).substr(18));
                        if (page < 0 || page > 10) throw std::runtime_error("Invalid settlement page");
                        for (int i = 0; i < page; ++i) SendMessageW(GetDlgItem(parent, 5400), WM_COMMAND, 5406, 0);
                    }
                    SendMessageW(GetDlgItem(parent, 5400), WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_CHILDREN);
                }
            }
            GdiFlush();
            BITMAPFILEHEADER file{}; file.bfType = 0x4d42; file.bfOffBits = sizeof(file) + sizeof(info.bmiHeader);
            file.bfSize = file.bfOffBits + 1200 * 850 * 4;
            std::ofstream output(argv[2], std::ios::binary); output.write(reinterpret_cast<const char *>(&file), sizeof(file));
            output.write(reinterpret_cast<const char *>(&info.bmiHeader), sizeof(info.bmiHeader));
            output.write(static_cast<const char *>(pixels), 1200 * 850 * 4);
            SelectObject(dc, old); DeleteObject(bitmap); DeleteDC(dc);
        }
        Gdiplus::GdiplusShutdown(gdiplus_token);
        DestroyWindow(parent); std::cout << "Rendered " << argv[2] << '\n'; return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
