#include "application.hpp"
#include "win32_utf.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int WINAPI WinMain(const HINSTANCE instance, HINSTANCE, LPSTR, const int nShowCmd) {
    try {
        const auto options = tenhou::ParseOptions(__argc, __argv);
        tenhou::Application application(options);
        return application.Run(instance, nShowCmd);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        if (__argc == 1)
            MessageBoxW(nullptr, tenhou::Utf8ToWide(error.what()).c_str(), L"天凤客户端", MB_OK | MB_ICONERROR);
        return 1;
    }
}
