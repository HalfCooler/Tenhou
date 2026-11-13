#include "replay_audio.hpp"
#include "resource_manager.hpp"
#include "replay_effects.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <cmath>
#include <mmreg.h>
#include <msacm.h>

namespace tenhou {
    namespace {
        unsigned U16(const Bytes &b, std::size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
        unsigned U32(const Bytes &b, std::size_t p) { return U16(b, p) | (U16(b, p + 2) << 16); }

        void AudioCheck(MMRESULT result, const char *action) {
            if (result != MMSYSERR_NOERROR) throw std::runtime_error(
                std::string(action) + " failed (waveOut " + std::to_string(result) + ")");
        }
    } // namespace
    SoundClip DecodeSound(const Bytes &wav) {
        Resource resource;
        resource.payload = wav;
        const auto info = InspectWave(resource);
        if ((info.channels != 1 && info.channels != 2) || (info.bits_per_sample != 8 && info.bits_per_sample != 16) ||
            info.sample_rate < 8000 || info.sample_rate > 96000)
            throw std::runtime_error("Replay sound requires mono/stereo 8/16-bit PCM at 8000..96000 Hz");
        SoundClip result;
        bool fmt = false, data = false;
        for (std::size_t offset = 12; offset + 8 <= wav.size();) {
            const auto size = U32(wav, offset + 4);
            if (size > wav.size() - offset - 8) throw std::runtime_error("Truncated replay sound chunk");
            if (!std::memcmp(wav.data() + offset, "fmt ", 4)) {
                if (fmt || size < 16 || U16(wav, offset + 8) != WAVE_FORMAT_PCM) throw std::runtime_error(
                    "Invalid replay PCM format");
                auto &f = result.format;
                f.wFormatTag = WAVE_FORMAT_PCM;
                f.nChannels = static_cast<WORD>(info.channels);
                f.nSamplesPerSec = info.sample_rate;
                f.wBitsPerSample = static_cast<WORD>(info.bits_per_sample);
                f.nBlockAlign = static_cast<WORD>(f.nChannels * f.wBitsPerSample / 8);
                f.nAvgBytesPerSec = f.nSamplesPerSec * f.nBlockAlign;
                if (U16(wav, offset + 20) != f.nBlockAlign || U32(wav, offset + 16) != f.nAvgBytesPerSec)
                    throw std::runtime_error("Inconsistent replay PCM alignment/rate");
                fmt = true;
            } else if (!std::memcmp(wav.data() + offset, "data", 4)) {
                if (data) throw std::runtime_error("Duplicate replay PCM data");
                result.pcm.assign(wav.begin() + offset + 8, wav.begin() + offset + 8 + size);
                data = true;
            }
            offset += 8 + size + (size & 1);
        }
        if (!fmt || !data || result.pcm.empty() || result.pcm.size() % result.format.nBlockAlign)
            throw std::runtime_error("Empty or unaligned replay PCM data");
        result.sha256 = Sha256(wav);
        return result;
    }

