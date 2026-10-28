#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <cstring>
#include <span>

namespace unimod::memory {

/**
 * @brief RAII Scoped page memory protection changer.
 * Restores original protection on destruction.
 */
class ScopedVirtualProtect {
public:
    ScopedVirtualProtect(void* address, size_t size, DWORD newProtect)
        : m_Address(address), m_Size(size) {
        if (!VirtualProtect(m_Address, m_Size, newProtect, &m_OldProtect)) {
            m_Success = false;
        }
    }

    ~ScopedVirtualProtect() {
        if (m_Success) {
            DWORD tempProtect = 0;
            VirtualProtect(m_Address, m_Size, m_OldProtect, &tempProtect);
        }
    }

    ScopedVirtualProtect(const ScopedVirtualProtect&) = delete;
    ScopedVirtualProtect& operator=(const ScopedVirtualProtect&) = delete;

    bool IsValid() const noexcept { return m_Success; }

private:
    void* m_Address{nullptr};
    size_t m_Size{0};
    DWORD m_OldProtect{0};
    bool m_Success{true};
};

/**
 * @brief Resolves a 64-bit RIP-relative target address.
 * 
 * In x64 instructions like:
 *   mov rax, [rip + 0x12345678]  (e.g., 48 8B 05 78 56 34 12)
 * The formula is: target = instructionAddress + instructionLength + displacement
 * 
 * @param instructionAddress Pointer to the start of the instruction
 * @param instructionLength Total length of the instruction in bytes
 * @param displacementOffset Offset from instructionAddress where the 32-bit displacement sits (default: 3)
 * @return Resolved absolute 64-bit address, or 0 if invalid
 */
inline uintptr_t ResolveRip(
    uintptr_t instructionAddress,
    size_t instructionLength,
    size_t displacementOffset = 3
) {
    if (!instructionAddress) return 0;
    const int32_t displacement = *reinterpret_cast<const int32_t*>(instructionAddress + displacementOffset);
    return instructionAddress + instructionLength + displacement;
}

/**
 * @brief Safe memory reading function with page validation and SEH support.
 */
template <typename T>
requires std::is_trivially_copyable_v<T>
bool ReadMemorySafe(uintptr_t address, T& outValue) {
    if (!address) return false;

#if defined(_MSC_VER)
    __try {
        outValue = *reinterpret_cast<const T*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    MEMORY_BASIC_INFORMATION mbi = {};
    if (VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi)) == 0) {
        return false;
    }

    if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
        return false;
    }

    std::memcpy(&outValue, reinterpret_cast<const void*>(address), sizeof(T));
    return true;
#endif
}

/**
 * @brief Safe memory writing function using ScopedVirtualProtect and page validation.
 */
template <typename T>
requires std::is_trivially_copyable_v<T>
bool WriteMemorySafe(uintptr_t address, const T& value) {
    if (!address) return false;

    ScopedVirtualProtect protect(reinterpret_cast<void*>(address), sizeof(T), PAGE_EXECUTE_READWRITE);
    if (!protect.IsValid()) return false;

#if defined(_MSC_VER)
    __try {
        *reinterpret_cast<T*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#else
    std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(T));
    return true;
#endif
}

} // namespace unimod::memory
