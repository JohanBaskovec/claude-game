#ifndef FUGUE_BOMB_H
#define FUGUE_BOMB_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "camera.h"
#include "config.h"

/* Métronome posé (la bombe du jeu, GDD §13) : tic-tac qui accélère, puis explosion. */
typedef struct Bomb {
    bool  active;
    bool  exploded;   /* explosion en cours (affichage seulement) */
    float x, y;       /* au sol ; un métronome posé ne bouge pas */
    float timer;      /* s depuis la pose, puis depuis l'explosion */
    float phase;      /* phase du balancier, rad */
    int   tick;       /* battement pendant ce pas : 0 aucun, 1 « tic », 2 « tac » (lu par game.c) */
} Bomb;

typedef struct BombPool {
    Bomb items[MAX_BOMBS];
} BombPool;

void bomb_pool_clear(BombPool *pool);
Bomb *bomb_spawn(BombPool *pool, float x, float y);

/* Avance d'un pas. Renvoie true au pas où la bombe explose. */
bool bomb_update(Bomb *b, float dt);

/* Métronome posé (trié par profondeur avec les personnages). */
void bomb_render(const Bomb *b, SDL_Renderer *r, Camera cam);
/* Explosion, par-dessus tout. */
void bomb_render_explosion(const Bomb *b, SDL_Renderer *r, Camera cam);

int bomb_count(const BombPool *pool);

#endif
