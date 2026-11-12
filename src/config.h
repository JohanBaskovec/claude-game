#ifndef FUGUE_CONFIG_H
#define FUGUE_CONFIG_H

#include <stdbool.h>

/* Résolution logique du jeu (GDD §10). Mise à l'échelle par facteur entier. */
#define GAME_W 960
#define GAME_H 540

/* Logique à pas de temps fixe : 60 mises à jour par seconde. */
#define TICK_HZ 60

/* Nombre maximal de projectiles simultanés (pool statique, pas de malloc). */
#define MAX_PROJECTILES 256

/* Nombre maximal d'ennemis simultanés (pool statique). */
#define MAX_ENEMIES 32

/* Autres pools statiques. */
#define MAX_PICKUPS  16
#define MAX_BOMBS    16
#define MAX_HAZARDS  48

/* Armes (GDD §4 et §14). */
typedef enum WeaponId {
    WEAPON_VOICE,
    WEAPON_OLIFANT,
    WEAPON_FLUTE,
    WEAPON_LYRE,
    WEAPON_TABOURIN,
    WEAPON_BELL,
    WEAPON_ORGAN,
    WEAPON_COUNT
} WeaponId;

/* Forme d'un tir. */
typedef enum ShotKind {
    SHOT_NOTE,   /* note qui vole et disparaît au premier ennemi touché */
    SHOT_WAVE,   /* onde de choc en arc : avance et traverse les ennemis */
    SHOT_RING,   /* onde circulaire autour d'Amadeus */
    SHOT_BEAM,   /* rayon instantané jusqu'au mur, traverse tout */
} ShotKind;

/* Attributs d'une arme : multiplient ceux du joueur (départ + objets). */
typedef struct WeaponTuning {
    ShotKind kind;
    float damage_mul;
    float fire_rate_mul;
    float shot_speed_mul;
    float range_mul;
    float size;        /* taille de la note (1 = voix) */
    int   count;       /* notes par tir */
    float spread;      /* angle total de l'éventail, rad */
    float stun;        /* étourdit l'ennemi touché, s */
    float lock_time;   /* joueur cloué sur place après chaque tir, s */
    bool  charge;      /* tir à charger (flûte) */
} WeaponTuning;

/*
 * Tous les paramètres de gameplay à ajuster, au même endroit.
 * Les valeurs par défaut sont dans config.c.
 * Unités : pixels du jeu (960x540) et secondes.
 */
