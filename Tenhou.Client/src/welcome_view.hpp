#pragma once
#include "resource_pack.hpp"
#include "network_client.hpp"
#include <windows.h>
#include <functional>
#include <memory>

namespace tenhou {
class WelcomeView {
public:
    enum class Action { Settings, Exit, Lobby, Register };
    WelcomeView(HWND parent, std::function<void(Action)> action,
                std::function<void(const std::string&)> log);
    ~WelcomeView();
    void Load(const std::filesystem::path& resources, const std::filesystem::path& config,
              int screen_width, int screen_height);
    void LoadPreferences(const std::filesystem::path& config);
    void Resize(int width, int height);
    void Show(bool visible);
    void Capture(HDC dc);
    void Save(const std::filesystem::path& config);
    bool Ready() const { return error_.empty() && !banner_.bgra.empty() && !background_.bgra.empty(); }
    HWND Window() const { return viewport_; }
    std::wstring Identity() const;
    std::wstring Password() const;
    std::wstring Nickname() const;
    void ClearPassword();
    char Gender() const { return gender_; }
    bool Connecting() const { return connecting_; }
    void SetLoginState(bool connecting, const std::wstring& status);
    void SetHiddenId(bool hidden);
private:
    struct Control;
    static LRESULT CALLBACK ViewProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK PanelProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK ButtonProc(HWND, UINT, WPARAM, LPARAM);
    void PaintView(HDC dc);
    void PaintPanel(HDC dc);
    void PaintButton(Control& control, HDC dc);
    void Arrange();
    void Command(int id);
    void SetGender(char gender);
    HWND parent_ = nullptr, viewport_ = nullptr, panel_ = nullptr, edit_ = nullptr;
    HWND password_ = nullptr, nickname_ = nullptr;
    HFONT font_ = nullptr;
    bool connecting_ = false;
    std::wstring login_status_;
    std::vector<std::unique_ptr<Control>> controls_;
    Image banner_, background_;
    std::string error_;
    char gender_ = 'M';
    std::wstring masked_identity_;
    std::function<void(Action)> action_;
    std::function<void(const std::string&)> log_;
};
}  // namespace tenhou
