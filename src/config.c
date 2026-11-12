#include "config.h"

Tuning tuning = {
    .start_hp         = 10,
    .start_max_hp     = 10,
    .start_speed      = 270.0f,
    .start_fire_rate  = 3.0f,
    .start_damage     = 1.0f,
    .start_shot_speed = 480.0f,
    .start_shot_range = 340.0f,

    .hp_cap = 100,

    .player_accel_time = 0.05f,
    .player_decel_time = 0.07f,
    .player_hitbox_w   = 30.0f,
    .player_hitbox_h   = 24.0f,
    .player_invuln_time = 1.0f,
    .player_blink_hz    = 10.0f,

    .shot_size     = 10.0f,
    .shot_height   = 34.0f,
    .shot_inherit  = 0.5f,
    .sing_duration = 0.12f,
    .shot_knockback = 170.0f,

    .baby_hp             = 5.0f,
    .baby_speed          = 115.0f,
    .baby_accel_time     = 0.35f,
    .baby_hitbox_w       = 26.0f,
    .baby_hitbox_h       = 20.0f,
    .baby_contact_damage = 1,
    .enemy_hurt_flash    = 0.12f,
    .enemy_spawn_min_dist = 260.0f,

    .floor_min_rooms            = 10,
    .floor_max_rooms            = 12,
    .floor_big_room_chance      = 0.35f,
    .floor_min_big_rooms        = 1,
    .room_max_babies            = 4,
    .room_babies_per_extra_cell = 2,
    .boss_room_babies           = 6,
    .room_transition_time       = 0.30f,

    .hud_bar_base_len   = 120.0f,
    .hud_bar_len_per_hp = 2.5f,
    .hud_bar_drain_time = 0.30f,
};
