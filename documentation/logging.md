# Logging

## Files

- API: `src\utilities\log.h`
- implementation: `src\utilities\log.cpp`
- startup hookup: `src\core\core.cpp`

## Output targets

- console window via `L::AttachConsole(...)`
- file via `L::OpenFile(L"gamerhack.log")`

The log file is currently created in:

`Documents\GamerHackCS2\gamerhack.log`

## Basic usage

```cpp
L_PRINT(LOG_INFO) << _XS("[MISC] initialized");
L_PRINT(LOG_WARNING) << _XS("[SDK_FUNC] FAILED: GetLocalPlayerController");
L_PRINT(LOG_ERROR) << _XS("[CORE] hook setup FAILED");
```

## Levels

- `LOG_INFO`
- `LOG_WARNING`
- `LOG_ERROR`

In Debug builds, `L_PRINT(...)` includes file and line automatically.

## Formatting helpers

`log.h` supports stream-style formatting flags.

Examples:

```cpp
L_PRINT(LOG_INFO) << L::AddFlags(LOG_MODE_BOOL_ALPHA) << true;
L_PRINT(LOG_INFO) << L::AddFlags(LOG_MODE_INT_HEX | LOG_MODE_SHOWBASE) << someValue;
```

## Logging rules for this project

- log lifecycle milestones
- log failures that explain why a system is unavailable
- log one-time diagnostics when chasing a bug
- do not spam every frame or every entity unless the log is explicitly temporary

## Good patterns

### One-time warning

```cpp
static bool bLogged = false;
if (!bLogged)
{
    bLogged = true;
    L_PRINT(LOG_WARNING) << _XS("[ESP] weapon draw faulted");
}
```

### First-call confirmation

```cpp
static bool bFirstCall = true;
if (bFirstCall)
{
    bFirstCall = false;
    L_PRINT(LOG_INFO) << _XS("[LEGITBOT] OnCreateMove first call");
}
```

## What to avoid

- noisy logs in entity loops
- logs that do not name the subsystem
- swallowing failures without any signal
- leaving temporary flood logs in finished code
