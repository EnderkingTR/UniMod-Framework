#pragma once

#include <sol/sol.hpp>

namespace unimod::scripting {

/**
 * @brief Binds core framework subsystems (logging, memory, pattern scanner, gui) into Lua.
 */
void RegisterCoreBindings(sol::state& lua);

} // namespace unimod::scripting
