# Build and Deploy

## Build scripts in the repo

- `build_debug.bat` -> calls `build.bat debug`
- `build_release.bat` -> calls `build.bat release`
- `move_debug.bat` -> moves the Debug DLL to the injector folder
- `move_release.bat` -> moves the Release DLL to the injector folder

## Manual build commands

### Debug

```powershell
Set-Location C:\Users\JAgro\source\repos\BenisCS2\GamerHackCS2
cmake --build build --config Debug
```

Output:

`build\bin\Debug\GamerHackCS2.dll`

### Release

```powershell
Set-Location C:\Users\JAgro\source\repos\BenisCS2\GamerHackCS2
cmake --build build --config Release
```

Output:

`build\bin\Release\GamerHackCS2.dll`

## Deployment scripts

### Debug

`move_debug.bat` currently moves:

- from: `C:\Users\JAgro\source\repos\BenisCS2\GamerHackCS2\build\bin\Debug\GamerHackCS2.dll`
- to: `C:\Users\JAgro\OneDrive\Documents\Game Hacking\dlls`

### Release

`move_release.bat` currently moves:

- from: `C:\Users\JAgro\source\repos\BenisCS2\GamerHackCS2\build\bin\Release\GamerHackCS2.dll`
- to: `C:\Users\JAgro\OneDrive\Documents\Game Hacking\dlls`

## When to update these scripts

Update the move scripts if:

- the repo path changes
- the injector folder changes
- the DLL name changes

## Common build/deploy issues

- new source file not added to `CMakeLists.txt`
- stale build files after heavy refactors
- wrong configuration deployed
- move script still points to an old directory

## Recommended workflow after code changes

1. build Debug
2. run `move_debug.bat`
3. inject
4. inspect logs
5. only switch to Release after the change is stable
