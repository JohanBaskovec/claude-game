#include "projectile.h"

#include <math.h>

void projectile_pool_clear(ProjectilePool *pool)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        pool->items[i].active = false;
    }
}

bool projectile_spawn(ProjectilePool *pool, float x, float y, float vx, float vy, float range, float damage)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *pr = &pool->items[i];
        if (!pr->active) {
            pr->active = true;
            pr->x = pr->prev_x = x;
            pr->y = pr->prev_y = y;
            pr->vx = vx;
            pr->vy = vy;
            pr->traveled = 0.0f;
            pr->range = range;
            pr->damage = damage;
            return true;
        }
    }
    return false;
}

static SDL_FRect hitbox(float x, float y)
{
    float s = tuning.shot_size;
    return (SDL_FRect){ x - s * 0.5f, y - s * 0.5f, s, s };
}

void projectile_update_all(ProjectilePool *pool, const Room *room, float dt)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *pr = &pool->items[i];
        if (!pr->active) {
            continue;
        }
        pr->prev_x = pr->x;
        pr->prev_y = pr->y;

        float dx = pr->vx * dt, dy = pr->vy * dt;
        pr->x += dx;
        pr->y += dy;
        pr->traveled += sqrtf(dx * dx + dy * dy);

        SDL_FRect box = hitbox(pr->x, pr->y);
        if (pr->traveled >= pr->range || room_overlaps_wall(room, &box)) {
            pr->active = false;
        }
    }
}

/* Une croche (♪) en rectangles : tête, hampe, crochet. */
static void draw_note(SDL_Renderer *r, float x, float y)
{
    x = SDL_floorf(x);
    y = SDL_floorf(y);
    SDL_SetRenderDrawColor(r, 25, 18, 30, 255);
    SDL_FRect outline = { x - 6.0f, y - 1.0f, 12.0f, 9.0f };
    SDL_RenderFillRect(r, &outline);
    SDL_FRect head   = { x - 5.0f, y,         10.0f, 7.0f };
    SDL_FRect stem   = { x + 3.0f, y - 14.0f, 2.0f, 15.0f };
    SDL_FRect flag1  = { x + 5.0f, y - 14.0f, 3.0f, 3.0f };
    SDL_FRect flag2  = { x + 7.0f, y - 12.0f, 2.0f, 4.0f };
    SDL_RenderFillRect(r, &stem);
    SDL_RenderFillRect(r, &flag1);
    SDL_RenderFillRect(r, &flag2);
    SDL_SetRenderDrawColor(r, 255, 70, 168, 255);
    SDL_RenderFillRect(r, &head);
}

void projectile_render_all(const ProjectilePool *pool, SDL_Renderer *renderer, float alpha)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        const Projectile *pr = &pool->items[i];
        if (pr->active) {
            float x = pr->prev_x + (pr->x - pr->prev_x) * alpha;
            float y = pr->prev_y + (pr->y - pr->prev_y) * alpha;
            SDL_SetRenderDrawColor(renderer, 150, 138, 116, 255);
            SDL_FRect shadow = { SDL_floorf(x) - 5.0f, SDL_floorf(y) - 2.0f, 10.0f, 4.0f };
            SDL_RenderFillRect(renderer, &shadow);
            draw_note(renderer, x, y - tuning.shot_height);
        }
    }
}

int projectile_count(const ProjectilePool *pool)
{
    int n = 0;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        n += pool->items[i].active ? 1 : 0;
    }
    return n;
}
