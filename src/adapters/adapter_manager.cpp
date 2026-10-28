#include <unimod/adapters/adapter_manager.hpp>
#include <unimod/core/logger.hpp>
#include <unimod/hooks/dx11_hook.hpp>

namespace unimod::adapters {

AdapterManager& AdapterManager::GetInstance() {
    static AdapterManager instance;
    return instance;
}

AdapterManager::~AdapterManager() {
    Shutdown();
}

void AdapterManager::RegisterAdapter(std::unique_ptr<IEngineAdapter> adapter) {
    if (!adapter) return;

    std::lock_guard<std::mutex> lock(m_Mutex);
    UNIMOD_LOG_INFO("Registering engine adapter: {} (v{})", adapter->GetName(), adapter->GetVersion());
    m_Adapters.push_back(std::move(adapter));
}

bool AdapterManager::DetectAndActivateAdapter() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (m_ActiveAdapter) {
        UNIMOD_LOG_WARN("An adapter is already active: {}", m_ActiveAdapter->GetName());
        return true;
    }

    UNIMOD_LOG_INFO("Scanning available adapters for current process environment (Total: {})...", m_Adapters.size());

    for (const auto& adapter : m_Adapters) {
        if (adapter && adapter->OnDetectEngine()) {
            UNIMOD_LOG_INFO("Engine detection positive: '{}'. Initiating OnAttach()...", adapter->GetName());

            if (adapter->OnAttach()) {
                m_ActiveAdapter = adapter.get();
                m_ActiveAdapter->OnInitRenderer();

                // Register adapter overlay with DirectX 11 ImGui rendering loop
                unimod::hooks::DX11Hook::GetInstance().RegisterRenderCallback([this]() {
                    this->RenderOverlay();
                });

                UNIMOD_LOG_INFO("Engine adapter '{}' successfully attached and activated.", m_ActiveAdapter->GetName());
                return true;
            } else {
                UNIMOD_LOG_ERROR("Failed to attach engine adapter: '{}'!", adapter->GetName());
            }
        }
    }

    UNIMOD_LOG_WARN("No matching engine adapter detected for this process.");
    return false;
}

void AdapterManager::Update(float deltaTime) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_ActiveAdapter) {
        m_ActiveAdapter->OnUpdate(deltaTime);
    }
}

void AdapterManager::RenderOverlay() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_ActiveAdapter) {
        m_ActiveAdapter->OnRenderOverlay();
    }
}

void AdapterManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_ActiveAdapter) {
        UNIMOD_LOG_INFO("Detaching active engine adapter: {}", m_ActiveAdapter->GetName());
        m_ActiveAdapter->OnDetach();
        m_ActiveAdapter = nullptr;
    }
    m_Adapters.clear();
}

size_t AdapterManager::GetRegisteredCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Adapters.size();
}

} // namespace unimod::adapters
