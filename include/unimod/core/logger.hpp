#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <spdlog/spdlog.h>

namespace unimod::core {

class Logger {
public:
    static void Initialize(const std::string& logFileName = "unimod.log", bool allocateConsole = true);
    static void Shutdown();

    static std::shared_ptr<spdlog::logger>& GetLogger();

private:
    static std::shared_ptr<spdlog::logger> s_Logger;
    static bool s_ConsoleAllocated;
};

} // namespace unimod::core

// Convenience Logging Macros with file & line context
#define UNIMOD_LOG_TRACE(...)    ::unimod::core::Logger::GetLogger()->trace(__VA_ARGS__)
#define UNIMOD_LOG_DEBUG(...)    ::unimod::core::Logger::GetLogger()->debug(__VA_ARGS__)
#define UNIMOD_LOG_INFO(...)     ::unimod::core::Logger::GetLogger()->info(__VA_ARGS__)
#define UNIMOD_LOG_WARN(...)     ::unimod::core::Logger::GetLogger()->warn(__VA_ARGS__)
#define UNIMOD_LOG_ERROR(...)    ::unimod::core::Logger::GetLogger()->error(__VA_ARGS__)
#define UNIMOD_LOG_CRITICAL(...) ::unimod::core::Logger::GetLogger()->critical(__VA_ARGS__)
