#ifndef FUGUE_PLAYER_H
#define FUGUE_PLAYER_H

#include <SDL3/SDL.h>

#include "input.h"
#include "room.h"

/* Attributs du joueur, comme dans TBOI (GDD §4). */
typedef struct PlayerStats {
    int   hp;           /* points de vie actuels */
    int   max_hp;       /* points de vie maximum */
    float speed;        /* vitesse de déplacement, px/s */
    float fire_rate;    /* cadence de tir, notes/s */
    float damage;       /* dégâts de chaque note, PV ennemis */
    float shot_speed;   /* vitesse des tirs, px/s */
    float shot_range;   /* portée des tirs, px */
} PlayerStats;

typedef struct Player {
    PlayerStats stats;
    SDL_FRect box;          /* boîte de collision (les pieds), coordonnées du jeu */
    float     prev_x, prev_y; /* position au pas précédent, pour l'interpolation */
    float     vx, vy;       /* vitesse, px/s */
    Dir       look;         /* direction de la tête */
    float     fire_cooldown;
    float     sing_timer;   /* > 0 : bouche ouverte */
    float     invuln_timer; /* > 0 : invincible (clignote) */
} Player;

void player_init(Player *p, float center_x, float center_y);

/* Valeurs de départ des attributs, lues dans tuning. */
PlayerStats player_start_stats(void);

/* Avance d'un pas. Renvoie la direction de tir si une note part, sinon DIR_NONE. */
Dir player_update(Player *p, const Input *in, const Room *room, float dt);

void player_center(const Player *p, float *x, float *y);

/* Inflige des dégâts si le joueur n'est pas invincible. Renvoie true si le coup porte. */
bool player_hurt(Player *p, int damage);

/* alpha : fraction entre le pas précédent et le pas courant (0..1). */
void player_render(const Player *p, SDL_Renderer *renderer, float alpha);

#endif
