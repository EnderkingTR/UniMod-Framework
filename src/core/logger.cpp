#include <unimod/core/logger.hpp>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <vector>
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace unimod::core {

std::shared_ptr<spdlog::logger> Logger::s_Logger = nullptr;
bool Logger::s_ConsoleAllocated = false;

void Logger::Initialize(const std::string& logFileName, bool allocateConsole) {
    if (s_Logger) {
        return; // Already initialized
    }

#ifdef _WIN32
    if (allocateConsole && GetConsoleWindow() == nullptr) {
        if (AllocConsole()) {
            FILE* dummy = nullptr;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            freopen_s(&dummy, "CONOUT$", "w", stderr);
            freopen_s(&dummy, "CONIN$", "r", stdin);

            // Enable virtual terminal processing for ANSI color sequences
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut != INVALID_HANDLE_VALUE) {
                DWORD mode = 0;
                if (GetConsoleMode(hOut, &mode)) {
                    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
                }
            }

            SetConsoleTitleA("UniMod Framework - Core Debug Console");
            s_ConsoleAllocated = true;
        }
    }
#endif

    std::vector<spdlog::sink_ptr> sinks;

    // Sink 1: Color stdout console sink
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::trace);
    consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    sinks.push_back(consoleSink);

    // Sink 2: Rotating or basic file sink
    try {
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFileName, true);
        fileSink->set_level(spdlog::level::trace);
        fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
        sinks.push_back(fileSink);
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "[UniMod] Failed to initialize file logger sink: " << ex.what() << std::endl;
    }

    s_Logger = std::make_shared<spdlog::logger>("UNIMOD", sinks.begin(), sinks.end());
    s_Logger->set_level(spdlog::level::trace);
    s_Logger->flush_on(spdlog::level::warn);

    spdlog::register_logger(s_Logger);
    spdlog::set_default_logger(s_Logger);

    s_Logger->info("UniMod Framework Core Logger initialized successfully.");
}

void Logger::Shutdown() {
    if (s_Logger) {
        s_Logger->info("Shutting down UniMod Framework Core Logger.");
        s_Logger->flush();
        spdlog::drop_all();
        s_Logger = nullptr;
    }

#ifdef _WIN32
    if (s_ConsoleAllocated) {
        FreeConsole();
        s_ConsoleAllocated = false;
    }
#endif
}

std::shared_ptr<spdlog::logger>& Logger::GetLogger() {
    if (!s_Logger) {
        // Fallback default logger if accessed before explicit initialization
        Initialize("unimod_fallback.log", false);
    }
    return s_Logger;
}

} // namespace unimod::core
