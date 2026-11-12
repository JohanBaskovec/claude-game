#ifndef FUGUE_ROOM_H
#define FUGUE_ROOM_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "camera.h"
#include "floor.h"

/*
 * Géométrie de la salle courante, construite depuis le plan de l'étage.
 * Coordonnées « page » (GDD §9) : le sol de la cellule (cx, cy) est le rectangle
 * [cx*768, (cx+1)*768[ x [cy*360, (cy+1)*360[. Les murs sont autour du sol.
 * Une salle 1x1 avec ses murs fait exactement un écran (960x540).
 */
#define CELL_W       768.0f
#define CELL_H       360.0f
#define WALL_TOP     108.0f
#define WALL_SIDE    96.0f
#define WALL_BOTTOM  72.0f
#define DOOR_W       84.0f    /* ouverture des portes du haut et du bas */
#define DOOR_H       64.0f    /* ouverture des portes de gauche et de droite */

#define ROOM_MAX_WALLS (ROOM_MAX_CELLS * 4 * 2)
#define ROOM_MAX_PROPS TREASURE_OFFERS
#define LUTRIN_W     52.0f    /* pied du lutrin (obstacle) */
#define LUTRIN_H     22.0f
#define LUTRIN_GAP   176.0f   /* écart entre deux lutrins */

typedef struct RoomDoor {
    Dir       side;
    int       target;          /* salle de l'autre côté */
    int       cell_x, cell_y;  /* cellule de cette salle où se trouve la porte */
    SDL_FRect opening;         /* trou dans le mur ; bloqué quand la porte est fermée */
    SDL_FRect trigger;         /* le centre du joueur ici = on passe la porte */
    float     entry_x, entry_y;/* centre du joueur quand il arrive par cette porte */
} RoomDoor;

typedef struct Room {
    int       index;           /* indice dans le plan de l'étage */
    int       cell_count;
    SDL_FRect cells[ROOM_MAX_CELLS];   /* sol de chaque cellule */
    SDL_FRect bounds;          /* sol + murs : ce que la caméra peut montrer */
    int       wall_count;
    SDL_FRect walls[ROOM_MAX_WALLS];
    int       door_count;
    RoomDoor  doors[ROOM_MAX_DOORS];
    bool      doors_open;
    int       prop_count;      /* lutrins de la salle du trésor : obstacles */
    SDL_FRect props[ROOM_MAX_PROPS];
    bool      trapdoor;        /* trappe vers l'étage suivant (salle du boss vaincu) */
    float     trapdoor_x, trapdoor_y;
} Room;

void room_build(Room *room, const Floor *floor, int index);

/* Vrai si le rectangle chevauche un mur, un lutrin ou une porte fermée. */
bool room_overlaps_wall(const Room *room, const SDL_FRect *r);

/*
 * Déplace une boîte de (dx, dy) en la bloquant contre les murs.
 * Résolution axe par axe, ce qui permet de glisser le long d'un mur.
 * Renvoie une combinaison de ROOM_BLOCKED_X / ROOM_BLOCKED_Y.
 */
#define ROOM_BLOCKED_X 1
#define ROOM_BLOCKED_Y 2
int room_move_box(const Room *room, SDL_FRect *box, float dx, float dy);

/* Ouvre ou ferme les portes. En fermant, repousse `keep_out` (le joueur) hors des portes. */
void room_set_doors_open(Room *room, bool open, SDL_FRect *keep_out);

/* Porte franchie si le point (centre du joueur) est dans sa zone de passage, sinon NULL. */
const RoomDoor *room_door_at(const Room *room, float x, float y);

/* Porte de cette salle sur le côté `side` de la cellule (cx, cy), sinon NULL. */
const RoomDoor *room_find_door(const Room *room, int cx, int cy, Dir side);

/* Point au hasard sur le sol, à `margin` px au moins des murs. */
void room_random_floor_point(const Room *room, float margin, float *x, float *y);

/* Centre de la première cellule. */
void room_center(const Room *room, float *x, float *y);

/* Centre de la cellule la plus centrale (prix, trappe, boss). */
void room_middle(const Room *room, float *x, float *y);

/* Fait apparaître la trappe au milieu de la salle. */
void room_open_trapdoor(Room *room);

/* Caméra qui suit le point (x, y), bornée aux murs de la salle. */
Camera room_camera(const Room *room, float x, float y);

/* Sol, murs, portes et trappe. */
void room_render(const Room *room, SDL_Renderer *renderer, Camera cam);

/* Un lutrin, trié par profondeur avec les personnages (bas = props[i].y + h). */
void room_render_lutrin(const Room *room, int i, SDL_Renderer *renderer, Camera cam);

#endif
