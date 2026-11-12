#ifndef FUGUE_HAZARD_H
#define FUGUE_HAZARD_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "camera.h"
#include "config.h"

/* Dangers et traces au sol :
 * - brique qui tombe du plafond (atterrissage du Gros Bébé, GDD §13), annoncée par une ombre ;
 * - flaque d'encre (Liquescence, GDD §14) qui ralentit les ennemis. */
typedef enum HazardType { HAZARD_BRICK, HAZARD_INK } HazardType;

typedef struct Hazard {
    bool  active;
    HazardType type;
    bool  landed;
    float x, y;       /* point d'impact au sol */
    float timer;      /* s depuis le début de la chute, puis depuis l'impact */
    float fall_time;  /* durée de la chute, s */
} Hazard;

typedef struct HazardPool {
    Hazard items[MAX_HAZARDS];
} HazardPool;

void hazard_pool_clear(HazardPool *pool);
Hazard *hazard_spawn_brick(HazardPool *pool, float x, float y, float fall_time);
Hazard *hazard_spawn_ink(HazardPool *pool, float x, float y);

/* Vrai si (x, y) est dans une flaque d'encre. */
bool hazard_in_ink(const HazardPool *pool, float x, float y);

/* Avance d'un pas. Renvoie true au pas où la brique touche le sol. */
bool hazard_update(Hazard *h, float dt);

/* Ombre et gravats, au niveau du sol. */
void hazard_render_ground(const Hazard *h, SDL_Renderer *r, Camera cam);
/* Brique en train de tomber, par-dessus les personnages. */
void hazard_render_air(const Hazard *h, SDL_Renderer *r, Camera cam);

int hazard_count(const HazardPool *pool);

#endif
