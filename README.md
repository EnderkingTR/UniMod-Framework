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
```

## Faz 1 Tamamlanan Bileşenler
- [x] Modern CMake & `vcpkg.json` manifest yapılandırması
- [x] Proje dizin hiyerarşisi (`include/unimod/`, `src/`, `proxy/`, `adapters/`, `assets/scripts/`)
- [x] Spdlog asenkron renkli konsol ve dosya sink (`unimod.log`) sarmalayıcısı
- [x] MinHook entegrasyonu ve C++20 standart doğrulaması
