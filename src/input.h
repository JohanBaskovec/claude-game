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
    bool  bomb;             /* poser un métronome (vrai pendant un seul pas après l'appui) */
    bool  aim_mouse;        /* clic gauche maintenu : tirer vers le curseur (GDD §17) */
    float mouse_x, mouse_y; /* curseur, en coordonnées de l'écran du jeu (960x540) */
} Input;

/* À appeler pour chaque événement SDL (suit l'ordre d'appui des flèches et la souris).
 * Les événements de souris doivent déjà être en coordonnées du jeu (SDL_ConvertEventToRenderCoordinates). */
void input_handle_event(const SDL_Event *event);

/* Le clic en cours a servi à l'interface (mini-carte, menu de debug) : il ne tire pas jusqu'au relâchement. */
void input_block_mouse(void);

/* Vrai si le viseur doit être affiché (souris bougée récemment ou clic maintenu), et sa position. */
bool input_mouse_visible(float *x, float *y);

/* Lit l'état courant du clavier. */
Input input_sample(void);

/* Vecteur unitaire d'une direction. */
void dir_vector(Dir d, float *x, float *y);

#endif
