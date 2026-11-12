#include "projectile.h"
#include "draw.h"

#include <math.h>

#define RING_FADE 0.12f   /* l'onde de la cloche reste un peu à sa taille finale, s */

void projectile_pool_clear(ProjectilePool *pool)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        pool->items[i].active = false;
        pool->items[i].ink_drop = false;
    }
}

static Projectile *alloc(ProjectilePool *pool)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *pr = &pool->items[i];
        if (!pr->active && !pr->ink_drop) {
            *pr = (Projectile){ .active = true, .kind = SHOT_NOTE, .size = 1.0f };
            return pr;
        }
    }
    return NULL;
}

Projectile *projectile_spawn(ProjectilePool *pool, float x, float y, float vx, float vy, float range, float damage)
{
    Projectile *pr = alloc(pool);
    if (!pr) return NULL;
    pr->x = pr->prev_x = x;
    pr->y = pr->prev_y = y;
    pr->vx = vx;
    pr->vy = vy;
    pr->range = range;
    pr->damage = damage;
    float len = sqrtf(vx * vx + vy * vy);
    pr->dir_x = len > 0.0f ? vx / len : 0.0f;
    pr->dir_y = len > 0.0f ? vy / len : 1.0f;
    return pr;
}

Projectile *projectile_spawn_ring(ProjectilePool *pool, float x, float y, float radius, float damage)
{
    Projectile *pr = alloc(pool);
    if (!pr) return NULL;
    pr->kind = SHOT_RING;
    pr->x = pr->prev_x = x;
    pr->y = pr->prev_y = y;
    pr->range = radius;
    pr->damage = damage;
    return pr;
}

Projectile *projectile_spawn_beam(ProjectilePool *pool, const Room *room, float x, float y, float dx, float dy, float damage)
{
    Projectile *pr = alloc(pool);
    if (!pr) return NULL;
    pr->kind = SHOT_BEAM;
    pr->x = pr->prev_x = x;
    pr->y = pr->prev_y = y;
    pr->dir_x = dx;
    pr->dir_y = dy;
    pr->damage = damage;
    /* Le rayon avance jusqu'au premier mur (il passe au-dessus des meubles). */
    float len = 0.0f;
    while (len < 4000.0f) {
        SDL_FRect probe = { x + dx * (len + 6.0f) - 2.0f, y + dy * (len + 6.0f) - 2.0f, 4.0f, 4.0f };
        if (room_overlaps_solid(room, &probe, false)) break;
        len += 6.0f;
    }
    pr->length = len;
    return pr;
}

static float note_box_size(const Projectile *pr)
{
    return tuning.shot_size * pr->size;
}

static float wave_width(const Projectile *pr)
{
    return (tuning.wave_width + pr->traveled * tuning.wave_growth) * pr->size;
}

static float ring_radius(const Projectile *pr)
{
    float grow = SDL_max(tuning.ring_grow_time, 0.01f);
    return pr->range * SDL_min(pr->age / grow, 1.0f);
}

bool projectile_pierces(const Projectile *pr)
{
    return pr->kind != SHOT_NOTE;
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
        pr->age += dt;

        if (pr->kind == SHOT_RING) {
            if (pr->age >= tuning.ring_grow_time + RING_FADE) pr->active = false;
            continue;
        }
        if (pr->kind == SHOT_BEAM) {
            if (pr->age >= tuning.beam_time) pr->active = false;
            continue;
        }

        /* Expectare : arrêt en l'air à une fraction de la portée, puis la note repart. */
        if (pr->pause && !pr->paused && pr->traveled >= pr->range * tuning.expectare_at) {
            pr->paused = true;
            pr->pause_left = tuning.expectare_pause;
        }
        if (pr->pause_left > 0.0f) {
            pr->pause_left -= dt;
            continue;
        }

        float dx = pr->vx * dt, dy = pr->vy * dt;
        pr->x += dx;
        pr->y += dy;
        pr->traveled += sqrtf(dx * dx + dy * dy);

        float s = pr->kind == SHOT_NOTE ? note_box_size(pr) : 6.0f;
        SDL_FRect box = { pr->x - s * 0.5f, pr->y - s * 0.5f, s, s };
        SDL_FPoint pos = { pr->x, pr->y };
        if (!SDL_PointInRectFloat(&pos, &room->bounds)) {
            pr->active = false;   /* sortie par une porte ouverte */
        } else if (room_overlaps_solid(room, &box, !pr->spectral)) {
            pr->active = false;
            pr->ink_drop = pr->ink;
            pr->stopped = true;
        } else if (pr->traveled >= pr->range) {
            pr->active = false;
            pr->ink_drop = pr->ink;
        }
    }
}

/* Demi-étendue d'une boîte le long de l'axe (ax, ay). */
static float extent(const SDL_FRect *b, float ax, float ay)
{
    return fabsf(ax) * b->w * 0.5f + fabsf(ay) * b->h * 0.5f;
}

