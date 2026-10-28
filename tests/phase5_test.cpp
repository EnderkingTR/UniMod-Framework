#include <iostream>
#include <fstream>
#include <memory>
#include <thread>
#include <chrono>
#include <cassert>

#include <unimod/core/logger.hpp>
#include <unimod/adapters/adapter_manager.hpp>
#include <unimod/scripting/lua_engine.hpp>
#include "../../adapters/mock_adapter/mock_adapter.hpp"

int main() {
    std::cout << "====================================================\n";
    std::cout << "  UniMod Framework - Phase 5 Verification Test\n";
    std::cout << "====================================================\n";

    unimod::core::Logger::Initialize("phase5_test.log", false);

    // -----------------------------------------------------------------------
    // Part 1: Attach Mock Adapter
    // -----------------------------------------------------------------------
    std::cout << "\n[Part 1] Setting up AdapterManager with MockAdapter...\n";
    auto& adapterMgr = unimod::adapters::AdapterManager::GetInstance();
    auto mockAdapter = std::make_unique<unimod::adapters::mock::MockAdapter>();
    auto* pMock = mockAdapter.get();
    adapterMgr.RegisterAdapter(std::move(mockAdapter));
    adapterMgr.DetectAndActivateAdapter();

    // -----------------------------------------------------------------------
    // Part 2: Initialize LuaEngine & Execute init.lua
    // -----------------------------------------------------------------------
    std::cout << "\n[Part 2] Initializing LuaEngine and auto-loading init.lua...\n";
    auto& luaEngine = unimod::scripting::LuaEngine::GetInstance();
    bool initOk = luaEngine.Initialize("assets/scripts");
    if (!initOk) {
        std::cerr << "FAIL: LuaEngine::Initialize failed! Error: " << luaEngine.GetLastError() << "\n";
        return 1;
    }

    sol::state& lua = luaEngine.GetState();
    bool initLoaded = lua["UNIMOD_INIT_LOADED"].get_or(false);
    std::cout << "-> init.lua executed. UNIMOD_INIT_LOADED = " << (initLoaded ? "true" : "false") << "\n";
    if (!initLoaded) {
        std::cerr << "FAIL: init.lua was not executed properly!\n";
        return 1;
    }

    // Verify Lua modified mock engine player speed to 15.0
    float speed = pMock->GetContext()->moveSpeed;
    std::cout << "-> Mock engine player speed updated by Lua: " << speed << " m/s (Expected: 15.0)\n";
    if (speed != 15.0f) {
        std::cerr << "FAIL: Mock engine speed not modified by Lua!\n";
        return 1;
    }

    // -----------------------------------------------------------------------
    // Part 3: Test C++ Memory Bindings inside Lua
    // -----------------------------------------------------------------------
    std::cout << "\n[Part 3] Testing low-level memory bindings in Lua...\n";
    int32_t nativeTarget = 12345;
    uintptr_t targetAddr = reinterpret_cast<uintptr_t>(&nativeTarget);

    std::string testScript = R"(
        local addr = ...
        local val = memory.read_i32(addr)
        unimod.log_info("Lua read native memory: " .. tostring(val))
        memory.write_i32(addr, 98765)
    )";

    bool execOk = luaEngine.ExecuteScript("memory.write_i32(" + std::to_string(targetAddr) + ", 98765)", "mem_test");
    if (!execOk) {
        std::cerr << "FAIL: Failed to execute memory write script in Lua!\n";
        return 1;
    }

    std::cout << "-> Native target value after Lua write: " << nativeTarget << " (Expected: 98765)\n";
    if (nativeTarget != 98765) {
        std::cerr << "FAIL: Memory write from Lua did not update native variable!\n";
        return 1;
    }

    // -----------------------------------------------------------------------
    // Part 4: Test Hot-Reload Mechanism
    // -----------------------------------------------------------------------
    std::cout << "\n[Part 4] Testing Script Hot-Reload...\n";
    std::string reloadScriptPath = "assets/scripts/hot_reload_test.lua";

    // Write initial version of script
    {
        std::ofstream out(reloadScriptPath);
        out << "_G.RELOAD_VALUE = 100\n";
    }

    bool loadOk = luaEngine.ExecuteFile(reloadScriptPath);
    if (!loadOk) {
        std::cerr << "FAIL: Could not load initial hot_reload_test.lua!\n";
        return 1;
    }

    int initialVal = lua["RELOAD_VALUE"].get_or(0);
    std::cout << "-> Initial RELOAD_VALUE in Lua: " << initialVal << "\n";
    if (initialVal != 100) {
        std::cerr << "FAIL: Initial RELOAD_VALUE mismatch!\n";
        return 1;
    }

    // Wait 1.2s to ensure file timestamp changes reliably on Windows FAT/NTFS
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    // Modify script on disk
    std::cout << "-> Modifying " << reloadScriptPath << " on disk...\n";
    {
        std::ofstream out(reloadScriptPath);
        out << "_G.RELOAD_VALUE = 999\n";
    }

    // Poll hot reload
    luaEngine.PollHotReload();

    int updatedVal = lua["RELOAD_VALUE"].get_or(0);
    std::cout << "-> Post hot-reload RELOAD_VALUE in Lua: " << updatedVal << "\n";
    if (updatedVal != 999) {
        std::cerr << "FAIL: Hot-Reload did not update RELOAD_VALUE to 999!\n";
        return 1;
    }
    std::cout << "-> Hot-Reload successfully verified!\n";

    // Clean up temporary script
    std::remove(reloadScriptPath.c_str());

    // -----------------------------------------------------------------------
    // Part 5: Error Isolation & Crash Prevention
    // -----------------------------------------------------------------------
    std::cout << "\n[Part 5] Testing Lua error boundary & crash protection...\n";
    bool failedExec = luaEngine.ExecuteScript("syntax error in lua !!! @@@", "faulty_chunk");
    if (failedExec) {
        std::cerr << "FAIL: Malformed Lua code unexpectedly reported success!\n";
        return 1;
    }
    std::cout << "-> Malformed Lua code safely intercepted without crashing.\n";
    std::cout << "-> Intercepted error details: " << luaEngine.GetLastError() << "\n";

    // -----------------------------------------------------------------------
    // Part 6: Clean Shutdown
    // -----------------------------------------------------------------------
    std::cout << "\n[Part 6] Shutting down LuaEngine and AdapterManager...\n";
    luaEngine.Shutdown();
    adapterMgr.Shutdown();
    unimod::core::Logger::Shutdown();

    std::cout << "\n====================================================\n";
    std::cout << "[SUCCESS] Phase 5 verification passed completely!\n";
    std::cout << "====================================================\n";
    return 0;
}
