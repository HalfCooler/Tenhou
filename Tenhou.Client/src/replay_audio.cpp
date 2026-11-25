#include "replay_audio.hpp"
#include "client_preferences.hpp"
#include "monotonic_clock.hpp"
#include "music_preferences.hpp"
#include "replay_effects.hpp"
#include "resource_manager.hpp"
#include "shared_assets.hpp"
#include "win32_utf.hpp"
#include <miniaudio.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    namespace {
        constexpr std::uint32_t kMixSampleRate = 44100;
        constexpr std::uint32_t kMixChannels = 2;
        constexpr std::size_t kMaxMusicBytes = 256u * 1024u * 1024u;
        constexpr unsigned kWaveFormatPcm = 1;

        unsigned U16(const Bytes &b, std::size_t p) { return b.at(p) | (unsigned(b.at(p + 1)) << 8); }
        unsigned U32(const Bytes &b, std::size_t p) { return U16(b, p) | (U16(b, p + 2) << 16); }

        void MaCheck(ma_result result, const char *action) {
            if (result != MA_SUCCESS) {
                throw std::runtime_error(std::string(action) + " failed (miniaudio " +
                                         ma_result_description(result) + ")");
            }
        }

        // Convert an arbitrary 8/16-bit mono/stereo SoundClip to stereo float32 at kMixSampleRate.
        std::vector<float> ConvertClipToMixFloat(const SoundClip &clip) {
            const auto &fmt = clip.format;
            const auto block = fmt.block_align();
            if (!block || !fmt.sample_rate || clip.pcm.empty() || clip.pcm.size() % block != 0)
                throw std::runtime_error("Invalid PCM clip format");
            const ma_format in_format = fmt.bits_per_sample == 8 ? ma_format_u8 :
                                        fmt.bits_per_sample == 16 ? ma_format_s16 : ma_format_unknown;
            if (in_format == ma_format_unknown)
                throw std::runtime_error("Unsupported PCM bit depth");

            const ma_uint64 in_frames = clip.pcm.size() / block;
            ma_data_converter_config config = ma_data_converter_config_init(
                in_format, ma_format_f32, fmt.channels, kMixChannels, fmt.sample_rate, kMixSampleRate);
            ma_data_converter converter{};
            MaCheck(ma_data_converter_init(&config, nullptr, &converter), "ma_data_converter_init");

            ma_uint64 expected_out = 0;
            ma_data_converter_get_expected_output_frame_count(&converter, in_frames, &expected_out);
            std::vector<float> out(static_cast<std::size_t>(expected_out + 64) * kMixChannels, 0.f);

            ma_uint64 consumed = in_frames;
            ma_uint64 produced = out.size() / kMixChannels;
            const ma_result res = ma_data_converter_process_pcm_frames(
                &converter, clip.pcm.data(), &consumed, out.data(), &produced);
            ma_data_converter_uninit(&converter, nullptr);
            MaCheck(res, "ma_data_converter_process_pcm_frames");

            out.resize(static_cast<std::size_t>(produced) * kMixChannels);
            return out;
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
                if (fmt || size < 16 || U16(wav, offset + 8) != kWaveFormatPcm)
                    throw std::runtime_error("Invalid replay PCM format");
                auto &f = result.format;
                f.channels = static_cast<std::uint16_t>(info.channels);
                f.sample_rate = info.sample_rate;
                f.bits_per_sample = static_cast<std::uint16_t>(info.bits_per_sample);
                if (U16(wav, offset + 20) != f.block_align() || U32(wav, offset + 16) != f.avg_bytes_per_sec())
                    throw std::runtime_error("Inconsistent replay PCM alignment/rate");
                fmt = true;
            } else if (!std::memcmp(wav.data() + offset, "data", 4)) {
                if (data) throw std::runtime_error("Duplicate replay PCM data");
                result.pcm.assign(wav.begin() + offset + 8, wav.begin() + offset + 8 + size);
                data = true;
            }
            offset += 8 + size + (size & 1);
        }
        if (!fmt || !data || result.pcm.empty() || result.pcm.size() % result.format.block_align())
            throw std::runtime_error("Empty or unaligned replay PCM data");
        result.sha256 = Sha256(wav);
        return result;
    }

    SoundClip DecodeMusic(const Bytes &wav) {
        if (wav.empty()) throw std::runtime_error("Empty BGM audio");
        // Fast path for standard uncompressed PCM WAV (keeps strict legacy checks).
        if (wav.size() >= 12 && !std::memcmp(wav.data(), "RIFF", 4) && !std::memcmp(wav.data() + 8, "WAVE", 4)) {
            bool is_pcm = false;
            for (std::size_t p = 12; p + 8 <= wav.size();) {
                const auto size = U32(wav, p + 4);
                if (size > wav.size() - p - 8) throw std::runtime_error("Truncated BGM WAV");
                if (!std::memcmp(wav.data() + p, "fmt ", 4)) {
                    if (size < 16) throw std::runtime_error("Missing BGM format");
                    is_pcm = U16(wav, p + 8) == kWaveFormatPcm;
                    break;
                }
                p += 8 + size + (size & 1);
            }
            if (is_pcm) return DecodeSound(wav);
        }

        // Decode compressed WAV (e.g. Microsoft ADPCM / IMA ADPCM) or MP3/FLAC/OGG via miniaudio.
        ma_decoder_config config = ma_decoder_config_init(ma_format_s16, 0, 0);
        ma_decoder decoder{};
        MaCheck(ma_decoder_init_memory(wav.data(), wav.size(), &config, &decoder), "ma_decoder_init_memory");

        SoundClip result;
        try {
            if ((decoder.outputChannels != 1 && decoder.outputChannels != 2) || decoder.outputSampleRate == 0)
                throw std::runtime_error("Unsupported BGM PCM format");
            result.format.channels = static_cast<std::uint16_t>(decoder.outputChannels);
            result.format.sample_rate = decoder.outputSampleRate;
            result.format.bits_per_sample = 16;
            const std::size_t frame_bytes = result.format.block_align();

            constexpr ma_uint64 kChunkFrames = 8192;
            Bytes chunk(static_cast<std::size_t>(kChunkFrames) * frame_bytes);
            while (true) {
                ma_uint64 read_frames = 0;
                const ma_result r = ma_decoder_read_pcm_frames(&decoder, chunk.data(), kChunkFrames, &read_frames);
                if (read_frames > 0) {
                    const std::size_t bytes = static_cast<std::size_t>(read_frames) * frame_bytes;
                    if (result.pcm.size() + bytes > kMaxMusicBytes)
                        throw std::runtime_error("BGM PCM exceeds capacity");
                    result.pcm.insert(result.pcm.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(bytes));
                }
                if (r == MA_AT_END || read_frames == 0) break;
                MaCheck(r, "ma_decoder_read_pcm_frames");
            }
        } catch (...) {
            ma_decoder_uninit(&decoder);
            throw;
        }
        ma_decoder_uninit(&decoder);
        if (result.pcm.empty()) throw std::runtime_error("Empty decoded BGM audio");
        result.sha256 = Sha256(wav);
        return result;
    }

    void ReplaySoundBank::Load(const std::filesystem::path &resources, int canvas_width,
                               const std::filesystem::path &config) {
        (void) canvas_width; // Audio shares the sole HD resource pack.
        const auto pack_path = ResourceManager(resources).Path(AssetId::DisplayPack);
        std::ostringstream fp;
        fp << SharedAssetStore::FileFingerprint(pack_path) << '\n';
        fp << SharedAssetStore::FileFingerprint(config) << '\n';

        std::map<unsigned, std::string> overrides;
        for (const auto &key : SoundKeys()) {
            const auto value = PreferenceString(config, "RESOURCE", key.second);
            if (value.size() >= 32766) throw std::runtime_error("Replay sound override path too long: " + key.second);
            if (!value.empty()) {
                overrides[key.first] = value;
                fp << key.first << '=' << value << '|';
                if (!AsciiCaseEqual(value, "none")) {
                    auto override_path = PathFromUtf8(value);
                    if (override_path.is_relative()) override_path = config.parent_path() / override_path;
                    fp << SharedAssetStore::FileFingerprint(override_path);
                }
                fp << '\n';
            }
        }

        clips_ = SharedAssets().GetSoundBank(fp.str(), [&]() -> SharedSoundBank {
            const auto pack = LoadResourcePack(pack_path,
                [](unsigned id, unsigned metadata) { return id > 100 && id < 172 && !metadata; });
            SharedSoundBank original, candidate;
            for (const auto &entry : pack.entries)
                original[entry.id] = std::make_shared<SoundClip>(DecodeSound(entry.payload));
            for (const auto &key : SoundKeys()) {
                const auto source = original.find(DefaultSoundResource(key.first));
                if (source != original.end()) candidate[key.first] = source->second;
                // The bundled pack has no game-end/login/logout WAV. These optional
                // settings remain configurable; previewing their default is silent.
                else if (key.first >= 115 && key.first <= 117) candidate[key.first].reset();

                const auto ov = overrides.find(key.first);
                if (ov != overrides.end()) {
                    if (AsciiCaseEqual(ov->second, "none")) {
                        candidate[key.first].reset();
                    } else {
                        auto override_path = PathFromUtf8(ov->second);
                        if (override_path.is_relative()) override_path = config.parent_path() / override_path;
                        try {
                            candidate[key.first] = std::make_shared<SoundClip>(DecodeSound(ReadFile(override_path)));
                        } catch (const std::exception &e) {
                            throw std::runtime_error("RESOURCE " + key.second + ": " + e.what());
                        }
                    }
                }
            }
            // All sounds used by the event scheduler must be available before committing.
            for (unsigned id : {
                     101u, 103u, 104u, 105u, 106u, 107u, 109u, 111u, 112u, 113u, 114u,
                     118u, 119u, 123u, 127u, 128u, 132u, 136u, 137u, 141u, 145u, 146u, 150u, 154u, 155u, 159u, 163u,
                     164u, 168u
                 })
                if (!candidate.count(id)) throw std::runtime_error("Missing replay sound resource " + std::to_string(id));
            return candidate;
        });
    }

    const std::shared_ptr<const SoundClip> &ReplaySoundBank::Get(unsigned id) const {
        const auto found = clips_->find(id);
        if (found == clips_->end()) throw std::runtime_error("Missing replay sound alias " + std::to_string(id));
        return found->second;
    }

    Bytes ScaleSoundPcm(const SoundClip &clip, double gain) {
        if (!std::isfinite(gain) || gain < 0 || gain > 4) throw std::runtime_error("Invalid audio gain");
        Bytes pcm = clip.pcm;
        if (clip.format.bits_per_sample == 8) {
            for (auto &sample : pcm) sample = static_cast<std::uint8_t>(std::clamp(
                                         128 + int((int(sample) - 128) * gain), 0, 255));
        } else if (clip.format.bits_per_sample == 16) {
            for (std::size_t i = 0; i + 1 < pcm.size(); i += 2) {
                const int sample = static_cast<std::int16_t>(unsigned(pcm[i]) | (unsigned(pcm[i + 1]) << 8));
                const auto scaled = static_cast<std::int16_t>(std::clamp(int(sample * gain), -32768, 32767));
                pcm[i] = static_cast<std::uint8_t>(scaled);
                pcm[i + 1] = static_cast<std::uint8_t>(static_cast<unsigned short>(scaled) >> 8);
            }
        } else throw std::runtime_error("Unsupported SE sample format");
        return pcm;
    }

    // -------------------------------------------------------------------------
    // BackgroundMusic (miniaudio WASAPI float32 stereo streaming + exact fade)
    // -------------------------------------------------------------------------
    struct BackgroundMusic::Impl {
        struct Track {
            std::filesystem::path path;
            std::vector<float> samples; // Interleaved stereo float32 at kMixSampleRate
            std::size_t cursor_frame = 0;
            double gain = 1.0;
            double envelope = 1.0;
            bool seek = false;
            std::uint64_t offset_ms = 0;
        };

        std::function<void(const std::string &)> log;
        ma_device device{};
        bool device_open = false;

        mutable std::mutex mutex;
        std::filesystem::path requested;
        bool active = false;
        Track current;
        std::optional<Track> pending_next;
        bool pending_stop = false;
        std::vector<std::string> pending_logs;

        std::atomic<float> peak_left{0.f};
        std::atomic<float> peak_right{0.f};

        explicit Impl(std::function<void(const std::string &)> logger) : log(std::move(logger)) {}

        ~Impl() { CloseDevice(); }

        void DrainLogs() {
            std::vector<std::string> msgs;
            {
                std::lock_guard lock(mutex);
                msgs.swap(pending_logs);
            }
            for (const auto &m : msgs) log(m);
        }

        static Track DecodeTrack(const std::filesystem::path &path, bool seek, double gain) {
            const auto clip = DecodeMusic(ReadFile(path));
            auto samples = ConvertClipToMixFloat(clip);
            const std::size_t frames = samples.size() / kMixChannels;
            if (!frames) throw std::runtime_error("Empty converted BGM track");
            const std::uint64_t duration_ms = static_cast<std::uint64_t>(frames) * 1000ULL / kMixSampleRate;
            const std::uint64_t offset_ms = seek && duration_ms ? (MonotonicMs() % duration_ms) : 0;
            const std::size_t cursor_frame = static_cast<std::size_t>(offset_ms * kMixSampleRate / 1000ULL) % frames;
            Track t;
            t.path = path;
            t.samples = std::move(samples);
            t.cursor_frame = cursor_frame;
            t.gain = gain;
            t.envelope = seek ? 0.0 : 1.0;
            t.seek = seek;
            t.offset_ms = offset_ms;
            return t;
        }

        void EnsureDevice() {
            if (device_open) return;
            ma_device_config config = ma_device_config_init(ma_device_type_playback);
            config.playback.format = ma_format_f32;
            config.playback.channels = kMixChannels;
            config.sampleRate = kMixSampleRate;
            config.dataCallback = &DataCallback;
            config.pUserData = this;
            MaCheck(ma_device_init(nullptr, &config, &device), "ma_device_init BGM");
            const ma_result started = ma_device_start(&device);
            if (started != MA_SUCCESS) {
                ma_device_uninit(&device);
                MaCheck(started, "ma_device_start BGM");
            }
            device_open = true;
        }

        void CloseDevice() noexcept {
            if (device_open) {
                ma_device_uninit(&device);
                device_open = false;
            }
            {
                std::lock_guard lock(mutex);
                active = false;
                current = {};
                pending_next.reset();
                pending_stop = false;
            }
            peak_left.store(0.f, std::memory_order_relaxed);
            peak_right.store(0.f, std::memory_order_relaxed);
        }

        static void DataCallback(ma_device *dev, void *output, const void *, ma_uint32 frame_count) {
            auto *self = static_cast<Impl *>(dev->pUserData);
            auto *out = static_cast<float *>(output);
            std::memset(out, 0, static_cast<std::size_t>(frame_count) * kMixChannels * sizeof(float));

            float max_l = 0.f, max_r = 0.f;
            {
                std::lock_guard lock(self->mutex);
                if (!self->active || self->current.samples.empty()) {
                    self->peak_left.store(0.f, std::memory_order_relaxed);
                    self->peak_right.store(0.f, std::memory_order_relaxed);
                    return;
                }
                // Original 00403f90 applies a 5e-6 step per channel sample at 44.1 kHz stereo.
                constexpr double kStep = 5e-6 * 88200.0 / (kMixSampleRate * double(kMixChannels));
                ma_uint32 i = 0;
                while (i < frame_count) {
                    if (!self->active || self->current.samples.empty()) break;
                    const bool fading_out = self->pending_stop || self->pending_next.has_value();
                    const std::size_t total_frames = self->current.samples.size() / kMixChannels;
                    while (i < frame_count && (!fading_out || self->current.envelope > 0.0)) {
                        const std::size_t base = self->current.cursor_frame * kMixChannels;
                        for (unsigned ch = 0; ch < kMixChannels; ++ch) {
                            const double val = std::clamp(
                                double(self->current.samples[base + ch]) * self->current.gain * self->current.envelope,
                                -1.0, 1.0);
                            const float f = static_cast<float>(val);
                            out[i * kMixChannels + ch] = f;
                            if (ch == 0) max_l = std::max(max_l, std::abs(f));
                            else max_r = std::max(max_r, std::abs(f));
                            self->current.envelope = fading_out
                                ? std::max(0.0, self->current.envelope - kStep)
                                : std::min(1.0, self->current.envelope + kStep);
                        }
                        if (++self->current.cursor_frame >= total_frames) self->current.cursor_frame = 0;
                        ++i;
                    }
                    if (fading_out && self->current.envelope <= 0.0) {
                        self->pending_logs.push_back(
                            "LOBBY BGM STOP file=" + PathToUtf8(self->current.path.filename()));
                        if (self->pending_next) {
                            self->current = std::move(*self->pending_next);
                            self->pending_next.reset();
                            self->pending_stop = false;
                            self->pending_logs.push_back(
                                "LOBBY BGM START file=" + PathToUtf8(self->current.path.filename()) +
                                " offset_ms=" + std::to_string(self->current.offset_ms) +
                                " fade_in=" + std::to_string(self->current.seek) + " loop=1");
                        } else {
                            self->active = false;
                            self->current = {};
                            self->pending_stop = false;
                            break;
                        }
                    }
                }
            }
            self->peak_left.store(max_l, std::memory_order_relaxed);
            self->peak_right.store(max_r, std::memory_order_relaxed);
        }
    };

    BackgroundMusic::BackgroundMusic(std::function<void(const std::string &)> log)
        : impl_(std::make_unique<Impl>(std::move(log))) {}

    BackgroundMusic::~BackgroundMusic() = default;

    void BackgroundMusic::Select(const std::filesystem::path &path, bool seek, double gain) {
        impl_->DrainLogs();
        const bool changed = impl_->requested != path;
        std::optional<Impl::Track> decoded;
        if (!path.empty()) {
            bool need_decode = false;
            {
                std::lock_guard lock(impl_->mutex);
                need_decode = !impl_->active || impl_->current.path != path;
            }
            if (need_decode) decoded = Impl::DecodeTrack(path, seek, gain);
        }

        std::string immediate_log;
        bool need_device = false;
        {
            std::lock_guard lock(impl_->mutex);
            if (changed && impl_->active && impl_->current.path == impl_->requested) {
                immediate_log = "LOBBY BGM FADE OUT file=" + PathToUtf8(impl_->current.path.filename());
            }
            impl_->requested = path;
            if (impl_->active) {
                if (impl_->current.path == path) {
                    impl_->current.gain = gain;
                    impl_->pending_next.reset();
                    impl_->pending_stop = false;
                } else if (path.empty()) {
                    impl_->pending_next.reset();
                    impl_->pending_stop = true;
                } else {
                    impl_->pending_next = std::move(decoded);
                    impl_->pending_stop = false;
                }
            } else if (decoded) {
                impl_->current = std::move(*decoded);
                impl_->pending_next.reset();
                impl_->pending_stop = false;
                impl_->active = true;
                need_device = true;
                immediate_log = "LOBBY BGM START file=" + PathToUtf8(impl_->current.path.filename()) +
                                " offset_ms=" + std::to_string(impl_->current.offset_ms) +
                                " fade_in=" + std::to_string(impl_->current.seek) + " loop=1";
            }
        }
        if (need_device) {
            try {
                impl_->EnsureDevice();
            } catch (...) {
                impl_->CloseDevice();
                throw;
            }
        }
        if (!immediate_log.empty()) impl_->log(immediate_log);
        impl_->DrainLogs();
    }

    void BackgroundMusic::Stop() {
        impl_->DrainLogs();
        {
            std::lock_guard lock(impl_->mutex);
            impl_->requested.clear();
        }
        impl_->CloseDevice();
    }

    StereoLevels BackgroundMusic::Levels() const {
        return {impl_->peak_left.load(std::memory_order_relaxed),
                impl_->peak_right.load(std::memory_order_relaxed)};
    }

    // -------------------------------------------------------------------------
    // ReplayAudio (single miniaudio WASAPI device, 4-voice float32 mixer)
    // -------------------------------------------------------------------------
    struct ReplayAudio::Impl {
        struct Voice {
            std::shared_ptr<const std::vector<float>> samples;
            std::size_t cursor_frame = 0;
            float gain = 0.6f;
            bool loop = false;
            bool active = false;
            bool finished = false;
            unsigned id = 0;
            std::uint64_t serial = 0;
        };

        struct Pending {
            std::uint64_t due;
            unsigned id;
            std::shared_ptr<const SoundClip> clip;
        };

        std::function<void(const std::string &)> log;
        ma_device device{};
        bool device_open = false;

        mutable std::mutex mutex;
        std::array<Voice, 4> voices{};
        std::vector<Pending> pending;
        std::map<const SoundClip *, std::weak_ptr<const std::vector<float>>> converted_cache;
        std::vector<std::shared_ptr<const std::vector<float>>> cache_keepalive;
        std::uint64_t serial = 0;
        std::uint64_t last_played_at = 0;
        int volume_percent = 100;

        explicit Impl(std::function<void(const std::string &)> logger) : log(std::move(logger)) {}

        ~Impl() {
            try { Stop(); } catch (...) {}
            if (device_open) {
                ma_device_uninit(&device);
                device_open = false;
            }
        }

        void EnsureDevice() {
            if (device_open) return;
            ma_device_config config = ma_device_config_init(ma_device_type_playback);
            config.playback.format = ma_format_f32;
            config.playback.channels = kMixChannels;
            config.sampleRate = kMixSampleRate;
            config.dataCallback = &DataCallback;
            config.pUserData = this;
            MaCheck(ma_device_init(nullptr, &config, &device), "ma_device_init SE");
            const ma_result started = ma_device_start(&device);
            if (started != MA_SUCCESS) {
                ma_device_uninit(&device);
                MaCheck(started, "ma_device_start SE");
            }
            device_open = true;
            log("AUDIO OPEN mixer=miniaudio");
        }

        std::shared_ptr<const std::vector<float>> GetConverted(const std::shared_ptr<const SoundClip> &clip) {
            const auto it = converted_cache.find(clip.get());
            if (it != converted_cache.end()) {
                if (auto existing = it->second.lock()) return existing;
            }
            auto converted = std::make_shared<std::vector<float>>(ConvertClipToMixFloat(*clip));
            if (converted_cache.size() >= 64) {
                converted_cache.clear();
                cache_keepalive.clear();
            }
            converted_cache[clip.get()] = converted;
            cache_keepalive.push_back(converted);
            return converted;
        }

        void ReapFinished() {
            std::vector<unsigned> done_ids;
            {
                std::lock_guard lock(mutex);
                for (auto &v : voices) {
                    if (v.finished) {
                        done_ids.push_back(v.id);
                        v.finished = false;
                        v.active = false;
                        v.samples.reset();
                    }
                }
            }
            for (unsigned id : done_ids) log("AUDIO DONE id=" + std::to_string(id));
        }

        void Stop() {
            std::vector<unsigned> stopped_ids;
            {
                std::lock_guard lock(mutex);
                pending.clear();
                last_played_at = 0;
                for (auto &v : voices) {
                    if (v.active || v.finished) {
                        stopped_ids.push_back(v.id);
                        v.active = false;
                        v.finished = false;
                        v.samples.reset();
                    }
                }
            }
            for (unsigned id : stopped_ids) log("AUDIO STOP id=" + std::to_string(id));
        }

        static void DataCallback(ma_device *dev, void *output, const void *, ma_uint32 frame_count) {
            auto *self = static_cast<Impl *>(dev->pUserData);
            auto *out = static_cast<float *>(output);
            std::memset(out, 0, static_cast<std::size_t>(frame_count) * kMixChannels * sizeof(float));

            std::lock_guard lock(self->mutex);
            for (auto &v : self->voices) {
                if (!v.active || !v.samples || v.samples->empty()) continue;
                const auto &src = *v.samples;
                const std::size_t total_frames = src.size() / kMixChannels;
                for (ma_uint32 f = 0; f < frame_count; ++f) {
                    if (v.cursor_frame >= total_frames) {
                        if (v.loop) {
                            v.cursor_frame = 0;
                        } else {
                            v.active = false;
                            v.finished = true;
                            break;
                        }
                    }
                    const std::size_t sb = v.cursor_frame * kMixChannels;
                    out[f * kMixChannels] += src[sb] * v.gain;
                    out[f * kMixChannels + 1] += src[sb + 1] * v.gain;
                    ++v.cursor_frame;
                }
            }
            const std::size_t total_samples = static_cast<std::size_t>(frame_count) * kMixChannels;
            for (std::size_t i = 0; i < total_samples; ++i) {
                out[i] = std::clamp(out[i], -1.0f, 1.0f);
            }
        }
    };

    ReplayAudio::ReplayAudio(std::function<void(const std::string &)> log)
        : impl_(std::make_unique<Impl>(std::move(log))) {}

    ReplayAudio::~ReplayAudio() = default;

    void ReplayAudio::SetVolumePercent(int percent) {
        std::lock_guard lock(impl_->mutex);
        impl_->volume_percent = std::clamp(percent, 0, 200);
    }

    void ReplayAudio::Poll() {
        impl_->ReapFinished();
        const auto now = MonotonicMs();
        bool tile_played = false;
        while (!impl_->pending.empty() && impl_->pending.front().due <= now) {
            if (impl_->pending.front().id == 104 && tile_played) {
                // A late UI timer must not burst several movement clicks into one voice.
                const auto shift = now + 25 - impl_->pending.front().due;
                for (auto &item : impl_->pending) if (item.id == 104) item.due += shift;
                std::stable_sort(impl_->pending.begin(), impl_->pending.end(),
                                 [](const Impl::Pending &a, const Impl::Pending &b) { return a.due < b.due; });
                continue;
            }
            auto item = std::move(impl_->pending.front());
            impl_->pending.erase(impl_->pending.begin());
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
        if (impl_->pending.size() >= 136) throw std::runtime_error("Replay sound schedule capacity exceeded");
        impl_->pending.push_back({MonotonicMs() + delay, id, std::move(clip)});
        std::stable_sort(impl_->pending.begin(), impl_->pending.end(),
                         [](const Impl::Pending &a, const Impl::Pending &b) { return a.due < b.due; });
    }

    void ReplayAudio::Stop() { impl_->Stop(); }

    bool ReplayAudio::Active() const {
        std::lock_guard lock(impl_->mutex);
        return !impl_->pending.empty() ||
               std::any_of(impl_->voices.begin(), impl_->voices.end(),
                           [](const Impl::Voice &v) { return v.active || v.finished; });
    }

    std::uint64_t ReplayAudio::LastPlayedAt() const {
        std::lock_guard lock(impl_->mutex);
        return impl_->last_played_at;
    }

    void ReplayAudio::Play(unsigned id, std::shared_ptr<const SoundClip> clip, bool loop, double base_gain) {
        if (!clip) return;
        impl_->ReapFinished();
        auto samples = impl_->GetConverted(clip);
        impl_->EnsureDevice();

        std::optional<unsigned> replaced_id;
        int vol_pct = 100;
        const auto now = MonotonicMs();
        {
            std::lock_guard lock(impl_->mutex);
            vol_pct = impl_->volume_percent;
            auto selected = std::find_if(impl_->voices.begin(), impl_->voices.end(),
                                         [](const Impl::Voice &v) { return !v.active && !v.finished; });
            // Prefer a free voice so a custom movement WAV may finish; at capacity
            // replace another movement click before the match opening or a declaration.
            if (selected == impl_->voices.end() && id == 104) {
                selected = std::find_if(impl_->voices.begin(), impl_->voices.end(),
                                        [](const Impl::Voice &v) { return v.active && v.id == 104; });
            }
            if (selected == impl_->voices.end()) {
                selected = std::min_element(impl_->voices.begin(), impl_->voices.end(),
                                            [](const Impl::Voice &a, const Impl::Voice &b) { return a.serial < b.serial; });
            }
            if (selected->active) replaced_id = selected->id;
            selected->samples = std::move(samples);
            selected->cursor_frame = 0;
            selected->gain = static_cast<float>(std::clamp(base_gain * (vol_pct * 0.01), 0.0, 4.0));
            selected->loop = loop;
            selected->id = id;
            selected->serial = ++impl_->serial;
            selected->finished = false;
            selected->active = true;
            impl_->last_played_at = now;
        }
        if (replaced_id) impl_->log("AUDIO STOP id=" + std::to_string(*replaced_id));
        impl_->log("AUDIO WRITE id=" + std::to_string(id) + " bytes=" + std::to_string(clip->pcm.size()) +
                   " sha256=" + clip->sha256 + " master=" + std::to_string(vol_pct) +
                   " tick=" + std::to_string(now));
    }
} // namespace tenhou