    SoundClip DecodeMusic(const Bytes &wav) {
        // Original FUN_0046ead0 converts compressed WAV through the system ACM.
        if (wav.size() < 12 || std::memcmp(wav.data(), "RIFF", 4) || std::memcmp(wav.data() + 8, "WAVE", 4))
            throw std::runtime_error("Invalid BGM WAV");
        Bytes format, data;
        for (std::size_t p = 12; p + 8 <= wav.size();) {
            const auto size = U32(wav, p + 4);
            if (size > wav.size() - p - 8) throw std::runtime_error("Truncated BGM WAV");
            if (!std::memcmp(wav.data() + p, "fmt ", 4)) format.assign(wav.begin() + p + 8, wav.begin() + p + 8 + size);
            if (!std::memcmp(wav.data() + p, "data", 4)) data.assign(wav.begin() + p + 8, wav.begin() + p + 8 + size);
            p += 8 + size + (size & 1);
        }
        if (format.size() < 16 || data.empty()) throw std::runtime_error("Missing BGM format/data");
        if (U16(format, 0) == WAVE_FORMAT_PCM) return DecodeSound(wav);
        if (format.size() < sizeof(WAVEFORMATEX) || U16(format, 16) > format.size() - sizeof(WAVEFORMATEX))
            throw std::runtime_error("Invalid compressed BGM format");
        auto *source = reinterpret_cast<WAVEFORMATEX *>(format.data());
        SoundClip result;
        result.format.wFormatTag = WAVE_FORMAT_PCM;
        AudioCheck(acmFormatSuggest(nullptr, source, &result.format, sizeof(WAVEFORMATEX),
                                    ACM_FORMATSUGGESTF_WFORMATTAG), "acmFormatSuggest");
        if ((result.format.nChannels != 1 && result.format.nChannels != 2) ||
            (result.format.wBitsPerSample != 8 && result.format.wBitsPerSample != 16))
            throw std::runtime_error("Unsupported BGM PCM format");
        HACMSTREAM stream = nullptr;
        ACMSTREAMHEADER header{};
        bool prepared = false;
        AudioCheck(acmStreamOpen(&stream, nullptr, source, &result.format, nullptr, 0, 0, ACM_STREAMOPENF_NONREALTIME),
                   "acmStreamOpen");
        try {
            DWORD size = 0;
            AudioCheck(acmStreamSize(stream, static_cast<DWORD>(data.size()), &size, ACM_STREAMSIZEF_SOURCE),
                       "acmStreamSize");
            if (size > 256u * 1024u * 1024u) throw std::runtime_error("BGM PCM exceeds capacity");
            result.pcm.resize(size);
            header.cbStruct = sizeof(header);
            header.pbSrc = data.data();
            header.cbSrcLength = static_cast<DWORD>(data.size());
            header.pbDst = result.pcm.data();
            header.cbDstLength = size;
            AudioCheck(acmStreamPrepareHeader(stream, &header, 0), "acmStreamPrepareHeader");
            prepared = true;
            AudioCheck(acmStreamConvert(stream, &header, 0), "acmStreamConvert");
            if (header.cbSrcLengthUsed != data.size() || !header.cbDstLengthUsed)
                throw std::runtime_error(
                    "Incomplete BGM conversion: " + std::to_string(header.cbSrcLengthUsed) + "/" + std::to_string(
                        data.size()));
            result.pcm.resize(header.cbDstLengthUsed);
            AudioCheck(acmStreamUnprepareHeader(stream, &header, 0), "acmStreamUnprepareHeader");
            prepared = false;
        } catch (...) {
            if (prepared) acmStreamUnprepareHeader(stream, &header, 0);
            acmStreamClose(stream, 0);
            throw;
        }
        AudioCheck(acmStreamClose(stream, 0), "acmStreamClose");
        result.sha256 = Sha256(wav);
        return result;
    }

    void ReplaySoundBank::Load(const std::filesystem::path &resources, int canvas_width,
                               const std::filesystem::path &config) {
        (void) canvas_width; // Audio shares the sole HD resource pack.
        const auto pack = LoadResourcePack(ResourceManager(resources).Path(AssetId::DisplayPack));
        std::map<unsigned, std::shared_ptr<const SoundClip> > original, candidate;
        for (const auto &entry: pack.entries)
            if (entry.id > 100 && entry.id < 172 && !entry.metadata)
                original[entry.id] = std::make_shared<SoundClip>(DecodeSound(entry.payload));
        for (const auto &key: SoundKeys()) {
            const auto source = original.find(DefaultSoundResource(key.first));
            if (source != original.end()) candidate[key.first] = source->second;
            wchar_t path[32768]{};
            const std::wstring name(key.second.begin(), key.second.end());
            const auto count = GetPrivateProfileStringW(L"RESOURCE", name.c_str(), L"", path, 32768, config.c_str());
            if (count >= 32766) throw std::runtime_error("Replay sound override path too long: " + key.second);
            if (_wcsicmp(path, L"none") == 0) candidate[key.first].reset();
            else if (count) {
                std::filesystem::path override_path(path);
                if (override_path.is_relative()) override_path = config.parent_path() / override_path;
                try { candidate[key.first] = std::make_shared<SoundClip>(DecodeSound(ReadFile(override_path))); } catch
                (const std::exception &e) { throw std::runtime_error("RESOURCE " + key.second + ": " + e.what()); }
            }
        }
        // All sounds used by the event scheduler must be available before committing.
        for (unsigned id: {
                 101u, 103u, 104u, 105u, 106u, 107u, 109u, 111u, 112u, 113u, 114u,
                 118u, 119u, 123u, 127u, 128u, 132u, 136u, 137u, 141u, 145u, 146u, 150u, 154u, 155u, 159u, 163u, 164u,
                 168u
             })
            if (!candidate.count(id)) throw std::runtime_error("Missing replay sound resource " + std::to_string(id));
        clips_ = std::move(candidate);
    }

