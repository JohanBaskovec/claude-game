#ifndef FUGUE_ITEM_H
#define FUGUE_ITEM_H

#include <SDL3/SDL.h>

#include "player.h"

/* Objets de la salle du trésor (GDD §13, liste provisoire). */
typedef enum ItemId {
    ITEM_NONE = -1,
    ITEM_LULLY,    /* Bâton de Lully */
    ITEM_HAYDN,    /* Crâne de Haydn */
    ITEM_SATIE,    /* Vexations */
    ITEM_GUIDO,    /* Main guidonienne */
    ITEM_SABOTS,   /* Sabots de Strasbourg */
    ITEM_CELERITER,   /* c : cadence + */
    ITEM_ALTIUS,      /* a : au-dessus des obstacles */
    ITEM_EXPECTARE,   /* x : pause en l'air */
    ITEM_LIQUESCENCE, /* flaque d'encre qui ralentit */
    ITEM_TROPE,       /* une note de plus */
    ITEM_COUNT
} ItemId;

const char *item_name(ItemId id);
const char *item_desc(ItemId id);

/* Applique l'effet permanent de l'objet aux attributs. */
void item_apply(ItemId id, PlayerStats *stats);

/* Icône de l'objet centrée en (cx, cy), coordonnées écran. */
void item_render(ItemId id, SDL_Renderer *r, float cx, float cy);

#endif
