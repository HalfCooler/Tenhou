#pragma once
#include "resource_pack.hpp"
#include <windows.h>
#include <mmsystem.h>
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace tenhou {
struct SoundClip {
    WAVEFORMATEX format{};
    Bytes pcm;
    std::string sha256;
};
SoundClip DecodeSound(const Bytes& wav);
SoundClip DecodeMusic(const Bytes& wav);
Bytes ScaleSoundPcm(const SoundClip& clip, double gain);
class ReplaySoundBank {
public:
    void Load(const std::filesystem::path& resources, int canvas_width, const std::filesystem::path& config);
    const std::shared_ptr<const SoundClip>& Get(unsigned id) const;
    const std::map<unsigned, std::shared_ptr<const SoundClip>>& clips() const { return clips_; }
private:
    std::map<unsigned, std::shared_ptr<const SoundClip>> clips_;
};
// Four reusable devices; no unbounded queue. Oldest voice is replaced at capacity.
class ReplayAudio {
public:
    explicit ReplayAudio(std::function<void(const std::string&)> log) : log_(std::move(log)) {}
    ~ReplayAudio();
    void Play(unsigned id, std::shared_ptr<const SoundClip> clip, bool loop = false, double base_gain = double(.6f));
    void Schedule(unsigned id, std::shared_ptr<const SoundClip> clip, unsigned delay_ms);
    void Poll();
    void Stop();
    bool Active() const;
    void SetVolumePercent(int percent);
private:
    struct Voice {
        HWAVEOUT device = nullptr;
        WAVEHDR header{};
        WAVEFORMATEX format{};
        std::shared_ptr<const SoundClip> clip;
        Bytes pcm;
        unsigned id = 0;
        std::uint64_t serial = 0;
    };
    void Release(Voice& voice, bool reset);
    std::array<Voice, 4> voices_{};
    struct Pending { ULONGLONG due; unsigned id; std::shared_ptr<const SoundClip> clip; };
    std::vector<Pending> pending_;
    std::uint64_t serial_ = 0;
    int volume_percent_ = 100;
    std::function<void(const std::string&)> log_;
};
} // namespace tenhou
