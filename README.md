# UniMod Framework

Universal Runtime Modding & Reverse Engineering Framework for Proprietary Game Engines.

## Sistem Mimarisi & Özellikler

- **Modern C++20:** Concepts, `std::span`, `std::format`, RAII bellek yönetimi, taşınabilir exception boundaries.
- **Modüler Katmanlar:** Evrensel `Core Runtime` ve tak-çıkar `Engine Adapters` (`IEngineAdapter`).
- **Enjeksiyon:** `dinput8.dll` Proxy DLL Hijacking ile harici enjektöre ihtiyaç duymayan doğal çalışma zamanı yüklenmesi.
- **Grafik & UI:** DirectX 11 `IDXGISwapChain` VTable kancası (Index 8: `Present`, Index 13: `ResizeBuffers`) ve Dear ImGui overlay entegrasyonu.
- **Hafıza Motoru:** SIMD hızlandırmalı AOB Pattern Scanner, RIP-relative adres çözücü ve sayfa korumalı SEH / Memory guardlama.
- **Scripting:** Sol2 & Lua (v5.4) çalışma zamanı, C++ çekirdek ve motora özel API bağlamları (Bindings), dosya sistemi izlemeli anında canlı güncelleme (Hot-Reload) ve PCall hata izolasyonu.

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

# Faz 1 Testi: Logger, MinHook & C++20
./build/phase1_test.exe

# Faz 2 Testi: dinput8.dll Proxy & Loader Lock Güvenliği
./build/phase2_test.exe

# Faz 3 Testi: DirectX 11 SwapChain Hook, Dear ImGui & AOB Scanner
./build/phase3_test.exe

# Faz 4 Testi: IEngineAdapter & Mock Adapter Canlı Manipülasyon
./build/phase4_test.exe

# Faz 5 Testi: Sol2 Lua Runtime & Canlı Script Hot-Reload
./build/phase5_test.exe
```

## Faz Durumu & Tamamlanan Kilometre Taşları
- [x] **Faz 1:** Modern CMake & `vcpkg.json` manifest yapılandırması, proje iskeleti, Spdlog renkli konsol & dosya sink, MinHook entegrasyonu.
- [x] **Faz 2:** `dinput8.dll` Proxy Hijacking, chained export forwarding (`DirectInput8Create`), Loader Lock emniyetli asenkron thread bootstrap, thread-safe `CoreRuntime` yaşam döngüsü yöneticisi.
- [x] **Faz 3:** DirectX 11 Dummy Device ile SwapChain VTable tespiti (Index 8: `Present`, Index 13: `ResizeBuffers`), Dear ImGui render ve Win32 `WndProc` girdi kancası, AOB Pattern Scanner & RIP-relative adres hesaplayıcısı.
- [x] **Faz 4:** `IEngineAdapter` soyut yaşam döngüsü arayüzü, `AdapterManager` orkestratörü, `MockAdapter` ile tak-çıkar motor doğrulaması, canlı bellek manipülasyonu ve ImGui entegrasyonu.
- [x] **Faz 5:** Sol2 ile Lua çalışma zamanı entegrasyonu, C++ çekirdek & motor binding'leri, `assets/scripts/` için dosya izlemeli Hot-Reload mekanizması ve çökme emniyetli hata sınırı (Error Boundary).
