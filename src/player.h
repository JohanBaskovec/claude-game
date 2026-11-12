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
    int   bombs;        /* métronomes */
    int   coins;        /* royalties */
    int   weapon;       /* arme tenue (WeaponId) */
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
    float     charge;       /* tir à charger : 0..1 */
    bool      charging;
    float     charge_x, charge_y; /* direction du tir en charge (vecteur unitaire) */
    float     lock_timer;   /* > 0 : cloué sur place (orgue) */
    float     walk_phase;   /* animation de marche : avance d'un temps tous les player_step_dist px */
    bool      slipping;     /* sur la cire : il glisse (réglé par game.c avant chaque pas) */
} Player;

/* Tir demandé par le joueur pendant un pas. */
typedef struct PlayerFire {
    bool  fire;     /* faux si aucun tir */
    float dx, dy;   /* direction (vecteur unitaire, n'importe quel angle) */
    float charge;   /* 0..1 pour un tir chargé, 1 sinon */
} PlayerFire;

void player_init(Player *p, float center_x, float center_y);

/* Valeurs de départ des attributs, lues dans tuning. */
PlayerStats player_start_stats(void);

/* Avance d'un pas. (aim_x, aim_y) : direction du tir demandé (flèche ou souris), vecteur unitaire,
 * ou (0, 0) si on ne tire pas. fire_rate : tirs par seconde de l'arme ; charge_time > 0 : arme
 * à charger (on tire en relâchant). Renvoie le tir qui part pendant ce pas. */
PlayerFire player_update(Player *p, const Input *in, float aim_x, float aim_y, const Room *room,
                         float fire_rate, float charge_time, float dt);

void player_center(const Player *p, float *x, float *y);

/* Inflige des dégâts si le joueur n'est pas invincible. Renvoie true si le coup porte. */
bool player_hurt(Player *p, int damage);

/* alpha : fraction entre le pas précédent et le pas courant (0..1). */
void player_render(const Player *p, SDL_Renderer *renderer, float alpha, Camera cam);

#endif
