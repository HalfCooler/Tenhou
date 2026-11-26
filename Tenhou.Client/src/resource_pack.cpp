#include "resource_pack.hpp"
#include <zlib.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace tenhou {
    namespace {
        constexpr std::size_t kMaxBytes = 64 * 1024 * 1024;

        std::uint32_t U32(const Bytes &data, std::size_t offset) {
            if (offset > data.size() || data.size() - offset < 4)
                throw std::runtime_error("Truncated 32-bit field");
            return data[offset] | (std::uint32_t(data[offset + 1]) << 8) |
                   (std::uint32_t(data[offset + 2]) << 16) | (std::uint32_t(data[offset + 3]) << 24);
        }

        unsigned U16(const Bytes &data, std::size_t offset) {
            if (offset > data.size() || data.size() - offset < 2)
                throw std::runtime_error("Truncated 16-bit field");
            return data[offset] | (unsigned(data[offset + 1]) << 8);
        }

        // Normal zlib for compressed images; gzip for the outer resource container.
        // Unlike a guessed buffer size, this loop also verifies CRC/Adler and EOF.
        Bytes Inflate(const Bytes &input, int window_bits, std::size_t limit) {
            z_stream stream{};
            if (inflateInit2(&stream, window_bits) != Z_OK)
                throw std::runtime_error("Cannot initialize zlib");
            struct End {
                z_stream *stream;
                ~End() { inflateEnd(stream); }
            } end{&stream};
            stream.next_in = const_cast<Bytef *>(input.data());
            stream.avail_in = static_cast<uInt>(input.size());
            Bytes result;
            std::array<std::uint8_t, 32768> chunk{};
            int status = Z_OK;
            do {
                stream.next_out = chunk.data();
                stream.avail_out = static_cast<uInt>(chunk.size());
                status = inflate(&stream, Z_NO_FLUSH);
                if (status != Z_OK && status != Z_STREAM_END)
                    throw std::runtime_error("Invalid or truncated compressed resource");
                const auto produced = chunk.size() - stream.avail_out;
                if (produced > limit - result.size())
                    throw std::runtime_error("Decoded resource exceeds size limit");
                result.insert(result.end(), chunk.begin(), chunk.begin() + produced);
                if (status == Z_OK && produced == 0 && stream.avail_in == 0)
                    throw std::runtime_error("Compressed resource has no end marker");
            } while (status != Z_STREAM_END);
            if (stream.avail_in != 0)
                throw std::runtime_error("Trailing bytes after compressed stream");
            return result;
        }
    } // namespace

    Bytes ReadFile(const std::filesystem::path &path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("Cannot open resource: " + path.string());
        const auto size = file.tellg();
        if (size < 0 || static_cast<std::uint64_t>(size) > kMaxBytes)
            throw std::runtime_error("Resource exceeds 64 MiB limit: " + path.string());
        Bytes data(static_cast<std::size_t>(size));
        file.seekg(0);
        if (!data.empty() && !file.read(reinterpret_cast<char *>(data.data()),
                                        static_cast<std::streamsize>(data.size())))
            throw std::runtime_error("Cannot read resource: " + path.string());
        return data;
    }

    void WriteFile(const std::filesystem::path &path, const Bytes &bytes) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file || (!bytes.empty() && !file.write(reinterpret_cast<const char *>(bytes.data()),
                                                    static_cast<std::streamsize>(bytes.size()))))
            throw std::runtime_error("Cannot write output: " + path.string());
    }

    Bytes InflateZlib(const Bytes &input, std::size_t limit) {
        if (input.size() > kMaxBytes || limit > kMaxBytes)
            throw std::runtime_error("Compressed resource exceeds 64 MiB limit");
        return Inflate(input, MAX_WBITS, limit);
    }

    static ResourcePack ParseResourcePack(const Bytes &decoded, ResourceFilter include) {
        if (decoded.size() < 4 || decoded.size() > kMaxBytes)
            throw std::runtime_error("Invalid resource container size");
        ResourcePack pack;
        pack.signature.assign(decoded.begin(), decoded.begin() + 4);
        if (pack.signature != "res0" && pack.signature != "res2")
            throw std::runtime_error("Invalid resource signature; expected res0 or res2");
        // FUN_0046bd20 advances by record+12+payload_size, not by alignment.
        std::set<std::uint32_t> ids;
        for (std::size_t offset = 4; offset < decoded.size();) {
            if (decoded.size() - offset < 12)
                throw std::runtime_error("Truncated resource record header");
            Resource entry;
            entry.id = U32(decoded, offset);
            entry.metadata = U32(decoded, offset + 4);
            const auto size = U32(decoded, offset + 8);
            offset += 12;
            if (size > decoded.size() - offset)
                throw std::runtime_error("Resource payload exceeds container");
            if (!ids.insert(entry.id).second)
                throw std::runtime_error("Duplicate resource ID");
            if (!include || include(entry.id, entry.metadata)) {
                entry.payload.assign(decoded.begin() + offset, decoded.begin() + offset + size);
                pack.entries.push_back(std::move(entry));
            }
            offset += size;
        }
        if (ids.empty()) throw std::runtime_error("Empty resource container");
        return pack;
    }

    ResourcePack LoadResourcePack(const std::filesystem::path &path, ResourceFilter include) {
        const auto raw = ReadFile(path);
        // The cached .dat files retain their own gzip layer after download.
        return ParseResourcePack(Inflate(raw, MAX_WBITS + 16, kMaxBytes), include);
    }

    Image DecodeImage(const Resource &resource) {
        Image image;
        image.width = static_cast<int>(resource.metadata & 0xffff);
        image.height = static_cast<int>(resource.metadata >> 16);
        if (image.width <= 0 || image.height <= 0 || image.width > 8192 || image.height > 8192)
            throw std::runtime_error("Invalid image dimensions");
        const auto size = std::size_t(image.width) * image.height * 4;
        if (size > kMaxBytes) throw std::runtime_error("Image exceeds 64 MiB limit");
        // FUN_004691a0: raw pixels if size=w*h*4; FUN_00469310: zlib otherwise.
        image.bgra = resource.payload.size() == size ? resource.payload : Inflate(resource.payload, MAX_WBITS, size);
        if (image.bgra.size() != size) throw std::runtime_error("Image pixel count mismatch");
        return image;
    }

    WaveInfo InspectWave(const Resource &resource) {
        const auto &data = resource.payload;
        if (data.size() < 12 || std::memcmp(data.data(), "RIFF", 4) != 0 ||
            std::memcmp(data.data() + 8, "WAVE", 4) != 0 || U32(data, 4) != data.size() - 8)
            throw std::runtime_error("Invalid RIFF/WAVE resource");
        WaveInfo info;
        bool format = false, samples = false;
        for (std::size_t offset = 12; offset < data.size();) {
            if (data.size() - offset < 8) throw std::runtime_error("Truncated WAV chunk");
            const auto size = U32(data, offset + 4);
            if (size > data.size() - offset - 8) throw std::runtime_error("WAV chunk exceeds file");
            if (std::memcmp(data.data() + offset, "fmt ", 4) == 0) {
                if (format || size < 16 || U16(data, offset + 8) != 1)
                    throw std::runtime_error("Expected one PCM WAV format chunk");
                info.channels = U16(data, offset + 10);
                info.sample_rate = U32(data, offset + 12);
                info.bits_per_sample = U16(data, offset + 22);
                const unsigned align = info.channels * info.bits_per_sample / 8;
                if (!info.channels || !info.sample_rate || !align ||
                    (info.bits_per_sample != 8 && info.bits_per_sample != 16) ||
                    U16(data, offset + 20) != align || U32(data, offset + 16) != info.sample_rate * align)
                    throw std::runtime_error("Invalid WAV format fields");
                format = true;
            } else if (std::memcmp(data.data() + offset, "data", 4) == 0) {
                if (samples) throw std::runtime_error("Duplicate WAV data chunk");
                info.data_bytes = size;
                samples = true;
            }
            offset += 8 + size;
            if (size & 1) {
                if (offset == data.size()) throw std::runtime_error("Missing WAV chunk padding");
                ++offset;
            }
        }
        if (!format || !samples || info.data_bytes % (info.channels * info.bits_per_sample / 8))
            throw std::runtime_error("Incomplete WAV resource");
        return info;
    }

    std::string Sha256(const Bytes &bytes) {
        // Standard FIPS 180-4 SHA-256 (zero OS/crypto library dependency).
        static constexpr std::uint32_t kRound[64]{
            0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
            0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
            0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
            0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
            0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
            0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
            0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
            0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffaU, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
        };
        const auto rotr = [](std::uint32_t x, unsigned n) -> std::uint32_t {
            return (x >> n) | (x << (32u - n));
        };
        std::array<std::uint32_t, 8> state{
            0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
            0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
        };
        const auto compress = [&](const std::uint8_t *block) {
            std::uint32_t w[64]{};
            for (int i = 0; i < 16; ++i) {
                w[i] = (std::uint32_t(block[i * 4]) << 24) | (std::uint32_t(block[i * 4 + 1]) << 16) |
                       (std::uint32_t(block[i * 4 + 2]) << 8) | std::uint32_t(block[i * 4 + 3]);
            }
            for (int i = 16; i < 64; ++i) {
                const std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
                const std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }
            std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
            std::uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
            for (int i = 0; i < 64; ++i) {
                const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
                const std::uint32_t ch = (e & f) ^ ((~e) & g);
                const std::uint32_t temp1 = h + s1 + ch + kRound[i] + w[i];
                const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
                const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                const std::uint32_t temp2 = s0 + maj;
                h = g; g = f; f = e; e = d + temp1;
                d = c; c = b; b = a; a = temp1 + temp2;
            }
            state[0] += a; state[1] += b; state[2] += c; state[3] += d;
            state[4] += e; state[5] += f; state[6] += g; state[7] += h;
        };

        const std::size_t total = bytes.size();
        std::size_t offset = 0;
        while (total - offset >= 64) {
            compress(bytes.data() + offset);
            offset += 64;
        }
        std::array<std::uint8_t, 128> tail{};
        const std::size_t rem = total - offset;
        if (rem) std::memcpy(tail.data(), bytes.data() + offset, rem);
        tail[rem] = 0x80u;
        const std::size_t padded_len = (rem < 56) ? 64u : 128u;
        const std::uint64_t bit_len = static_cast<std::uint64_t>(total) * 8u;
        for (int i = 0; i < 8; ++i)
            tail[padded_len - 1 - i] = static_cast<std::uint8_t>((bit_len >> (i * 8)) & 0xffu);
        compress(tail.data());
        if (padded_len == 128u) compress(tail.data() + 64);

        static constexpr char kHex[] = "0123456789abcdef";
        std::string out(64, '0');
        for (int i = 0; i < 8; ++i) {
            for (int nibble = 0; nibble < 8; ++nibble) {
                out[static_cast<std::size_t>(i * 8 + nibble)] = kHex[(state[i] >> (28 - nibble * 4)) & 0xfu];
            }
        }
        return out;
    }
} // namespace tenhou
