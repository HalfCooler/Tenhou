#include "application.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    try {
        const auto options = tenhou::ParseOptions(__argc, __argv);
        tenhou::Application application(options);
        return application.Run(instance, show);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        if (__argc==1) MessageBoxA(nullptr, error.what(), "天凤客户端", MB_OK | MB_ICONERROR);
        return 1;
    }
}
