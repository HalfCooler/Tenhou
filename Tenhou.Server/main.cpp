#include "service.hpp"
#include "storage.hpp"
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>

namespace {
    HANDLE stop_event = nullptr;
    BOOL WINAPI control(DWORD) {
        if (stop_event) SetEvent(stop_event);
        return TRUE;
    }
}

int wmain(int argc, wchar_t **argv) {
    try {
        wchar_t module[32768]{};
        GetModuleFileNameW(nullptr, module, 32768);
        auto path = std::filesystem::path(module).parent_path() / L"server.ini";
        bool stop = false;
        std::filesystem::path diagnostic;
        for (int i = 1; i < argc; ++i) {
            if (std::wstring(argv[i]) == L"--config" && i + 1 < argc) path = std::filesystem::absolute(argv[++i]);
            else if (std::wstring(argv[i]) == L"--stop") stop = true;
            else if (std::wstring(argv[i]) == L"--verify-record" && i + 1 < argc)
                diagnostic = std::filesystem::absolute(argv[++i]);
            else throw std::runtime_error("Usage: tenhou_server [--config server.ini] [--stop] | --verify-record FILE");
        }
        if (!diagnostic.empty()) {
            std::istringstream input(authority::read_state(diagnostic));
            std::string line;
            if (!std::getline(input, line)) throw std::runtime_error("Missing diagnostic record");
            auto [header_tag, header_attrs] = authority::parse(line);
            if (header_tag != "DIAGNOSTIC") throw std::runtime_error("Invalid diagnostic record");
            auto a = header_attrs;
            auto seed = std::stoull(a.at("seed"));
            bool general = a.at("general") == "1";
            int type = authority::number(a.at("type"));
            for (const char *key: {"seed", "general", "type"}) a.erase(key);
            authority::Game game(general ? authority::Config::general(type) : authority::Config::from(a), seed);
            unsigned commands = 0;
            while (std::getline(input, line)) {
                if (auto [tag, attrs] = authority::parse(line); tag == "START") game.start();
                else if (tag == "ACT") game.submit(authority::number(attrs.at("seat")), { attrs.at("kind"), authority::integers(attrs.at("tiles")) });
                else throw std::runtime_error("Unsupported diagnostic command");
                game.invariants();
                ++commands;
            }
            std::cout << "{\"verified_commands\":" << commands << ",\"hands\":" << game.hand_number << R"(,"phase":")" << game.phase << "\"}" << std::endl;
            return 0;
        }
        if (!std::filesystem::is_regular_file(path)) throw std::runtime_error("Missing explicit server.ini");
        auto text = [&](const wchar_t *key, const wchar_t *fallback) {
            wchar_t value[32768]{};
            GetPrivateProfileStringW(L"server", key, fallback, value, 32768, path.c_str());
            return std::wstring(value);
        };
        auto ascii = [](const std::wstring &value) {
            std::string result;
            for (const auto c: value) {
                if (c > 127) throw std::runtime_error("Expected ASCII endpoint/number");
                result += static_cast<char>(c);
            }
            return result;
        };
        auto num = [&](const wchar_t *key, const int fallback, const int low, const int high) {
            return authority::number(ascii(text(key, std::to_wstring(fallback).c_str())), low, high);
        };
        authority::Settings settings;
        settings.listen = ascii(text(L"listen", L"127.0.0.1"));
        settings.port = num(L"port", 10080, 1, 65535);
        settings.humans = num(L"humans", 1, 1, 4);
        settings.bot_delay = num(L"bot_delay_ms", 900, 0, 60000);
        settings.max_connections = num(L"max_connections", 60, 1, 60);
        settings.max_sessions = num(L"max_sessions", 4096, 1, 10000);
        settings.max_tables = num(L"max_tables", 64, 1, 1000);
        settings.shutdown_ms = num(L"shutdown_ms", 5000, 0, 60000);
        auto relative = [&](const wchar_t *key, const wchar_t *fallback) {
            auto p = std::filesystem::path(text(key, fallback));
            return p.is_absolute() ? p : path.parent_path() / p;
        };
        settings.state = relative(L"state", L"state/authority.dat");
        settings.logs = relative(L"logs", L"logs");
        settings.replays = relative(L"replays", L"games");
        auto event_name = L"Local\\TenhouAuthority26-" + std::to_wstring(settings.port);
        if (stop) {
            HANDLE h = OpenEventW(EVENT_MODIFY_STATE, FALSE, event_name.c_str());
            if (!h) throw std::runtime_error("Service is not running");
            SetEvent(h);
            CloseHandle(h);
            return 0;
        }
        stop_event = CreateEventW(nullptr, TRUE, FALSE, event_name.c_str());
        if (!stop_event || GetLastError() == ERROR_ALREADY_EXISTS) throw std::runtime_error(
            "Service endpoint already running");
        SetConsoleCtrlHandler(control, TRUE);
        auto lockpath = settings.state.wstring() + L".lock";
        std::filesystem::create_directories(settings.state.parent_path());
        HANDLE lock = CreateFileW(lockpath.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                  nullptr);
        if (lock == INVALID_HANDLE_VALUE) throw std::runtime_error("State already owned by another process");
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data)) throw std::runtime_error("Winsock initialization failed");
        int result;
        {
            authority::Service service(settings);
            result = service.run(stop_event);
        }
        WSACleanup();
        CloseHandle(lock);
        CloseHandle(stop_event);
        return result;
    } catch (const std::exception &e) {
        std::cerr << "Native authority failed: " << e.what() << std::endl;
        return 1;
    }
}
