#pragma once
#include "client_preferences.hpp"
#include "offline_lobby.hpp"
#include "original_display.hpp"
#include "paint_buffer.hpp"
#include <array>
#include <functional>
#include <vector>

namespace tenhou {
    struct LobbySnapshot;

    class OfflineLobbyView {
    public:
        enum class Action { Settings, OpenReplay, Logout, RoomConfirmed };
        using Submit = std::function<void(const std::string &, const std::string &)>;
        OfflineLobbyView(HWND parent, std::filesystem::path config, std::function<void(Action)> action,
                         std::function<void(const std::string &)> log, Submit submit = {});
        ~OfflineLobbyView();

        void ConfigureArtwork(const std::filesystem::path &resources, int canvas_width = 1440);
        void Resize(int width, int height) const;
        void Show(bool visible);
        void SetServiceSnapshot(const LobbySnapshot &snapshot);
        void Capture(HDC dc);
        bool HandleKey(UINT key);
        bool contest() const { return state_.contest(); }

    private:
        static LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM);
        static LRESULT CALLBACK ButtonProcedure(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

        void AddButton(unsigned id, RECT bounds, TextLabelKey label);
        void Command(unsigned id);
        void Refresh();
        void RequestMove(const std::string &room);
        void CompleteMove();
        void RoomChanged();
        void Tick();
        void Hover(int slot);
        void ClearRules();
        void SetCertificationVisible(bool show);
        void ConfirmCertification();
        void PaintCertification(HDC dc);
        void PaintLobby(HDC dc);
        void PaintViewport(HDC dc);
        void PaintPanel(HDC dc);
        void PaintCandidates(HDC dc);
        void PaintContest(HDC dc) const;
        void PaintProfile(HDC dc) const;
        void PaintRules(HDC dc);
        void PaintButton(const DRAWITEMSTRUCT &item);
        void ShowMenu();
        void ShowPractice() const;
        void ShowProfileExportMenu();
        void CopyProfileText() const;
        void SaveProfileImage();
        void ChooseProfile(bool monthly);
        void LoadProfilePreferences();
        void SetMonthlyProfile(unsigned type);

        std::filesystem::path config_;
        std::function<void(Action)> action_;
        std::function<void(const std::string &)> log_;
        Submit submit_;
        OfflineLobby state_{FixtureProfile()};
        std::vector<std::string> history_;
        RankedTier tier_ = RankedTier::General;
        unsigned outer_ = 0, lifetime_ = kRankedFourProfile, monthly_ = kDefaultMonthlyProfile;
        std::array<LobbyStatistics, 2> private_lifetime_{};
        OfflineLobbyTiming timing_;
        std::uint64_t seconds_ = 0;
        std::uint64_t service_clock_ = 0, service_clock_at_ = 0;
        std::string service_pending_, service_error_;
        bool visible_ = false, move_declined_ = false, toutenkou_ = false, official_ = false, service_ready_ = true;
        HWND parent_, viewport_ = nullptr, panel_ = nullptr;
        HFONT font_ = nullptr;
        HWND certificate_ = nullptr, certificate_ok_ = nullptr;
        std::uint64_t certificate_receipt_ = 0;
        std::wstring certificate_body_;
        int certificate_width_ = 812, certificate_height_ = 658;
        float certificate_scale_ = 1.4f;
        std::unique_ptr<Gdiplus::Bitmap> certificate_image_;
        PaintBuffer certificate_buffer_;
        std::map<unsigned, HWND> buttons_;
        DisplayArtwork artwork_;
        Image background_;
        PaintBuffer viewport_buffer_, panel_buffer_;
    };
}
