#pragma once

#include <sol/sol.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <filesystem>
#include <mutex>

namespace unimod::scripting {

class LuaEngine {
public:
    static LuaEngine& GetInstance();

    LuaEngine(const LuaEngine&) = delete;
    LuaEngine& operator=(const LuaEngine&) = delete;

    /**
     * @brief Initializes Sol2 Lua state, opens standard libraries, binds C++ APIs,
     * and executes the primary entry script (init.lua).
     */
    bool Initialize(const std::string& scriptsDir = "assets/scripts");

    /**
     * @brief Safely shuts down Lua runtime and clears all bindings.
     */
    void Shutdown();

    /**
     * @brief Executes raw Lua code with safe error boundary.
     * @return true on success, false if a syntax or runtime error occurred.
     */
    bool ExecuteScript(const std::string& scriptCode, const std::string& chunkName = "chunk");

    /**
     * @brief Executes a Lua file from disk and adds it to the hot-reload watch list.
     */
    bool ExecuteFile(const std::string& filePath);

    /**
     * @brief Polls watched scripts for file modification time changes.
     * If a file was edited, re-executes the script dynamically without restarting.
     */
    void PollHotReload();

    sol::state& GetState() noexcept { return m_Lua; }
    bool IsInitialized() const noexcept { return m_Initialized; }
    const std::string& GetLastError() const noexcept { return m_LastError; }

private:
    LuaEngine() = default;
    ~LuaEngine();

    sol::state m_Lua;
    bool m_Initialized{false};
    std::string m_ScriptsDir;
    std::string m_LastError;

    std::unordered_map<std::string, std::filesystem::file_time_type> m_WatchedFiles;
    mutable std::recursive_mutex m_Mutex;
};

} // namespace unimod::scripting
