#pragma once

#include <unimod/adapters/i_engine_adapter.hpp>

#include <vector>
#include <memory>
#include <mutex>
#include <string_view>

namespace unimod::adapters {

class AdapterManager {
public:
    static AdapterManager& GetInstance();

    AdapterManager(const AdapterManager&) = delete;
    AdapterManager& operator=(const AdapterManager&) = delete;

    /**
     * @brief Registers an engine adapter into the registry.
     */
    void RegisterAdapter(std::unique_ptr<IEngineAdapter> adapter);

    /**
     * @brief Iterates over registered adapters and invokes OnDetectEngine().
     * The first adapter returning true will be attached via OnAttach() and set active.
     * @return true if an adapter was detected and attached successfully, false otherwise.
     */
    bool DetectAndActivateAdapter();

    /**
     * @brief Returns the currently active adapter, or nullptr if none active.
     */
    IEngineAdapter* GetActiveAdapter() const noexcept { return m_ActiveAdapter; }

    /**
     * @brief Dispatches the frame update tick to the active adapter.
     */
    void Update(float deltaTime);

    /**
     * @brief Dispatches the ImGui overlay render call to the active adapter.
     */
    void RenderOverlay();

    /**
     * @brief Safely detaches the active adapter and releases all registered adapters.
     */
    void Shutdown();

    size_t GetRegisteredCount() const noexcept;

private:
    AdapterManager() = default;
    ~AdapterManager();

    std::vector<std::unique_ptr<IEngineAdapter>> m_Adapters;
    IEngineAdapter* m_ActiveAdapter{nullptr};
    mutable std::mutex m_Mutex;
};

} // namespace unimod::adapters
