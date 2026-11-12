#ifndef FUGUE_INPUT_H
#define FUGUE_INPUT_H

#include <SDL3/SDL.h>
#include <stdbool.h>

typedef enum Dir {
    DIR_NONE = -1,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_COUNT
} Dir;

/* État des commandes, échantillonné à chaque pas de logique. */
typedef struct Input {
    float move_x, move_y;   /* direction de déplacement, normalisée (ou 0,0) */
    Dir   shoot;            /* direction de tir, DIR_NONE si aucune flèche */
} Input;

/* À appeler pour chaque événement SDL (suit l'ordre d'appui des flèches). */
void input_handle_event(const SDL_Event *event);

/* Lit l'état courant du clavier. */
Input input_sample(void);

/* Vecteur unitaire d'une direction. */
void dir_vector(Dir d, float *x, float *y);

#endif
