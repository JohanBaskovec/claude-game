#ifndef FUGUE_PROJECTILE_H
#define FUGUE_PROJECTILE_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "config.h"
#include "room.h"

typedef struct Projectile {
    bool  active;
    float x, y;            /* position au sol (collisions) ; la note est dessinée plus haut */
    float prev_x, prev_y;  /* position au pas précédent, pour l'interpolation */
    float vx, vy;          /* px/s */
    float traveled;        /* distance parcourue, px */
    float range;           /* distance maximale, px */
} Projectile;

typedef struct ProjectilePool {
    Projectile items[MAX_PROJECTILES];
} ProjectilePool;

void projectile_pool_clear(ProjectilePool *pool);

/* Renvoie false si le pool est plein (le tir est alors ignoré). */
bool projectile_spawn(ProjectilePool *pool, float x, float y, float vx, float vy, float range);

void projectile_update_all(ProjectilePool *pool, const Room *room, float dt);

void projectile_render_all(const ProjectilePool *pool, SDL_Renderer *renderer, float alpha);

int projectile_count(const ProjectilePool *pool);

#endif
