#pragma once
#include "network_protocol.hpp"
#include <windows.h>
#include <functional>

namespace tenhou {
    class VariantTableView {
    public:
        using Submit = std::function<void(std::uint64_t, std::size_t)>;
        VariantTableView(HWND parent, Submit submit, std::function<void()> next, std::function<void()> exit);
        ~VariantTableView();
        void Move(RECT bounds);
        void Show(bool visible);
        void Set(const LobbySnapshot &snapshot);
        void Capture(HDC dc);
    private:
        static LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM);
        void Paint(HDC dc);
        void Act();
        HWND window_ = nullptr, choices_ = nullptr, submit_ = nullptr, return_ = nullptr, exit_ = nullptr;
        HFONT font_ = nullptr;
        Submit on_submit_;
        std::function<void()> on_return_, on_exit_;
        LobbySnapshot snapshot_;
    };
}
