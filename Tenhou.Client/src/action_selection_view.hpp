#pragma once
#include "action_selection_scene.hpp"
#include "table_scene.hpp"
#include "paint_buffer.hpp"
#include <functional>

namespace tenhou {
class ActionSelectionView {
public:
    ActionSelectionView(HWND parent, std::function<void(std::uint64_t, std::size_t)> submit, std::function<void()> cancel);
    ~ActionSelectionView();
    void Configure(const std::filesystem::path& directory, int width);
    void Set(const ActionSelection& selection, const ActionSnapshot& actions, const OnlineState& game, bool visible);
    void Move(RECT bounds);
    void Capture(HDC dc);
    void Focus();
private:
    static LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM);
    void Arrange();
    void Paint(HDC dc);
    void PaintChoice(const DRAWITEMSTRUCT& item);
    void Command(int id);
    HWND window_ = nullptr;
    std::array<HWND, 9> buttons_{}; // six choices, previous, next, cancel
    HFONT font_ = nullptr;
    PaintBuffer buffer_;
    TableArtwork artwork_;
    std::filesystem::path directory_;
    int width_ = 1440;
    bool red_ = true;
    std::size_t page_ = 0;
    ActionSelection selection_;
    ActionSnapshot actions_;
    OnlineState game_;
    std::function<void(std::uint64_t, std::size_t)> submit_;
    std::function<void()> cancel_;
};
} // namespace tenhou
