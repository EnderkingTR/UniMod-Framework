#include <unimod/core/logger.hpp>
#include <minhook.h>

#include <iostream>
#include <fstream>
#include <concepts>
#include <span>
#include <vector>

// C++20 Concept verification
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<Numeric T>
T Square(T value) {
    return value * value;
}

int main() {
    std::cout << "====================================================\n";
    std::cout << "  UniMod Framework - Phase 1 Verification Test\n";
    std::cout << "====================================================\n";

    const std::string logFile = "phase1_test.log";

    // 1. Initialize Logger
    unimod::core::Logger::Initialize(logFile, false);

    UNIMOD_LOG_INFO("--- Phase 1: Environment & Toolchain Verification ---");

    // 2. Test C++20 Standard Features
    UNIMOD_LOG_INFO("Verifying C++20 standard compliance...");
    
    // Concepts test
    int intVal = 7;
    double dblVal = 4.5;
    UNIMOD_LOG_DEBUG("Concept check: Square(7) = {}, Square(4.5) = {}", Square(intVal), Square(dblVal));

    // std::span test
    std::vector<uint8_t> testBytes = { 0x48, 0x89, 0x5C, 0x24, 0x08, 0x57, 0x48, 0x83, 0xEC, 0x20 };
    std::span<const uint8_t> byteSpan(testBytes.data(), testBytes.size());
    UNIMOD_LOG_DEBUG("std::span check: byteSpan size = {}, first byte = 0x{:02X}", byteSpan.size(), byteSpan[0]);

    // 3. Test MinHook integration
    UNIMOD_LOG_INFO("Verifying MinHook integration...");
    MH_STATUS mhStatus = MH_Initialize();
    if (mhStatus == MH_OK) {
        UNIMOD_LOG_INFO("MinHook initialized successfully (MH_OK).");
        MH_Uninitialize();
        UNIMOD_LOG_INFO("MinHook uninitialized cleanly.");
    } else {
        UNIMOD_LOG_ERROR("MinHook initialization failed with status code: {}", static_cast<int>(mhStatus));
        return 1;
    }

    // 4. Test Log Levels
    UNIMOD_LOG_TRACE("Trace level log message verification.");
    UNIMOD_LOG_DEBUG("Debug level log message verification.");
    UNIMOD_LOG_INFO("Info level log message verification.");
    UNIMOD_LOG_WARN("Warning level log message verification.");
    UNIMOD_LOG_ERROR("Error level log message verification.");
    UNIMOD_LOG_CRITICAL("Critical level log message verification.");

    // 5. Shutdown and flush
    unimod::core::Logger::Shutdown();

    // 6. Verify log file existence and readability
    std::ifstream logReader(logFile);
    if (!logReader.is_open()) {
        std::cerr << "FAIL: Log file " << logFile << " was not created on disk!\n";
        return 1;
    }

    std::string line;
    bool foundInit = false;
    while (std::getline(logReader, line)) {
        if (line.find("Phase 1: Environment & Toolchain Verification") != std::string::npos) {
            foundInit = true;
            break;
        }
    }
    logReader.close();

    if (!foundInit) {
        std::cerr << "FAIL: Verification message not found in " << logFile << "!\n";
        return 1;
    }

    std::cout << "\n[SUCCESS] Phase 1 verification passed completely!\n";
    std::cout << "Log file created at: " << logFile << "\n";
    return 0;
}
