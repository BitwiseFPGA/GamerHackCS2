# GamerHackCS2 — Pattern Reference

> **Source of Truth:** All patterns, offsets, and version-dependent constants are defined in
> [`src/core/patterns.h`](src/core/patterns.h). This document provides context and instructions
> for updating them, but `patterns.h` is the canonical location used by the code.

This document explains every byte pattern used in the project, what it resolves to, how to find it in a disassembler (IDA Pro / Ghidra), and when it was last verified.

**Patterns break on every major CS2 update.** When the cheat fails to initialize after an update, update the patterns in `src/core/patterns.h`.

> **D3D Hook note:** As of the current build, the primary D3D11/Present hook is via
> `gameoverlayrenderer64.dll` (`PRESENT_OVERLAY` pattern). The `SWAP_CHAIN_HOOK` in
> `rendersystemdx11.dll` is a **secondary/fallback** path. If the overlay renderer isn't
> available at injection time, the code falls back to the swap chain vtable hook.

---

## Table of Contents

1. [Interface Patterns](#interface-patterns) — non-exported pointers found via pattern scanning
2. [Hook Patterns](#hook-patterns) — function addresses for detour hooks
3. [Offsets](#key-offsets) — hardcoded struct offsets that may change
4. [How to Update a Pattern](#how-to-update-a-pattern)

---

## Interface Patterns

These are defined in `src/core/patterns.h` under `PATTERNS::FUNCTIONS::` and referenced from `src/core/interfaces.cpp`.

### IGlobalVars

| Field         | Value |
|---------------|-------|
| **Pattern**   | `48 89 15 ? ? ? ? 48 89 42` |
| **Module**    | `engine2.dll` |
| **Resolves to** | `IGlobalVars*` — global timing, tick count, curtime, etc. |
| **Search type** | `PTR` (RIP-relative pointer dereference) |
| **Last verified** | 2024-XX-XX (placeholder) |

**How to find in IDA/Ghidra:**
1. Open `engine2.dll` in the disassembler.
2. Search for the string `"CBaseEntity::SetParent"` or `"GlobalVars"`.
3. Follow the cross-reference to the function that uses the string.
4. Near the top of that function, look for a `mov [rip+disp], rdx` or `mov [rip+disp], rax` instruction that stores into a global pointer.
5. The destination of that `mov` is the `IGlobalVars*` pointer.
6. Copy the byte pattern of that instruction and surrounding context.

**What it looks like:**
```asm
48 89 15 XX XX XX XX    ; mov [rip+IGlobalVars], rdx
48 89 42 ??             ; mov [rdx+??], rax
```

---

### CCSGOInput

| Field         | Value |
|---------------|-------|
| **Pattern**   | `48 8B 0D ? ? ? ? 48 8B 01 FF 50 ? 8B DF` |
| **Module**    | `client.dll` |
| **Resolves to** | `CCSGOInput*` — input wrapper (view angles, user commands) |
| **Search type** | `PTR` (RIP-relative pointer dereference) |
| **Last verified** | 2024-XX-XX (placeholder) |

**How to find in IDA/Ghidra:**
1. Open `client.dll`.
2. Search for strings related to `"CCSGOInput"` or `"GetUserCmd"`.
3. Look for a function that loads a global pointer via `mov rcx, [rip+disp]` and then calls a virtual function (`call [rax+??]`).
4. The `mov rcx, [rip+disp]` is loading the `CCSGOInput*`.
5. The surrounding `8B DF` (`mov ebx, edi`) helps narrow down the exact site.

**What it looks like:**
```asm
48 8B 0D XX XX XX XX    ; mov rcx, [rip+CCSGOInput]
48 8B 01                ; mov rax, [rcx]       ; load vtable
FF 50 ??                ; call [rax+??]        ; virtual call
8B DF                   ; mov ebx, edi
```

---

### ViewMatrix

| Field         | Value |
|---------------|-------|
| **Pattern**   | `48 8D 0D ? ? ? ? 48 C1 E0 06` |
| **Module**    | `client.dll` |
| **Resolves to** | `ViewMatrix*` — 4x4 world-to-projection matrix for ESP |
| **Search type** | `PTR` (RIP-relative LEA) |
| **Last verified** | 2024-XX-XX (placeholder) |

**How to find in IDA/Ghidra:**
1. Open `client.dll`.
2. Look for `CViewRender` class methods, specifically `OnRenderStart` or view-related rendering code.
3. Search for a `lea rcx, [rip+disp]` instruction followed by `shl rax, 6` — the `shl rax, 6` (multiply by 64) is a distinctive operation used to index into a matrix array (each 4x4 matrix = 64 bytes).
4. The LEA target is the `ViewMatrix` array base.

**What it looks like:**
```asm
48 8D 0D XX XX XX XX    ; lea rcx, [rip+ViewMatrix]
48 C1 E0 06             ; shl rax, 6
```

---

### CGameTraceManager

| Field         | Value |
|---------------|-------|
| **Pattern**   | `4C 8B 3D ? ? ? ? 24 C9 0C 49 66 0F 7F 45` |
| **Module**    | `client.dll` |
| **Resolves to** | `CGameTraceManager*` — raycasting / collision detection |
| **Search type** | `PTR` (RIP-relative pointer load) |
| **Last verified** | 2024-XX-XX (placeholder) |

**How to find in IDA/Ghidra:**
1. Open `client.dll`.
2. Search for `"TraceShape"` or trace/ray-related strings.
3. In the trace function, look for a `mov r15, [rip+disp]` that loads the trace manager global.
4. The `24 C9 0C 49` and `66 0F 7F 45` (MOVDQA store) are distinctive context bytes in the same function.

**What it looks like:**
```asm
4C 8B 3D XX XX XX XX    ; mov r15, [rip+CGameTraceManager]
24 C9                   ; and cl, 0C9h
0C 49                   ; or al, 49h
66 0F 7F 45 ??          ; movdqa [rbp+??], xmm0
```

---

### ISwapChainDx11

| Field         | Value |
|---------------|-------|
| **Pattern**   | `48 89 2D ? ? ? ? 66 0F 7F 05` |
| **Module**    | `rendersystemdx11.dll` |
| **Resolves to** | `ISwapChainDx11*` — the game's swap chain wrapper |
| **Search type** | `PTR` (RIP-relative, 3-byte prefix before displacement) |
| **Last verified** | 2026-04-09 |

**How to find in IDA/Ghidra:**
1. Open `rendersystemdx11.dll`.
2. Search for strings around `"CRenderDeviceDx11"` or DX11 initialization code.
3. Look for a `mov [rip+disp], rbp` instruction (`48 89 2D`) that stores the swap chain pointer to a global — this is `ISwapChainDx11*`.
4. Immediately after that, there is a `movdqa [rip+disp], xmm0` (`66 0F 7F 05`) storing XMM state.

> **Why not `66 0F 7F 05`?** The MOVDQA opcode is 4 bytes (`66 0F 7F 05`), but `ESearchType::PTR` uses
> `GetAbsoluteAddress(addr, 3, 7)` — 3-byte opcode + 4-byte displacement. Using MOVDQA directly
> would read the wrong displacement. The `48 89 2D` instruction before it is a 3-byte opcode, so
> we match that instead.

**What it looks like:**
```asm
48 89 2D XX XX XX XX     ; mov [rip+ISwapChainDx11], rbp
66 0F 7F 05 XX XX XX XX  ; movdqa [rip+??], xmm0
```

---

### SwapChain (secondary hook path)

| Field         | Value |
|---------------|-------|
| **Pattern**   | `48 8B 0D ? ? ? ? 48 85 C9 74 ? FF` |
| **Module**    | `rendersystemdx11.dll` |
| **Resolves to** | `CSwapChainDx11*` — used to obtain `IDXGISwapChain` for vtable hook |
| **Search type** | `PTR` (RIP-relative) |
| **Last verified** | 2026-04-09 |

**⚠️ This is the SECONDARY hook path.** The primary path is `PRESENT_OVERLAY` in `gameoverlayrenderer64.dll`.
This pattern was previously `48 8B 0D ? ? ? ? 48 85 C9 74 ? 8B 41` — the trailing `8B 41` bytes changed in the current CS2 build.

**How to find in IDA/Ghidra:**
1. Open `rendersystemdx11.dll`.
2. Look for code that loads the swap chain pointer, null-checks it (`test rcx, rcx; jz skip`), and then calls a method on it (`FF 50 ?? = call [rax+??]`).
3. The pattern: load global rcx → null test → jz → virtual call.

**What it looks like:**
```asm
48 8B 0D XX XX XX XX    ; mov rcx, [rip+SwapChainObj]
48 85 C9                ; test rcx, rcx
74 ??                   ; jz skip
FF ??                   ; call [rax+??] or call [rcx+??]
```

---

## Hook Patterns

These are defined in `src/core/patterns.h` under `PATTERNS::FUNCTIONS::` and referenced from `src/core/hooks.cpp`.

### CreateMove

| Field         | Value |
|---------------|-------|
| **Pattern**   | `48 8B C4 4C 89 40 ? 48 89 48 ? 55 53 56 57 48 8D A8` |
| **Module**    | `client.dll` |
| **Resolves to** | `CCSGOInput::CreateMove` — called every tick to produce a user command |
| **Search type** | `NONE` (direct match) |
| **Last verified** | 2024-XX-XX (placeholder) |

**How to find in IDA/Ghidra:**
1. Open `client.dll`.
2. Search for the string `"cl: CreateMove clamped invalid attack history index"`.
3. Follow the xref — it leads directly into the `CreateMove` function body.
4. The function prologue starts with `mov rax, rsp` / `mov [rax+??], r8` / stack frame setup.
5. Copy bytes from the function start.

**Signature of the function:**
```cpp
bool __fastcall CreateMove(CCSGOInput* pInput, int nSlot, CUserCmd* pCmd)
```

---

### GetMatrixForView

| Field         | Value |
|---------------|-------|
| **Pattern**   | `40 53 48 81 EC ? ? ? ? 49 8B C1` |
| **Module**    | `client.dll` |
| **Resolves to** | `GetMatricesForView` — computes world-to-view, view-to-projection, world-to-projection matrices |
| **Search type** | `NONE` (direct match) |
| **Last verified** | 2024-XX-XX (placeholder) |

**How to find in IDA/Ghidra:**
1. Open `client.dll`.
2. Find `CViewRender::OnRenderStart` (search for `"OnRenderStart"` string).
3. Inside `OnRenderStart`, look for a call to a function that takes 5 parameters (this + 4 matrix pointers).
4. That called function is `GetMatricesForView`.
5. Its prologue is `push rbx` / `sub rsp, large_value` / `mov rax, r9`.

**Signature:**
```cpp
ViewMatrix* __fastcall GetMatrixForView(void* pRenderGameSystem, void* pViewRender,
    ViewMatrix* pOutW2V, ViewMatrix* pOutV2P, ViewMatrix* pOutW2P)
```

---

### LevelInit

| Field         | Value |
|---------------|-------|
| **Pattern**   | `48 89 5C 24 ? 56 48 83 EC ? 48 8B 0D ? ? ? ? 48 8B F2` |
| **Module**    | `client.dll` |
| **Resolves to** | Level initialization handler — called on map load |
| **Search type** | `NONE` (direct match) |
| **Last verified** | 2026-04-09 |

**How to find in IDA/Ghidra:**
1. Open `client.dll`.
2. Search for the string `"game_newmap"`.
3. The xref leads to a function that registers for the `"game_newmap"` event.
4. Look at the first function called in that context — it's the LevelInit handler.
5. Prologue: save rbx → push rsi → sub rsp → load global pointer → mov rsi, rdx (map name).

---

### Present Hook (Primary — GameOverlayRenderer)

| Field         | Value |
|---------------|-------|
| **Pattern**   | `48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 41 8B E8` |
| **Module**    | `gameoverlayrenderer64.dll` |
| **Resolves to** | Steam overlay's `Present` wrapper — the best hook site for D3D11 rendering |
| **Search type** | `NONE` (direct match) |
| **Last verified** | 2026-04-09 |

**Why use this instead of IDXGISwapChain vtable?**  
The Steam overlay already hooks `IDXGISwapChain::Present` internally. By hooking this Steam
wrapper directly, we avoid fighting with it for the vtable slot and get the `IDXGISwapChain*`
as the first argument (`rcx`) at call time.

**How to find in IDA/Ghidra:**
1. Open `gameoverlayrenderer64.dll` (loaded at Steam game startup).
2. Search for a function that saves `rbx, rbp, rsi, rdi, r12, r13, r14` and has `41 8B E8` near the prologue.
3. This is the wrapper that the overlay injected around `IDXGISwapChain::Present`.
4. Alternatively: xref from the DXGI vtable hook site in the DLL.

**What it looks like:**
```asm
48 89 5C 24 ??           ; mov [rsp+??], rbx
48 89 6C 24 ??           ; mov [rsp+??], rbp
56 57                    ; push rsi; push rdi
41 54 41 56 41 57        ; push r12; push r14; push r15
48 83 EC ??              ; sub rsp, ??
41 8B E8                 ; mov ebp, r8d    ; SyncInterval arg
```

---

### SetLocalPlayerReady

| Field         | Value |
|---------------|-------|
| **Pattern**   | `48 83 EC ? 4C 8B 05 ? ? ? ? 0F B6 D1` |
| **Module**    | `client.dll` |
| **Resolves to** | Marks local player as ready (used in buy phase / warmup) |
| **Search type** | `CALL` or `NONE` |
| **Last verified** | 2026-04-09 |

> **Pattern changed:** Previously `48 83 EC ? 48 8B 05 ? ? ? ? 0F B6 D1` using `mov rax, [rip+X]` (`48 8B 05`).
> In the current CS2 build the compiler uses `mov r8, [rip+X]` (`4C 8B 05`) — r8 instead of rax.

**How to find in IDA/Ghidra:**
1. Open `client.dll`.
2. Search for `"IsLocalPlayerReady"` or `"SetLocalPlayerReady"`.
3. The function takes a `bool` parameter (`cl` register), converts it with `movzx edx, cl`, loads a global object pointer into r8/rax, then calls a virtual function on it.
4. Function found at `client.dll+0x8CB420` in the current build.

**What it looks like:**
```asm
48 83 EC ??              ; sub rsp, ??          (prologue)
4C 8B 05 XX XX XX XX     ; mov r8, [rip+CCSGOInput]  (changed from 48 8B 05)
0F B6 D1                 ; movzx edx, cl        (bReady bool arg)
49 8B C8                 ; mov rcx, r8          (this ptr)
49 8B 00                 ; mov rax, [r8]        (load vtable)
FF 50 40                 ; call [rax+40h]       (virtual call)
```

---

| Field         | Value |
|---------------|-------|
| **Pattern**   | `48 83 EC ? 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? 45 33 C9 45 33 C0 48 8B 01 FF 50` |
| **Module**    | `client.dll` |
| **Resolves to** | Level shutdown handler — called on map unload |
| **Search type** | `NONE` (direct match) |
| **Last verified** | 2024-XX-XX (placeholder) |

**How to find in IDA/Ghidra:**
1. Open `client.dll`.
2. Search for the string `"map_shutdown"`.
3. The xref leads to `ClientModeShared` code that handles map shutdown.
4. The function loads a global pointer, sets up arguments (xor r9d,r9d / xor r8d,r8d), loads vtable, and calls a virtual function.

---

### OverrideView

| Field         | Value |
|---------------|-------|
| **Pattern**   | `48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B FA E8` |
| **Module**    | `client.dll` |
| **Resolves to** | `ClientModeCSNormal::OverrideView` — FOV / thirdperson camera control |
| **Search type** | `NONE` (direct match) |
| **Last verified** | 2024-XX-XX (placeholder) |

**How to find in IDA/Ghidra:**
1. Open `client.dll`.
2. Search for `"ClientModeCSNormal"` or look in the ClientMode vtable.
3. `OverrideView` is a virtual function — it takes `(this, CViewSetup*)`.
4. The prologue saves many registers (rbx, rbp, rsi, rdi, r14, r15) and calls a function immediately after setup (`E8` at the end of the pattern).
5. It modifies the view setup for FOV and third-person camera.

---

## Key Offsets

These are hardcoded offsets defined in `src/core/patterns.h` under `PATTERNS::OFFSETS::` that may change between game builds.

| Offset | Used in | Description | How to verify |
|--------|---------|-------------|---------------|
| `+0x58` | `interfaces.cpp` | `CGameEntitySystem*` inside `IGameResourceService` | In `engine2.dll`, find `GameResourceServiceClientV001` and check member at 0x58 — should be entity system ptr. |
| `+0x5C0` | `schema.cpp` | `CUtlTSHash` bucket array inside `CSchemaSystemTypeScope` | In `schemasystem.dll`, examine `FindTypeScopeForModule` return value; the CUtlMemoryPool header is 0x38 bytes at 0x588, so bucket array starts at 0x5C0. 256 buckets. |
| `+0x28` | `schema.cpp` | `pFields` pointer inside `SchemaClassInfoData_t` | Schema class field array. Was at 0x24 in older builds — verify by checking the struct layout in schemasystem.dll vtable dump. |
| `+0x8` | `schema.cpp` | `szBinaryName` in `CSchemaClassBinding` | Binary (module) name string. |
| `+0x18` | `schema.cpp` | `pFirst` in `HashBucket_t` | First node pointer in the TSHash bucket linked list. |

> **Removed:** `+0x170` (IDXGISwapChain* inside ISwapChainDx11) is no longer used — D3D device is
> resolved lazily inside the Present hook callback, not by reading from the swap chain object.

---

## How to Update a Pattern

### Step-by-step workflow

1. **Identify which pattern broke** — check the console log output. The cheat logs each pattern scan result. A `0` result means the pattern wasn't found.

2. **Open the correct module** in IDA Pro or Ghidra:
   - `client.dll` → most game hooks and SDK pointers
   - `engine2.dll` → engine interfaces and GlobalVars
   - `rendersystemdx11.dll` → D3D/DXGI swap chain

3. **Find the target** using the search method described above for each pattern:
   - Search for the reference string (e.g., `"game_newmap"`, `"CreateMove"`)
   - Follow xrefs to the target function
   - Identify the instruction or function prologue

4. **Extract the new pattern:**
   - Select the bytes at the target location
   - Replace bytes that change between builds (addresses, variable offsets) with `?` wildcards
   - Keep enough unique context bytes for a reliable match (typically 12-20 bytes)
   - Test that the pattern matches exactly once in the module

5. **Update the code:**
   - Edit `src/core/patterns.h` — this is the ONLY file that needs pattern/offset changes
   - Interface patterns → `PATTERNS::FUNCTIONS::` namespace
   - Hook patterns → `PATTERNS::FUNCTIONS::` namespace
   - Struct offsets → `PATTERNS::OFFSETS::` namespace

6. **Test:**
   - Rebuild and inject
   - Check console output — all patterns should resolve to non-zero addresses
   - Verify features work in-game

### Tips for writing good patterns

- **Start from the function prologue** when possible — these tend to be more stable across updates.
- **Wildcard variable data**: immediate values, stack offsets, and RIP displacements should be `?`.
- **Keep fixed opcodes**: register-to-register moves, push/pop, and well-known instruction prefixes rarely change.
- **Aim for 12+ non-wildcard bytes** to ensure uniqueness.
- **Verify uniqueness**: search the pattern in the module — it must match exactly once.

---

## CreateInterface Version Strings

These are defined in `src/core/patterns.h` under `PATTERNS::INTERFACES::` and must match the game build:

| Interface | Version String | Module |
|-----------|---------------|--------|
| `ISchemaSystem` | `SchemaSystem_001` | `schemasystem.dll` |
| `IInputSystem` | `InputSystemVersion001` | `inputsystem.dll` |
| `IEngineClient` | `Source2EngineToClient001` | `engine2.dll` |
| `ISource2Client` | `Source2Client002` | `client.dll` |
| `IEngineCVar` | `Source2EngineToClientStringTable001` | `engine2.dll` |
| `IGameResourceService` | `GameResourceServiceClientV001` | `engine2.dll` |

If a `CreateInterface` capture fails, the version string may have changed. Check the module's export table for `CreateInterface` and enumerate available interfaces to find the new version.

---

## VTable Indices

| Hook | VTable | Index | Notes |
|------|--------|-------|-------|
| `Present` | `IDXGISwapChain` | 8 | Standard DXGI, unlikely to change |
| `ResizeBuffers` | `IDXGISwapChain` | 13 | Standard DXGI, unlikely to change |
| `MouseInputEnabled` | `ISource2Client` | 19 | May shift on major updates |
| `CreateMove` | `ISource2Client` | 21 | Usually pattern-hooked instead |
| `FrameStageNotify` | `ISource2Client` | 36 | May shift on major updates |

---

*Last full update: 2026-04-09 — All 5 interface patterns + Present hook + SetLocalPlayerReady re-verified against current CS2 build.*
