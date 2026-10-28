#pragma once

namespace unimod::adapters {

/**
 * @brief Abstract interface that all game engine adapters must implement.
 * Provides a standardized lifecycle for engine-specific reverse engineering,
 * object traversal, and GUI diagnostics.
 */
class IEngineAdapter {
public:
    virtual ~IEngineAdapter() = default;

    /**
     * @brief Human-readable name of the adapter (e.g. "Unreal Engine 4/5", "Unity IL2CPP", "Mock Adapter").
     */
    virtual const char* GetName() const noexcept = 0;

    /**
     * @brief Version string of the adapter.
     */
    virtual const char* GetVersion() const noexcept = 0;

    /**
     * @brief Probes the running process to detect whether this engine is present.
     * @return true if the engine matches the current process, false otherwise.
     */
    virtual bool OnDetectEngine() = 0;

    /**
     * @brief Called when the adapter is selected and activated.
     * Performs memory scans, resolves engine pointers, and installs engine-level hooks.
     * @return true if attachment succeeded, false on fatal error.
     */
    virtual bool OnAttach() = 0;

    /**
     * @brief Called once the DirectX/ImGui rendering subsystem is ready.
     * Use this to initialize adapter-specific textures, fonts, or UI states.
     */
    virtual void OnInitRenderer() = 0;

    /**
     * @brief Synchronous update tick called every frame.
     * @param deltaTime Time elapsed since last frame in seconds.
     */
    virtual void OnUpdate(float deltaTime) = 0;

    /**
     * @brief Called inside the Dear ImGui render loop.
     * Renders adapter-specific diagnostic panels, cheat menus, and object inspectors.
     */
    virtual void OnRenderOverlay() = 0;

    /**
     * @brief Called during framework shutdown or adapter unloading.
     * Must safely restore all hooks and release allocated memory.
     */
    virtual void OnDetach() = 0;
};

// C-ABI Export signatures for dynamic plugin modules (*.dll)
using CreateEngineAdapterFn = IEngineAdapter* (*)();
using DestroyEngineAdapterFn = void (*)(IEngineAdapter*);

} // namespace unimod::adapters
