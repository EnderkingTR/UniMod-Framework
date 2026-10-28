#include "dinput8_proxy.hpp"
#include <unimod/core/core_runtime.hpp>
#include <unimod/core/logger.hpp>

namespace {

DWORD WINAPI UniModBootstrapThread(LPVOID lpParam) {
    HMODULE hModule = static_cast<HMODULE>(lpParam);

    // Initialize logger outside OS Loader Lock
    unimod::core::Logger::Initialize("unimod.log", true);
    UNIMOD_LOG_INFO("=================================================");
    UNIMOD_LOG_INFO("       UniMod Framework - Runtime Proxy Loaded   ");
    UNIMOD_LOG_INFO("=================================================");
    UNIMOD_LOG_INFO("Injected Proxy Module Handle: {:p}", static_cast<void*>(hModule));

    // Initialize proxy forwarding table
    if (!unimod::proxy::dinput8::LoadOriginalDinput8()) {
        UNIMOD_LOG_CRITICAL("CRITICAL: Failed to chain system dinput8.dll! Target game may fail to start input.");
        return FALSE;
    }
    UNIMOD_LOG_INFO("Proxy forwarding to system dinput8.dll active.");

    // Initialize Core Runtime
    if (!unimod::core::CoreRuntime::GetInstance().Initialize(hModule)) {
        UNIMOD_LOG_ERROR("Failed to initialize CoreRuntime.");
        return FALSE;
    }

    UNIMOD_LOG_INFO("UniMod Bootstrap completed successfully.");
    return TRUE;
}

} // anonymous namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        // Spawn async bootstrap thread to escape OS Loader Lock immediately
        CreateThread(
            nullptr,
            0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(UniModBootstrapThread),
            hModule,
            0,
            nullptr
        );
        break;

    case DLL_PROCESS_DETACH:
        if (!lpReserved) {
            // Dynamic unload (e.g. FreeLibrary)
            unimod::core::CoreRuntime::GetInstance().Shutdown();
            unimod::proxy::dinput8::UnloadOriginalDinput8();
            unimod::core::Logger::Shutdown();
        }
        break;
    }

    return TRUE;
}
