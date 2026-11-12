#ifndef FUGUE_SPLATTER_H
#define FUGUE_SPLATTER_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "camera.h"
#include "config.h"
#include "room.h"

/*
 * Mort des ennemis (GDD §18) : un éclat, des gouttes qui retombent, et des taches
 * qui restent sur le sol de la salle. Purement visuel.
 */
typedef enum SplatColor { SPLAT_BLOOD, SPLAT_GREEN, SPLAT_INK } SplatColor;

typedef struct Drop {
    bool       active;
    SplatColor color;
    float      x, y, z;            /* position au sol et hauteur, px */
    float      prev_x, prev_y, prev_z;
    float      vx, vy, vz;         /* px/s */
    float      size;               /* px */
} Drop;

typedef struct Stain {
    bool       active;
    SplatColor color;
    float      x, y, r;            /* centre au sol et rayon, px */
    Uint32     seed;               /* forme : position des gouttelettes autour */
} Stain;

typedef struct Burst {
    bool       active;
    SplatColor color;
    float      x, y, z, scale, t;  /* éclat bref au moment de la mort */
} Burst;

typedef struct SplatterPool {
    Drop  drops[MAX_DROPS];
    Stain stains[MAX_STAINS];
    int   next_stain;              /* les plus vieilles taches sont remplacées quand c'est plein */
    Burst bursts[MAX_BURSTS];
} SplatterPool;

void splatter_clear(SplatterPool *pool);

/* Explosion en (x, y) au sol, depuis la hauteur z ; scale = 1 pour un bébé. */
void splatter_burst(SplatterPool *pool, float x, float y, float z, float scale, SplatColor color);

void splatter_update(SplatterPool *pool, const Room *room, float dt);

/* Taches et ombres des gouttes, sur le sol (avant les flaques et les personnages). */
void splatter_render_ground(const SplatterPool *pool, SDL_Renderer *renderer, float alpha, Camera cam);
/* Éclats et gouttes en vol, par-dessus les personnages. */
void splatter_render_air(const SplatterPool *pool, SDL_Renderer *renderer, float alpha, Camera cam);

#endif
