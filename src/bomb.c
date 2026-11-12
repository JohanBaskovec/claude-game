#include "bomb.h"
#include "draw.h"

#include <math.h>

#define EXPLOSION_TIME 0.35f   /* durée de l'affichage de l'explosion, s */

void bomb_pool_clear(BombPool *pool)
{
    for (int i = 0; i < MAX_BOMBS; i++) pool->items[i].active = false;
}

Bomb *bomb_spawn(BombPool *pool, float x, float y)
{
    for (int i = 0; i < MAX_BOMBS; i++) {
        Bomb *b = &pool->items[i];
        if (!b->active) {
            *b = (Bomb){ .active = true, .x = x, .y = y };
            return b;
        }
    }
    return NULL;
}

bool bomb_update(Bomb *b, float dt)
{
    if (!b->active) return false;
    b->timer += dt;
    if (b->exploded) {
        if (b->timer >= EXPLOSION_TIME) b->active = false;
        return false;
    }
    /* Le balancier accélère : de 2 à 10 battements par seconde. */
    float fuse = SDL_max(tuning.bomb_fuse, 0.01f);
    float u = SDL_min(b->timer / fuse, 1.0f);
    b->phase += (2.0f + 8.0f * u * u) * SDL_PI_F * dt;
    if (b->timer >= fuse - 1e-4f) {
        b->exploded = true;
        b->timer = 0.0f;
        return true;
    }
    return false;
}

void bomb_render(const Bomb *b, SDL_Renderer *r, Camera cam)
{
    if (!b->active || b->exploded) return;
    float x = b->x - cam.x, y = b->y - cam.y;
    draw_ellipse(r, x, y, 15.0f, 5.0f, (SDL_Color){ 60, 40, 30, 110 });
    draw_metronome(r, x, y - 14.0f, 1.8f, sinf(b->phase) * 0.6f);
    /* Dernière demi-seconde : il rougeoie. */
    float left = tuning.bomb_fuse - b->timer;
    if (left < 0.5f && SDL_fmodf(left, 0.12f) < 0.06f) {
        draw_ellipse(r, x, y - 14.0f, 16.0f, 18.0f, (SDL_Color){ 255, 60, 40, 110 });
    }
}

void bomb_render_explosion(const Bomb *b, SDL_Renderer *r, Camera cam)
{
    if (!b->active || !b->exploded) return;
    float x = b->x - cam.x, y = b->y - cam.y;
    float u = SDL_min(b->timer / EXPLOSION_TIME, 1.0f);
    float rad = tuning.bomb_radius * (0.5f + 0.5f * sqrtf(u));
    Uint8 a = (Uint8)(220.0f * (1.0f - u));
    /* Vue de dessus en perspective : l'explosion est un peu aplatie. */
    draw_ellipse(r, x, y, rad, rad * 0.75f, (SDL_Color){ 255, 150, 40, a });
    draw_ellipse(r, x, y - 6.0f, rad * 0.65f, rad * 0.5f, (SDL_Color){ 255, 230, 140, a });
}

int bomb_count(const BombPool *pool)
{
    int n = 0;
    for (int i = 0; i < MAX_BOMBS; i++) n += (pool->items[i].active && !pool->items[i].exploded) ? 1 : 0;
    return n;
}
