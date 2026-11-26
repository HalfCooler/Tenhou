#pragma once
#include "network_client.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace tenhou {
    // Pure C++ ViewModel for the Welcome / Login screen (zero Win32/HWND dependencies).
    class WelcomeViewModel {
    public:
        static constexpr std::string_view kMaskedIdentity = "ID########-########";

        void LoadPreferences(const std::filesystem::path &config);
        void SavePreferences(const std::filesystem::path &config) const;

        void SetServer(bool official);
        void SetGender(char gender);
        void SetIdentity(std::string value);
        void SetOfficialIdentity(std::string value);
        void SetHost(std::string value);
        void SetPortText(std::string value);
        void SetNickname(std::string value);
        void SetPassword(std::string value);
        void ClearPassword();
        void SetHiddenId(bool hidden);
        void SetLoginState(bool connecting, std::string status);

        bool official_server() const { return official_server_; }
        char gender() const { return gender_; }
        bool hidden_id() const { return hidden_id_; }
        bool connecting() const { return connecting_; }
        const std::string &status() const { return status_; }
        const std::string &nickname() const { return nickname_; }
        const std::string &password() const { return password_; }
        const std::string &port_text() const { return port_text_; }

        // Display value for the local identity edit field (returns mask when hidden_id is enabled).
        std::string DisplayIdentity() const;
        // True unmasked identity to submit / persist.
        std::string EffectiveIdentity() const;
        std::string TrimmedOfficialIdentity() const;
        std::string TrimmedHost() const;
        unsigned ParsedPort() const;
        bool HasValidOfficialId() const;

        struct LoginValidation {
            std::optional<ConnectionOptions> options;
            std::string error_message;
        };

        LoginValidation BuildConnection(bool register_account,
                                        const std::filesystem::path &config_file,
                                        bool service_lobby) const;

    private:
        static std::string Trim(std::string_view value);

        bool official_server_ = false;
        bool hidden_id_ = false;
        bool connecting_ = false;
        char gender_ = 'M';
        std::string identity_ = "NoName";
        std::string official_identity_ = "NoName";
        std::string host_ = "localhost";
        std::string port_text_ = "10080";
        std::string nickname_;
        std::string password_;
        std::string status_;
    };
} // namespace tenhou
