#ifndef FUGUE_PICKUP_H
#define FUGUE_PICKUP_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "camera.h"
#include "config.h"

/* Prix de fin de salle (GDD §13) : ils apparaissent, puis sont aspirés par le joueur. */
typedef enum PickupType {
    PICKUP_BOMB,   /* métronome */
    PICKUP_COIN,   /* royalties */
    PICKUP_TYPE_COUNT
} PickupType;

typedef struct Pickup {
    bool       active;
    PickupType type;
    float      x, y;            /* position au sol */
    float      prev_x, prev_y;
    float      speed;           /* vitesse d'aspiration, px/s */
    float      age;             /* s depuis l'apparition */
} Pickup;

typedef struct PickupPool {
    Pickup items[MAX_PICKUPS];
} PickupPool;

void pickup_pool_clear(PickupPool *pool);
Pickup *pickup_spawn(PickupPool *pool, PickupType type, float x, float y);

/* Fait voler les prix vers (target_x, target_y) après le délai d'apparition. */
void pickup_update_all(PickupPool *pool, float target_x, float target_y, float dt);

/* Vrai si le prix est assez près de (x, y) pour être ramassé. */
bool pickup_reached(const Pickup *p, float x, float y);

float pickup_feet_y(const Pickup *p, float alpha);
void pickup_render(const Pickup *p, SDL_Renderer *r, float alpha, Camera cam);

int pickup_count(const PickupPool *pool);

#endif
