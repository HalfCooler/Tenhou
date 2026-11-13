#include "resource_manager.hpp"
#include "object_pack.hpp"
#include "background.hpp"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: tenhou_resource_check RUNTIME_RESOURCES SOURCE_ASSETS\n";
        return 1;
    }
    ULONG_PTR token = 0;
    try {
        Gdiplus::GdiplusStartupInput startup;
        if (Gdiplus::GdiplusStartup(&token, &startup, nullptr) != Gdiplus::Ok)
            throw std::runtime_error("Cannot initialize GDI+");
        tenhou::ResourceManager resources(argv[1]);
        for (const auto &asset : tenhou::kClientAssets) {
            const auto original = std::filesystem::path(argv[2]) / asset.path;
            if (!asset.required && !std::filesystem::exists(original)) continue;
            if (resources.Read(asset.id) != tenhou::ReadFile(original))
                throw std::runtime_error(std::string("Asset bytes differ: ") + asset.name);
        }
        for (const auto id : {tenhou::AssetId::Background, tenhou::AssetId::BackgroundAlternate}) {
            const auto image = tenhou::BuildBackground(resources.Path(id), 640, 480);
            if (image.width != 640 || image.height != 480 || image.bgra.size() != 640u * 480u * 4u)
                throw std::runtime_error("Background decode failed");
        }
        const auto display = tenhou::LoadResourcePack(resources.Path(tenhou::AssetId::DisplayPack));
        if (display.entries.empty()) throw std::runtime_error("Empty display resource pack");
        const auto table = tenhou::LoadObjectPack(resources.Path(tenhou::AssetId::TablePack));
        const auto companion = tenhou::LoadObjectPack(resources.Path(tenhou::AssetId::TableCompanionPack));
        if (table.entries.empty() || companion.entries.empty()) throw std::runtime_error("Empty table resource pack");
        // Exercise the compatibility path without copying the large packs.
        tenhou::ResourceManager legacy(std::filesystem::path(argv[2]) / "packs");
        if (legacy.Path(tenhou::AssetId::DisplayPack) != resources.Path(tenhou::AssetId::DisplayPack)
            && tenhou::ReadFile(legacy.Path(tenhou::AssetId::DisplayPack)) != resources.Read(tenhou::AssetId::DisplayPack))
            throw std::runtime_error("Legacy flat resource lookup failed");
        Gdiplus::GdiplusShutdown(token);
        std::cout << "PASS client asset bytes, embedded resources, background/pack decoding and legacy paths\n";
        return 0;
    } catch (const std::exception &error) {
        if (token) Gdiplus::GdiplusShutdown(token);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
