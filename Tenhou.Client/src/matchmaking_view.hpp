#pragma once
#include "matchmaking.hpp"
#include "matchmaking_view_model.hpp"
#include "network_protocol.hpp"
#include "original_display.hpp"
#include <windows.h>
#include <functional>

namespace tenhou {
    class MatchmakingView {
    public:
        MatchmakingView(HWND parent, std::function<void(const std::string &)> log);

        ~MatchmakingView();

        void Move(RECT bounds);

        void Show(bool visible);

        void Capture(HDC dc);
        void ConfigureArtwork(const std::filesystem::path &directory);

        void Reset();

        const MatchmakingDraft &draft() const { return vm_.draft(); }
        using Request = std::function<void(const std::string &, unsigned, const std::map<std::string, std::string> &)>;
        void Bind(Request request) { request_ = std::move(request); }

        void SetSnapshot(const LobbySnapshot &snapshot);

    private:
        static LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM);

        void Command(int id);

        void Update();

        void Paint(HDC dc);

        void PaintButton(const DRAWITEMSTRUCT &item);

        HWND window_ = nullptr;
        HFONT font_ = nullptr;
        std::map<int, HWND> controls_;
        MatchmakingViewModel vm_;
        std::wstring status_;
        std::function<void(const std::string &)> log_;
        Request request_;
        DisplayArtwork artwork_;
    };
}
