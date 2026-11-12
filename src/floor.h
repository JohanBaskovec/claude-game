#ifndef FUGUE_FLOOR_H
#define FUGUE_FLOOR_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "input.h"   /* Dir */

/*
 * Plan d'un étage (GDD §9 et §12) : la double page est une grille de
 * 7 x 10 cellules ; chaque salle occupe 1 à 4 cellules.
 * Ce module ne fait que la topologie (quelles salles, où, reliées comment).
 */
#define FLOOR_COLS 7
#define FLOOR_ROWS 10
#define FLOOR_START_X 3   /* salle de départ au centre, sur la reliure */
#define FLOOR_START_Y 4
#define FLOOR_MAX_ROOMS 32
#define ROOM_MAX_CELLS 4
#define ROOM_MAX_DOORS 8
#define TREASURE_OFFERS 3   /* objets au choix dans la salle du trésor */

typedef enum RoomShape {
    SHAPE_1x1,
    SHAPE_2x1,       /* 2 de large, 1 de haut */
    SHAPE_1x2,
    SHAPE_2x2,
    SHAPE_L_NO_TL,   /* 2x2 sans la cellule en haut à gauche */
    SHAPE_L_NO_TR,
    SHAPE_L_NO_BL,
    SHAPE_L_NO_BR,
    SHAPE_COUNT
} RoomShape;

typedef enum RoomKind {
    ROOM_NORMAL,
    ROOM_START,
    ROOM_BOSS,
    ROOM_TREASURE,
} RoomKind;

/* Porte : sur le côté `side` de la cellule (cell_x, cell_y), vers la salle `target`. */
typedef struct FloorDoor {
    int cell_x, cell_y;
    Dir side;
    int target;
} FloorDoor;

typedef struct FloorRoom {
    RoomShape shape;
    RoomKind  kind;
    int       cell_count;
    int       cell_x[ROOM_MAX_CELLS], cell_y[ROOM_MAX_CELLS];
    int       door_count;
    FloorDoor doors[ROOM_MAX_DOORS];
    int       depth;          /* nombre de salles depuis le départ */
    int       babies;         /* bébés à faire apparaître à la première visite */
    bool      visited;
    bool      cleared;        /* vidée de ses ennemis : portes ouvertes pour de bon */
    int       offers[TREASURE_OFFERS]; /* salle du trésor : objet sur chaque lutrin (ItemId), -1 si aucun.
                                          Rempli par le jeu, qui sait quels objets sont déjà pris. */
} FloorRoom;

typedef struct Floor {
    Uint64    seed;
    int       grid[FLOOR_ROWS][FLOOR_COLS];   /* indice de salle, -1 si vide */
    int       room_count;
    FloorRoom rooms[FLOOR_MAX_ROOMS];
    int       start;
    int       boss;
    int       treasure;       /* -1 si aucune */
} Floor;

/* Génère un étage. Même graine = même étage. */
void floor_generate(Floor *f, Uint64 seed);

/* Salle qui occupe la cellule, -1 si aucune ou hors de la grille. */
int floor_room_at(const Floor *f, int cx, int cy);

/* Vrai si la salle a été visitée ou touche une salle visitée (mini-carte). */
bool floor_room_discovered(const Floor *f, int index);

const char *floor_shape_name(RoomShape shape);

#endif
