#include "dinput8_proxy.hpp"
#include <unimod/core/logger.hpp>

#include <mutex>
#include <string>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

namespace {

using FnDirectInput8Create = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
using FnDllCanUnloadNow = HRESULT(WINAPI*)();
using FnDllGetClassObject = HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*);
using FnDllRegisterServer = HRESULT(WINAPI*)();
using FnDllUnregisterServer = HRESULT(WINAPI*)();

HMODULE g_OriginalDinput8 = nullptr;
std::mutex g_ProxyMutex;

FnDirectInput8Create   g_OriginalDirectInput8Create = nullptr;
FnDllCanUnloadNow      g_OriginalDllCanUnloadNow = nullptr;
FnDllGetClassObject   g_OriginalDllGetClassObject = nullptr;
FnDllRegisterServer    g_OriginalDllRegisterServer = nullptr;
FnDllUnregisterServer  g_OriginalDllUnregisterServer = nullptr;

} // anonymous namespace

namespace unimod::proxy::dinput8 {

bool LoadOriginalDinput8() {
    std::lock_guard<std::mutex> lock(g_ProxyMutex);
    if (g_OriginalDinput8) {
        return true;
    }

    wchar_t systemDir[MAX_PATH] = { 0 };
    UINT len = GetSystemDirectoryW(systemDir, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        UNIMOD_LOG_CRITICAL("Failed to retrieve Windows System directory path.");
        return false;
    }

    std::wstring originalDllPath = std::wstring(systemDir) + L"\\dinput8.dll";
    g_OriginalDinput8 = LoadLibraryW(originalDllPath.c_str());

    if (!g_OriginalDinput8) {
        UNIMOD_LOG_CRITICAL("Failed to load original dinput8.dll from: {}", 
            std::string(originalDllPath.begin(), originalDllPath.end()));
        return false;
    }

    g_OriginalDirectInput8Create = reinterpret_cast<FnDirectInput8Create>(
        GetProcAddress(g_OriginalDinput8, "DirectInput8Create"));
    g_OriginalDllCanUnloadNow = reinterpret_cast<FnDllCanUnloadNow>(
        GetProcAddress(g_OriginalDinput8, "DllCanUnloadNow"));
    g_OriginalDllGetClassObject = reinterpret_cast<FnDllGetClassObject>(
        GetProcAddress(g_OriginalDinput8, "DllGetClassObject"));
    g_OriginalDllRegisterServer = reinterpret_cast<FnDllRegisterServer>(
        GetProcAddress(g_OriginalDinput8, "DllRegisterServer"));
    g_OriginalDllUnregisterServer = reinterpret_cast<FnDllUnregisterServer>(
        GetProcAddress(g_OriginalDinput8, "DllUnregisterServer"));

    if (!g_OriginalDirectInput8Create) {
        UNIMOD_LOG_ERROR("Could not locate DirectInput8Create in original system dinput8.dll!");
    } else {
        UNIMOD_LOG_INFO("Successfully bound original DirectInput8Create ({:p})", 
            reinterpret_cast<void*>(g_OriginalDirectInput8Create));
    }

    return true;
}

void UnloadOriginalDinput8() {
    std::lock_guard<std::mutex> lock(g_ProxyMutex);
    if (g_OriginalDinput8) {
        FreeLibrary(g_OriginalDinput8);
        g_OriginalDinput8 = nullptr;
        g_OriginalDirectInput8Create = nullptr;
        g_OriginalDllCanUnloadNow = nullptr;
        g_OriginalDllGetClassObject = nullptr;
        g_OriginalDllRegisterServer = nullptr;
        g_OriginalDllUnregisterServer = nullptr;
        UNIMOD_LOG_INFO("Original dinput8.dll unloaded from process.");
    }
}

bool IsOriginalDinput8Loaded() {
    std::lock_guard<std::mutex> lock(g_ProxyMutex);
    return g_OriginalDinput8 != nullptr;
}

} // namespace unimod::proxy::dinput8

// Forwarded C-ABI proxy function implementations
extern "C" {

HRESULT WINAPI DirectInput8Create_Proxy(
    HINSTANCE hinst,
    DWORD dwVersion,
    REFIID riidltf,
    LPVOID* ppvOut,
    LPUNKNOWN punkOuter
) {
    if (!unimod::proxy::dinput8::IsOriginalDinput8Loaded()) {
        if (!unimod::proxy::dinput8::LoadOriginalDinput8()) {
            return E_FAIL;
        }
    }

    if (g_OriginalDirectInput8Create) {
        return g_OriginalDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
    }
    return E_FAIL;
}

HRESULT WINAPI DllCanUnloadNow_Proxy() {
    if (!unimod::proxy::dinput8::IsOriginalDinput8Loaded()) {
        unimod::proxy::dinput8::LoadOriginalDinput8();
    }
    if (g_OriginalDllCanUnloadNow) {
        return g_OriginalDllCanUnloadNow();
    }
    return S_OK;
}

HRESULT WINAPI DllGetClassObject_Proxy(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    if (!unimod::proxy::dinput8::IsOriginalDinput8Loaded()) {
        unimod::proxy::dinput8::LoadOriginalDinput8();
    }
    if (g_OriginalDllGetClassObject) {
        return g_OriginalDllGetClassObject(rclsid, riid, ppv);
    }
    return E_FAIL;
}

HRESULT WINAPI DllRegisterServer_Proxy() {
    if (!unimod::proxy::dinput8::IsOriginalDinput8Loaded()) {
        unimod::proxy::dinput8::LoadOriginalDinput8();
    }
    if (g_OriginalDllRegisterServer) {
        return g_OriginalDllRegisterServer();
    }
    return E_FAIL;
}

HRESULT WINAPI DllUnregisterServer_Proxy() {
    if (!unimod::proxy::dinput8::IsOriginalDinput8Loaded()) {
        unimod::proxy::dinput8::LoadOriginalDinput8();
    }
    if (g_OriginalDllUnregisterServer) {
        return g_OriginalDllUnregisterServer();
    }
    return E_FAIL;
}

} // extern "C"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
