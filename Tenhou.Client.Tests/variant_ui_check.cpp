#include "variant_table_view.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
    if (argc != 3) return 1;
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
        snapshot.rules[s.at("rule")] = {s.at("rule"), s.at("title"), "mahjong-v1", {}};
        snapshot.actions.enabled = true; snapshot.actions.revision = 1;
        const auto &encoded = s.at("actions");
        std::size_t start = 0;
        while (start < encoded.size()) {
            const auto end = encoded.find(';', start);
            const auto choice = encoded.substr(start, end == std::string::npos ? end : end - start);
            const auto colon = choice.find(':'); const auto kind = choice.substr(0, colon);
            snapshot.actions.choices.push_back({kind, (kind == "exchange" ? "换三张 " : "打出 ") + choice.substr(colon + 1), {}, {}});
            if (end == std::string::npos) break;
            start = end + 1;
        }
        HWND parent = CreateWindowExW(0, L"STATIC", L"Offscreen UI check", WS_OVERLAPPEDWINDOW,
            0, 0, 1200, 850, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        {
            tenhou::VariantTableView view(parent, [](std::uint64_t, std::size_t) {}, [] {}, [] {});
            view.Move({0, 0, 1200, 850}); view.Set(snapshot); view.Show(true);
            BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth = 1200;
            info.bmiHeader.biHeight = -850; info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
            void *pixels = nullptr; HDC dc = CreateCompatibleDC(nullptr);
            HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0); const auto old = SelectObject(dc, bitmap);
            view.Capture(dc); GdiFlush();
            BITMAPFILEHEADER file{}; file.bfType = 0x4d42; file.bfOffBits = sizeof(file) + sizeof(info.bmiHeader);
            file.bfSize = file.bfOffBits + 1200 * 850 * 4;
            std::ofstream output(argv[2], std::ios::binary); output.write(reinterpret_cast<const char *>(&file), sizeof(file));
            output.write(reinterpret_cast<const char *>(&info.bmiHeader), sizeof(info.bmiHeader));
            output.write(static_cast<const char *>(pixels), 1200 * 850 * 4);
            SelectObject(dc, old); DeleteObject(bitmap); DeleteDC(dc);
        }
        DestroyWindow(parent); std::cout << "Rendered " << argv[2] << '\n'; return 0;
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
