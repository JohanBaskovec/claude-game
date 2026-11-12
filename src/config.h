#ifndef FUGUE_CONFIG_H
#define FUGUE_CONFIG_H

/* Résolution logique du jeu (GDD §10). Mise à l'échelle par facteur entier. */
#define GAME_W 960
#define GAME_H 540

/* Logique à pas de temps fixe : 60 mises à jour par seconde. */
#define TICK_HZ 60

/* Nombre maximal de projectiles simultanés (pool statique, pas de malloc). */
#define MAX_PROJECTILES 256

/*
 * Tous les paramètres de gameplay à ajuster, au même endroit.
 * Les valeurs par défaut sont dans config.c.
 * Unités : pixels du jeu (960x540) et secondes.
 */
typedef struct Tuning {
    /* Joueur */
    float player_speed;        /* vitesse max, px/s */
    float player_accel_time;   /* temps pour atteindre la vitesse max, s (0 = instantané) */
    float player_decel_time;   /* temps pour s'arrêter, s (0 = arrêt net) */
    float player_hitbox_w;     /* boîte de collision, px */
    float player_hitbox_h;

    /* Tir */
    float fire_rate;           /* notes par seconde */
    float shot_speed;          /* px/s */
    float shot_range;          /* distance parcourue avant disparition, px */
    float shot_size;           /* boîte de collision du projectile (carrée), px */
    float shot_height;         /* hauteur de vol au-dessus du sol (affichage), px */
    float shot_inherit;        /* part de la vitesse du joueur transmise au projectile (0 = aucune) */
    float sing_duration;       /* durée de la bouche ouverte après un tir, s */
} Tuning;

extern Tuning tuning;

#endif
