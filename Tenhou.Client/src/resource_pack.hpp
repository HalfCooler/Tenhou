#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace tenhou {
    using Bytes = std::vector<std::uint8_t>;

    struct Resource {
        std::uint32_t id = 0;
        // Original +4 field: low 16 bits = width; high 16 bits = height.
        // Zero denotes a non-image record (WAV in the two current packs).
        std::uint32_t metadata = 0;
        Bytes payload;
    };

    struct ResourcePack {
        std::string signature;
        std::vector<Resource> entries;
    };

    struct Image {
        int width = 0;
        int height = 0;
        Bytes bgra; // Top-down PixelFormat32bppARGB, as in FUN_004691a0.
    };

    struct WaveInfo {
        unsigned channels = 0;
        unsigned sample_rate = 0;
        unsigned bits_per_sample = 0;
        std::uint32_t data_bytes = 0;
    };

    Bytes ReadFile(const std::filesystem::path &path);

    Bytes InflateZlib(const Bytes &input, std::size_t limit);

    // Select records by ID and metadata before copying their payloads.
    using ResourceFilter = bool (*)(std::uint32_t id, std::uint32_t metadata);
    ResourcePack LoadResourcePack(const std::filesystem::path &path, ResourceFilter include = nullptr);

    Image DecodeImage(const Resource &resource);

    WaveInfo InspectWave(const Resource &resource);

    std::string Sha256(const Bytes &bytes);

    void WriteFile(const std::filesystem::path &path, const Bytes &bytes);
} // namespace tenhou
