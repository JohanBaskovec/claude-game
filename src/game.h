#ifndef FUGUE_GAME_H
#define FUGUE_GAME_H

#include <SDL3/SDL.h>

#include "enemy.h"
#include "hud.h"
#include "input.h"
#include "player.h"
#include "projectile.h"
#include "room.h"

/* État d'une partie et règles du jeu, indépendants de la fenêtre. */
typedef struct Game {
    Room           room;
    Player         player;
    ProjectilePool shots;
    EnemyPool      enemies;
    Hud            hud;
    int            deaths;   /* compteur, pour le debug */
} Game;

void game_init(Game *g);

/* Recommence la salle (mort provisoire, GDD §11) : le joueur garde ses attributs
 * mais retrouve tous ses PV, les ennemis sont remplacés par un bébé. */
void game_restart(Game *g);

/* Un pas de logique à durée fixe. */
void game_step(Game *g, const Input *in, float dt);

void game_spawn_baby(Game *g);

void game_render(const Game *g, SDL_Renderer *renderer, float alpha);

#endif
