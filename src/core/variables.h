#pragma once
#include "config.h"

// ---------------------------------------------------------------
// cheat configuration variables
// registered via C_ADD_VARIABLE macros using hash-based storage
// ---------------------------------------------------------------

// ---- menu / UI ----
C_ADD_VARIABLE(bool,  menu_particles,        true);
C_ADD_VARIABLE(bool,  menu_dim_background,   true);
C_ADD_VARIABLE(bool,  menu_glow_effect,      true);
C_ADD_VARIABLE(float, menu_animation_speed,  1.0f);    // 0.5 to 3.0
C_ADD_VARIABLE(int,   menu_dpi_scale,        0);       // 0=100%, 1=125%, 2=150%, 3=175%, 4=200%
C_ADD_VARIABLE(bool,  menu_watermark,        true);

// accent colors
C_ADD_VARIABLE(Color, menu_accent_color,     Color(85, 90, 160));
C_ADD_VARIABLE(Color, menu_accent_hover,     Color(100, 105, 175));
C_ADD_VARIABLE(Color, menu_accent_active,    Color(115, 120, 190));

// ---- aimbot ----
C_ADD_VARIABLE(bool,  aimbot_enabled,      false);
C_ADD_VARIABLE(float, aimbot_fov,          5.0f);
C_ADD_VARIABLE(float, aimbot_smooth,       3.0f);
C_ADD_VARIABLE(int,   aimbot_bone,         6);       // head bone index
C_ADD_VARIABLE(bool,  aimbot_visible_only, true);
C_ADD_VARIABLE(int,   aimbot_key,          0x05);    // VK_XBUTTON1 (Mouse4)

// ---- ESP ----
C_ADD_VARIABLE(bool,  esp_enabled,     false);
C_ADD_VARIABLE(bool,  esp_box,         true);
C_ADD_VARIABLE(bool,  esp_name,        true);
C_ADD_VARIABLE(bool,  esp_health,      true);
C_ADD_VARIABLE(bool,  esp_armor,       false);
C_ADD_VARIABLE(bool,  esp_weapon,      true);
C_ADD_VARIABLE(bool,  esp_skeleton,    false);
C_ADD_VARIABLE(bool,  esp_snaplines,   false);
C_ADD_VARIABLE(int,   esp_box_type,    0);           // 0 = normal, 1 = corner
C_ADD_VARIABLE(Color, esp_box_color_t,  Color(255, 0, 0));
C_ADD_VARIABLE(Color, esp_box_color_ct, Color(0, 100, 255));

// ---- glow ----
C_ADD_VARIABLE(bool,  glow_enabled,   false);
C_ADD_VARIABLE(Color, glow_color_t,   Color(255, 50, 50));
C_ADD_VARIABLE(Color, glow_color_ct,  Color(50, 50, 255));

// ---- misc ----
C_ADD_VARIABLE(bool,  misc_bhop,                 false);
C_ADD_VARIABLE(bool,  misc_autostrafe,           false);
C_ADD_VARIABLE(bool,  misc_noflash,              false);
C_ADD_VARIABLE(float, misc_noflash_alpha,        0.0f);
C_ADD_VARIABLE(bool,  misc_thirdperson,          false);
C_ADD_VARIABLE(float, misc_thirdperson_distance, 150.0f);
C_ADD_VARIABLE(float, misc_fov_changer,          90.0f);

// ---- inventory ----
C_ADD_VARIABLE(bool, inventory_enabled, false);
