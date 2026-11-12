#include "pickup.h"
#include "draw.h"

#include <math.h>

void pickup_pool_clear(PickupPool *pool)
{
    for (int i = 0; i < MAX_PICKUPS; i++) pool->items[i].active = false;
}

Pickup *pickup_spawn(PickupPool *pool, PickupType type, float x, float y)
{
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickup *p = &pool->items[i];
        if (!p->active) {
            *p = (Pickup){ .active = true, .type = type, .x = x, .y = y, .prev_x = x, .prev_y = y };
            return p;
        }
    }
    return NULL;
}

void pickup_update_all(PickupPool *pool, float tx, float ty, float dt)
{
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickup *p = &pool->items[i];
        if (!p->active) continue;
        p->prev_x = p->x;
        p->prev_y = p->y;
        p->age += dt;
        if (p->age < tuning.pickup_magnet_delay) continue;

        /* Aspiré : vole droit vers le joueur en accélérant (il traverse tout). */
        p->speed = SDL_min(p->speed + tuning.pickup_magnet_accel * dt, tuning.pickup_magnet_max_speed);
        float dx = tx - p->x, dy = ty - p->y;
        float len = sqrtf(dx * dx + dy * dy);
        float step = p->speed * dt;
        if (len <= step) {
            p->x = tx;
            p->y = ty;
        } else {
            p->x += dx / len * step;
            p->y += dy / len * step;
        }
    }
}

bool pickup_reached(const Pickup *p, float x, float y)
{
    float dx = x - p->x, dy = y - p->y;
    return p->active && dx * dx + dy * dy <= tuning.pickup_collect_dist * tuning.pickup_collect_dist;
}

float pickup_feet_y(const Pickup *p, float alpha)
{
    return p->prev_y + (p->y - p->prev_y) * alpha;
}

void pickup_render(const Pickup *p, SDL_Renderer *r, float alpha, Camera cam)
{
    float x = p->prev_x + (p->x - p->prev_x) * alpha - cam.x;
    float y = pickup_feet_y(p, alpha) - cam.y;

    /* Petit bond à l'apparition, puis il flotte à hauteur de main. */
    float delay = SDL_max(tuning.pickup_magnet_delay, 0.01f);
    float z = 14.0f;
    if (p->age < delay) z += sinf(p->age / delay * SDL_PI_F) * 26.0f;

    draw_ellipse(r, x, y, 11.0f, 4.0f, (SDL_Color){ 60, 40, 30, 90 });
    if (p->type == PICKUP_BOMB) {
        draw_metronome(r, x, y - z, 1.3f, sinf(p->age * 6.0f) * 0.5f);
    } else {
        draw_crown(r, x, y - z, 1.3f);
    }
}

int pickup_count(const PickupPool *pool)
{
    int n = 0;
    for (int i = 0; i < MAX_PICKUPS; i++) n += pool->items[i].active ? 1 : 0;
    return n;
}
