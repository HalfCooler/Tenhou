#include "object_pack.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <set>
#include <sstream>
#include <stdexcept>

namespace tenhou {
namespace {
std::uint32_t U32(const Bytes& bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) throw std::runtime_error("Truncated object field");
    return bytes[offset] | (std::uint32_t(bytes[offset + 1]) << 8) |
           (std::uint32_t(bytes[offset + 2]) << 16) | (std::uint32_t(bytes[offset + 3]) << 24);
}
int I16(const Bytes& bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 2) throw std::runtime_error("Truncated object coordinate");
    const unsigned value = bytes[offset] | (unsigned(bytes[offset + 1]) << 8);
    return value >= 32768 ? int(value) - 65536 : int(value);
}
std::uint32_t Key(const std::array<std::uint8_t, 4>& key) {
    return key[0] | (std::uint32_t(key[1]) << 8) | (std::uint32_t(key[2]) << 16) | (std::uint32_t(key[3]) << 24);
}
const ObjectRecord& Find(const ObjectPack& pack, std::array<std::uint8_t, 4> key) {
    const auto found = pack.key_index.find(Key(key));
    if (found == pack.key_index.end()) throw std::runtime_error("Object companion base is missing");
    return pack.entries[found->second];
}
ObjectImage RawImage(const ObjectPack& pack, const ObjectRecord& record) {
    ObjectImage result;
    result.x = record.x;
    result.y = record.y;
    result.image.width = record.width;
    result.image.height = record.height;
    result.image.bgra = InflateObject(pack, record);
    if (result.image.bgra.size() != std::size_t(record.width) * record.height * 4)
        throw std::runtime_error("Object BGRA length mismatch");
    return result;
}
ObjectImage ColorBase(const ObjectPack& odd, const ObjectRecord& base, const ObjectPack* even, bool startup,
                      const std::array<float,3>* rgb) {
    auto result = RawImage(odd, base);
    if (!startup) return result;
    if (!even) throw std::runtime_error("Startup object colors require the even mask pack");
    if ((odd.signature == "obj1" && even->signature != "obj0") ||
        (odd.signature == "obj3" && even->signature != "obj2"))
        throw std::runtime_error("Wrong object mask resolution");
    auto key = base.key;
    key[2] = base.key[2] == 37 ? 39 : 38;
    const auto& mask = Find(*even, key);
    if (mask.x != base.x || mask.y != base.y || mask.width != base.width || mask.height != base.height)
        throw std::runtime_error("Object mask/base rectangles differ");
    const auto pixels = RawImage(*even, mask).image.bgra;
    std::uint32_t blue_bits = 0x3e189899, reciprocal_bits = 0x3b808081;
    float blue = 0, reciprocal = 0;
    std::memcpy(&blue, &blue_bits, 4);
    std::memcpy(&reciprocal, &reciprocal_bits, 4);
    for (std::size_t offset = 0; offset < pixels.size(); offset += 4) {
        if (!(pixels[offset] | pixels[offset + 1] | pixels[offset + 2] | pixels[offset + 3])) continue;
        const double alpha = pixels[offset + 3] * double(reciprocal);
        std::uint32_t channels[3]{};
        for (unsigned channel = 0; channel < 3; ++channel) {
            const double scale = rgb ? double((*rgb)[2-channel]) : channel == 0 ? double(blue) : 0.0;
            // x87 retains precision across this expression; float intermediates
            // change truncation. Exhaustive alpha/gray tests cover the byte domain.
            const int value = int(pixels[offset + 2] * ((1.0 - alpha) + alpha * scale));
            channels[channel]=static_cast<std::uint32_t>(value);
        }
        // Original integer shifts/OR also define the edit-box-only value 256.
        const auto packed=(std::uint32_t(result.image.bgra[offset+3])<<24)|channels[0]|(channels[1]<<8)|(channels[2]<<16);
        for(unsigned channel=0;channel<4;++channel) result.image.bgra[offset+channel]=std::uint8_t(packed>>(channel*8));
    }
    return result;
}
}  // namespace

ObjectPack LoadObjectPack(const std::filesystem::path& path) { return ParseObjectPack(ReadFile(path)); }

