#ifndef FUGUE_PROJECTILE_H
#define FUGUE_PROJECTILE_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "config.h"
#include "room.h"

/* Un tir : note, onde du tabourin, onde de la cloche ou rayon de l'orgue (GDD §14). */
typedef struct Projectile {
    bool     active;
    ShotKind kind;
    float    x, y;            /* position au sol (collisions) ; la note est dessinée plus haut.
                                 Cloche : centre de l'onde. Orgue : départ du rayon. */
    float    prev_x, prev_y;  /* position au pas précédent, pour l'interpolation */
    float    vx, vy;          /* px/s */
    float    traveled;        /* distance parcourue, px */
    float    range;           /* distance maximale (rayon final pour la cloche), px */
    float    damage;          /* dégâts infligés, fixés au moment du tir */
    float    size;            /* taille (1 = note de la voix) */
    NoteValue note;           /* apparence de la note (selon la cadence au moment du tir) */
    float    stun;            /* étourdit l'ennemi touché, s */
    bool     spectral;        /* passe au-dessus des obstacles (Altius) */
    bool     pause;           /* s'arrête un instant en l'air (Expectare) */
    bool     ink;             /* laisse une flaque d'encre (Liquescence) */
    bool     paused;          /* la pause a déjà eu lieu */
    float    pause_left;      /* s de pause restantes */
    float    age;             /* s depuis le tir */
    float    dir_x, dir_y;    /* direction (onde, rayon) */
    float    length;          /* longueur du rayon, px */
    Uint32   hits;            /* ennemis déjà touchés (formes qui traversent), un bit par case du pool */
    bool     ink_drop;        /* vient de s'arrêter : game.c pose une flaque en (x, y) */
    bool     stopped;         /* vient d'être arrêté par un mur ou un obstacle (game.c casse l'obstacle) */
    Uint64   obstacle_hits;   /* obstacles déjà touchés (formes qui traversent), un bit par obstacle */
} Projectile;

typedef struct ProjectilePool {
    Projectile items[MAX_PROJECTILES];
} ProjectilePool;

void projectile_pool_clear(ProjectilePool *pool);

/* Note ou onde qui vole. Renvoie NULL si le pool est plein (le tir est alors ignoré). */
Projectile *projectile_spawn(ProjectilePool *pool, float x, float y, float vx, float vy, float range, float damage);
/* Onde circulaire centrée en (x, y), qui grandit jusqu'au rayon `radius`. */
Projectile *projectile_spawn_ring(ProjectilePool *pool, float x, float y, float radius, float damage);
/* Rayon de (x, y) dans la direction (dx, dy), arrêté par les murs. */
Projectile *projectile_spawn_beam(ProjectilePool *pool, const Room *room, float x, float y, float dx, float dy, float damage);

void projectile_update_all(ProjectilePool *pool, const Room *room, float dt);

/* Vrai si le tir touche la boîte. */
bool projectile_hits_box(const Projectile *pr, const SDL_FRect *box);

/* Vrai si le tir traverse les ennemis (il ne touche chacun qu'une fois). */
bool projectile_pierces(const Projectile *pr);

void projectile_render_all(const ProjectilePool *pool, SDL_Renderer *renderer, float alpha, Camera cam);

int projectile_count(const ProjectilePool *pool);

#endif
