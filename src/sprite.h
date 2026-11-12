#ifndef FUGUE_SPRITE_H
#define FUGUE_SPRITE_H

#include <SDL3/SDL.h>
#include <stdbool.h>

/*
 * Sprites des personnages : une case d'une planche PNG de assets/sprites/ quand
 * elle existe, sinon une grille de caractères de sprite_data.h. Chaque image a
 * aussi une version toute blanche pour l'éclair quand on est touché.
 */
typedef enum SpriteId {
    SPR_AMA_HEAD_FRONT,
    SPR_AMA_HEAD_FRONT_SING,
    SPR_AMA_HEAD_BACK,
    SPR_AMA_HEAD_BACK_SING,
    SPR_AMA_HEAD_SIDE,
    SPR_AMA_HEAD_SIDE_SING,
    SPR_AMA_BODY_FRONT,       /* puis _STEP1 et _STEP2 : les deux temps de la marche */
    SPR_AMA_BODY_FRONT_STEP1,
    SPR_AMA_BODY_FRONT_STEP2,
    SPR_AMA_BODY_BACK,
    SPR_AMA_BODY_BACK_STEP1,
    SPR_AMA_BODY_BACK_STEP2,
    SPR_AMA_BODY_SIDE,
    SPR_AMA_BODY_SIDE_STEP1,
    SPR_AMA_BODY_SIDE_STEP2,
    SPR_BABY_0,               /* trois images de marche, de profil vers la droite */
    SPR_BABY_1,
    SPR_BABY_2,
    /* Ennemis de la phase 7 : planches PNG seulement (pas de grille de secours). */
    SPR_MONK_0, SPR_MONK_1, SPR_MONK_SING,                 /* marche 1, marche 2, chante */
    SPR_QUILL_0, SPR_QUILL_1, SPR_QUILL_SPIT,              /* flotte 1, flotte 2, se penche */
    SPR_WORM_HEAD, SPR_WORM_HALF, SPR_WORM_UP_0, SPR_WORM_UP_1,
    SPR_WORM_HOLE_BUMP, SPR_WORM_HOLE_CRACK, SPR_WORM_HOLE,
    SPR_RINGER_0, SPR_RINGER_1, SPR_RINGER_RING,           /* marche 1, marche 2, bras levés */
    SPR_BELL, SPR_BELL_LEFT, SPR_BELL_RIGHT,
    /* Obstacles de la phase 12 : planche PNG seulement. */
    SPR_BOOKS_0, SPR_BOOKS_1, SPR_INKWELL, SPR_CANDLE_0, SPR_CANDLE_1,
    SPR_NEEDLES_IN, SPR_NEEDLES_TIPS, SPR_NEEDLES_OUT,
    SPR_COUNT
} SpriteId;

/* Décalage vertical entre la ligne 0 d'une tête d'Amadeus et la ligne 0 de son corps
 * (planches : le bas de la case de la tête recouvre les 8 premières lignes du corps). */
int sprite_ama_head_above_body(void);

int sprite_width(SpriteId id);
int sprite_height(SpriteId id);

/* Dernière ligne non vide : sert à poser les pieds au sol. */
int sprite_bottom(SpriteId id);

/* Dessine le sprite, coin haut-gauche en (x, y) écran. flip : miroir horizontal ; white : tout blanc. */
void sprite_draw(SDL_Renderer *renderer, SpriteId id, float x, float y, bool flip, bool white);

void sprite_quit(void);

#endif
