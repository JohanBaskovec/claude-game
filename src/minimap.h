#ifndef FUGUE_MINIMAP_H
#define FUGUE_MINIMAP_H

#include <SDL3/SDL.h>

#include "floor.h"

/*
 * Mini-carte (GDD §12) : la double page en miniature, en haut à droite.
 * Salles visitées éclairées, salle actuelle en rose, salles découvertes en sombre.
 */
void minimap_render(const Floor *floor, int current, SDL_Renderer *renderer);

#endif
