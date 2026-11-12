#ifndef FUGUE_BULLET_H
#define FUGUE_BULLET_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "camera.h"
#include "config.h"
#include "room.h"

/*
 * Tirs ennemis (GDD §16), à part des notes d'Amadeus :
 * - note rouge qui vole en ligne droite (moine, sonneur de cloche) ;
 * - goutte d'encre lancée en cloche vers un point du sol (plume), annoncée par son ombre.
 */
typedef enum BulletKind { BULLET_NOTE, BULLET_INK } BulletKind;

typedef struct Bullet {
    bool       active;
    BulletKind kind;
    float      x, y;            /* position au sol (collisions) */
    float      prev_x, prev_y;  /* position au pas précédent, pour l'interpolation */
    float      vx, vy;          /* note : px/s */
    float      size;            /* note : 1 = note du moine */
    float      height;          /* note : hauteur de vol (affichage), px */
    float      traveled, range; /* note : distance parcourue et maximale, px */
    float      from_x, from_y;  /* goutte : départ ... */
    float      to_x, to_y;      /* ... et point d'impact */
    float      t, flight;       /* goutte : s depuis le lancer, durée du vol */
} Bullet;

typedef struct BulletPool {
    Bullet items[MAX_BULLETS];
} BulletPool;

void bullet_pool_clear(BulletPool *pool);

/* Note partant de (x, y) au sol. Renvoie NULL si le pool est plein. */
Bullet *bullet_spawn_note(BulletPool *pool, float x, float y, float vx, float vy, float range, float size, float height);

/* Goutte d'encre de (x, y) jusqu'à (tx, ty) en `flight` secondes. */
Bullet *bullet_spawn_ink(BulletPool *pool, float x, float y, float tx, float ty, float flight);

/* Avance d'un pas. Renvoie true au pas où une goutte touche le sol (en to_x, to_y). */
bool bullet_update(Bullet *b, const Room *room, float dt);

/* Vrai si la note touche la boîte (une goutte en vol ne touche personne). */
bool bullet_hits_box(const Bullet *b, const SDL_FRect *box);

/* Ombres au sol, puis tirs en l'air par-dessus les personnages. */
void bullet_render_shadow(const Bullet *b, SDL_Renderer *renderer, float alpha, Camera cam);
void bullet_render(const Bullet *b, SDL_Renderer *renderer, float alpha, Camera cam);

int bullet_count(const BulletPool *pool);

#endif
