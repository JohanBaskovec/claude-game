#ifndef FUGUE_ROOM_H
#define FUGUE_ROOM_H

#include <SDL3/SDL.h>
#include <stdbool.h>

/*
 * Salle 1x1 plein écran (GDD §9, concept v2) :
 * sol de 768x360 px, de x=96 à 864 et de y=108 à 468.
 * Murs : haut 108 px, côtés 96 px, bas 72 px.
 */
#define ROOM_FLOOR_X 96.0f
#define ROOM_FLOOR_Y 108.0f
#define ROOM_FLOOR_W 768.0f
#define ROOM_FLOOR_H 360.0f

#define ROOM_MAX_WALLS 16

typedef struct Room {
    SDL_FRect floor;
    SDL_FRect walls[ROOM_MAX_WALLS];
    int       wall_count;
} Room;

void room_init(Room *room);

/* Vrai si le rectangle chevauche un mur. */
bool room_overlaps_wall(const Room *room, const SDL_FRect *r);

/*
 * Déplace une boîte de (dx, dy) en la bloquant contre les murs.
 * Résolution axe par axe, ce qui permet de glisser le long d'un mur.
 * Renvoie une combinaison de ROOM_BLOCKED_X / ROOM_BLOCKED_Y.
 */
#define ROOM_BLOCKED_X 1
#define ROOM_BLOCKED_Y 2
int room_move_box(const Room *room, SDL_FRect *box, float dx, float dy);

void room_render(const Room *room, SDL_Renderer *renderer);

#endif
