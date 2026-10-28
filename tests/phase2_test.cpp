#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <unknwn.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

// DirectInput8 GUID definition for testing
static const IID IID_IDirectInput8W_Test = {
    0xBF798031, 0x483A, 0x4DA2, { 0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00 }
};

using FnDirectInput8Create = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

int main() {
    std::cout << "====================================================\n";
    std::cout << "  UniMod Framework - Phase 2 Verification Test\n";
    std::cout << "====================================================\n";

    std::cout << "[Step 1] Loading dinput8.dll proxy...\n";
    HMODULE hProxy = LoadLibraryA("dinput8.dll");
    if (!hProxy) {
        std::cerr << "FAIL: Could not load dinput8.dll (Error code: " << GetLastError() << ")\n";
        return 1;
    }
    std::cout << "[Step 1] Loaded dinput8.dll successfully at: " << static_cast<void*>(hProxy) << "\n";

    std::cout << "[Step 2] Waiting for asynchronous bootstrap thread...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    std::cout << "[Step 3] Resolving DirectInput8Create export...\n";
    auto pDirectInput8Create = reinterpret_cast<FnDirectInput8Create>(
        GetProcAddress(hProxy, "DirectInput8Create")
    );
    if (!pDirectInput8Create) {
        std::cerr << "FAIL: GetProcAddress failed for DirectInput8Create! (Error: " << GetLastError() << ")\n";
        FreeLibrary(hProxy);
        return 1;
    }
    std::cout << "[Step 3] DirectInput8Create resolved at: " << reinterpret_cast<void*>(pDirectInput8Create) << "\n";

    std::cout << "[Step 4] Calling DirectInput8Create via proxy...\n";
    void* pDIInstance = nullptr;
    HRESULT hr = pDirectInput8Create(
        GetModuleHandle(nullptr),
        0x0800, // DIRECTINPUT_VERSION
        IID_IDirectInput8W_Test,
        &pDIInstance,
        nullptr
    );

    std::cout << "[Step 4] DirectInput8Create returned HRESULT: 0x" << std::hex << hr << std::dec << "\n";
    if (FAILED(hr) && hr != static_cast<HRESULT>(0x80070057L)) {
        std::cout << "[Step 4] Call reached system driver / DLL without crashing.\n";
    }

    if (pDIInstance) {
        auto pUnknown = static_cast<IUnknown*>(pDIInstance);
        pUnknown->Release();
        std::cout << "[Step 4] Released DirectInput8 interface instance.\n";
    }

    std::cout << "[Step 5] Unloading dinput8.dll to verify clean shutdown...\n";
    FreeLibrary(hProxy);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::cout << "[Step 6] Verifying unimod.log output on disk...\n";
    std::ifstream logFile("unimod.log");
    if (!logFile.is_open()) {
        std::cerr << "FAIL: unimod.log was not created!\n";
        return 1;
    }

    std::string line;
    bool foundBootstrap = false;
    bool foundChained = false;
    while (std::getline(logFile, line)) {
        if (line.find("Runtime Proxy Loaded") != std::string::npos) {
            foundBootstrap = true;
        }
        if (line.find("Proxy forwarding to system dinput8.dll active") != std::string::npos) {
            foundChained = true;
        }
    }
    logFile.close();

    if (!foundBootstrap || !foundChained) {
        std::cerr << "FAIL: Expected proxy log entries were not found in unimod.log!\n";
        return 1;
    }

    std::cout << "\n[SUCCESS] Phase 2 verification passed completely!\n";
    return 0;
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
