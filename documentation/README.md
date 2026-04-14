# GamerHackCS2 Documentation

This folder is the maintainer handbook for the project.

## Start here

1. `architecture.md` - how the project is laid out and how code flows from startup to feature execution
2. `adding-settings-and-features.md` - how to add config settings, menu controls, and new feature logic
3. `patterns-and-sdk-updates.md` - how to update patterns, schema-backed fields, and SDK wrappers after a game update
4. `validation-and-debugging.md` - how to build, deploy, verify startup, and diagnose failures
5. `logging.md` - how the logging system works and how to use it without spamming
6. `build-and-deploy.md` - build commands, scripts, and DLL deployment paths

## Important project facts

- The main entrypoint is `src\dllmain.cpp`
- Startup and shutdown orchestration lives in `src\core\core.cpp`
- Feature dispatch lives in `src\features\features.cpp`
- Feature folders now use a thin public coordinator plus focused internal modules
- Config variables are registered in `src\core\variables.h`
- Config files are saved to **Documents\\GamerHack\\**
- Logs are written to **Documents\\GamerHackCS2\\gamerhack.log**

## Current feature module layout

- `src\features\bypass\`
  - `bypass.cpp` public coordinator
  - `bypass_lifecycle.cpp`, `bypass_create_move.cpp`, `bypass_actions.cpp`, `bypass_crc.cpp`
- `src\features\legitbot\`
  - `legitbot.cpp` public coordinator
  - `legitbot_core.cpp`, `legitbot_math.cpp`, `legitbot_targeting.cpp`, `legitbot_aim.cpp`
- `src\features\misc\`
  - `misc.cpp` public coordinator
  - `misc_core.cpp`, `misc_movement.cpp`, `misc_effects.cpp`, `misc_camera.cpp`
- `src\features\inventory\`
  - `inventory.cpp` public coordinator
  - `inventory_state.cpp`, `inventory_runtime.cpp`, `inventory_actions.cpp`
- `src\features\visuals\`
  - `visuals.cpp` public coordinator
  - `esp.cpp`, `radar.cpp`, `sniper_crosshair.cpp`

Use this split as the default pattern for new feature work.