typedef struct Tuning {
    /* Valeurs de départ des attributs du joueur (GDD §4).
     * Le joueur en garde une copie qu'objets et menu de debug modifient. */
    int   start_hp;            /* points de vie actuels */
    int   start_max_hp;        /* points de vie maximum */
    float start_speed;         /* vitesse de déplacement, px/s */
    float start_fire_rate;     /* cadence de tir, notes/s */
    float start_damage;        /* dégâts de chaque note, PV ennemis */
    float start_shot_speed;    /* vitesse des tirs, px/s */
    float start_shot_range;    /* portée des tirs : distance avant disparition, px */
    int   start_bombs;         /* métronomes */
    int   start_coins;         /* royalties */

    /* Limites des attributs */
    int   hp_cap;              /* plafond absolu des PV maximum */

    /* Joueur : sensations de déplacement */
    float player_accel_time;   /* temps pour atteindre la vitesse max, s (0 = instantané) */
    float player_decel_time;   /* temps pour s'arrêter, s (0 = arrêt net) */
    float player_hitbox_w;     /* boîte de collision, px */
    float player_hitbox_h;
    float player_invuln_time;  /* invincibilité après un coup, s */
    float player_blink_hz;     /* clignotements par seconde pendant l'invincibilité */

    /* Tir */
    float shot_size;           /* boîte de collision du projectile (carrée), px */
    float shot_height;         /* hauteur de vol au-dessus du sol (affichage), px */
    float shot_inherit;        /* part de la vitesse du joueur transmise au projectile (0 = aucune) */
    float sing_duration;       /* durée de la bouche ouverte après un tir, s */
    float shot_knockback;      /* poussée donnée à l'ennemi touché, px/s */

    /* Bébé médiéval (GDD §4) */
    float baby_hp;
    float baby_speed;          /* px/s */
    float baby_accel_time;     /* inertie : temps pour changer de direction, s */
    float baby_hitbox_w;
    float baby_hitbox_h;
    int   baby_contact_damage; /* PV retirés au joueur par contact */
    float enemy_hurt_flash;    /* durée du clignotement quand un ennemi est touché, s */
    float enemy_spawn_min_dist;/* distance minimale au joueur pour apparaître, px */

    /* Étage (GDD §12) */
    int   floor_min_rooms;
    int   floor_max_rooms;
    float floor_big_room_chance;      /* probabilité de tenter une grande salle */
    int   floor_min_big_rooms;        /* au moins tant de grandes salles par étage */
    int   room_max_babies;            /* salle normale : 0 à N bébés ... */
    int   room_babies_per_extra_cell; /* ... + tant par cellule supplémentaire */
    int   boss_room_babies;           /* bébés en plus du boss */
    float room_transition_time;       /* glissement d'une salle à l'autre, s */

    /* Prix de fin de salle (GDD §13) */
    float reward_chance;              /* probabilité d'un prix quand on vide une salle */
    float pickup_magnet_delay;        /* temps avant que le prix soit aspiré, s */
    float pickup_magnet_accel;        /* accélération vers le joueur, px/s² */
    float pickup_magnet_max_speed;    /* px/s */
    float pickup_collect_dist;        /* ramassé quand il est à moins de tant du joueur, px */

    /* Métronome (bombe) */
    float bomb_fuse;                  /* durée du tic-tac, s */
    float bomb_radius;                /* rayon de l'explosion, px */
    float bomb_damage;                /* dégâts aux ennemis */
    int   bomb_player_damage;         /* PV retirés au joueur */
    float bomb_knockback;             /* poussée des ennemis, px/s */

    /* Gros Bébé, boss du premier étage */
    float boss_hp;
    float boss_speed;                 /* errance, px/s */
    float boss_hitbox_w;
    float boss_hitbox_h;
    float boss_jump_min_delay;        /* entre deux sauts, s */
    float boss_jump_max_delay;
    float boss_crouch_time;           /* s */
    float boss_air_time;              /* s */
    float boss_recover_time;          /* s */
    int   boss_contact_damage;
    int   boss_bricks;                /* briques qui tombent à chaque atterrissage */
    float brick_fall_min;             /* temps de chute d'une brique, s */
    float brick_fall_max;
    float brick_radius;               /* zone de dégâts, px */
    int   brick_damage;
    float shake_time;                 /* tremblement de l'écran à l'atterrissage, s */
    float shake_amplitude;            /* px */
    float trapdoor_radius;            /* le centre du joueur à moins de tant = on tombe, px */

    /* Salle du trésor et objets (effets provisoires, GDD §13) */
    float item_banner_time;           /* nom et effet affichés, s */
    float item_lully_damage;
    int   item_lully_foot_chance;     /* une note sur N frappe le pied */
    int   item_lully_foot_damage;
    int   item_haydn_max_hp;          /* + PV max, et autant de soin */
    float item_satie_fire_rate;
    float item_guido_shot_speed;
    float item_guido_shot_range;
    float item_sabots_speed;
    float item_celeriter_fire_rate;   /* c : cadence + */
    float expectare_at;               /* x : la note s'arrête à cette fraction de sa portée ... */
    float expectare_pause;            /* ... pendant tant de s */
    float altius_height_mul;          /* a : les notes volent plus haut (affichage) */
    float trope_spread;               /* Trope : angle ajouté par note en plus, rad */
    float ink_time;                   /* Liquescence : durée de la flaque, s */
    float ink_radius;                 /* px */
    float ink_slow;                   /* vitesse des ennemis dans l'encre (fraction) */

    /* Armes (GDD §4) */
    WeaponTuning weapons[WEAPON_COUNT];
    float flute_charge_time;          /* charge complète à la cadence de départ, s */
    float flute_min_damage;           /* multiplicateur de dégâts sans charge ... */
    float flute_max_damage;           /* ... et à pleine charge */
    float flute_max_size;             /* taille de la note à pleine charge */
    float flute_recover;              /* pause après avoir relâché, s */
    float wave_width;                 /* largeur de l'onde du tabourin au départ, px ... */
    float wave_growth;                /* ... + tant par px parcouru */
    float ring_grow_time;             /* la cloche résonne jusqu'à sa portée en tant de s */
    float beam_time;                  /* durée d'affichage du rayon de l'orgue, s */
    float beam_width;                 /* px */
    float stun_star_hz;               /* étoiles des ennemis étourdis, tours/s */

    /* Barre de vie (soufflet de crescendo) */
    float hud_bar_base_len;    /* longueur du soufflet, px ... */
    float hud_bar_len_per_hp;  /* ... + tant de px par PV maximum */
    float hud_bar_drain_time;  /* constante de temps de la descente animée, s */
} Tuning;

extern Tuning tuning;

#endif
