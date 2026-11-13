#ifndef FUGUE_PICKUP_H
#define FUGUE_PICKUP_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "camera.h"
#include "config.h"

/* Prix de fin de salle (GDD §13) et contenu des boîtes à musique (§22) :
 * ils apparaissent (ou se répandent), puis sont aspirés par le joueur. */
typedef enum PickupType {
    PICKUP_BOMB,      /* métronome */
    PICKUP_COIN,      /* royalties */
    PICKUP_KEY_SOL,   /* clefs, dans l'ordre KeyKind */
    PICKUP_KEY_FA,
    PICKUP_KEY_UT,
    PICKUP_TYPE_COUNT
} PickupType;

typedef struct Pickup {
    bool       active;
    PickupType type;
    float      x, y;            /* position au sol */
    float      prev_x, prev_y;
    float      speed;           /* vitesse d'aspiration, px/s */
    float      age;             /* s depuis l'apparition */
    float      delay;           /* aspiré à partir de cet âge, s */
    float      vx, vy;          /* en se répandant (boîte à musique), px/s */
} Pickup;

typedef struct PickupPool {
    Pickup items[MAX_PICKUPS];
} PickupPool;

void pickup_pool_clear(PickupPool *pool);
Pickup *pickup_spawn(PickupPool *pool, PickupType type, float x, float y);
/* Prix qui jaillit d'une boîte à musique avec la vitesse (vx, vy), aspiré après `delay` s. */
Pickup *pickup_spill(PickupPool *pool, PickupType type, float x, float y, float vx, float vy, float delay);

/* Fait voler les prix vers (target_x, target_y) après le délai d'apparition. */
void pickup_update_all(PickupPool *pool, float target_x, float target_y, float dt);

/* Vrai si le prix est assez près de (x, y) pour être ramassé. */
bool pickup_reached(const Pickup *p, float x, float y);

float pickup_feet_y(const Pickup *p, float alpha);
void pickup_render(const Pickup *p, SDL_Renderer *r, float alpha, Camera cam);

int pickup_count(const PickupPool *pool);

#endif
