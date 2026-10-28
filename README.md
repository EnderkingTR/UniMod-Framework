<div align="center">

# 🛠️ UniMod Framework

**A Universal, Modular, and Extensible C++20 Game Modding & Reverse Engineering Framework**

[![Language](https://img.shields.io/badge/Language-C%2B%2B20-00599C.svg?style=flat-square&logo=c%2B%2B)](https://isocpp.org/)
[![Build System](https://img.shields.io/badge/Build-CMake%203.25%2B-064F8C.svg?style=flat-square&logo=cmake)](https://cmake.org/)
[![Package Manager](https://img.shields.io/badge/Dependencies-vcpkg-5C2D91.svg?style=flat-square&logo=vcpkg)](https://vcpkg.io/)
[![Scripting](https://img.shields.io/badge/Scripting-Lua%205.4%20%2F%20Sol2-000080.svg?style=flat-square&logo=lua)](https://www.lua.org/)
[![Graphics](https://img.shields.io/badge/Hooks-DirectX%2011%20%26%20ImGui-orange.svg?style=flat-square)](https://learn.microsoft.com/en-us/windows/win32/direct3d11/)
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=flat-square)](LICENSE)

*Bridges the gap between low-level x64 C++ runtime manipulation and high-level Lua scripting through a plug-and-play, engine-agnostic adapter architecture.*

---

[📖 Overview](#-overview) •
[✨ Key Features](#-key-features) •
[🏗️ Architecture](#-architecture--project-structure) •
[🎯 Use Cases](#-use-cases) •
[🚀 Getting Started](#-getting-started) •
[💻 How to Use](#-how-to-use) •
[🧪 Testing](#-testing) •
[🗺️ Roadmap](#-roadmap)

</div>

---

## 📖 Overview

**UniMod Framework** is a modern, lightweight, and universal modding framework written in C++20 for Windows x64. Traditional game modding tools are typically tightly coupled to a single target game or engine, requiring reverse engineers and developers to repeatedly reinvent proxy injection, memory scanning, DirectX hooks, and scripting bridges for every new title.

UniMod solves this fragmentation by decoupling the **Core Runtime** from target-specific engine logic using an **Engine Adapter Pattern** (`IEngineAdapter`). 

Whether you are targeting proprietary in-house engines or commercial engines, UniMod provides immediate out-of-the-box infrastructure for:
- Transparent DLL Proxy Hijacking (`dinput8.dll`) without external injectors.
- SIMD-accelerated AOB (Array of Bytes) pattern scanning and 64-bit RIP-relative address resolution.
- DirectX 11 SwapChain VTable hooking with Dear ImGui overlay integration.
- An embedded, sandboxed Lua scripting runtime with live file **Hot-Reloading**.

---

## ✨ Key Features

- 🔌 **Engine-Agnostic Adapter Architecture:** Adapt to any game engine by implementing the `IEngineAdapter` interface and letting `AdapterManager` automatically probe and activate it at runtime.
- 💉 **Native DLL Proxy Hijacking:** Built-in `dinput8.dll` export forwarding chains original calls to Windows `System32\dinput8.dll` while safely escaping the OS Loader Lock via asynchronous initialization.
- 🧠 **Robust Memory Manipulation & AOB Scanner:** Scan memory with IDA-style wildcards (`48 8B 05 ? ? ? ? 48 85 C0`), resolve RIP-relative offsets, and patch code using RAII `ScopedVirtualProtect`.
- 🎨 **DirectX 11 SwapChain Hooking:** Dummy device VTable discovery intercepts `Present` (Index 8) and `ResizeBuffers` (Index 13). Includes `WndProc` message filtering and Dear ImGui input capture.
- 📜 **Sol2 & Lua Scripting Runtime:** Full C++ API bindings (`unimod.*`, `memory.*`, `gui.*`) with PCall exception boundaries to prevent game crashes from script errors.
- ⚡ **Live Script Hot-Reloading:** Edit Lua scripts on disk with any text editor and see gameplay modifications update instantly without restarting the game.
- 🛡️ **Comprehensive Crash Protection:** Loader Lock deadlock prevention, x64 16-byte stack alignment safety, and page-validation memory wrappers.
- 🧪 **5-Phase Validation Test Suite:** Every subsystem is backed by standalone unit and integration tests.

---

## 🏗️ Architecture & Project Structure

UniMod Framework is strictly decoupled into modular subsystems:

```text
UniMod-Framework/
├── include/unimod/            # Public Header API
│   ├── adapters/              # IEngineAdapter interface & AdapterManager orchestrator
│   ├── core/                  # CoreRuntime lifecycle & thread-safe Logger
│   ├── hooks/                 # DirectX 11 SwapChain hooks & WndProc input capture
│   ├── memory/                # AOB Pattern Scanner & ScopedVirtualProtect utilities
│   └── scripting/             # LuaEngine (Sol2) & C++ to Lua API bindings
├── src/                       # Subsystem Implementations (.cpp)
│   ├── adapters/              # Adapter registration & dispatch logic
│   ├── core/                  # CoreRuntime state machine & spdlog sink config
│   ├── hooks/                 # Dummy device creation, MinHook detours & ImGui rendering
│   ├── memory/                # Fast byte searching & PE section scanning
│   └── scripting/             # Sol2 runtime setup, bindings & hot-reload tracker
├── proxy/
│   └── dinput8/               # dinput8.dll proxy stub, export forwarder & DllMain
├── adapters/
│   └── mock_adapter/          # Reference implementation simulating a proprietary game engine
├── assets/
│   └── scripts/
│       └── init.lua           # Default entry script executed upon runtime boot
├── cmake/
│   └── CompilerWarnings.cmake # Strict compiler warning configurations (/W4, /WX, -Wall)
├── tests/                     # Phase 1 to Phase 5 validation executables
├── CMakeLists.txt             # Target-based CMake build configuration
└── vcpkg.json                 # Declarative dependency manifest
```

### ⚙️ How It Works Under the Hood

```
[ Target Game Process (x64) ]
             │
             ▼ (Loads local dinput8.dll proxy)
[ proxy/dinput8 (DllMain) ] ───(Spawns Async Thread)───► [ CoreRuntime::Initialize ]
             │                                                      │
             ▼ (Forwards original API calls)                        ▼
[ C:\Windows\System32\dinput8.dll ]             [ AdapterManager::DetectAndActivate ]
                                                                    │
                    ┌───────────────────────────────────────────────┴───────────────────────────────┐
                    ▼                                                                               ▼
         [ DX11Hook & ImGui Overlay ]                                                   [ Sol2 LuaEngine & Hot-Reload ]
         - Intercepts SwapChain Present                                                  - Executes assets/scripts/init.lua
         - Blocks game input when menu open                                              - Live watches *.lua file modifications
```

---

## 🎯 Use Cases

- **Universal Mod Loaders:** Create standardized mod frameworks for games lacking official modding SDKs.
- **Lua Gameplay Scripting:** Expose internal C++ engine functions, player stats, and inventory tables to high-level Lua scripts.
- **Diagnostic In-Game Overlays:** Render real-time memory inspectors, telemetry dashboards, free-cam tools, and debug consoles using DirectX 11 and ImGui.
- **Signature-Based Game Patching:** Use the pattern scanner and RIP resolver to build resilient game cheats/patches that survive game updates.
- **Automated Reverse Engineering Research:** Rapidly prototype hooks and object traversal routines using the plug-and-play adapter interface.

---

## 🚀 Getting Started

### Prerequisites

Ensure you have the following installed on your system:
- **Operating System:** Windows 10 / 11 (x64)
- **Compiler:** Visual Studio 2022 (MSVC v143 with C++20) **OR** MinGW-w64 / GCC 15+ (UCRT64)
- **Build System:** CMake 3.25 or higher
- **Package Manager (Optional):** [vcpkg](https://vcpkg.io/) (Framework automatically fetches missing dependencies via `FetchContent` if vcpkg is not present)

### Building from Source

1. **Clone the repository:**
   ```bash
   git clone https://github.com/EnderkingTR/UniMod-Framework.git
   cd UniMod-Framework
   ```

2. **Configure with CMake:**
   - *Using standard CMake (FetchContent fallback mode):*
     ```powershell
     cmake -B build -G "MinGW Makefiles"
     ```
   - *Using Visual Studio & vcpkg manifest mode:*
     ```powershell
     cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release
     ```

3. **Compile the project:**
   ```powershell
   cmake --build build --config Release
   ```

---

## 💻 How to Use

### 1. Deploying to a Target Game

Deploying UniMod into a target DirectX 11 game requires no external injector:

1. Copy the compiled **`dinput8.dll`** from your build output directory into the game's root directory (alongside the main `.exe`).
2. Copy the **`assets/`** folder (containing `scripts/init.lua`) into the same directory.
3. Launch the game normally. UniMod will automatically load, chain DirectInput calls, attach to DirectX 11, and execute your Lua scripts.
4. Press **`[INSERT]`** in-game to toggle the Dear ImGui overlay dashboard.

---

### 2. Writing Lua Mods (`assets/scripts/init.lua`)

Modders can create custom features in Lua without touching C++:

```lua
-- assets/scripts/init.lua
unimod.log_info("UniMod Lua Runtime initialized!")

-- Check if active engine adapter is available
if mock_engine then
    unimod.log_info("Target Character: " .. mock_engine.get_name())
    unimod.log_info("Current HP: " .. tostring(mock_engine.get_health()))

    -- Modifying game attributes in real time
    mock_engine.set_speed(15.0)
    mock_engine.set_god_mode(true)
    unimod.log_info("God Mode activated & movement speed boosted to 15.0 m/s!")
end

-- Native memory manipulation example
local target_ptr = 0x140001000
local current_val = memory.read_i32(target_ptr)
if current_val then
    memory.write_i32(target_ptr, 9999)
end
```

> **Tip:** Edit `init.lua` while the game is running—UniMod will detect the file change and **Hot-Reload** your changes automatically!

---

### 3. Creating a Custom Engine Adapter (C++ Developers)

To build support for a specific engine (e.g. Unreal, Unity, or a custom engine), implement `IEngineAdapter`:

```cpp
#include <unimod/adapters/i_engine_adapter.hpp>
#include <unimod/core/logger.hpp>
#include <unimod/memory/pattern_scanner.hpp>
#include <imgui.h>
#include <sol/sol.hpp>

namespace unimod::adapters {

class MyGameAdapter : public IEngineAdapter {
public:
    const char* GetName() const noexcept override { return "My Custom Game Engine"; }
    const char* GetVersion() const noexcept override { return "1.0.0"; }

    bool OnDetectEngine() override {
        // Detect engine via signature, module, or window class
        return memory::PatternScanner::ScanModule("", memory::Pattern::FromString("48 89 5C 24 ? 57 48 83 EC 20")) != 0;
    }

    bool OnAttach() override {
        UNIMOD_LOG_INFO("MyGameAdapter attached! Hooking engine loops...");
        return true;
    }

    void OnInitRenderer() override {
        // Initialize textures or UI states
    }

    void OnUpdate(float deltaTime) override {
        // Synchronous per-frame logic
    }

    void OnRenderOverlay() override {
        // Render custom ImGui menu widgets
        ImGui::Begin("My Game Cheats & Tools");
        ImGui::Text("Engine Status: Online");
        ImGui::End();
    }

    void OnBindScripting(sol::state& lua) override {
        // Expose engine-specific functions to Lua scripts
        auto myEngine = lua.create_named_table("my_engine");
        myEngine["spawn_item"] = [](int itemId) { /* ... */ };
    }

    void OnDetach() override {
        UNIMOD_LOG_INFO("MyGameAdapter detached cleanly.");
    }
};

} // namespace unimod::adapters
```

---

## 🧪 Testing

UniMod Framework includes a dedicated 5-phase test suite ensuring stability across every subsystem:

```powershell
# Run individual phase verifications:
./build/phase1_test.exe   # Phase 1: Core Logger, MinHook integration & C++20 features
./build/phase2_test.exe   # Phase 2: dinput8.dll Proxy export forwarding & Loader Lock safety
./build/phase3_test.exe   # Phase 3: DirectX 11 SwapChain Hook, Dear ImGui & AOB Scanner
./build/phase4_test.exe   # Phase 4: IEngineAdapter lifecycle & Mock Adapter live memory manipulation
./build/phase5_test.exe   # Phase 5: Sol2 Lua runtime, C++ API bindings & live Hot-Reloading
```

---

## 🗺️ Roadmap

- [x] **Phase 1:** Core Runtime, Logger architecture & MinHook integration
- [x] **Phase 2:** `dinput8.dll` Proxy Hijacking, export forwarding & Loader Lock deadlock safety
- [x] **Phase 3:** DirectX 11 SwapChain VTable Hooking (`Present` / `ResizeBuffers`) & AOB Pattern Scanner
- [x] **Phase 4:** Engine-agnostic adapter lifecycle (`IEngineAdapter`) & Mock Adapter live verification
- [x] **Phase 5:** Sol2 & Lua (v5.4) scripting engine, C++ API bindings & live script Hot-Reloading
- [x] Built-in Dear ImGui Overlay & In-Game Dashboard
- [ ] DirectX 12 & Vulkan swapchain hooking support
- [ ] Unreal Engine 4/5 Adapter (`GObjects`, `GNames`, `ProcessEvent` hooking)
- [ ] Unity IL2CPP / Mono Adapter (domain traversal & internal calls)

---

## 🤝 Contributing

Contributions, issues, and feature requests are welcome!

1. Fork the repository.
2. Create your feature branch (`git checkout -b feature/AmazingFeature`).
3. Commit your changes (`git commit -m 'feat: Add some AmazingFeature'`).
4. Push to the branch (`git push origin feature/AmazingFeature`).
5. Open a Pull Request.

---

## 📄 License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.