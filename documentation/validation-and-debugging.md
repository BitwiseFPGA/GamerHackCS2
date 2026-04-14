# Validation and Debugging

## Normal debug workflow

1. Build Debug

```powershell
Set-Location C:\Users\JAgro\source\repos\BenisCS2\GamerHackCS2
cmake --build build --config Debug
```

2. Deploy the DLL

```powershell
C:\Users\JAgro\source\repos\BenisCS2\move_debug.bat
```

3. Inject and watch the console/log output

## Release workflow

```powershell
Set-Location C:\Users\JAgro\source\repos\BenisCS2\GamerHackCS2
cmake --build build --config Release
C:\Users\JAgro\source\repos\BenisCS2\move_release.bat
```

## What to verify first

### Startup

Look for these stages in the log:

- logging started
- interfaces OK
- SDK functions initialized
- schema initialized
- config initialized
- features initialized
- hooks OK
- initialization complete

These stages come from `src\core\core.cpp`.

### Runtime

Check:

- menu opens with `INSERT`
- unload works with `DELETE`
- no constant exception spam
- config saves and loads
- the feature you changed only runs in the right game states

## Where to look when something breaks

### Build error

- missing include
- missing namespace qualification
- source not added to `CMakeLists.txt`
- mismatch between public header and split internal files

### Feature does nothing

- setting default is off in `variables.h`
- menu path was never added
- feature is not dispatched from `features.cpp`
- early return from `IsInGame`, null controller, null pawn, or config guard
- needed pattern/function pointer failed to resolve

### Feature crashes or faults

- stale schema field
- stale pattern
- incorrect entity type or handle path
- accessing game state in wrong hook stage

## Current output locations

- logs: `Documents\GamerHackCS2\gamerhack.log`
- configs: `Documents\GamerHack\default.json`

Note the directories are different.

## Good debugging habits in this project

- use Debug builds first
- add one-time diagnostics instead of every-tick spam
- log pointer resolution and important early-return reasons
- remove temporary noisy logs once the issue is solved
- validate game-state guards before blaming rendering or hooks

## Quick checklist before calling a change done

1. It builds in Debug
2. The DLL is deployed with the move script
3. The project initializes cleanly
4. The changed feature actually runs
5. No new exception log spam appears
