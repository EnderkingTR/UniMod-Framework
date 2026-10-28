#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <functional>
#include <vector>
#include <atomic>
#include <mutex>

namespace unimod::hooks {

class DX11Hook {
public:
    using RenderCallback = std::function<void()>;

    static DX11Hook& GetInstance();

    DX11Hook(const DX11Hook&) = delete;
    DX11Hook& operator=(const DX11Hook&) = delete;

    bool Initialize();
    void Shutdown();

    bool IsInitialized() const noexcept { return m_Initialized.load(std::memory_order_acquire); }
    bool IsMenuOpen() const noexcept { return m_MenuOpen.load(std::memory_order_relaxed); }
    void SetMenuOpen(bool open) noexcept { m_MenuOpen.store(open, std::memory_order_relaxed); }
    HWND GetWindowHandle() const noexcept { return m_GameWindow; }

    void RegisterRenderCallback(RenderCallback callback);

private:
    DX11Hook() = default;
    ~DX11Hook();

    bool CreateDummyDeviceAndGetVTable(void** outVTable, size_t numMethods);
    void InitImGui(IDXGISwapChain* pSwapChain);
    void RenderUI();

    static HRESULT WINAPI HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
    static HRESULT WINAPI HookedResizeBuffers(
        IDXGISwapChain* pSwapChain,
        UINT BufferCount,
        UINT Width,
        UINT Height,
        DXGI_FORMAT NewFormat,
        UINT SwapChainFlags
    );
    static LRESULT WINAPI HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    std::atomic<bool> m_Initialized{false};
    std::atomic<bool> m_ImGuiInitialized{false};
    std::atomic<bool> m_MenuOpen{true};

    HWND m_GameWindow{nullptr};
    WNDPROC m_OriginalWndProc{nullptr};

    ID3D11Device* m_Device{nullptr};
    ID3D11DeviceContext* m_DeviceContext{nullptr};
    ID3D11RenderTargetView* m_RenderTargetView{nullptr};

    void* m_OriginalPresent{nullptr};
    void* m_OriginalResizeBuffers{nullptr};

    std::vector<RenderCallback> m_RenderCallbacks;
    std::mutex m_CallbackMutex;
};

} // namespace unimod::hooks
