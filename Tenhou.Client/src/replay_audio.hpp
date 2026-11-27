#pragma once
#include "resource_pack.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace tenhou {
    struct AudioFormat {
        std::uint32_t sample_rate = 0;
        std::uint16_t channels = 0;
        std::uint16_t bits_per_sample = 0;

        std::uint16_t block_align() const { return static_cast<std::uint16_t>(channels * (bits_per_sample / 8)); }
        std::uint32_t avg_bytes_per_sec() const { return sample_rate * block_align(); }
    };

    struct SoundClip {
        AudioFormat format{};
        Bytes pcm;
        std::string sha256;
    };

    SoundClip DecodeSound(const Bytes &wav);

    SoundClip DecodeMusic(const Bytes &wav);

    Bytes ScaleSoundPcm(const SoundClip &clip, double gain);

    using StereoLevels = std::array<float, 2>;

    class BackgroundMusic {
    public:
        explicit BackgroundMusic(std::function<void(const std::string &)> log);
        ~BackgroundMusic();

        BackgroundMusic(const BackgroundMusic &) = delete;
        BackgroundMusic &operator=(const BackgroundMusic &) = delete;

        void Select(const std::filesystem::path &path = {}, bool seek = false, double gain = 1) const;
        void Stop() const;
        StereoLevels Levels() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

    class ReplaySoundBank {
    public:
        void Load(const std::filesystem::path &resources, int canvas_width, const std::filesystem::path &config);

        const std::shared_ptr<const SoundClip> &Get(unsigned id) const;

        const std::map<unsigned, std::shared_ptr<const SoundClip>> &clips() const { return *clips_; }

    private:
        std::shared_ptr<const std::map<unsigned, std::shared_ptr<const SoundClip>>> clips_ =
            std::make_shared<const std::map<unsigned, std::shared_ptr<const SoundClip>>>();
    };

    // Four concurrent mixer voices; no unbounded queue. Oldest voice is replaced at capacity.
    class ReplayAudio {
    public:
        explicit ReplayAudio(std::function<void(const std::string &)> log);
        ~ReplayAudio();

        ReplayAudio(const ReplayAudio &) = delete;
        ReplayAudio &operator=(const ReplayAudio &) = delete;

        void Play(unsigned id, std::shared_ptr<const SoundClip> clip, bool loop = false,
                  double base_gain = static_cast<double>(.6f)) const;

        void Schedule(unsigned id, std::shared_ptr<const SoundClip> clip, unsigned delay_ms);

        void Poll();

        void Stop() const;

        bool Active() const;
        std::uint64_t LastPlayedAt() const;

        void SetVolumePercent(int percent) const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
} // namespace tenhou
