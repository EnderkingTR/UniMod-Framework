#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <atomic>
#include <string>
#include <thread>
#include <functional>

namespace unimod::core {

enum class RuntimeState {
    Uninitialized,
    Initializing,
    Running,
    ShuttingDown,
    Terminated
};

class CoreRuntime {
public:
    static CoreRuntime& GetInstance();

    // Deleted copy and move constructors for singleton
    CoreRuntime(const CoreRuntime&) = delete;
    CoreRuntime& operator=(const CoreRuntime&) = delete;
    CoreRuntime(CoreRuntime&&) = delete;
    CoreRuntime& operator=(CoreRuntime&&) = delete;

    bool Initialize(HMODULE hModule);
    void Shutdown();

    bool IsRunning() const noexcept { return m_Running.load(std::memory_order_acquire); }
    RuntimeState GetState() const noexcept { return m_State.load(std::memory_order_acquire); }
    HMODULE GetModuleHandle() const noexcept { return m_ModuleHandle; }

private:
    CoreRuntime() = default;
    ~CoreRuntime();

    void WorkerLoop();

    std::atomic<bool> m_Running{false};
    std::atomic<RuntimeState> m_State{RuntimeState::Uninitialized};
    HMODULE m_ModuleHandle{nullptr};
    std::thread m_WorkerThread;
};

} // namespace unimod::core
