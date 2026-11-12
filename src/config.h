#ifndef FUGUE_CONFIG_H
#define FUGUE_CONFIG_H

/* Résolution logique du jeu (GDD §10). Mise à l'échelle par facteur entier. */
#define GAME_W 960
#define GAME_H 540

/* Logique à pas de temps fixe : 60 mises à jour par seconde. */
#define TICK_HZ 60

/* Nombre maximal de projectiles simultanés (pool statique, pas de malloc). */
#define MAX_PROJECTILES 256

/* Nombre maximal d'ennemis simultanés (pool statique). */
#define MAX_ENEMIES 32

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
    int   boss_room_babies;
    float room_transition_time;       /* glissement d'une salle à l'autre, s */

    /* Barre de vie (soufflet de crescendo) */
    float hud_bar_base_len;    /* longueur du soufflet, px ... */
    float hud_bar_len_per_hp;  /* ... + tant de px par PV maximum */
    float hud_bar_drain_time;  /* constante de temps de la descente animée, s */
} Tuning;

extern Tuning tuning;

#endif
