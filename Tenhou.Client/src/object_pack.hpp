#pragma once
#include "resource_pack.hpp"
#include <array>
#include <unordered_map>

namespace tenhou {
struct ObjectRecord {
    std::uint32_t slot = 0, file_offset = 0;
    int x = 0, y = 0, width = 0, height = 0;
    std::array<std::uint8_t, 4> key{};
    std::uint32_t reference = 0, stored_bytes = 0;
};

struct ObjectPack {
    std::string signature;
    std::uint32_t header_bytes = 0;
    std::array<float, 3> cached_color_scale{1.0f, 1.0f, 1.0f};
    Bytes file;
    std::vector<ObjectRecord> entries;
    std::unordered_map<std::uint32_t, std::size_t> key_index;
};

struct ObjectImage {
    int x = 0, y = 0;
    Image image;
};

ObjectPack LoadObjectPack(const std::filesystem::path& path);
ObjectPack ParseObjectPack(Bytes bytes);
Bytes InflateObject(const ObjectPack& pack, const ObjectRecord& record);
ObjectImage DecodeObject(const ObjectPack& pack, std::size_t index,
                         const ObjectPack* companion, bool original_color = true,
                         const std::array<float,3>* rgb = nullptr);
std::string ObjectKey(const ObjectRecord& record);
}  // namespace tenhou
