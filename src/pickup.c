#include "pickup.h"
#include "draw.h"
#include "sprite.h"

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
            *p = (Pickup){ .active = true, .type = type, .x = x, .y = y, .prev_x = x, .prev_y = y,
                           .delay = tuning.pickup_magnet_delay };
            return p;
        }
    }
    return NULL;
}

Pickup *pickup_spill(PickupPool *pool, PickupType type, float x, float y, float vx, float vy, float delay)
{
    Pickup *p = pickup_spawn(pool, type, x, y);
    if (p) {
        p->vx = vx;
        p->vy = vy;
        p->delay = delay;
    }
    return p;
}

void pickup_update_all(PickupPool *pool, float tx, float ty, float dt)
{
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickup *p = &pool->items[i];
        if (!p->active) continue;
        p->prev_x = p->x;
        p->prev_y = p->y;
        p->age += dt;
        if (p->age < p->delay) {
            /* Il glisse en ralentissant (boîte à musique). */
            p->x += p->vx * dt;
            p->y += p->vy * dt;
            float k = expf(-tuning.box_spill_friction * dt);
            p->vx *= k;
            p->vy *= k;
            continue;
        }

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
    /* Pas tout de suite : on le voit apparaître (ou jaillir de la boîte) avant de le prendre. */
    if (!p->active || p->age < p->delay * 0.6f) return false;
    float dx = x - p->x, dy = y - p->y;
    return dx * dx + dy * dy <= tuning.pickup_collect_dist * tuning.pickup_collect_dist;
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
    float delay = SDL_max(SDL_min(p->delay, 0.6f), 0.01f);
    float z = 14.0f;
    if (p->age < delay) z += sinf(p->age / delay * SDL_PI_F) * 26.0f;

    draw_ellipse(r, x, y, 11.0f, 4.0f, (SDL_Color){ 60, 40, 30, 90 });
    switch (p->type) {
    case PICKUP_BOMB: draw_metronome(r, x, y - z, 1.3f, sinf(p->age * 6.0f) * 0.5f); break;
    case PICKUP_COIN: draw_crown(r, x, y - z, 1.3f); break;
    default: {
        SpriteId id = (SpriteId)(SPR_KEY_SOL + (p->type - PICKUP_KEY_SOL));
        sprite_draw(r, id, SDL_floorf(x - sprite_width(id) * 0.5f), SDL_floorf(y - z - sprite_height(id) * 0.5f), false, false);
        break;
    }
    }
}

int pickup_count(const PickupPool *pool)
{
    int n = 0;
    for (int i = 0; i < MAX_PICKUPS; i++) n += pool->items[i].active ? 1 : 0;
    return n;
}
