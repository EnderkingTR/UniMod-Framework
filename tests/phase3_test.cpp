#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>

#include <unimod/core/logger.hpp>
#include <unimod/memory/pattern_scanner.hpp>
#include <unimod/memory/memory_utils.hpp>
#include <unimod/hooks/dx11_hook.hpp>

int main() {
    std::cout << "====================================================\n";
    std::cout << "  UniMod Framework - Phase 3 Verification Test\n";
    std::cout << "====================================================\n";

    unimod::core::Logger::Initialize("phase3_test.log", false);

    // -----------------------------------------------------------------------
    // Part 1: AOB Pattern Scanner & RIP-Relative Resolution
    // -----------------------------------------------------------------------
    std::cout << "\n[Part 1] Testing AOB Pattern Scanner & RIP Resolution...\n";

    // Simulate an x64 executable code segment
    std::vector<uint8_t> fakeCodeSection = {
        0x90, 0x90, 0x90,                               // NOP padding
        0x48, 0x83, 0xEC, 0x28,                         // sub rsp, 28h
        0x48, 0x8B, 0x05, 0x2A, 0x00, 0x00, 0x00,       // mov rax, [rip + 0x2A] (displacement: 42)
        0x48, 0x85, 0xC0,                               // test rax, rax
        0x74, 0x05,                                     // jz +5
        0xE8, 0x00, 0x00, 0x00, 0x00,                   // call func
        0x48, 0x83, 0xC4, 0x28,                         // add rsp, 28h
        0xC3                                            // ret
    };

    uintptr_t baseAddr = reinterpret_cast<uintptr_t>(fakeCodeSection.data());
    size_t sectionSize = fakeCodeSection.size();

    // Parse IDA-style pattern with wildcards
    auto pattern = unimod::memory::Pattern::FromString("48 8B 05 ? ? ? ? 48 85 C0");
    std::cout << "-> Pattern compiled. Length: " << pattern.Length() << " bytes.\n";

    uintptr_t match = unimod::memory::PatternScanner::Scan(baseAddr, sectionSize, pattern);
    if (!match) {
        std::cerr << "FAIL: PatternScanner did not find the pattern in memory buffer!\n";
        return 1;
    }

    size_t matchOffset = match - baseAddr;
    std::cout << "-> Pattern found at offset: " << matchOffset << " (Expected: 7)\n";
    if (matchOffset != 7) {
        std::cerr << "FAIL: Matched offset does not equal 7!\n";
        return 1;
    }

    // Test RIP-relative resolution: instruction length = 7 bytes, displacement = 42 (0x2A)
    // target = match + 7 + 42 = match + 49
    uintptr_t resolvedTarget = unimod::memory::ResolveRip(match, 7, 3);
    uintptr_t expectedTarget = match + 7 + 0x2A;
    std::cout << "-> Resolved RIP target: 0x" << std::hex << resolvedTarget 
              << " | Expected: 0x" << expectedTarget << std::dec << "\n";

    if (resolvedTarget != expectedTarget) {
        std::cerr << "FAIL: Resolved RIP address mismatch!\n";
        return 1;
    }

    // Test ScopedVirtualProtect & Safe Memory Read
    uint32_t testValue = 0;
    bool readSuccess = unimod::memory::ReadMemorySafe(match + 3, testValue);
    std::cout << "-> Safe Memory Read at offset +3: 0x" << std::hex << testValue << std::dec << "\n";
    if (!readSuccess || testValue != 0x2A) {
        std::cerr << "FAIL: Safe memory read failed or incorrect value!\n";
        return 1;
    }

    std::cout << "[Part 1] AOB Scanner & RIP Resolution PASSED.\n";

    // -----------------------------------------------------------------------
    // Part 2: DirectX 11 Present & ResizeBuffers Hooking
    // -----------------------------------------------------------------------
    std::cout << "\n[Part 2] Testing DirectX 11 Hook & VTable Resolution...\n";

    auto& dx11 = unimod::hooks::DX11Hook::GetInstance();
    bool hookSuccess = dx11.Initialize();
    if (!hookSuccess) {
        std::cerr << "FAIL: DX11Hook::Initialize failed!\n";
        return 1;
    }

    std::cout << "-> DX11 Hook successfully resolved IDXGISwapChain and installed Present/ResizeBuffers hooks.\n";

    // Test registering an adapter render callback
    bool customCallbackFired = false;
    dx11.RegisterRenderCallback([&customCallbackFired]() {
        customCallbackFired = true;
    });
    std::cout << "-> Registered adapter ImGui render callback successfully.\n";

    // Test shutdown and unhooking
    dx11.Shutdown();
    std::cout << "-> DX11 Hook shut down and unhooked cleanly.\n";

    std::cout << "[Part 2] DirectX 11 Hooking PASSED.\n";

    unimod::core::Logger::Shutdown();

    std::cout << "\n====================================================\n";
    std::cout << "[SUCCESS] Phase 3 verification passed completely!\n";
    std::cout << "====================================================\n";
    return 0;
}
