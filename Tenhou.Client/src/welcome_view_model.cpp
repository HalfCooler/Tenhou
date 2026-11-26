#include "welcome_view_model.hpp"
#include "client_preferences.hpp"
#include "resource_pack.hpp"
#include <algorithm>
#include <charconv>

namespace tenhou {
    std::string WelcomeViewModel::Trim(std::string_view value) {
        const auto start = value.find_first_not_of(" \t\r\n");
        if (start == std::string_view::npos) return {};
        const auto end = value.find_last_not_of(" \t\r\n");
        return std::string(value.substr(start, end - start + 1));
    }

    void WelcomeViewModel::LoadPreferences(const std::filesystem::path &config) {
        identity_ = PreferenceString(config, "REBUILD", "WelcomeName", "NoName");
        if (identity_.empty()) identity_ = "NoName";
        const auto official = PreferenceString(config, "STATUS", "UNAME", "NoName");
        official_identity_ = official.empty() ? "NoName" : official;
        host_ = PreferenceString(config, "REBUILD", "NativeHost", "localhost");
        port_text_ = std::to_string(PreferenceInt(config, "REBUILD", "NativePort", 10080));
        gender_ = PreferenceInt(config, "REBUILD", "WelcomeFemale", 0) ? 'F' : 'M';
        official_server_ = PreferenceInt(config, "REBUILD", "OfficialServer", 0) != 0;
    }

    void WelcomeViewModel::SavePreferences(const std::filesystem::path &config) const {
        const auto name = EffectiveIdentity();
        const auto official = TrimmedOfficialIdentity();
        // Never persist passwords.
        WritePreference(config, "REBUILD", "WelcomeName", name.empty() ? "NoName" : name.c_str());
        WritePreferenceInt(config, "REBUILD", "WelcomeFemale", gender_ == 'F' ? 1 : 0);
        WritePreference(config, "STATUS", "UNAME", official.empty() ? "NoName" : official.c_str());
        WritePreferenceInt(config, "REBUILD", "OfficialServer", official_server_ ? 1 : 0);
        const auto host = TrimmedHost();
        if (!host.empty()) WritePreference(config, "REBUILD", "NativeHost", host.c_str());
        if (const auto port = ParsedPort()) WritePreferenceInt(config, "REBUILD", "NativePort", static_cast<int>(port));
    }

    void WelcomeViewModel::SetServer(bool official) { official_server_ = official; }
    void WelcomeViewModel::SetGender(char gender) { gender_ = gender == 'F' ? 'F' : 'M'; }

    void WelcomeViewModel::SetIdentity(std::string value) {
        if (hidden_id_ && value == kMaskedIdentity) return;
        identity_ = std::move(value);
    }

    void WelcomeViewModel::SetOfficialIdentity(std::string value) { official_identity_ = std::move(value); }
    void WelcomeViewModel::SetHost(std::string value) { host_ = std::move(value); }
    void WelcomeViewModel::SetPortText(std::string value) { port_text_ = std::move(value); }
    void WelcomeViewModel::SetNickname(std::string value) { nickname_ = std::move(value); }
    void WelcomeViewModel::SetPassword(std::string value) { password_ = std::move(value); }
    void WelcomeViewModel::ClearPassword() { password_.clear(); }
    void WelcomeViewModel::SetHiddenId(bool hidden) { hidden_id_ = hidden; }

    void WelcomeViewModel::SetLoginState(bool connecting, std::string status) {
        connecting_ = connecting;
        status_ = std::move(status);
    }

    std::string WelcomeViewModel::DisplayIdentity() const {
        if (hidden_id_ && !identity_.empty() && identity_ != "NoName")
            return std::string(kMaskedIdentity);
        return identity_;
    }

    std::string WelcomeViewModel::EffectiveIdentity() const { return identity_; }

    std::string WelcomeViewModel::TrimmedOfficialIdentity() const { return Trim(official_identity_); }

    std::string WelcomeViewModel::TrimmedHost() const { return Trim(host_); }

    unsigned WelcomeViewModel::ParsedPort() const {
        const auto value = Trim(port_text_);
        unsigned port = 0;
        const auto end = value.data() + value.size();
        const auto parsed = std::from_chars(value.data(), end, port);
        return parsed.ec == std::errc{} && parsed.ptr == end && port >= 1 && port <= 65535 ? port : 0;
    }

    bool WelcomeViewModel::HasValidOfficialId() const {
        const auto id = TrimmedOfficialIdentity();
        if (id == "NoName") return true;
        const auto alnum = [](char c) {
            return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        };
        return id.size() == 19 && id.compare(0, 2, "ID") == 0 && id[10] == '-' &&
               std::all_of(id.begin() + 2, id.begin() + 10, alnum) &&
               std::all_of(id.begin() + 11, id.end(), alnum);
    }

    WelcomeViewModel::LoginValidation WelcomeViewModel::BuildConnection(
        bool register_account, const std::filesystem::path &config_file, bool service_lobby) const {
        ConnectionOptions connection;
        connection.official = official_server_;
        if (connection.official && !HasValidOfficialId()) {
            return {std::nullopt, "请输入 19 位天凤官方 ID（IDxxxxxxxx-xxxxxxxx）或 NoName"};
        }
        connection.host = connection.official ? "b.tenhou.net" : TrimmedHost();
        connection.port = connection.official ? 10080u : ParsedPort();
        if (connection.host.empty() || !connection.port) {
            return {std::nullopt, "请输入服务器主机和 1～65535 之间的端口"};
        }
        connection.identity = connection.official ? TrimmedOfficialIdentity() : EffectiveIdentity();
        connection.gender = gender_;
        connection.password = password_;
        connection.nickname = nickname_;
        connection.register_account = register_account;
        connection.config_file = config_file;
        connection.native_service = !connection.official;
        connection.origin_lobby = connection.official || service_lobby;
        connection.interactive_games = true;
        connection.match_games = true;
        connection.recover_games = true;
        connection.auto_reconnect = true;
        connection.presentation = true;

        const auto session_host = connection.host == "localhost" ? "127.0.0.1" : connection.host;
        const auto session_key = session_host + ":" + connection.identity;
        const auto account_key = Sha256(Bytes(session_key.begin(), session_key.end())).substr(0, 16);
        const std::string filename =
            std::string(connection.official ? "official-session-" : "platform-session-") +
            std::to_string(connection.port) + "-" + account_key + ".txt";
        connection.session_file = config_file.parent_path() / std::filesystem::u8path(filename);
        return {std::move(connection), {}};
    }
} // namespace tenhou
