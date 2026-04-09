# GamerHackCS2

**Internal cheat base for Counter-Strike 2** — implemented as a DLL that is injected into the CS2 process.

> ⚠️ **For educational purposes only.** Using cheats in online games violates the terms of service and may result in permanent bans. This project is intended for studying game internals, reverse engineering techniques, and anti-cheat research.

---

## Features

| Category   | Feature             | Description                                        |
|------------|---------------------|----------------------------------------------------|
| **Aimbot** | Aimbot              | Smooth aim assistance with configurable FOV & bone  |
| **ESP**    | Bounding Box        | 2D box (normal or corner style) around players      |
|            | Name ESP            | Player name above box                               |
|            | Health Bar          | Gradient health bar on the side                     |
|            | Armor               | Armor value display                                 |
|            | Weapon ESP          | Current weapon name below box                       |
|            | Skeleton            | Bone-based skeleton rendering                       |
|            | Snaplines           | Lines from screen bottom to player                  |
| **Glow**   | Player Glow         | Colored glow for T/CT                               |
| **Misc**   | Bunny Hop           | Auto-jump when holding space                        |
|            | Auto Strafe         | Air strafe assist                                   |
|            | No Flash            | Reduce/remove flashbang effect                      |
|            | Third Person        | Configurable third-person camera                    |
|            | Watermark           | On-screen branding overlay                          |
| **Other**  | Inventory Changer   | Client-side skin/item modifications                 |

### Keybinds

| Key      | Action                |
|----------|-----------------------|
| `INSERT` | Toggle menu open/close |
| `DELETE` | Unload the cheat DLL  |

---

## Build Requirements

- **OS:** Windows 10/11 (x64)
- **Compiler:** Visual Studio 2022 (v143 toolset) or any MSVC-compatible C++20 compiler
- **C++ Standard:** C++20
- **Windows SDK:** 10.0.22621.0 or later
- **CMake:** 3.20+ (if using CMake build)

---

## Dependencies

The project relies on three external libraries. **They are not included** — you must download them yourself and place them in `dependencies/`:

| Library    | Purpose                              | Where to get it                                         | Place in                            |
|------------|--------------------------------------|---------------------------------------------------------|-------------------------------------|
| **ImGui**  | Menu rendering & overlay drawing     | https://github.com/ocornut/imgui                        | `dependencies/imgui/`               |
| **MinHook**| Inline function hooking (x64)        | https://github.com/TsudaKageworking/minhook             | `dependencies/minhook/`             |
| **FreeType**| Font rasterization for ImGui        | https://freetype.org/                                   | `dependencies/freetype/`            |

### Setting up ImGui

1. Clone or download ImGui from the link above.
2. Copy the following files into `dependencies/imgui/`:
   - `imgui.h`, `imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`, `imgui_internal.h`, `imconfig.h`, `imstb_rectpack.h`, `imstb_textedit.h`, `imstb_truetype.h`
3. Copy the DX11 and Win32 backends:
   - `backends/imgui_impl_dx11.h`, `backends/imgui_impl_dx11.cpp`
   - `backends/imgui_impl_win32.h`, `backends/imgui_impl_win32.cpp`
