# Architecture

## Startup flow

1. `src\dllmain.cpp`
   - waits for `client.dll` and `engine2.dll`
   - starts `CORE::Setup`
   - polls `VK_DELETE` to unload
2. `src\core\core.cpp`
   - starts logging first
   - captures interfaces
   - resolves SDK function pointers
   - initializes schema
   - initializes config
   - initializes input
   - leaves render initialization for first Present
   - initializes features
   - installs hooks last
3. Hooks dispatch into feature entrypoints
   - Present -> `F::OnPresent`
   - CreateMove -> `F::OnCreateMove`
   - FrameStageNotify -> `F::OnFrameStageNotify`
   - OverrideView -> `F::OnOverrideView`

## Core folders

- `src\core\`
  - bootstrapping, interfaces, hooks, schema, config, patterns, menu
- `src\sdk\`
  - entity wrappers, interfaces, function declarations, datatypes, econ types
- `src\utilities\`
  - render, memory, trace, input, logging, bones
- `src\features\`
  - gameplay and visual feature logic

## Feature structure standard

Each feature folder should follow the same pattern:

1. Keep the public header stable (`feature.h`)
2. Keep the public cpp thin (`feature.cpp`)
3. Put real logic in focused internal files
4. Wire all new `.cpp` files into `CMakeLists.txt`

Example:

```text
src\features\misc\
  misc.h
  misc.cpp              # public coordinator
  misc_core.cpp         # setup/destroy/present
  misc_movement.cpp     # CreateMove movement features
  misc_effects.cpp      # FrameStage features
  misc_camera.cpp       # OverrideView work
```

## Config architecture

- Settings are declared in `src\core\variables.h`
- Registration happens through `C_ADD_VARIABLE(...)` and related macros
- Values are stored in `C::mapVariables`
- Save/load lives in `src\core\config.cpp`
- Config JSON keys are **hash values**, not human-readable names

### Important consequence

Renaming a variable changes its hash and breaks compatibility with old configs for that setting. Prefer adding new variables over renaming old ones unless you are fine with config migration loss.

## Schema and SDK architecture

- `src\core\schema.cpp`
  - resolves runtime offsets from the schema system
  - maintains hardcoded fallback offsets
- `src\sdk\entity.h`
  - exposes most schema-backed entity accessors
- `src\sdk\functionlist.h/.cpp`
  - stores pattern-resolved game function pointers
- `src\core\patterns.h`
  - single source of truth for most signatures and static offsets

## Render architecture

- `src\utilities\render.cpp`
  - owns ImGui draw backend
  - provides `D::Draw*` helpers
  - provides `D::WorldToScreen`
- Visual features should only issue draw calls through the `D::` layer

## Maintenance rule of thumb

If you are adding a new behavior:

- settings -> `variables.h`
- UI -> `menu.cpp`
- runtime logic -> matching feature folder
- game field -> schema/entity wrapper or SDK declaration
- game function -> `functionlist.h/.cpp` and `patterns.h`
