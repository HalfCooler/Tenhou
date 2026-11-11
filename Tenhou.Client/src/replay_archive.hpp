#pragma once
#include "network_protocol.hpp"
#include <filesystem>
namespace tenhou {
class ReplayArchive {
public:
    bool Request(const LobbySnapshot& snapshot,const std::filesystem::path& config);
    void Receive(const NetworkMessage& message);
    void Reset();
    const std::string& status() const { return status_; }
private:
    std::filesystem::path directory_;
    std::string id_,data_,status_,requested_;
    std::size_t expected_=0;
};
}
