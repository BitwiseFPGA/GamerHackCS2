# Adding Settings and Features

## Adding a new setting

### 1. Register the setting

Add it in `src\core\variables.h`.

Examples:

```cpp
C_ADD_VARIABLE(bool,  misc_example_toggle, false);
C_ADD_VARIABLE(int,   misc_example_mode,   0);
C_ADD_VARIABLE(float, misc_example_value,  25.0f);
C_ADD_VARIABLE(Color, misc_example_color,  Color(255, 255, 255, 255));
```

### 2. Use the setting in code

Read it with `C::Get<T>(setting_name)`.

```cpp
if (!C::Get<bool>(misc_example_toggle))
    return;

const float flValue = C::Get<float>(misc_example_value);
```

### 3. Expose it in the menu

Edit `src\core\menu.cpp` and add the control in the right section.

Examples:

```cpp
MENU::Checkbox("Example", &C::Get<bool>(misc_example_toggle));
MENU::SliderFloat("Example Value", &C::Get<float>(misc_example_value), 0.0f, 100.0f);
MENU::ColorEdit("Example Color", &C::Get<Color>(misc_example_color));
```

### 4. Verify save/load

Config serialization is automatic as long as the variable is registered in `variables.h`.

## Adding a new feature to an existing feature section

### Recommended workflow

1. Pick the right feature folder
2. Put the implementation in a focused internal file
3. Keep the public `feature.cpp` thin
4. Add the `.cpp` file to `CMakeLists.txt`
5. Add settings in `variables.h`
6. Add menu controls in `menu.cpp`
7. Dispatch from `src\features\features.cpp` if a new hook entrypoint is needed

### Example: adding a new misc CreateMove feature

1. Add settings in `variables.h`
2. Add UI in the Misc tab in `menu.cpp`
3. Implement logic in `src\features\misc\misc_movement.cpp`
4. Keep `misc.cpp` unchanged unless you need a new public entrypoint

## Adding a brand new feature section

### Minimum steps

1. Create `src\features\<name>\<name>.h`
2. Create `src\features\<name>\<name>.cpp` as the public coordinator
3. Split real logic into internal files from the start
4. Include the feature header in `src\features\features.cpp`
5. Call its `Setup`, `Destroy`, and hook handlers from `features.cpp`
6. Add all new `.cpp` files to `CMakeLists.txt`
7. Add menu/config integration if the feature is user-controlled

## Best practices

- Keep per-tick logic guarded by config and game state checks early
- Reuse existing utilities before adding new helpers
- Prefer schema/entity wrappers over raw offsets in feature code
- Prefer one-time logs or gated diagnostics over spammy every-frame logs
- Do not bury multiple unrelated behaviors in one file

## Common mistakes

- forgetting `CMakeLists.txt`
- renaming a variable and unexpectedly breaking old config values
- adding runtime logic without a menu/config path
- reading raw game memory directly in feature code when an entity helper already exists
- putting everything in the public coordinator instead of an internal module
