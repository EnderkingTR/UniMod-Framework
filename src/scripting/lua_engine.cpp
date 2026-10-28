#include <unimod/scripting/lua_engine.hpp>
#include <unimod/scripting/bindings.hpp>
#include <unimod/core/logger.hpp>
#include <unimod/adapters/adapter_manager.hpp>

#include <fstream>
#include <sstream>

namespace unimod::scripting {

LuaEngine& LuaEngine::GetInstance() {
    static LuaEngine instance;
    return instance;
}

LuaEngine::~LuaEngine() {
    Shutdown();
}

bool LuaEngine::Initialize(const std::string& scriptsDir) {
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    if (m_Initialized) return true;

    m_ScriptsDir = scriptsDir;
    UNIMOD_LOG_INFO("Initializing Sol2 Lua runtime engine (Scripts directory: '{}')...", m_ScriptsDir);

    try {
        // Open standard safe Lua libraries
        m_Lua.open_libraries(
            sol::lib::base,
            sol::lib::package,
            sol::lib::coroutine,
            sol::lib::string,
            sol::lib::table,
            sol::lib::math,
            sol::lib::io,
            sol::lib::os
        );

        // Register core bindings
        unimod::scripting::RegisterCoreBindings(m_Lua);

        // Bind active engine adapter APIs if present
        auto* activeAdapter = adapters::AdapterManager::GetInstance().GetActiveAdapter();
        if (activeAdapter) {
            activeAdapter->OnBindScripting(m_Lua);
            UNIMOD_LOG_INFO("Bound active adapter ('{}') into Lua runtime.", activeAdapter->GetName());
        }

        m_Initialized = true;

        // Auto-load entry script if present
        std::string initScriptPath = m_ScriptsDir + "/init.lua";
        if (std::filesystem::exists(initScriptPath)) {
            UNIMOD_LOG_INFO("Auto-executing entry script: {}", initScriptPath);
            ExecuteFile(initScriptPath);
        }

        UNIMOD_LOG_INFO("LuaEngine successfully initialized.");
        return true;
    } catch (const std::exception& ex) {
        m_LastError = ex.what();
        UNIMOD_LOG_CRITICAL("Fatal error initializing LuaEngine: {}", m_LastError);
        return false;
    }
}

void LuaEngine::Shutdown() {
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    if (!m_Initialized) return;

    UNIMOD_LOG_INFO("Shutting down LuaEngine...");
    m_WatchedFiles.clear();
    m_Initialized = false;
    UNIMOD_LOG_INFO("LuaEngine shut down cleanly.");
}

bool LuaEngine::ExecuteScript(const std::string& scriptCode, const std::string& chunkName) {
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    if (!m_Initialized) {
        UNIMOD_LOG_ERROR("ExecuteScript failed: LuaEngine is not initialized.");
        return false;
    }

    sol::protected_function_result result = m_Lua.safe_script(scriptCode, &sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        m_LastError = err.what();
        UNIMOD_LOG_ERROR("[Lua Error in '{}'] {}", chunkName, m_LastError);
        return false;
    }

    return true;
}

bool LuaEngine::ExecuteFile(const std::string& filePath) {
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);

    if (!std::filesystem::exists(filePath)) {
        m_LastError = "File not found: " + filePath;
        UNIMOD_LOG_WARN("LuaEngine: {}", m_LastError);
        return false;
    }

    // Read file contents
    std::ifstream file(filePath);
    if (!file.is_open()) {
        m_LastError = "Unable to open file: " + filePath;
        UNIMOD_LOG_ERROR("LuaEngine: {}", m_LastError);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    // Register into watched files for hot-reload
    try {
        m_WatchedFiles[filePath] = std::filesystem::last_write_time(filePath);
    } catch (...) {}

    return ExecuteScript(buffer.str(), filePath);
}

void LuaEngine::PollHotReload() {
    std::lock_guard<std::recursive_mutex> lock(m_Mutex);
    if (!m_Initialized) return;

    for (auto& [path, lastWriteTime] : m_WatchedFiles) {
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) {
            auto currentWriteTime = std::filesystem::last_write_time(path, ec);
            if (!ec && currentWriteTime > lastWriteTime) {
                UNIMOD_LOG_INFO(">>> Hot-Reload detected file change in '{}'! Re-evaluating script...", path);
                lastWriteTime = currentWriteTime;

                std::ifstream file(path);
                if (file.is_open()) {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    file.close();
                    ExecuteScript(buffer.str(), path);
                }
            }
        }
    }
}

} // namespace unimod::scripting
