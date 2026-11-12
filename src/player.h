#ifndef FUGUE_PLAYER_H
#define FUGUE_PLAYER_H

#include <SDL3/SDL.h>

#include "input.h"
#include "room.h"

typedef struct Player {
    SDL_FRect box;          /* boîte de collision (les pieds), coordonnées du jeu */
    float     prev_x, prev_y; /* position au pas précédent, pour l'interpolation */
    float     vx, vy;       /* vitesse, px/s */
    Dir       look;         /* direction de la tête */
    float     fire_cooldown;
    float     sing_timer;   /* > 0 : bouche ouverte */
} Player;

void player_init(Player *p, float center_x, float center_y);

/* Avance d'un pas. Renvoie la direction de tir si une note part, sinon DIR_NONE. */
Dir player_update(Player *p, const Input *in, const Room *room, float dt);

void player_center(const Player *p, float *x, float *y);

/* alpha : fraction entre le pas précédent et le pas courant (0..1). */
void player_render(const Player *p, SDL_Renderer *renderer, float alpha);

#endif
