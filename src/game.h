#ifndef FUGUE_GAME_H
#define FUGUE_GAME_H

#include <SDL3/SDL.h>

#include "bomb.h"
#include "bullet.h"
#include "camera.h"
#include "enemy.h"
#include "floor.h"
#include "hazard.h"
#include "hud.h"
#include "input.h"
#include "item.h"
#include "pickup.h"
#include "player.h"
#include "projectile.h"
#include "room.h"
#include "splatter.h"
#include "weapon.h"

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
    BulletPool     bullets;    /* tirs ennemis */
    EnemyPool      enemies;
    PickupPool     pickups;
    BombPool       bombs;
    HazardPool     hazards;
    SplatterPool   splatter;   /* gouttes et taches des ennemis morts */
    Hud            hud;
    Transition     transition;
    int            deaths;     /* compteur, pour le debug */

    /* La partie en cours (remise à zéro à la mort) */
    int            floor_number;          /* 1 = premier étage */
    bool           owned[ITEM_COUNT];     /* objets pris pendant la partie */
    bool           trapdoor_armed;        /* la trappe ne marche qu'après en être sorti une fois */
    bool           socle_armed;           /* idem pour le socle d'arme (sinon on échangerait en boucle) */
    float          shake_timer;           /* tremblement de l'écran, s */
    float          time;                  /* s de jeu, pour les animations */

    /* Affichage de la mini-carte, réglé par la fenêtre (souris, menu de debug). */
    bool           reveal_map; /* toute la carte visible et téléportation partout (debug) */
    int            map_hover;  /* salle survolée sur la mini-carte, -1 si aucune */
} Game;

/* Initialise le jeu et commence une partie. */
void game_init(Game *g, Uint64 seed);

/* Nouvelle partie (à la mort) : étage 1, attributs de départ, aucun objet. */
void game_new_run(Game *g, Uint64 seed);

/* Étage suivant (trappe, bouton de debug) : on garde tout, PV compris. */
void game_next_floor(Game *g, Uint64 seed);

/* Un pas de logique à durée fixe. */
void game_step(Game *g, const Input *in, float dt);

/* Téléportation par la mini-carte (GDD §12) : vers une salle visitée, hors combat.
 * Avec reveal_map (debug) : vers n'importe quelle salle, à tout moment. */
bool game_can_teleport(const Game *g, int room);

/* Téléporte au centre de la cellule (cx, cy) de la salle. Renvoie false si interdit. */
bool game_teleport(Game *g, int room, int cx, int cy);

/* Fait apparaître un ennemi loin du joueur (debug). */
void game_spawn_enemy(Game *g, EnemyType type);

/* Debug : donne le prochain objet passif pas encore pris. */
void game_give_next_item(Game *g);
void game_kill_enemies(Game *g);

/* Caméra de la salle courante (alpha : interpolation du rendu). */
Camera game_camera(const Game *g, float alpha);

void game_render(const Game *g, SDL_Renderer *renderer, float alpha);

#endif
