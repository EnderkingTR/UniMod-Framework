#include <unimod/hooks/dx11_hook.hpp>
#include <unimod/core/logger.hpp>
#include <minhook.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace unimod::hooks {

DX11Hook& DX11Hook::GetInstance() {
    static DX11Hook instance;
    return instance;
}

DX11Hook::~DX11Hook() {
    Shutdown();
}

bool DX11Hook::CreateDummyDeviceAndGetVTable(void** outVTable, size_t numMethods) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"UniMod_DX11_Dummy_Class";

    RegisterClassExW(&wc);
    HWND hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"UniMod Dummy Window",
        WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    if (!hWnd) {
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL featureLevel;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* pSwapChain = nullptr;
    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        featureLevels,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &pSwapChain,
        &pDevice,
        &featureLevel,
        &pContext
    );

    if (FAILED(hr)) {
        // Fallback to WARP software renderer (e.g., in CI or headless VM)
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            featureLevels,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &pSwapChain,
            &pDevice,
            &featureLevel,
            &pContext
        );
    }

    if (FAILED(hr) || !pSwapChain) {
        DestroyWindow(hWnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }

    void** vTable = *reinterpret_cast<void***>(pSwapChain);
    for (size_t i = 0; i < numMethods; ++i) {
        outVTable[i] = vTable[i];
    }

    pSwapChain->Release();
    pContext->Release();
    pDevice->Release();
    DestroyWindow(hWnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return true;
}

bool DX11Hook::Initialize() {
    if (m_Initialized.load(std::memory_order_acquire)) {
        return true;
    }

    UNIMOD_LOG_INFO("Initializing DirectX 11 Hook subsystem...");

    void* swapChainVTable[18] = { nullptr };
    if (!CreateDummyDeviceAndGetVTable(swapChainVTable, 18)) {
        UNIMOD_LOG_CRITICAL("Failed to obtain IDXGISwapChain VTable via dummy device!");
        return false;
    }

    UNIMOD_LOG_INFO("Discovered IDXGISwapChain::Present at: {:p}", swapChainVTable[8]);
    UNIMOD_LOG_INFO("Discovered IDXGISwapChain::ResizeBuffers at: {:p}", swapChainVTable[13]);

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
        UNIMOD_LOG_ERROR("MinHook initialization failed in DX11Hook: {}", static_cast<int>(status));
        return false;
    }

    // Hook Present (Index 8)
    status = MH_CreateHook(
        swapChainVTable[8],
        reinterpret_cast<LPVOID>(&DX11Hook::HookedPresent),
        reinterpret_cast<LPVOID*>(&m_OriginalPresent)
    );
    if (status != MH_OK) {
        UNIMOD_LOG_ERROR("Failed to hook IDXGISwapChain::Present! Status: {}", static_cast<int>(status));
        return false;
    }

    // Hook ResizeBuffers (Index 13)
    status = MH_CreateHook(
        swapChainVTable[13],
        reinterpret_cast<LPVOID>(&DX11Hook::HookedResizeBuffers),
        reinterpret_cast<LPVOID*>(&m_OriginalResizeBuffers)
    );
    if (status != MH_OK) {
        UNIMOD_LOG_ERROR("Failed to hook IDXGISwapChain::ResizeBuffers! Status: {}", static_cast<int>(status));
        MH_RemoveHook(swapChainVTable[8]);
        return false;
    }

    status = MH_EnableHook(MH_ALL_HOOKS);
    if (status != MH_OK) {
        UNIMOD_LOG_ERROR("Failed to enable MinHook hooks in DX11Hook! Status: {}", static_cast<int>(status));
        return false;
    }

    m_Initialized.store(true, std::memory_order_release);
    UNIMOD_LOG_INFO("DirectX 11 Present and ResizeBuffers hooks enabled successfully.");
    return true;
}

void DX11Hook::InitImGui(IDXGISwapChain* pSwapChain) {
    if (m_ImGuiInitialized.load(std::memory_order_acquire)) {
        return;
    }

    if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&m_Device)))) {
        UNIMOD_LOG_ERROR("Failed to get ID3D11Device from SwapChain!");
        return;
    }

    m_Device->GetImmediateContext(&m_DeviceContext);

    DXGI_SWAP_CHAIN_DESC desc;
    pSwapChain->GetDesc(&desc);
    m_GameWindow = desc.OutputWindow;

    ID3D11Texture2D* pBackBuffer = nullptr;
    if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) {
        m_Device->CreateRenderTargetView(pBackBuffer, nullptr, &m_RenderTargetView);
        pBackBuffer->Release();
    }

    // Subclass WndProc
    m_OriginalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(m_GameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&DX11Hook::HookedWndProc))
    );

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Apply Premium Modern Dark Theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.12f, 0.94f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.35f, 0.80f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.22f, 0.30f, 0.45f, 0.80f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.38f, 0.58f, 1.00f);

    ImGui_ImplWin32_Init(m_GameWindow);
    ImGui_ImplDX11_Init(m_Device, m_DeviceContext);

    m_ImGuiInitialized.store(true, std::memory_order_release);
    UNIMOD_LOG_INFO("Dear ImGui successfully initialized on game window ({:p}).", static_cast<void*>(m_GameWindow));
}

