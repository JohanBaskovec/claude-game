#ifndef FUGUE_GAME_H
#define FUGUE_GAME_H

#include <SDL3/SDL.h>

#include "camera.h"
#include "enemy.h"
#include "floor.h"
#include "hud.h"
#include "input.h"
#include "player.h"
#include "projectile.h"
#include "room.h"

/* Glissement de l'écran d'une salle à l'autre (GDD §12). */
typedef struct Transition {
    bool   active;
    float  t;          /* 0 -> 1 */
    Dir    dir;        /* sens du déplacement */
    Room   from;       /* salle qu'on quitte, pour l'afficher pendant le glissement */
    Camera from_cam;
} Transition;

/* État d'une partie et règles du jeu, indépendants de la fenêtre. */
typedef struct Game {
    Floor          floor;
    Room           room;       /* salle courante : g->room.index dans g->floor */
    Player         player;
    ProjectilePool shots;
    EnemyPool      enemies;
    Hud            hud;
    Transition     transition;
    int            deaths;     /* compteur, pour le debug */
} Game;

/* Nouvelle partie sur un étage généré avec cette graine. */
void game_init(Game *g, Uint64 seed);

/* Nouvel étage (mort provisoire, bouton de debug) : le joueur garde ses attributs
 * mais retrouve tous ses PV. */
void game_new_floor(Game *g, Uint64 seed);

/* Un pas de logique à durée fixe. */
void game_step(Game *g, const Input *in, float dt);

void game_spawn_baby(Game *g);
void game_kill_enemies(Game *g);

/* Caméra de la salle courante (alpha : interpolation du rendu). */
Camera game_camera(const Game *g, float alpha);

void game_render(const Game *g, SDL_Renderer *renderer, float alpha);

#endif