    const std::shared_ptr<const SoundClip> &ReplaySoundBank::Get(unsigned id) const {
        const auto found = clips_.find(id);
        if (found == clips_.end()) throw std::runtime_error("Missing replay sound alias " + std::to_string(id));
        return found->second;
    }

    Bytes ScaleSoundPcm(const SoundClip &clip, double gain) {
        if (!std::isfinite(gain) || gain < 0 || gain > 4) throw std::runtime_error("Invalid audio gain");
        Bytes pcm = clip.pcm;
        if (clip.format.wBitsPerSample == 8) {
            for (auto &sample: pcm) sample = static_cast<std::uint8_t>(std::clamp(
                                        128 + int((int(sample) - 128) * gain), 0, 255));
        } else if (clip.format.wBitsPerSample == 16) {
            for (std::size_t i = 0; i + 1 < pcm.size(); i += 2) {
                const int sample = static_cast<std::int16_t>(unsigned(pcm[i]) | (unsigned(pcm[i + 1]) << 8));
                const auto scaled = static_cast<std::int16_t>(std::clamp(int(sample * gain), -32768, 32767));
                pcm[i] = static_cast<std::uint8_t>(scaled);
                pcm[i + 1] = static_cast<std::uint8_t>(static_cast<unsigned short>(scaled) >> 8);
            }
        } else throw std::runtime_error("Unsupported SE sample format");
        return pcm;
    }

    void ReplayAudio::SetVolumePercent(int percent) { volume_percent_ = std::clamp(percent, 0, 200); }

    ReplayAudio::~ReplayAudio() {
        // Windows releases remaining device buffers on process termination. In normal
        // teardown reset/unprepare/close succeeds; never unwind through a destructor.
        try { Stop(); } catch (...) {
        }
        for (auto &voice: voices_) if (voice.device) waveOutClose(voice.device);
    }

    void ReplayAudio::Release(Voice &voice, bool reset) {
        if (!voice.clip) return;
        if (reset) AudioCheck(waveOutReset(voice.device), "waveOutReset");
        AudioCheck(waveOutUnprepareHeader(voice.device, &voice.header, sizeof(WAVEHDR)), "waveOutUnprepareHeader");
        log_(std::string(reset ? "AUDIO STOP id=" : "AUDIO DONE id=") + std::to_string(voice.id));
        voice.header = {};
        voice.clip.reset();
        voice.pcm.clear();
    }

    void ReplayAudio::Poll() {
        for (auto &voice: voices_) if (voice.clip && (voice.header.dwFlags & WHDR_DONE)) Release(voice, false);
        const auto now = GetTickCount64();
        bool tile_played = false;
        while (!pending_.empty() && pending_.front().due <= now) {
            if (pending_.front().id == 104 && tile_played) {
                // A late UI timer must not burst several movement clicks into one voice.
                const auto shift = now + 25 - pending_.front().due;
                for (auto &item: pending_) if (item.id == 104) item.due += shift;
                std::stable_sort(pending_.begin(), pending_.end(), [](const Pending &a, const Pending &b) {
                    return a.due < b.due;
                });
                continue;
            }
            auto item = std::move(pending_.front());
            pending_.erase(pending_.begin());
            if (item.id == 104) tile_played = true;
            Play(item.id, std::move(item.clip));
        }
    }