void DX11Hook::RenderUI() {
    if (!m_MenuOpen.load(std::memory_order_relaxed)) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(550, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("UniMod Framework - Universal Modding Console", nullptr, ImGuiWindowFlags_None)) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "UniMod Core Runtime v0.1.0");
        ImGui::Separator();

        if (ImGui::BeginTabBar("UniModTabs")) {
            if (ImGui::BeginTabItem("Dashboard")) {
                ImGui::Spacing();
                ImGui::Text("DirectX 11 Hook: Active");
                ImGui::Text("Application FPS: %.1f (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
                ImGui::Text("Menu Toggle Key: [INSERT]");
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text("Active Adapters: Generic DX11 Mock");
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Memory Scanner")) {
                ImGui::Spacing();
                static char patternBuffer[128] = "48 8B 05 ? ? ? ? 48 85 C0";
                ImGui::InputText("AOB Signature", patternBuffer, sizeof(patternBuffer));
                if (ImGui::Button("Scan Current Process")) {
                    UNIMOD_LOG_INFO("Initiating memory scan from ImGui UI...");
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Diagnostics")) {
                ImGui::Spacing();
                ImGui::Text("Window Handle: %p", static_cast<void*>(m_GameWindow));
                ImGui::Text("Device Pointer: %p", static_cast<void*>(m_Device));
                ImGui::Text("Device Context: %p", static_cast<void*>(m_DeviceContext));
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // Fire registered custom callbacks from engine adapters
        {
            std::lock_guard<std::mutex> lock(m_CallbackMutex);
            for (const auto& cb : m_RenderCallbacks) {
                if (cb) cb();
            }
        }
    }
    ImGui::End();
}

void DX11Hook::RegisterRenderCallback(RenderCallback callback) {
    std::lock_guard<std::mutex> lock(m_CallbackMutex);
    m_RenderCallbacks.push_back(std::move(callback));
}

HRESULT WINAPI DX11Hook::HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    auto& instance = GetInstance();

    if (!instance.m_ImGuiInitialized.load(std::memory_order_acquire)) {
        instance.InitImGui(pSwapChain);
    }

    if (instance.m_ImGuiInitialized.load(std::memory_order_acquire)) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        instance.RenderUI();

        ImGui::Render();
        instance.m_DeviceContext->OMSetRenderTargets(1, &instance.m_RenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    using FnPresent = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
    return reinterpret_cast<FnPresent>(instance.m_OriginalPresent)(pSwapChain, SyncInterval, Flags);
}

HRESULT WINAPI DX11Hook::HookedResizeBuffers(
    IDXGISwapChain* pSwapChain,
    UINT BufferCount,
    UINT Width,
    UINT Height,
    DXGI_FORMAT NewFormat,
    UINT SwapChainFlags
) {
    auto& instance = GetInstance();

    if (instance.m_RenderTargetView) {
        instance.m_RenderTargetView->Release();
        instance.m_RenderTargetView = nullptr;
    }

    using FnResizeBuffers = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
    HRESULT hr = reinterpret_cast<FnResizeBuffers>(instance.m_OriginalResizeBuffers)(
        pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags
    );

    if (SUCCEEDED(hr) && instance.m_Device) {
        ID3D11Texture2D* pBackBuffer = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) {
            instance.m_Device->CreateRenderTargetView(pBackBuffer, nullptr, &instance.m_RenderTargetView);
            pBackBuffer->Release();
        }
    }

    return hr;
}

LRESULT WINAPI DX11Hook::HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    auto& instance = GetInstance();

    if (uMsg == WM_KEYDOWN && wParam == VK_INSERT) {
        instance.SetMenuOpen(!instance.IsMenuOpen());
    }

    if (instance.m_ImGuiInitialized.load(std::memory_order_acquire)) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

        ImGuiIO& io = ImGui::GetIO();
        if (instance.IsMenuOpen() && (io.WantCaptureMouse || io.WantCaptureKeyboard)) {
            // Block input from propagating to target game window while interacting with ImGui
            return TRUE;
        }
    }

    return CallWindowProcW(instance.m_OriginalWndProc, hWnd, uMsg, wParam, lParam);
}

void DX11Hook::Shutdown() {
    if (!m_Initialized.load(std::memory_order_acquire)) {
        return;
    }

    UNIMOD_LOG_INFO("Shutting down DirectX 11 Hook subsystem...");

    // Disable and remove hooks
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    // Restore window procedure
    if (m_GameWindow && m_OriginalWndProc) {
        SetWindowLongPtrW(m_GameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_OriginalWndProc));
        m_OriginalWndProc = nullptr;
    }

    // Clean up ImGui
    if (m_ImGuiInitialized.load(std::memory_order_acquire)) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_ImGuiInitialized.store(false, std::memory_order_release);
    }

    // Release DX11 resources
    if (m_RenderTargetView) {
        m_RenderTargetView->Release();
        m_RenderTargetView = nullptr;
    }
    if (m_DeviceContext) {
        m_DeviceContext->Release();
        m_DeviceContext = nullptr;
    }
    if (m_Device) {
        m_Device->Release();
        m_Device = nullptr;
    }

    m_Initialized.store(false, std::memory_order_release);
    UNIMOD_LOG_INFO("DirectX 11 Hook subsystem shut down cleanly.");
}

} // namespace unimod::hooks