4. Remove or rename the `imgui_stub.h` file (it's only a placeholder for compilation without ImGui).

### Setting up MinHook

1. Clone or download MinHook from the link above.
2. Copy into `dependencies/minhook/`:
   - `include/MinHook.h`
   - `src/buffer.h`, `src/buffer.c`, `src/hook.c`, `src/trampoline.h`, `src/trampoline.c`
   - `src/hde/hde64.h`, `src/hde/hde64.c`, `src/hde/table64.h`, `src/hde/pstdint.h`
3. The stub `MinHook.h` in this repo can then be removed.

### Setting up FreeType

1. Download a prebuilt FreeType static library for x64, or build from source.
2. Place headers in `dependencies/freetype/include/` (the `ft2build.h` and `freetype/` folder).
3. Place the static library (e.g., `freetype.lib`) in `dependencies/freetype/lib/`.

---

## How to Build

### Option A: CMake (recommended)

```powershell
cd GamerHackCS2
cmake -B build -A x64
cmake --build build --config Release
```

The output DLL will be in `build/bin/Release/GamerHackCS2.dll`.

> **Note:** Before building, uncomment the ImGui and MinHook source file lines in `CMakeLists.txt` once you've placed the real libraries.

### Option B: Visual Studio Project

1. Open Visual Studio 2022.
2. Create a new **Dynamic-Link Library (DLL)** project.
3. Set **Platform** to **x64**, **C++ Language Standard** to **C++20**.
4. Add all `.cpp` files under `src/` to the project.
5. Add ImGui and MinHook `.cpp`/`.c` files.
6. Set **Additional Include Directories** to:
   - `src`
   - `dependencies`
   - `dependencies/imgui`
   - `dependencies/imgui/backends`
   - `dependencies/minhook/include`
   - `dependencies/freetype/include`
7. Link against: `d3d11.lib`, `dxgi.lib`, `dxguid.lib`, and optionally `freetype.lib`.
8. Add preprocessor definitions: `WIN32_LEAN_AND_MEAN`, `NOMINMAX`, `_CRT_SECURE_NO_WARNINGS`.
9. Build in **Release x64**.

---

## Project Structure

```
GamerHackCS2/
├── CMakeLists.txt                  # CMake build configuration
├── README.md                       # This file
├── PATTERNS.md                     # Pattern documentation & update guide
│
├── src/
│   ├── dllmain.cpp                 # DLL entry point, init thread, unload loop
│   │
│   ├── core/
│   │   ├── core.h / core.cpp       # Initialization orchestrator (8-stage startup)
│   │   ├── hooks.h / hooks.cpp     # MinHook-based detour hooks (D3D + game)
│   │   ├── interfaces.h / .cpp     # Game interface capture (CreateInterface + patterns)
│   │   ├── schema.h / schema.cpp   # Runtime schema offset resolution
│   │   ├── config.h / config.cpp   # JSON-based config system with hash storage
│   │   └── variables.h             # All cheat config variable definitions
│   │
│   ├── sdk/
│   │   ├── common.h                # Precompiled header: Windows, STL, D3D, macros
│   │   ├── const.h                 # Game constants and enumerations
│   │   ├── entity.h / entity.cpp   # Entity class hierarchy (C_BaseEntity → C_CSPlayerPawn)
│   │   ├── entity_handle.h         # CBaseHandle type
│   │   │
│   │   ├── datatypes/
│   │   │   ├── vector.h            # Vector2D, Vector3, Vector4, VectorAligned
│   │   │   ├── qangle.h            # QAngle (pitch/yaw/roll)
│   │   │   ├── matrix.h            # Matrix3x4, ViewMatrix (4x4), Matrix2x4
│   │   │   ├── color.h             # Color (RGBA with ImGui/float conversion)
│   │   │   ├── usercmd.h           # CUserCmd, CInButtonState, CCSGOUserCmdPB
│   │   │   └── utlvector.h         # CUtlVector (Valve's dynamic array)
│   │   │
│   │   ├── interfaces/
│   │   │   ├── iswapchain.h        # ISwapChainDx11
│   │   │   ├── isource2client.h    # ISource2Client
│   │   │   ├── ischemasystem.h     # ISchemaSystem
│   │   │   ├── iinputsystem.h      # IInputSystem
│   │   │   ├── iglobalvars.h       # IGlobalVars
│   │   │   ├── igameresourceservice.h  # IGameResourceService
│   │   │   ├── ienginecvar.h       # IEngineCVar
│   │   │   ├── iengineclient.h     # IEngineClient
│   │   │   ├── cgametracemanager.h # CGameTraceManager
│   │   │   ├── cgameentitysystem.h # CGameEntitySystem
│   │   │   └── ccsgoinput.h        # CCSGOInput
│   │   │
│   │   └── econ/
│   │       ├── inventory.h         # Inventory changer types
│   │       ├── econitemschema.h    # CEconItemSchema
│   │       └── econitem.h          # C_EconItemView, C_EconEntity
│   │
│   └── utilities/
│       ├── render.h / render.cpp   # ImGui draw system + double-buffered render stack
│       ├── memory.h / memory.cpp   # Pattern scan, PEB module enum, VFunc access
│       ├── math.h / math.cpp       # Angle math, W2S projection, FOV calculation
│       ├── log.h / log.cpp         # Thread-safe console + file logging
│       ├── input.h / input.cpp     # WndProc hook, key state tracking
│       ├── detourhook.h            # CBaseHookObject<T> MinHook wrapper
│       ├── fnv1a.h                 # Compile-time & runtime FNV-1a hashing
│       └── xorstr.h                # Compile-time string encryption
│
├── dependencies/
│   ├── imgui/                      # Place ImGui source files here
│   │   └── imgui_stub.h            # Stub (remove after adding real ImGui)
│   ├── minhook/
│   │   └── include/
│   │       └── MinHook.h           # Stub (remove after adding real MinHook)
│   └── freetype/
│       └── include/                # Place FreeType headers here
│
└── resources/
    └── fonts/                      # Custom fonts (embedded in render.cpp)
```

---

## How It Works

### Initialization Sequence

1. `DllMain` → creates init thread, waits for `client.dll` and `engine2.dll` to load
2. **Logging** — opens console window and log file
3. **Interfaces** — captures game interfaces via `CreateInterface` exports and pattern scanning
4. **Schema** — dumps the game's runtime type system to resolve entity field offsets
5. **Config** — loads saved settings from `Documents/GamerHack/default.json`
6. **Input** — hooks the game window's WndProc for key/mouse input
7. **Draw** — initializes ImGui with D3D11 backend
8. **Features** — sets up aimbot, ESP, misc features
9. **Hooks** — installs MinHook detours on D3D Present, CreateMove, etc.

### Hook Architecture

| Hook                | Target                   | Purpose                          |
|---------------------|--------------------------|----------------------------------|
| `Present`           | IDXGISwapChain VFunc 8   | Render overlay + menu            |
| `ResizeBuffers`     | IDXGISwapChain VFunc 13  | Handle resolution changes        |
| `CreateMove`        | client.dll pattern       | Aimbot, movement hacks           |
| `GetMatrixForView`  | client.dll pattern       | Capture view matrix for ESP W2S  |
| `LevelInit`         | client.dll pattern       | Map load notification            |
| `LevelShutdown`     | client.dll pattern       | Map unload cleanup               |
| `OverrideView`      | client.dll pattern       | FOV override, third person       |
| `FrameStageNotify`  | ISource2Client VTable    | Per-frame feature dispatch       |
| `MouseInputEnabled` | IInputSystem VTable      | Block game input when menu open  |
| `WndProc`           | SetWindowLongPtr         | ImGui input, menu toggle         |

---

## How to Inject

The DLL must be injected into a running `cs2.exe` process. Common methods:

1. **Manual Map Injector** (recommended) — maps the DLL manually without LoadLibrary, avoids easy detection.
2. **LoadLibrary Injection** — basic injection via `CreateRemoteThread` + `LoadLibraryA`. Detectable but simple for testing.
3. **Process Hollowing** — advanced, replaces a legitimate DLL.

**Basic LoadLibrary injection steps:**
1. Build in Release x64.
2. Start CS2 and wait until you reach the main menu.
3. Use any DLL injector that supports x64.
4. Inject `GamerHackCS2.dll` into `cs2.exe`.
5. A console window should appear with initialization logs.
6. Press `INSERT` to open the menu in-game.
7. Press `DELETE` to unload cleanly.

---

## Configuration System

Settings are stored as JSON in `Documents/GamerHack/` (or DLL-relative directory).

- **Auto-save**: Config is automatically saved to `default.json` on unload.
- **Manual save/load**: Use the config tab in the menu.
- Variables are registered via the `C_ADD_VARIABLE` macro in `src/core/variables.h` and stored using FNV-1a hash keys.

### Variable Categories

| Category  | Variables                                                              |
|-----------|------------------------------------------------------------------------|
| Aimbot    | `enabled`, `fov`, `smooth`, `bone` (6=head), `visible_only`           |
| ESP       | `enabled`, `box`, `name`, `health`, `armor`, `weapon`, `skeleton`, `snaplines`, `box_type`, `box_color_t`, `box_color_ct` |
| Glow      | `enabled`, `color_t`, `color_ct`                                       |
| Misc      | `bhop`, `autostrafe`, `noflash`, `noflash_alpha`, `thirdperson`, `thirdperson_distance`, `watermark` |
| Inventory | `enabled`                                                              |

---

## How to Update Patterns

Patterns break when the game updates. See **[PATTERNS.md](PATTERNS.md)** for the full pattern reference with search instructions.

### Quick Guide

1. Open the updated `client.dll` / `engine2.dll` / `rendersystemdx11.dll` in **IDA Pro** or **Ghidra**.
2. Search for the reference string noted in `PATTERNS.md` (e.g., `"game_newmap"`).
3. Follow the xref chain described in the documentation.
4. Copy the new byte pattern (with wildcards for variable bytes as `?`).
5. Update the pattern in `src/core/interfaces.cpp` (for interface pointers) or `src/core/hooks.cpp` (for hook targets).
6. Rebuild and test.

### Pattern Locations in Code

- **Interface patterns**: `src/core/interfaces.cpp` — `s_arrPatterns[]` array
- **Hook patterns**: `src/core/hooks.cpp` — inline in `H::Setup()`
- **Swap chain pattern**: `src/core/hooks.cpp` — D3D hook setup section

---

## How to Add New Features

1. Create a new file in `src/features/` (e.g., `src/features/radar.cpp`).
2. Add your feature function in the `F` namespace:
   ```cpp
   namespace F {
       void OnRadar(); // called from OnPresent or OnCreateMove
   }
   ```
3. Add config variables in `src/core/variables.h`:
   ```cpp
   C_ADD_VARIABLE(bool, radar_enabled, false);
   C_ADD_VARIABLE(float, radar_range, 1000.0f);
   ```
4. Hook into the feature dispatch in `hooks.cpp` — call from `OnPresent()` for visuals or `OnCreateMove()` for movement/aim.
5. Add the `.cpp` file to `CMakeLists.txt` under a new `SOURCES_FEATURES` list.
6. Add a menu tab in the menu rendering code.

---

## Troubleshooting

| Problem                        | Solution                                                     |
|--------------------------------|--------------------------------------------------------------|
| DLL doesn't load               | Ensure x64 Release build; check injector supports x64        |
| Crash on inject                | Patterns may be outdated — check console log for which failed |
| Menu doesn't appear            | D3D hooks may have failed — check swap chain pattern         |
| Schema offsets wrong            | Game updated — run schema dump, compare with previous build   |
| "interface capture FAILED"     | Interface version strings changed — check `interfaces.cpp`    |
| Black screen after resize      | `ResizeBuffers` hook issue — check render target recreation   |

---

## License

This project is provided as-is for educational and research purposes. No warranty is provided. Use at your own risk.
