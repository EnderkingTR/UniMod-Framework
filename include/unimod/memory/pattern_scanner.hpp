#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <span>

namespace unimod::memory {

/**
 * @brief Represents a compiled byte pattern with optional wildcards.
 */
struct Pattern {
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> mask; // 1: must match byte, 0: wildcard (any byte matches)

    bool IsEmpty() const noexcept { return bytes.empty(); }
    size_t Length() const noexcept { return bytes.size(); }

    /**
     * @brief Parses an IDA-style signature string into a Pattern.
     * Example formats:
     *   "48 8B 05 ? ? ? ? 48 85 C0"
     *   "48 8b 05 ?? ?? ?? ?? 48 85 c0"
     */
    static Pattern FromString(std::string_view patternStr);

    /**
     * @brief Constructs a pattern from explicit byte vector and mask string ('x' = match, '?' = wildcard).
     */
    static Pattern FromBytes(std::span<const uint8_t> bytes, std::string_view maskStr);
};

/**
 * @brief High-performance AOB (Array of Bytes) pattern scanner for x64 processes.
 */
class PatternScanner {
public:
    /**
     * @brief Scans a raw memory region for the first occurrence of pattern.
     */
    static uintptr_t Scan(uintptr_t baseAddress, size_t size, const Pattern& pattern);

    /**
     * @brief Scans an entire loaded PE module (by name, or nullptr/empty for main executable).
     */
    static uintptr_t ScanModule(std::string_view moduleName, const Pattern& pattern);

    /**
     * @brief Scans a specific PE section within a module (e.g. ".text", ".rdata").
     */
    static uintptr_t ScanSection(
        std::string_view moduleName,
        std::string_view sectionName,
        const Pattern& pattern
    );

    /**
     * @brief Finds all occurrences of the pattern in the given range.
     */
    static std::vector<uintptr_t> FindAll(
        uintptr_t baseAddress,
        size_t size,
        const Pattern& pattern
    );
};

} // namespace unimod::memory
