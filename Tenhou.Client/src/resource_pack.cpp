#include "resource_pack.hpp"
#include <windows.h>
#include <bcrypt.h>
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

std::uint32_t U32(const Bytes& data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 4)
        throw std::runtime_error("Truncated 32-bit field");
    return data[offset] | (std::uint32_t(data[offset + 1]) << 8) |
           (std::uint32_t(data[offset + 2]) << 16) | (std::uint32_t(data[offset + 3]) << 24);
}

unsigned U16(const Bytes& data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 2)
        throw std::runtime_error("Truncated 16-bit field");
    return data[offset] | (unsigned(data[offset + 1]) << 8);
}

// Normal zlib for compressed images; gzip for the outer resource container.
// Unlike a guessed buffer size, this loop also verifies CRC/Adler and EOF.
Bytes Inflate(const Bytes& input, int window_bits, std::size_t limit) {
    z_stream stream{};
    if (inflateInit2(&stream, window_bits) != Z_OK)
        throw std::runtime_error("Cannot initialize zlib");
    struct End { z_stream* stream; ~End() { inflateEnd(stream); } } end{&stream};
    stream.next_in = const_cast<Bytef*>(input.data());
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
}  // namespace

Bytes ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Cannot open resource: " + path.string());
    const auto size = file.tellg();
    if (size < 0 || static_cast<std::uint64_t>(size) > kMaxBytes)
        throw std::runtime_error("Resource exceeds 64 MiB limit: " + path.string());
    Bytes data(static_cast<std::size_t>(size));
    file.seekg(0);
    if (!data.empty() && !file.read(reinterpret_cast<char*>(data.data()),
                                    static_cast<std::streamsize>(data.size())))
        throw std::runtime_error("Cannot read resource: " + path.string());
    return data;
}

void WriteFile(const std::filesystem::path& path, const Bytes& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file || (!bytes.empty() && !file.write(reinterpret_cast<const char*>(bytes.data()),
                                              static_cast<std::streamsize>(bytes.size()))))
        throw std::runtime_error("Cannot write output: " + path.string());
}

ResourcePack LoadResourcePack(const std::filesystem::path& path) {
    const auto raw = ReadFile(path);
    // The cached .dat files retain their own gzip layer after download.
    return ParseResourcePack(Inflate(raw, MAX_WBITS + 16, kMaxBytes));
}

Bytes InflateZlib(const Bytes& input, std::size_t limit) {
    if (input.size() > kMaxBytes || limit > kMaxBytes)
        throw std::runtime_error("Compressed resource exceeds 64 MiB limit");
    return Inflate(input, MAX_WBITS, limit);
}

ResourcePack ParseResourcePack(Bytes decoded) {
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
        entry.payload.assign(decoded.begin() + offset, decoded.begin() + offset + size);
        pack.entries.push_back(std::move(entry));
        offset += size;
    }
    if (pack.entries.empty()) throw std::runtime_error("Empty resource container");
    pack.decoded = std::move(decoded);
    return pack;
}

Image DecodeImage(const Resource& resource) {
    Image image;
    image.width = static_cast<int>(resource.metadata & 0xffff);
    image.height = static_cast<int>(resource.metadata >> 16);
    if (image.width <= 0 || image.height <= 0 || image.width > 8192 || image.height > 8192)
        throw std::runtime_error("Invalid image dimensions");
    const auto size = std::size_t(image.width) * image.height * 4;
    if (size > kMaxBytes) throw std::runtime_error("Image exceeds 64 MiB limit");
    // FUN_004691a0: raw pixels if size=w*h*4; FUN_00469310: zlib otherwise.
    image.bgra = resource.payload.size() == size ? resource.payload :
                 Inflate(resource.payload, MAX_WBITS, size);
    if (image.bgra.size() != size) throw std::runtime_error("Image pixel count mismatch");
    return image;
}

WaveInfo InspectWave(const Resource& resource) {
    const auto& data = resource.payload;
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

std::string Sha256(const Bytes& bytes) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    struct Cleanup {
        BCRYPT_ALG_HANDLE& algorithm; BCRYPT_HASH_HANDLE& hash;
        ~Cleanup() { if (hash) BCryptDestroyHash(hash); if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0); }
    } cleanup{algorithm, hash};
    std::array<unsigned char, 32> digest{};
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0 ||
        BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) < 0 ||
        BCryptHashData(hash, const_cast<PUCHAR>(bytes.data()), static_cast<ULONG>(bytes.size()), 0) < 0 ||
        BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0)
        throw std::runtime_error("Windows SHA-256 failed");
    std::ostringstream text;
    text << std::hex << std::setfill('0');
    for (const auto byte : digest) text << std::setw(2) << unsigned(byte);
    return text.str();
}
}  // namespace tenhou
