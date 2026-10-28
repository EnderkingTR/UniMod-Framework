#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <unknwn.h>

namespace unimod::proxy::dinput8 {

bool LoadOriginalDinput8();
void UnloadOriginalDinput8();
bool IsOriginalDinput8Loaded();

} // namespace unimod::proxy::dinput8

// Forwarded C-ABI proxy function declarations
extern "C" {
    HRESULT WINAPI DirectInput8Create_Proxy(
        HINSTANCE hinst,
        DWORD dwVersion,
        REFIID riidltf,
        LPVOID* ppvOut,
        LPUNKNOWN punkOuter
    );

    HRESULT WINAPI DllCanUnloadNow_Proxy();

    HRESULT WINAPI DllGetClassObject_Proxy(
        REFCLSID rclsid,
        REFIID riid,
        LPVOID* ppv
    );

    HRESULT WINAPI DllRegisterServer_Proxy();

    HRESULT WINAPI DllUnregisterServer_Proxy();
}
