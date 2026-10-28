#include <unimod/scripting/bindings.hpp>
#include <unimod/core/logger.hpp>
#include <unimod/core/core_runtime.hpp>
#include <unimod/memory/memory_utils.hpp>
#include <unimod/memory/pattern_scanner.hpp>
#include <unimod/hooks/dx11_hook.hpp>

namespace unimod::scripting {

void RegisterCoreBindings(sol::state& lua) {
    // 1. unimod table
    auto unimodTable = lua.create_named_table("unimod");
    unimodTable["log_info"] = [](const std::string& msg) {
        UNIMOD_LOG_INFO("[Lua] {}", msg);
    };
    unimodTable["log_warn"] = [](const std::string& msg) {
        UNIMOD_LOG_WARN("[Lua] {}", msg);
    };
    unimodTable["log_error"] = [](const std::string& msg) {
        UNIMOD_LOG_ERROR("[Lua] {}", msg);
    };
    unimodTable["is_running"] = []() -> bool {
        return core::CoreRuntime::GetInstance().IsRunning();
    };

    // 2. memory table
    auto memTable = lua.create_named_table("memory");
    memTable["read_i32"] = [](uintptr_t addr) -> sol::optional<int32_t> {
        int32_t val = 0;
        if (memory::ReadMemorySafe(addr, val)) {
            return val;
        }
        return sol::nullopt;
    };
    memTable["write_i32"] = [](uintptr_t addr, int32_t val) -> bool {
        return memory::WriteMemorySafe(addr, val);
    };
    memTable["read_float"] = [](uintptr_t addr) -> sol::optional<float> {
        float val = 0.0f;
        if (memory::ReadMemorySafe(addr, val)) {
            return val;
        }
        return sol::nullopt;
    };
    memTable["write_float"] = [](uintptr_t addr, float val) -> bool {
        return memory::WriteMemorySafe(addr, val);
    };
    memTable["resolve_rip"] = [](uintptr_t insnAddr, size_t insnLen, size_t dispOffset) -> uintptr_t {
        return memory::ResolveRip(insnAddr, insnLen, dispOffset);
    };
    memTable["pattern_scan"] = [](const std::string& patternStr, sol::optional<std::string> moduleName) -> uintptr_t {
        auto pattern = memory::Pattern::FromString(patternStr);
        std::string mod = moduleName.value_or("");
        return memory::PatternScanner::ScanModule(mod, pattern);
    };

    // 3. gui table
    auto guiTable = lua.create_named_table("gui");
    guiTable["is_menu_open"] = []() -> bool {
        return hooks::DX11Hook::GetInstance().IsMenuOpen();
    };
    guiTable["set_menu_open"] = [](bool open) {
        hooks::DX11Hook::GetInstance().SetMenuOpen(open);
    };

    UNIMOD_LOG_INFO("Core Lua bindings ('unimod', 'memory', 'gui') registered successfully.");
}

} // namespace unimod::scripting
