#include <unimod/memory/pattern_scanner.hpp>
#include <unimod/core/logger.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <sstream>
#include <charconv>
#include <cstring>

namespace unimod::memory {

Pattern Pattern::FromString(std::string_view patternStr) {
    Pattern pattern;
    std::string s(patternStr);
    std::istringstream stream(s);
    std::string token;

    while (stream >> token) {
        if (token == "?" || token == "??") {
            pattern.bytes.push_back(0);
            pattern.mask.push_back(0);
        } else {
            uint8_t byteVal = 0;
            auto result = std::from_chars(token.data(), token.data() + token.size(), byteVal, 16);
            if (result.ec == std::errc()) {
                pattern.bytes.push_back(byteVal);
                pattern.mask.push_back(1);
            }
        }
    }

    return pattern;
}

Pattern Pattern::FromBytes(std::span<const uint8_t> bytes, std::string_view maskStr) {
    Pattern pattern;
    pattern.bytes.assign(bytes.begin(), bytes.end());
    pattern.mask.resize(bytes.size(), 1);

    for (size_t i = 0; i < maskStr.size() && i < pattern.mask.size(); ++i) {
        pattern.mask[i] = (maskStr[i] != '?') ? 1 : 0;
    }

    return pattern;
}

uintptr_t PatternScanner::Scan(uintptr_t baseAddress, size_t size, const Pattern& pattern) {
    if (!baseAddress || size < pattern.Length() || pattern.IsEmpty()) {
        return 0;
    }

    const uint8_t* scanStart = reinterpret_cast<const uint8_t*>(baseAddress);
    const size_t scanLen = size - pattern.Length();
    const size_t patternLen = pattern.Length();
    const uint8_t* pBytes = pattern.bytes.data();
    const uint8_t* pMask = pattern.mask.data();

    // Fast scanning loop
    for (size_t i = 0; i <= scanLen; ++i) {
        bool match = true;
        for (size_t j = 0; j < patternLen; ++j) {
            if (pMask[j] && scanStart[i + j] != pBytes[j]) {
                match = false;
                break;
            }
        }

        if (match) {
            return reinterpret_cast<uintptr_t>(scanStart + i);
        }
    }

    return 0;
}

std::vector<uintptr_t> PatternScanner::FindAll(
    uintptr_t baseAddress,
    size_t size,
    const Pattern& pattern
) {
    std::vector<uintptr_t> results;
    if (!baseAddress || size < pattern.Length() || pattern.IsEmpty()) {
        return results;
    }

    const uint8_t* scanStart = reinterpret_cast<const uint8_t*>(baseAddress);
    const size_t scanLen = size - pattern.Length();
    const size_t patternLen = pattern.Length();
    const uint8_t* pBytes = pattern.bytes.data();
    const uint8_t* pMask = pattern.mask.data();

    for (size_t i = 0; i <= scanLen; ++i) {
        bool match = true;
        for (size_t j = 0; j < patternLen; ++j) {
            if (pMask[j] && scanStart[i + j] != pBytes[j]) {
                match = false;
                break;
            }
        }

        if (match) {
            results.push_back(reinterpret_cast<uintptr_t>(scanStart + i));
        }
    }

    return results;
}

uintptr_t PatternScanner::ScanModule(std::string_view moduleName, const Pattern& pattern) {
    HMODULE hModule = nullptr;
    if (moduleName.empty()) {
        hModule = GetModuleHandleA(nullptr);
    } else {
        std::string modStr(moduleName);
        hModule = GetModuleHandleA(modStr.c_str());
    }

    if (!hModule) {
        UNIMOD_LOG_WARN("PatternScanner::ScanModule failed to obtain handle for module: {}", moduleName);
        return 0;
    }

    const auto dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(hModule);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }

    const auto ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(
        reinterpret_cast<const uint8_t*>(hModule) + dosHeader->e_lfanew
    );
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return 0;
    }

    const size_t imageSize = ntHeaders->OptionalHeader.SizeOfImage;
    return Scan(reinterpret_cast<uintptr_t>(hModule), imageSize, pattern);
}

uintptr_t PatternScanner::ScanSection(
    std::string_view moduleName,
    std::string_view sectionName,
    const Pattern& pattern
) {
    HMODULE hModule = nullptr;
    if (moduleName.empty()) {
        hModule = GetModuleHandleA(nullptr);
    } else {
        std::string modStr(moduleName);
        hModule = GetModuleHandleA(modStr.c_str());
    }

    if (!hModule) return 0;

    const auto dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(hModule);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return 0;

    const auto ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(
        reinterpret_cast<const uint8_t*>(hModule) + dosHeader->e_lfanew
    );
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return 0;

    const auto sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i) {
        const auto& sec = sectionHeader[i];
        char currentSecName[9] = { 0 };
        std::memcpy(currentSecName, sec.Name, 8);

        if (sectionName == currentSecName) {
            uintptr_t sectionStart = reinterpret_cast<uintptr_t>(hModule) + sec.VirtualAddress;
            size_t sectionSize = sec.Misc.VirtualSize;
            return Scan(sectionStart, sectionSize, pattern);
        }
    }

    return 0;
}

} // namespace unimod::memory
