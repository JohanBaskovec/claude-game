#ifndef FUGUE_DECOR_H
#define FUGUE_DECOR_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "camera.h"
#include "room.h"

/*
 * Décor du premier étage (GDD §9 et §15), calculé par le jeu, sans fichier image :
 * murs de briques en perspective, portes sous un arc de claveaux, dalles du sol,
 * manuscrit à neumes sur toute la double page et reliure dans la colonne centrale.
 * Chaque salle est dessinée une fois dans une texture, la première fois qu'on l'affiche.
 */

/* Charge la police du sol et prépare le bruit. Sans police, le sol n'a pas de texte. */
void decor_init(void);
void decor_quit(void);

/* Étage affiché : à chaque nouvelle graine, met en page la double page et oublie les salles déjà dessinées. */
void decor_set_floor(Uint64 seed);

/* Sol, murs et portes (le bois des portes seulement si elles sont fermées). */
void decor_render_room(const Room *room, SDL_Renderer *renderer, Camera cam);

/* Dessine à l'avance une salle voisine de `room_index` (au plus une par appel). */
void decor_prefetch(const Floor *floor, int room_index, SDL_Renderer *renderer);

/* Nombre d'éléments (syllabes, neumes, rubriques, initiales) posés sur la double page. */
int decor_page_ops(void);

#endif
