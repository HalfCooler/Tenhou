#include "ini_store.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <map>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace tenhou {
    namespace {
        struct IniLine {
            std::string raw;
            std::string section; // Lowercase ASCII section name if header, or owning section if entry.
            std::string key;     // Lowercase ASCII key if entry, empty otherwise.
            std::string value;
            bool is_section = false;
            bool is_entry = false;
        };

        struct IniDocument {
            std::filesystem::file_time_type mtime{};
            bool has_mtime = false;
            std::vector<IniLine> lines;
        };

        std::string LowerAscii(const std::string_view text) {
            std::string out(text);
            for (char &c : out) {
                if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
            }
            return out;
        }

        std::string_view Trim(std::string_view s) {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n'))
                s.remove_prefix(1);
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
                s.remove_suffix(1);
            return s;
        }

        std::string Utf16LeToUtf8(const std::uint8_t *data, const std::size_t bytes) {
            std::string out;
            out.reserve(bytes);
            const std::size_t count = bytes / 2;
            for (std::size_t i = 0; i < count; ++i) {
                std::uint32_t cp = static_cast<std::uint32_t>(data[i * 2]) |
                                   (static_cast<std::uint32_t>(data[i * 2 + 1]) << 8);
                if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < count) {
                    const std::uint32_t low = static_cast<std::uint32_t>(data[(i + 1) * 2]) |
                                              (static_cast<std::uint32_t>(data[(i + 1) * 2 + 1]) << 8);
                    if (low >= 0xDC00 && low <= 0xDFFF) {
                        cp = 0x10000u + ((cp - 0xD800u) << 10) + (low - 0xDC00u);
                        ++i;
                    }
                }
                if (cp < 0x80) {
                    out.push_back(static_cast<char>(cp));
                } else if (cp < 0x800) {
                    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                } else if (cp < 0x10000) {
                    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                } else {
                    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                }
            }
            return out;
        }

        std::string DecodeIniBytes(const std::string &raw) {
            if (raw.size() >= 2 && static_cast<unsigned char>(raw[0]) == 0xFF &&
                static_cast<unsigned char>(raw[1]) == 0xFE) {
                return Utf16LeToUtf8(reinterpret_cast<const std::uint8_t *>(raw.data() + 2), raw.size() - 2);
            }
            if (raw.size() >= 3 && static_cast<unsigned char>(raw[0]) == 0xEF &&
                static_cast<unsigned char>(raw[1]) == 0xBB && static_cast<unsigned char>(raw[2]) == 0xBF) {
                return raw.substr(3);
            }
            return raw;
        }

        std::vector<IniLine> ParseIni(std::string_view content) {
            std::vector<IniLine> lines;
            std::string current_section;
            std::size_t pos = 0;
            while (pos < content.size()) {
                std::size_t end = content.find('\n', pos);
                std::string_view line_view = end == std::string_view::npos
                    ? content.substr(pos)
                    : content.substr(pos, end - pos);
                pos = end == std::string_view::npos ? content.size() : end + 1;
                if (!line_view.empty() && line_view.back() == '\r') line_view.remove_suffix(1);

                IniLine item;
                item.raw.assign(line_view);
                const auto trimmed = Trim(line_view);
                if (!trimmed.empty() && trimmed.front() == '[') {
                    const auto close = trimmed.find(']');
                    if (close != std::string_view::npos) {
                        current_section = LowerAscii(Trim(trimmed.substr(1, close - 1)));
                        item.section = current_section;
                        item.is_section = true;
                    }
                } else if (!trimmed.empty() && trimmed.front() != ';' && trimmed.front() != '#') {
                    if (const auto eq = line_view.find('='); eq != std::string_view::npos) {
                        if (const auto key = Trim(line_view.substr(0, eq)); !key.empty()) {
                            auto val = Trim(line_view.substr(eq + 1));
                            // Match Win32 GetPrivateProfileString stripping matching surrounding quotes.
                            if (val.size() >= 2 &&
                                ((val.front() == '"' && val.back() == '"') ||
                                 (val.front() == '\'' && val.back() == '\''))) {
                                val.remove_prefix(1);
                                val.remove_suffix(1);
                            }
                            item.section = current_section;
                            item.key = LowerAscii(key);
                            item.value.assign(val);
                            item.is_entry = true;
                        }
                    }
                }
                lines.push_back(std::move(item));
            }
            return lines;
        }

        std::mutex g_mutex;
        std::map<std::filesystem::path, IniDocument> g_cache;

        std::filesystem::path CacheKey(const std::filesystem::path &path) {
            std::error_code ec;
            const auto abs = std::filesystem::absolute(path, ec);
            return (ec ? path : abs).lexically_normal();
        }

        IniDocument &LoadDocumentLocked(const std::filesystem::path &path) {
            const auto key = CacheKey(path);
            std::error_code ec;
            const bool exists = std::filesystem::is_regular_file(key, ec) && !ec;
            std::filesystem::file_time_type mtime{};
            if (exists) mtime = std::filesystem::last_write_time(key, ec);
            const bool has_mtime = exists && !ec;
            const auto it = g_cache.find(key);
            if (it != g_cache.end() && it->second.has_mtime == has_mtime &&
                (!has_mtime || it->second.mtime == mtime)) {
                return it->second;
            }

            IniDocument doc;
            doc.mtime = mtime;
            doc.has_mtime = has_mtime;
            if (exists) {
                if (std::ifstream in(key, std::ios::binary); in) {
                    const std::string raw((std::istreambuf_iterator(in)), std::istreambuf_iterator<char>());
                    doc.lines = ParseIni(DecodeIniBytes(raw));
                }
            }
            return g_cache.insert_or_assign(key, std::move(doc)).first->second;
        }

        void SaveDocumentLocked(const std::filesystem::path &path, IniDocument &doc) {
            const auto key = CacheKey(path);
            if (!key.parent_path().empty()) {
                std::error_code ec;
                std::filesystem::create_directories(key.parent_path(), ec);
            }
            std::string serialized;
            for (const auto &line : doc.lines) {
                serialized += line.raw;
                serialized += "\r\n";
            }
            auto tmp = key;
            tmp += ".ini.tmp";
            {
                std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
                if (!out) throw std::runtime_error("Cannot open INI temporary file for writing");
                out.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
                out.flush();
                if (!out) throw std::runtime_error("Cannot write INI temporary file");
            }
            std::error_code ec;
            std::filesystem::rename(tmp, key, ec);
            if (ec) {
                std::filesystem::remove(key, ec);
                ec.clear();
                std::filesystem::rename(tmp, key, ec);
                if (ec) {
                    std::error_code rm_ec;
                    std::filesystem::remove(tmp, rm_ec);
                    throw std::runtime_error("Cannot commit INI file: " + ec.message());
                }
            }
            ec.clear();
            const auto mtime = std::filesystem::last_write_time(key, ec);
            doc.mtime = mtime;
            doc.has_mtime = !ec;
        }
    } // namespace

    std::string ReadIniString(const std::filesystem::path &path, const std::string_view section,
                              const std::string_view key, const std::string_view fallback) {
        if (path.empty()) return std::string(fallback);
        std::scoped_lock lock(g_mutex);
        const auto &doc = LoadDocumentLocked(path);
        const auto want_sec = LowerAscii(section);
        const auto want_key = LowerAscii(key);
        for (const auto &line : doc.lines) {
            if (line.is_entry && line.section == want_sec && line.key == want_key)
                return line.value;
        }
        return std::string(fallback);
    }

    int ReadIniInt(const std::filesystem::path &path, const std::string_view section,
                   const std::string_view key, const int fallback) {
        const auto text = ReadIniString(path, section, key, "");
        if (text.empty()) return fallback;
        char *end = nullptr;
        const long long value = std::strtoll(text.c_str(), &end, 0);
        if (end == text.c_str()) return fallback;
        return static_cast<int>(value);
    }

    void WriteIniString(const std::filesystem::path &path, std::string_view section,
                        std::string_view key, std::optional<std::string_view> value) {
        if (path.empty()) return;
        std::scoped_lock lock(g_mutex);
        auto &doc = LoadDocumentLocked(path);
        const auto want_sec = LowerAscii(section);
        const auto want_key = LowerAscii(key);

        std::size_t section_end = std::string::npos;
        bool in_section = false;
        for (std::size_t i = 0; i < doc.lines.size(); ++i) {
            if (doc.lines[i].is_section) {
                if (in_section) {
                    section_end = i;
                    break;
                }
                if (doc.lines[i].section == want_sec) {
                    in_section = true;
                }
            } else if (in_section && doc.lines[i].is_entry && doc.lines[i].key == want_key) {
                if (!value.has_value()) {
                    doc.lines.erase(doc.lines.begin() + static_cast<std::ptrdiff_t>(i));
                } else {
                    if (doc.lines[i].value == *value) return;
                    doc.lines[i].value.assign(*value);
                    doc.lines[i].raw = std::string(key) + "=" + std::string(*value);
                }
                SaveDocumentLocked(path, doc);
                return;
            }
        }

        if (!value.has_value()) return; // Key didn't exist; deleting is a no-op.

        IniLine entry;
        entry.raw = std::string(key) + "=" + std::string(*value);
        entry.section = want_sec;
        entry.key = want_key;
        entry.value.assign(*value);
        entry.is_entry = true;

        if (in_section) {
            std::size_t insert_at = section_end == std::string::npos ? doc.lines.size() : section_end;
            while (insert_at > 0 && Trim(doc.lines[insert_at - 1].raw).empty()) --insert_at;
            doc.lines.insert(doc.lines.begin() + static_cast<std::ptrdiff_t>(insert_at), std::move(entry));
        } else {
            IniLine header;
            header.raw = "[" + std::string(section) + "]";
            header.section = want_sec;
            header.is_section = true;
            doc.lines.push_back(std::move(header));
            doc.lines.push_back(std::move(entry));
        }
        SaveDocumentLocked(path, doc);
    }

    void WriteIniInt(const std::filesystem::path &path, const std::string_view section,
                     const std::string_view key, const int value) {
        const auto text = std::to_string(value);
        WriteIniString(path, section, key, std::string_view(text));
    }

    void InvalidateIniCache(const std::filesystem::path &path) {
        std::scoped_lock lock(g_mutex);
        if (path.empty()) g_cache.clear();
        else g_cache.erase(CacheKey(path));
    }
} // namespace tenhou
