#pragma once
#include "matchmaking.hpp"
#include "network_protocol.hpp"
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

        void Reset();

        const MatchmakingDraft &draft() const { return draft_; }
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
        MatchmakingDraft draft_;
        bool custom_ = false, editing_ = false;
        std::wstring status_;
        std::function<void(const std::string &)> log_;
        Request request_;
        LobbySnapshot snapshot_;
    };
}