ObjectPack ParseObjectPack(Bytes bytes) {
    if (bytes.size() < 12) throw std::runtime_error("Truncated object pack header");
    ObjectPack pack;
    pack.signature.assign(bytes.begin(), bytes.begin() + 4);
    const bool even = pack.signature == "obj0" || pack.signature == "obj2";
    const bool odd = pack.signature == "obj1" || pack.signature == "obj3";
    if (!even && !odd) throw std::runtime_error("Invalid object pack signature");
    pack.header_bytes = U32(bytes, 4);
    if (pack.header_bytes != (even ? 0x19564u : 0xf10u) || bytes.size() < pack.header_bytes ||
        U32(bytes, 8) != bytes.size() - pack.header_bytes)
        throw std::runtime_error("Invalid object pack header/payload size");
    const std::size_t prefix = even ? 12 : 24;
    if (odd) {
        for (unsigned i = 0; i < 3; ++i) {
            const auto bits = U32(bytes, 12 + 4 * i);
            std::memcpy(&pack.cached_color_scale[i], &bits, sizeof(float));
            if (!std::isfinite(pack.cached_color_scale[i]) || pack.cached_color_scale[i] < 0 || pack.cached_color_scale[i] > 1)
                throw std::runtime_error("Invalid object cached color scale");
        }
    }
    std::set<std::uint32_t> offsets;
    for (std::size_t position = prefix; position < pack.header_bytes; position += 4) {
        const auto offset = U32(bytes, position);
        if (!offset) continue;
        if (offset < pack.header_bytes || offset > bytes.size() || bytes.size() - offset < 20 || !offsets.insert(offset).second)
            throw std::runtime_error("Invalid or duplicate object offset");
        ObjectRecord record;
        record.slot = static_cast<std::uint32_t>((position - prefix) / 4);
        record.file_offset = offset;
        record.x = I16(bytes, offset);
        record.y = I16(bytes, offset + 2);
        record.width = I16(bytes, offset + 4);
        record.height = I16(bytes, offset + 6);
        if (record.width <= 0 || record.height <= 0 || record.width > 4096 || record.height > 4096)
            throw std::runtime_error("Invalid object dimensions");
        std::copy_n(bytes.begin() + offset + 8, 4, record.key.begin());
        record.reference = U32(bytes, offset + 12);
        record.stored_bytes = U32(bytes, offset + 16);
        if (record.reference != 0 || record.stored_bytes > bytes.size() - offset - 20)
            throw std::runtime_error("Object compressed bytes exceed file");
        if (!pack.key_index.emplace(Key(record.key), pack.entries.size()).second)
            throw std::runtime_error("Duplicate object key");
        pack.entries.push_back(record);
    }
    if (pack.entries.empty()) throw std::runtime_error("Object pack has no records");
    // These release packs cover the payload with contiguous indexed records.
    if (*offsets.begin() != pack.header_bytes) throw std::runtime_error("Gap before first object record");
    for (const auto& record : pack.entries) {
        const auto next = offsets.upper_bound(record.file_offset);
        const auto expected_end = next != offsets.end() ? std::uint64_t(*next) : std::uint64_t(bytes.size());
        if (std::uint64_t(record.file_offset) + 20 + record.stored_bytes != expected_end)
            throw std::runtime_error("Object record overlap, gap or trailing bytes");
    }
    pack.file = std::move(bytes);
    return pack;
}

Bytes InflateObject(const ObjectPack& pack, const ObjectRecord& record) {
    const auto begin = std::size_t(record.file_offset) + 20;
    if (begin > pack.file.size() || record.stored_bytes > pack.file.size() - begin)
        throw std::runtime_error("Invalid object payload range");
    const Bytes compressed(pack.file.begin() + begin, pack.file.begin() + begin + record.stored_bytes);
    // Delta pixels need one index per pixel plus a small palette; raw needs 4 B/pixel.
    return InflateZlib(compressed, std::size_t(record.width) * record.height * 4 + 1024);
}

std::string ObjectKey(const ObjectRecord& record) {
    std::ostringstream label;
    label << char(record.key[0]) << '/' << unsigned(record.key[1]) << '/' << unsigned(record.key[2]) << '/' << unsigned(record.key[3]);
    return label.str();
}

ObjectImage DecodeObject(const ObjectPack& pack, std::size_t index, const ObjectPack* companion, bool original_color,
                          const std::array<float,3>* rgb) {
    if (index >= pack.entries.size()) throw std::runtime_error("Object index outside pack");
    const auto& record = pack.entries[index];
    const bool odd = pack.signature == "obj1" || pack.signature == "obj3";
    if (odd) return ColorBase(pack, record, companion, original_color,rgb);
    if (record.key[2] == 38 || record.key[2] == 39) return RawImage(pack, record);
    if (!companion) throw std::runtime_error("Delta object requires its odd companion pack");
    auto base_key = record.key;
    base_key[2] = 34;
    const auto& base = Find(*companion, base_key);
    if ((pack.signature == "obj0" && companion->signature != "obj1") ||
        (pack.signature == "obj2" && companion->signature != "obj3"))
        throw std::runtime_error("Wrong object companion resolution");
    auto result = ColorBase(*companion, base, &pack, original_color,rgb);
    const auto decoded = InflateObject(pack, record);
    if (decoded.empty()) throw std::runtime_error("Empty object palette");
    const std::size_t palette_count = decoded[0];
    const std::size_t pixels_offset = 1 + palette_count * 4;
    if (!palette_count || decoded.size() != pixels_offset + std::size_t(record.width) * record.height)
        throw std::runtime_error("Object palette/index length mismatch");
    const int left = record.x - base.x, top = record.y - base.y;
    if (left < 0 || top < 0 || left + record.width > base.width || top + record.height > base.height)
        throw std::runtime_error("Delta object exceeds base rectangle");
    for (int y = 0; y < record.height; ++y) {
        for (int x = 0; x < record.width; ++x) {
            const auto palette_index = decoded[pixels_offset + std::size_t(y) * record.width + x];
            if (palette_index >= palette_count) throw std::runtime_error("Object palette index is invalid");
            const auto value = U32(decoded, 1 + std::size_t(palette_index) * 4);
            const auto destination = (std::size_t(y + top) * base.width + x + left) * 4;
            for (unsigned channel = 0; channel < 4; ++channel)
                result.image.bgra[destination + channel] ^= std::uint8_t(value >> (channel * 8));
        }
    }
    return result;
}
}  // namespace tenhou
