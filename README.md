# UniMod Framework

Universal Runtime Modding & Reverse Engineering Framework for Proprietary Game Engines.

## Sistem Mimarisi & Özellikler

- **Modern C++20:** Concepts, `std::span`, `std::format`, RAII bellek yönetimi.
- **Modüler Katmanlar:** Evrensel `Core Runtime` ve tak-çıkar `Engine Adapters` (`IEngineAdapter`).
- **Enjeksiyon:** `dinput8.dll` Proxy DLL Hijacking ile harici enjektöre ihtiyaç duymayan doğal çalışma zamanı yüklenmesi.
- **Grafik & UI:** DirectX 11 `IDXGISwapChain` VTable kancası ve Dear ImGui overlay entegrasyonu.
- **Hafıza Motoru:** SIMD hızlandırmalı AOB Pattern Scanner, RIP-relative adres çözücü ve SEH / VEH çökme koruması.
- **Scripting:** Sol2 & LuaJIT ile C++ fonksiyon bağlamları ve anında canlı güncelleme (Hot-Reload).

## Derleme Talimatları

### Gereksinimler
- CMake 3.25+
- C++20 destekli derleyici (MSVC v143 veya GCC 15+ / Clang)

### Derleme (CLI)
```powershell
# Yapılandırma
cmake -B build -G "MinGW Makefiles"

# Derleme
cmake --build build --config Release

# Faz 1 Doğrulama Testi
./build/phase1_test.exe

# Faz 2 Doğrulama Testi
./build/phase2_test.exe

# Faz 3 Doğrulama Testi
./build/phase3_test.exe
```

## Faz Durumu & Tamamlanan Bileşenler
- [x] **Faz 1:** Modern CMake & `vcpkg.json` manifest yapılandırması, proje iskeleti, Spdlog renkli konsol & dosya sink, MinHook entegrasyonu.
- [x] **Faz 2:** `dinput8.dll` Proxy Hijacking, chained export forwarding (`DirectInput8Create`), Loader Lock emniyetli asenkron thread bootstrap, thread-safe `CoreRuntime` yaşam döngüsü yöneticisi.
- [x] **Faz 3:** DirectX 11 Dummy Device ile SwapChain VTable tespiti (Index 8: `Present`, Index 13: `ResizeBuffers`), Dear ImGui render ve Win32 `WndProc` girdi kancası, AOB Pattern Scanner & RIP-relative adres hesaplayıcısı.
- [ ] **Faz 4:** `IEngineAdapter` arayüzü ve Mock Adapter ile tak-çıkar doğrulaması.
- [ ] **Faz 5:** Sol2 ile Lua çalışma zamanı entegrasyonu & Hot-Reload mekanizması.
