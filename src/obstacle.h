#ifndef FUGUE_OBSTACLE_H
#define FUGUE_OBSTACLE_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "camera.h"
#include "layout.h"

/*
 * Obstacles des salles (GDD §21) : affichage et état qui dépend du temps (aiguilles).
 * Ils sont rangés dans la disposition de la salle (layout.h) ; les règles (casse, brûlure,
 * piqûre) sont dans game.c.
 */

/* Aiguilles du relieur : 0 rentrées, 1 pointes visibles (annonce), 2 sorties (blessent). */
int needle_stage(const Obstacle *o, float time);

/* Posé à plat sur le sol (aiguilles) : dessiné avec le sol, sous les personnages. */
bool obstacle_flat(int type);

/* Ligne des pieds, pour le tri par profondeur avec les personnages. */
float obstacle_feet_y(const RoomLayout *L, const Obstacle *o);

/* Centre au sol. */
void obstacle_center(const RoomLayout *L, const Obstacle *o, float *x, float *y);

void obstacle_render(const RoomLayout *L, const Obstacle *o, SDL_Renderer *renderer, Camera cam, float time);

#endif