bool projectile_hits_box(const Projectile *pr, const SDL_FRect *b)
{
    float bx = b->x + b->w * 0.5f, by = b->y + b->h * 0.5f;
    float rx = bx - pr->x, ry = by - pr->y;
    switch (pr->kind) {
    case SHOT_NOTE: {
        float s = note_box_size(pr);
        return fabsf(rx) < (s + b->w) * 0.5f && fabsf(ry) < (s + b->h) * 0.5f;
    }
    case SHOT_WAVE: {
        float along = rx * pr->dir_x + ry * pr->dir_y;
        float across = rx * -pr->dir_y + ry * pr->dir_x;
        return fabsf(along) <= 10.0f + extent(b, pr->dir_x, pr->dir_y) &&
               fabsf(across) <= wave_width(pr) * 0.5f + extent(b, -pr->dir_y, pr->dir_x);
    }
    case SHOT_RING:
        return sqrtf(rx * rx + ry * ry) <= ring_radius(pr) + SDL_min(b->w, b->h) * 0.5f;
    case SHOT_BEAM: {
        float along = rx * pr->dir_x + ry * pr->dir_y;
        float across = rx * -pr->dir_y + ry * pr->dir_x;
        float ea = extent(b, pr->dir_x, pr->dir_y);
        return along >= -ea && along <= pr->length + ea &&
               fabsf(across) <= tuning.beam_width * 0.5f + extent(b, -pr->dir_y, pr->dir_x);
    }
    }
    return false;
}

/* Une croche (♪) en rectangles : tête, hampe, crochet ; s = taille. */
static void draw_note(SDL_Renderer *r, float x, float y, float s)
{
    SDL_Color ink = { 25, 18, 30, 255 }, pink = { 255, 70, 168, 255 };
    x = SDL_floorf(x);
    y = SDL_floorf(y);
    draw_rect(r, x - 6.0f * s, y - 1.0f * s, 12.0f * s, 9.0f * s, ink);
    draw_rect(r, x + 3.0f * s, y - 14.0f * s, 2.0f * s, 15.0f * s, ink);
    draw_rect(r, x + 5.0f * s, y - 14.0f * s, 3.0f * s, 3.0f * s, ink);
    draw_rect(r, x + 7.0f * s, y - 12.0f * s, 2.0f * s, 4.0f * s, ink);
    draw_rect(r, x - 5.0f * s, y, 10.0f * s, 7.0f * s, pink);
}

static void render_one(const Projectile *pr, SDL_Renderer *r, float alpha, Camera cam)
{
    float x = pr->prev_x + (pr->x - pr->prev_x) * alpha - cam.x;
    float y = pr->prev_y + (pr->y - pr->prev_y) * alpha - cam.y;

    switch (pr->kind) {
    case SHOT_NOTE: {
        float h = tuning.shot_height * (pr->spectral ? tuning.altius_height_mul : 1.0f);
        draw_ellipse(r, x, y, 5.0f * pr->size, 2.0f * pr->size, (SDL_Color){ 60, 40, 30, 90 });
        float wobble = pr->pause_left > 0.0f ? sinf(pr->age * 40.0f) * 1.5f : 0.0f;   /* elle frémit pendant la pause */
        draw_note(r, x + wobble, y - h, pr->size);
        break;
    }
    case SHOT_WAVE: {
        /* Arc de petits pavés, bombé vers l'avant, qui s'efface en fin de course. */
        float w = wave_width(pr);
        Uint8 a = (Uint8)(255.0f * SDL_clamp(1.4f - pr->traveled / SDL_max(pr->range, 1.0f), 0.2f, 1.0f));
        float px = -pr->dir_y, py = pr->dir_x;
        for (int k = -6; k <= 6; k++) {
            float t = (float)k / 6.0f;
            float bulge = (1.0f - t * t) * w * 0.18f;
            float cx = x + pr->dir_x * bulge + px * t * w * 0.5f;
            float cy = y + pr->dir_y * bulge + py * t * w * 0.5f - 8.0f;
            draw_rect(r, cx - 4.0f, cy - 4.0f, 8.0f, 8.0f, (SDL_Color){ 90, 40, 30, a });
            draw_rect(r, cx - 2.5f, cy - 2.5f, 5.0f, 5.0f, (SDL_Color){ 236, 170, 90, a });
        }
        break;
    }
    case SHOT_RING: {
        float rad = ring_radius(pr);
        float fade = SDL_clamp(1.0f - (pr->age - tuning.ring_grow_time) / RING_FADE, 0.0f, 1.0f);
        Uint8 a = (Uint8)(230.0f * fade);
        draw_ellipse(r, x, y, rad, rad * 0.75f, (SDL_Color){ 255, 220, 120, (Uint8)(50.0f * fade) });
        for (int k = 0; k < 40; k++) {
            float ang = (float)k / 40.0f * 2.0f * SDL_PI_F;
            draw_rect(r, x + cosf(ang) * rad - 3.0f, y + sinf(ang) * rad * 0.75f - 3.0f, 6.0f, 6.0f,
                      (SDL_Color){ 226, 182, 52, a });
        }
        break;
    }
    case SHOT_BEAM: {
        float u = SDL_clamp(pr->age / SDL_max(tuning.beam_time, 0.01f), 0.0f, 1.0f);
        float h = tuning.shot_height;
        float ex = x + pr->dir_x * pr->length, ey = y + pr->dir_y * pr->length;
        float wdt = tuning.beam_width * (1.0f - 0.6f * u);
        Uint8 a = (Uint8)(235.0f * (1.0f - u));
        draw_line(r, x, y, ex, ey, wdt, (SDL_Color){ 60, 40, 30, (Uint8)(60.0f * (1.0f - u)) });   /* ombre */
        draw_line(r, x, y - h, ex, ey - h, wdt, (SDL_Color){ 236, 170, 60, a });
        draw_line(r, x, y - h, ex, ey - h, wdt * 0.45f, (SDL_Color){ 255, 246, 210, a });
        break;
    }
    }
}

void projectile_render_all(const ProjectilePool *pool, SDL_Renderer *renderer, float alpha, Camera cam)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (pool->items[i].active) render_one(&pool->items[i], renderer, alpha, cam);
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
