#ifndef FUGUE_MINIMAP_H
#define FUGUE_MINIMAP_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "floor.h"

/*
 * Mini-carte (GDD §12) : la double page en miniature, en haut à droite.
 * Salles visitées éclairées, salle actuelle en rose, salles découvertes en sombre.
 * reveal_all (menu de debug ouvert) : toutes les salles, les inconnues en contour.
 * hover : salle survolée par la souris (contour blanc), -1 si aucune.
 */
void minimap_render(const Floor *floor, int current, SDL_Renderer *renderer, bool reveal_all, int hover);

/* Salle affichée sous le point (x, y) en coordonnées écran, et sa cellule ; -1 si aucune. */
int minimap_room_at(const Floor *floor, float x, float y, bool reveal_all, int *cell_x, int *cell_y);

/* Bas de la mini-carte à l'écran, pour placer d'autres éléments en dessous. */
float minimap_bottom(void);

#endif
