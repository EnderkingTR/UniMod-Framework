#include <unimod/core/core_runtime.hpp>
#include <unimod/core/logger.hpp>

#include <chrono>

namespace unimod::core {

CoreRuntime& CoreRuntime::GetInstance() {
    static CoreRuntime instance;
    return instance;
}

CoreRuntime::~CoreRuntime() {
    Shutdown();
}

bool CoreRuntime::Initialize(HMODULE hModule) {
    RuntimeState expected = RuntimeState::Uninitialized;
    if (!m_State.compare_exchange_strong(expected, RuntimeState::Initializing, std::memory_order_acq_rel)) {
        UNIMOD_LOG_WARN("CoreRuntime::Initialize called when not uninitialized. Current state: {}", static_cast<int>(expected));
        return false;
    }

    m_ModuleHandle = hModule;
    m_Running.store(true, std::memory_order_release);

    UNIMOD_LOG_INFO("CoreRuntime initializing with module handle: {:p}", static_cast<void*>(m_ModuleHandle));

    // Spawn internal worker loop thread
    m_WorkerThread = std::thread(&CoreRuntime::WorkerLoop, this);

    m_State.store(RuntimeState::Running, std::memory_order_release);
    UNIMOD_LOG_INFO("CoreRuntime successfully entered RUNNING state.");
    return true;
}

void CoreRuntime::WorkerLoop() {
    UNIMOD_LOG_INFO("CoreRuntime worker thread active (ID: {}).", GetCurrentThreadId());

    while (m_Running.load(std::memory_order_relaxed)) {
        // Yield/Sleep to avoid high CPU usage while waiting for events
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    UNIMOD_LOG_INFO("CoreRuntime worker thread exiting.");
}

void CoreRuntime::Shutdown() {
    RuntimeState expected = RuntimeState::Running;
    if (!m_State.compare_exchange_strong(expected, RuntimeState::ShuttingDown, std::memory_order_acq_rel)) {
        // If already shutting down or terminated, return immediately
        return;
    }

    UNIMOD_LOG_INFO("CoreRuntime shutdown sequence initiated.");
    m_Running.store(false, std::memory_order_release);

    // CRITICAL: Inside DllMain / DLL_PROCESS_DETACH, NEVER block on .join() because
    // the OS Loader Lock is held on the detaching thread, causing an instant deadlock!
    if (m_WorkerThread.joinable()) {
        m_WorkerThread.detach();
    }

    m_State.store(RuntimeState::Terminated, std::memory_order_release);
    UNIMOD_LOG_INFO("CoreRuntime terminated cleanly.");
}

} // namespace unimod::core