    void ReplayAudio::Schedule(unsigned id, std::shared_ptr<const SoundClip> clip, unsigned delay) {
        if (!clip) return; // Original RESOURCE/key=none removes this alias only.
        if (!delay) {
            Play(id, std::move(clip));
            return;
        }
        if (pending_.size() >= 136) throw std::runtime_error("Replay sound schedule capacity exceeded");
        pending_.push_back({GetTickCount64() + delay, id, std::move(clip)});
        std::stable_sort(pending_.begin(), pending_.end(),
                         [](const Pending &a, const Pending &b) { return a.due < b.due; });
    }

    void ReplayAudio::Stop() {
        pending_.clear();
        for (auto &voice: voices_) Release(voice, true);
    }

    bool ReplayAudio::Active() const {
        return !pending_.empty() || std::any_of(voices_.begin(), voices_.end(),
                                                [](const Voice &v) { return bool(v.clip); });
    }

    void ReplayAudio::Play(unsigned id, std::shared_ptr<const SoundClip> clip, bool loop, double base_gain) {
        if (!clip) return;
        for (auto &voice: voices_) if (voice.clip && (voice.header.dwFlags & WHDR_DONE)) Release(voice, false);
        auto selected = std::find_if(voices_.begin(), voices_.end(), [](const Voice &v) { return !v.clip; });
        // Prefer a free voice so a custom movement WAV may finish; at capacity
        // replace another movement click before the match opening or a declaration.
        if (selected == voices_.end() && id == 104) selected = std::find_if(voices_.begin(), voices_.end(),
                                                                            [](const Voice &v) {
                                                                                return v.clip && v.id == 104;
                                                                            });
        if (selected == voices_.end())
            selected = std::min_element(voices_.begin(), voices_.end(),
                                        [](const Voice &a, const Voice &b) { return a.serial < b.serial; });
        auto &voice = *selected;
        Release(voice, true);
        if (voice.device && std::memcmp(&voice.format, &clip->format, sizeof(WAVEFORMATEX))) {
            AudioCheck(waveOutClose(voice.device), "waveOutClose");
            voice.device = nullptr;
        }
        if (!voice.device) {
            AudioCheck(waveOutOpen(&voice.device, WAVE_MAPPER, &clip->format, 0, 0, CALLBACK_NULL), "waveOutOpen");
            voice.format = clip->format;
            log_("AUDIO OPEN slot=" + std::to_string(selected - voices_.begin()));
        }
        voice.clip = std::move(clip);
        voice.id = id;
        voice.serial = ++serial_;
        // FUN_0046f550 center gain (.6f) times FUN_0046e450 master SE_VOL/100.
        // Software PCM conversion adapts this scalar to the existing waveOut backend.
        voice.pcm = ScaleSoundPcm(*voice.clip, base_gain * (volume_percent_ * .01));
        voice.header = {};
        voice.header.lpData = reinterpret_cast<LPSTR>(voice.pcm.data());
        voice.header.dwBufferLength = static_cast<DWORD>(voice.clip->pcm.size());
        if (loop) {
            voice.header.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
            voice.header.dwLoops = MAXDWORD;
        }
        const auto prepared = waveOutPrepareHeader(voice.device, &voice.header, sizeof(WAVEHDR));
        if (prepared != MMSYSERR_NOERROR) {
            voice.header = {};
            voice.clip.reset();
        }
        AudioCheck(prepared, "waveOutPrepareHeader");
        AudioCheck(waveOutWrite(voice.device, &voice.header, sizeof(WAVEHDR)), "waveOutWrite");
        log_("AUDIO WRITE id=" + std::to_string(id) + " bytes=" + std::to_string(voice.clip->pcm.size()) + " sha256=" +
             voice.clip->sha256
             + " master=" + std::to_string(volume_percent_)
             + " tick=" + std::to_string(GetTickCount64()));
    }
} // namespace tenhou
