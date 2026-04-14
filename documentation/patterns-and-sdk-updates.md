# Patterns and SDK Updates

This is the maintenance guide for game updates.

## When CS2 updates, check these layers in order

1. `src\core\patterns.h`
2. `src\sdk\functionlist.cpp`
3. `src\core\schema.cpp`
4. schema-backed accessors in `src\sdk\entity.h`
5. feature behavior that depends on those functions or offsets

## Updating a pattern

### Files involved

- declarations/constants: `src\core\patterns.h`
- pattern resolution: `src\sdk\functionlist.cpp`
- function pointer storage: `src\sdk\functionlist.h`

### Existing pattern flow

1. add or edit the signature in `PATTERNS::FUNCTIONS`
2. resolve it in `SDK_FUNC::Initialize()` with `RESOLVE(...)`
3. use the function pointer from `SDK_FUNC::<Name>`

### Adding a new SDK function pointer

1. Add a declaration in `src\sdk\functionlist.h`

```cpp
DECLARE_SDK_FUNCTION(ReturnType, MyFunction, Arg1, Arg2);
```

2. Add the signature in `src\core\patterns.h`

```cpp
constexpr const char* MY_FUNCTION = "48 89 ...";
```

3. Resolve it in `src\sdk\functionlist.cpp`

```cpp
RESOLVE(MyFunction, CLIENT, PATTERNS::FUNCTIONS::MY_FUNCTION);
```

4. Check logs for a successful resolved address

## Updating schema-backed SDK fields

### Files involved

- runtime schema system: `src\core\schema.cpp`
- schema API: `src\core\schema.h`
- field wrappers/accessors: `src\sdk\entity.h`

### Normal path

Use schema-backed field access wherever possible instead of hardcoding offsets in feature code.

### Fallback path

If runtime schema resolution misses a field or a build is unstable, add a fallback in `s_arrFallbackOffsets` in `src\core\schema.cpp`.

Example format:

```cpp
{"C_CSPlayerPawn->m_bIsScoped", 0x26F8},
```

### Adding a new entity field wrapper

1. Find the class and field name
2. Confirm the schema name exactly matches the game data
3. Add an accessor in `src\sdk\entity.h`
4. Add a fallback offset in `schema.cpp` if the field is important enough to keep the project usable when runtime schema lookup fails

## Static offsets and non-schema constants

`src\core\patterns.h` also contains `PATTERNS::OFFSETS`. Use that section for static non-schema offsets that belong with the pattern system.

## Recommended update workflow after a game patch

1. Build the project
2. Inject and open the log
3. Look for `[SDK_FUNC] FAILED` lines
4. Fix broken signatures first
5. If features still misbehave, inspect schema-dependent fields
6. Update fallback offsets if runtime schema no longer fills a required field
7. Rebuild and verify again

## What success looks like

- core setup completes
- hooks install
- expected `SDK_FUNC` entries resolve
- no obvious feature exceptions in the log
- core gameplay features behave normally in match

## Useful references already in the repo

- `PATTERNS.md`
- `src\core\patterns.h` comments
- `src\core\schema.h` comments
- `src\core\schema.cpp` fallback offset table
