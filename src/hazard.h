#ifndef FUGUE_HAZARD_H
#define FUGUE_HAZARD_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "camera.h"
#include "config.h"

/* Brique qui tombe du plafond (atterrissage du Gros Bébé, GDD §13).
 * Son ombre grandit au sol avant l'impact. */
typedef struct Hazard {
    bool  active;
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

/* Avance d'un pas. Renvoie true au pas où la brique touche le sol. */
bool hazard_update(Hazard *h, float dt);

/* Ombre et gravats, au niveau du sol. */
void hazard_render_ground(const Hazard *h, SDL_Renderer *r, Camera cam);
/* Brique en train de tomber, par-dessus les personnages. */
void hazard_render_air(const Hazard *h, SDL_Renderer *r, Camera cam);

int hazard_count(const HazardPool *pool);

#endif
