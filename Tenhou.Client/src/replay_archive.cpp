#include "replay_archive.hpp"
#include "ini_store.hpp"
#include "replay.hpp"
#include "win32_utf.hpp"
#include <algorithm>
#include <fstream>

namespace tenhou {
    void ReplayArchive::Reset() {
        directory_.clear();
        id_.clear();
        data_.clear();
        status_.clear();
        requested_.clear();
        expected_ = 0;
    }

    bool ReplayArchive::Request(const LobbySnapshot &s, const std::filesystem::path &config) {
        if (s.variant_active) return false;
        if (config.empty() || !s.game.complete || s.exit_table.empty() || requested_ == s.exit_table || !s.profile.
            count("account") || s.profile.at("account").empty()) return false;
        const auto path = ReadIniString(config, "LOG", "PATH");
        if (path.empty()) return false;
        directory_ = PathFromUtf8(path);
        if (directory_.is_relative()) directory_ = config.parent_path() / directory_;
        requested_ = s.exit_table;
        status_ = "正在保存牌谱";
        return true;
    }

    void ReplayArchive::Receive(const NetworkMessage &m) {
        auto field = [&](const char *key) -> const std::string & { return m.attributes.at(key); };
        if (m.tag == "REPLAYERROR") {
            status_ = "牌谱保存失败：" + field("text");
            data_.clear();
            return;
        }
        if (m.tag == "REPLAYBEGIN") {
            if (field("table") != requested_ || !id_.empty()) throw std::runtime_error("牌谱传输身份不匹配");
            const auto size = std::stoull(field("bytes"));
            if (!size || size > 8 * 1024 * 1024) throw std::runtime_error("牌谱大小超出限制");
            id_ = requested_;
            expected_ = static_cast<std::size_t>(size);
            data_.clear();
            return;
        }
        if (m.tag == "REPLAYCHUNK") {
            const auto &hex = field("data");
            if (id_.empty() || std::stoull(field("offset")) != data_.size() || hex.size() % 2 || hex.size() > 3000 ||
                data_.size() + hex.size() / 2 > expected_) throw std::runtime_error("牌谱分块顺序或大小错误");
            auto digit = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                throw std::runtime_error("牌谱编码错误");
            };
            for (std::size_t i = 0; i < hex.size(); i += 2)
                data_ += static_cast<char>((digit(hex[i]) << 4) | digit(hex[i + 1]));
            return;
        }
        if (m.tag != "REPLAYEND") return;
        if (id_.empty() || field("table") != id_ || data_.size() != expected_ || id_.size() != 32 || !std::all_of(
                id_.begin(), id_.end(),
                [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); })) throw std::runtime_error(
            "牌谱不完整");
        ReplaySession verification(ReplayLog::Parse(data_, id_));
        while (verification.Step()) {
        }
        if (!verification.state().ended) throw std::runtime_error("牌谱缺少结算");
        std::filesystem::create_directories(directory_);
        const auto final_path = directory_ / PathFromUtf8(id_ + ".xml");
        auto temporary = final_path;
        temporary += ".tmp";
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            if (!out) throw std::runtime_error("无法创建牌谱文件");
            out.write(data_.data(), static_cast<std::streamsize>(data_.size()));
            out.flush();
            if (!out) {
                out.close();
                std::error_code ec;
                std::filesystem::remove(temporary, ec);
                throw std::runtime_error("牌谱写入失败");
            }
        }
        std::error_code ec;
        std::filesystem::rename(temporary, final_path, ec);
        if (ec) {
            std::filesystem::remove(final_path, ec);
            ec.clear();
            std::filesystem::rename(temporary, final_path, ec);
            if (ec) {
                std::error_code rm_ec;
                std::filesystem::remove(temporary, rm_ec);
                throw std::runtime_error("牌谱写入或提交失败");
            }
        }
        status_ = "牌谱已保存：" + PathToUtf8(final_path);
        id_.clear();
        data_.clear();
    }
} // namespace tenhou
