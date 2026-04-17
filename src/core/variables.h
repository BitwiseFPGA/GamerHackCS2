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
C_ADD_VARIABLE(bool,  aimbot_always_on,    false);
C_ADD_VARIABLE(int,   aimbot_target_filter, 0);      // 0=closest angle, 1=lowest health, 2=closest distance
C_ADD_VARIABLE(bool,  aimbot_rcs,          false);    // recoil compensation
C_ADD_VARIABLE(bool,  aimbot_team_check,   true);     // only target enemies (skip teammates)

// ---- ragebot ----
C_ADD_VARIABLE(bool,  rage_enabled,          false);
C_ADD_VARIABLE(int,   rage_key,              0x05);    // VK_XBUTTON1 (Mouse4)
C_ADD_VARIABLE(bool,  rage_always_on,        false);
C_ADD_VARIABLE(bool,  rage_silent,           true);
C_ADD_VARIABLE(bool,  rage_auto_shoot,       true);
C_ADD_VARIABLE(bool,  rage_auto_stop,        true);
C_ADD_VARIABLE(bool,  rage_auto_scope,       true);
C_ADD_VARIABLE(bool,  rage_team_check,       true);
C_ADD_VARIABLE(float, rage_min_damage,       10.0f);   // minimum damage to shoot
C_ADD_VARIABLE(bool,  rage_multipoint,       true);
C_ADD_VARIABLE(float, rage_multipoint_scale, 70.0f);   // 0-100 point spread scale
C_ADD_VARIABLE(bool,  rage_hitbox_head,      true);
C_ADD_VARIABLE(bool,  rage_hitbox_chest,     true);
C_ADD_VARIABLE(bool,  rage_hitbox_stomach,   false);
C_ADD_VARIABLE(bool,  rage_hitbox_pelvis,    false);

// ---- triggerbot ----
C_ADD_VARIABLE(bool,  triggerbot_enabled,       false);
C_ADD_VARIABLE(int,   triggerbot_key,           0x06);   // VK_XBUTTON2 (Mouse5)
C_ADD_VARIABLE(bool,  triggerbot_always_on,     false);
C_ADD_VARIABLE(bool,  triggerbot_team_check,    true);
C_ADD_VARIABLE(bool,  triggerbot_visible_only,  true);
C_ADD_VARIABLE(float, triggerbot_delay,         0.0f);   // ms delay before firing
C_ADD_VARIABLE(float, triggerbot_delay_rand,    0.0f);   // random ms added to delay
C_ADD_VARIABLE(int,   triggerbot_hitgroup,      0);      // 0=any, 1=head, 2=chest, 3=stomach
C_ADD_VARIABLE(int,   triggerbot_hold_min,      1);      // minimum ticks to hold fire
C_ADD_VARIABLE(int,   triggerbot_hold_max,      4);      // maximum ticks to hold fire

// ---- ESP ----
C_ADD_VARIABLE(bool,  esp_enabled,     false);
C_ADD_VARIABLE(bool,  esp_team,        false);
C_ADD_VARIABLE(bool,  esp_box,         true);
C_ADD_VARIABLE(bool,  esp_name,        true);
C_ADD_VARIABLE(bool,  esp_health,      true);
C_ADD_VARIABLE(bool,  esp_armor,       false);
C_ADD_VARIABLE(bool,  esp_weapon,      true);
C_ADD_VARIABLE(bool,  esp_dropped_items, false);
C_ADD_VARIABLE(bool,  esp_snaplines,   false);
C_ADD_VARIABLE(int,   esp_box_type,    0);           // 0 = normal, 1 = corner
C_ADD_VARIABLE(Color, esp_box_color_t,  Color(255, 0, 0));
C_ADD_VARIABLE(Color, esp_box_color_ct, Color(0, 100, 255));
C_ADD_VARIABLE(Color, esp_name_color,   Color(255, 255, 255, 230));
C_ADD_VARIABLE(Color, esp_weapon_color, Color(220, 220, 220, 210));
C_ADD_VARIABLE(Color, esp_dropped_item_color, Color(255, 220, 140, 230));
C_ADD_VARIABLE(Color, esp_armor_color,  Color(0, 128, 255, 255));
C_ADD_VARIABLE(Color, esp_snapline_color_t,  Color(255, 100, 100, 160));
C_ADD_VARIABLE(Color, esp_snapline_color_ct, Color(100, 170, 255, 160));
C_ADD_VARIABLE(bool, esp_skeleton, false);
C_ADD_VARIABLE(Color, esp_skeleton_color_t, Color(255, 150, 150));
C_ADD_VARIABLE(Color, esp_skeleton_color_ct, Color(150, 150, 255));
C_ADD_VARIABLE(float, esp_skeleton_thickness, 1.5f);

// ---- glow ----
C_ADD_VARIABLE(bool,  glow_enabled,   false);
C_ADD_VARIABLE(Color, glow_color_t,   Color(255, 50, 50));
C_ADD_VARIABLE(Color, glow_color_ct,  Color(50, 50, 255));

// ---- misc ----
C_ADD_VARIABLE(bool,  misc_bhop,                 false);
C_ADD_VARIABLE(bool,  misc_autostrafe,           false);
C_ADD_VARIABLE(float, misc_strafe_smooth,        50.0f);  // 1-100, smoothness of air strafe
C_ADD_VARIABLE(bool,  misc_auto_stop,            false);  // standalone auto stop (also triggered by ragebot)
C_ADD_VARIABLE(bool,  misc_noflash,              false);
C_ADD_VARIABLE(float, misc_noflash_alpha,        0.0f);
C_ADD_VARIABLE(bool,  misc_thirdperson,          false);
C_ADD_VARIABLE(float, misc_thirdperson_distance, 150.0f);
C_ADD_VARIABLE(float, misc_fov_changer,          90.0f);
C_ADD_VARIABLE(bool,  misc_sniper_crosshair,     false);
C_ADD_VARIABLE(bool,  misc_radar_enabled,        false);
C_ADD_VARIABLE(float, misc_radar_size,           120.0f);  // radar circle diameter
C_ADD_VARIABLE(float, misc_radar_range,          1500.0f); // world units range
C_ADD_VARIABLE(float, misc_radar_pos_x,          1.0f);    // horizontal position as screen %
C_ADD_VARIABLE(float, misc_radar_pos_y,          1.0f);    // vertical position as screen %

// ---- inventory ----
C_ADD_VARIABLE(bool, inventory_enabled, false);
