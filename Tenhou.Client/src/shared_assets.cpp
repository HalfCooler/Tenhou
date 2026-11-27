#include "shared_assets.hpp"
#include "resource_manager.hpp"
#include "text_catalog.hpp"
#include "text_renderer.hpp"
#include "win32_utf.hpp"
#include <algorithm>
#include <list>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace tenhou {
    namespace {
        struct FileStamp {
            bool exists = false;
            std::uint64_t size = 0;
            std::int64_t mtime = 0;

            bool operator==(const FileStamp &other) const {
                return exists == other.exists && size == other.size && mtime == other.mtime;
            }
            bool operator!=(const FileStamp &other) const { return !(*this == other); }
        };

        std::filesystem::path NormalizePath(const std::filesystem::path &path) {
            std::error_code ec;
            auto normalized = std::filesystem::weakly_canonical(path, ec);
            if (ec) normalized = path.lexically_normal();
            return normalized;
        }

        FileStamp ReadFileStamp(const std::filesystem::path &path) {
            std::error_code ec;
            if (!std::filesystem::exists(path, ec) || ec) return {};
            const auto size = std::filesystem::file_size(path, ec);
            if (ec) return {};
            const auto time = std::filesystem::last_write_time(path, ec);
            if (ec) return {};
            return {true, static_cast<std::uint64_t>(size),
                    static_cast<std::int64_t>(time.time_since_epoch().count())};
        }

        struct BackgroundCacheKey {
            std::string source_path;
            FileStamp source_stamp;
            int width = 0, height = 0;
            std::string background_source;
            std::array<std::string, 10> lhsv;

            bool operator==(const BackgroundCacheKey &other) const {
                return source_path == other.source_path && source_stamp == other.source_stamp &&
                       width == other.width && height == other.height &&
                       background_source == other.background_source && lhsv == other.lhsv;
            }
        };

        constexpr std::size_t kMaxBackgroundEntries = 4;
        constexpr std::size_t kMaxSoundBankEntries = 4;
    } // namespace

    struct SharedAssetStore::Impl {
        mutable std::mutex mutex;
        SharedAssetStats stats;

        struct ObjectPackEntry {
            FileStamp stamp;
            std::shared_ptr<const ObjectPack> pack;
        };
        std::map<std::string, ObjectPackEntry> object_packs;

        struct DisplayPackEntry {
            FileStamp stamp;
            std::shared_ptr<const ResourcePack> pack;
        };
        std::map<std::pair<std::string, int>, DisplayPackEntry> display_packs;

        // Weak references ensure TextFontService never outlives GdiplusShutdown.
        struct FontEntry {
            FileStamp stamp;
            std::weak_ptr<const TextFontService> font;
        };
        std::map<std::string, FontEntry> fonts;

        struct BackgroundEntry {
            BackgroundCacheKey key;
            std::shared_ptr<const Image> image;
        };
        std::list<BackgroundEntry> backgrounds;

        struct SoundBankEntry {
            std::string fingerprint;
            std::shared_ptr<const SharedSoundBank> bank;
        };
        std::list<SoundBankEntry> sound_banks;
    };

    SharedAssetStore::SharedAssetStore() : impl_(std::make_unique<Impl>()) {}
    SharedAssetStore::~SharedAssetStore() = default;

    std::string SharedAssetStore::FileFingerprint(const std::filesystem::path &path) {
        const auto norm = NormalizePath(path);
        const auto stamp = ReadFileStamp(norm);
        std::ostringstream out;
        out << PathToUtf8(norm) << '|' << (stamp.exists ? '1' : '0') << '|' << stamp.size << '|' << stamp.mtime;
        return out.str();
    }

    std::shared_ptr<const ObjectPack> SharedAssetStore::GetObjectPack(const std::filesystem::path &path) const
    {
        const auto norm = NormalizePath(path);
        const auto key = PathToUtf8(norm);
        const auto stamp = ReadFileStamp(norm);
        {
            std::lock_guard lock(impl_->mutex);
            const auto it = impl_->object_packs.find(key);
            if (it != impl_->object_packs.end() && it->second.stamp == stamp && it->second.pack) {
                ++impl_->stats.object_pack_hits;
                return it->second.pack;
            }
        }
        auto loaded = std::make_shared<ObjectPack>(LoadObjectPack(norm));
        std::lock_guard lock(impl_->mutex);
        ++impl_->stats.object_pack_loads;
        impl_->object_packs[key] = {stamp, loaded};
        return loaded;
    }

    std::shared_ptr<const ResourcePack> SharedAssetStore::GetDisplayPack(const std::filesystem::path &path, int width) const
    {
        const auto norm = NormalizePath(path);
        const auto key = std::make_pair(PathToUtf8(norm), width);
        const auto stamp = ReadFileStamp(norm);
        {
            std::lock_guard lock(impl_->mutex);
            const auto it = impl_->display_packs.find(key);
            if (it != impl_->display_packs.end() && it->second.stamp == stamp && it->second.pack) {
                ++impl_->stats.display_pack_hits;
                return it->second.pack;
            }
        }
        auto pack = std::make_shared<ResourcePack>(LoadResourcePack(norm,
            [](const unsigned id, const unsigned metadata) { return metadata && FindTextImageByOrigin(id); }));
        if (pack->signature != "res2" && !(width == 1024 && pack->signature == "res0"))
            throw std::runtime_error("Original display pack has the wrong canvas signature");
        std::lock_guard lock(impl_->mutex);
        ++impl_->stats.display_pack_loads;
        impl_->display_packs[key] = {stamp, pack};
        return pack;
    }

    std::shared_ptr<const TextFontService> SharedAssetStore::GetTextFont(const std::filesystem::path &resources_dir) const
    {
        ResourceManager manager(resources_dir);
        const auto font_path = NormalizePath(manager.Path(AssetId::TextFont));
        const auto key = PathToUtf8(font_path);
        const auto stamp = ReadFileStamp(font_path);
        std::lock_guard lock(impl_->mutex);
        const auto it = impl_->fonts.find(key);
        if (it != impl_->fonts.end() && it->second.stamp == stamp) {
            if (auto existing = it->second.font.lock()) {
                ++impl_->stats.font_hits;
                return existing;
            }
        }
        auto loaded = std::make_shared<TextFontService>(manager);
        ++impl_->stats.font_loads;
        impl_->fonts[key] = {stamp, loaded};
        return loaded;
    }

    std::shared_ptr<const Image> SharedAssetStore::GetBackground(
        const std::filesystem::path &directory, const std::filesystem::path &config,
        const int width, const int height, const ClientPreferences &preferences,
        const std::function<Image()> &builder) const
    {
        BackgroundCacheKey key;
        key.width = width;
        key.height = height;
        key.background_source = preferences.background_source;
        key.lhsv = preferences.background_lhsv;
        if (preferences.background_source == "#FF0000") {
            key.source_path = "#FF0000";
        } else {
            auto source = preferences.background_source == "./BG1"
                              ? ResourceManager(directory).Path(AssetId::BackgroundAlternate)
                              : PathFromUtf8(preferences.background_source);
            if (source.empty()) source = ResourceManager(directory).Path(AssetId::Background);
            else if (source.is_relative() && preferences.background_source != "./BG1")
                source = config.parent_path() / source;
            const auto norm = NormalizePath(source);
            key.source_path = PathToUtf8(norm);
            key.source_stamp = ReadFileStamp(norm);
        }
        {
            std::lock_guard lock(impl_->mutex);
            for (auto it = impl_->backgrounds.begin(); it != impl_->backgrounds.end(); ++it) {
                if (it->key == key) {
                    ++impl_->stats.background_hits;
                    impl_->backgrounds.splice(impl_->backgrounds.begin(), impl_->backgrounds, it);
                    return it->image;
                }
            }
        }
        auto image = std::make_shared<Image>(builder());
        std::lock_guard lock(impl_->mutex);
        ++impl_->stats.background_builds;
        impl_->backgrounds.push_front({std::move(key), image});
        while (impl_->backgrounds.size() > kMaxBackgroundEntries) impl_->backgrounds.pop_back();
        return image;
    }

    std::shared_ptr<const SharedSoundBank> SharedAssetStore::GetSoundBank(
        const std::string &fingerprint, const std::function<SharedSoundBank()> &loader) const
    {
        {
            std::lock_guard lock(impl_->mutex);
            for (auto it = impl_->sound_banks.begin(); it != impl_->sound_banks.end(); ++it) {
                if (it->fingerprint == fingerprint) {
                    ++impl_->stats.sound_bank_hits;
                    impl_->sound_banks.splice(impl_->sound_banks.begin(), impl_->sound_banks, it);
                    return it->bank;
                }
            }
        }
        auto bank = std::make_shared<SharedSoundBank>(loader());
        std::lock_guard lock(impl_->mutex);
        ++impl_->stats.sound_bank_loads;
        impl_->sound_banks.push_front({fingerprint, bank});
        while (impl_->sound_banks.size() > kMaxSoundBankEntries) impl_->sound_banks.pop_back();
        return bank;
    }

    SharedAssetStats SharedAssetStore::Stats() const {
        std::lock_guard lock(impl_->mutex);
        return impl_->stats;
    }

    void SharedAssetStore::ResetStats() const
    {
        std::lock_guard lock(impl_->mutex);
        impl_->stats = {};
    }

    void SharedAssetStore::Clear() const
    {
        std::lock_guard lock(impl_->mutex);
        impl_->object_packs.clear();
        impl_->display_packs.clear();
        impl_->fonts.clear();
        impl_->backgrounds.clear();
        impl_->sound_banks.clear();
    }

    SharedAssetStore &SharedAssets() {
        static SharedAssetStore instance;
        return instance;
    }
} // namespace tenhou
