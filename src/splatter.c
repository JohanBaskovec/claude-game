#include "splatter.h"
#include "draw.h"

#include <math.h>

#define BURST_TIME 0.18f   /* durée de l'éclat, s */

/* Deux teintes par couleur : claire (gouttes, centre des taches) et sombre (bord). */
static const SDL_Color LIGHT[3] = {
    [SPLAT_BLOOD] = { 168, 22, 28, 255 },
    [SPLAT_GREEN] = { 104, 170, 52, 255 },
    [SPLAT_INK]   = { 34, 36, 58, 255 },
};
static const SDL_Color DARK[3] = {
    [SPLAT_BLOOD] = { 104, 10, 16, 255 },
    [SPLAT_GREEN] = { 54, 104, 28, 255 },
    [SPLAT_INK]   = { 14, 14, 24, 255 },
};

static float rand_range(float a, float b)
{
    return a + SDL_randf() * (b - a);
}

static SDL_Color with_alpha(SDL_Color c, Uint8 a)
{
    c.a = a;
    return c;
}

void splatter_clear(SplatterPool *pool)
{
    *pool = (SplatterPool){ 0 };
}

static void add_stain(SplatterPool *pool, float x, float y, float r, SplatColor color)
{
    Stain *s = &pool->stains[pool->next_stain];
    pool->next_stain = (pool->next_stain + 1) % MAX_STAINS;
    *s = (Stain){ true, color, x, y, r, (Uint32)SDL_rand_bits() };
}

void splatter_burst(SplatterPool *pool, float x, float y, float z, float scale, SplatColor color)
{
    for (int i = 0; i < MAX_BURSTS; i++) {
        if (!pool->bursts[i].active) {
            pool->bursts[i] = (Burst){ true, color, x, y, z, scale, 0.0f };
            break;
        }
    }
    add_stain(pool, x, y, tuning.splat_stain_radius * scale, color);
    int n = (int)((float)tuning.splat_drops * scale);
    for (int k = 0, i = 0; k < n && i < MAX_DROPS; i++) {
        Drop *d = &pool->drops[i];
        if (d->active) continue;
        float a = SDL_randf() * 2.0f * SDL_PI_F;
        float sp = rand_range(tuning.splat_speed_min, tuning.splat_speed_max) * SDL_sqrtf(scale);
        *d = (Drop){ .active = true, .color = color, .x = x, .y = y, .z = z, .prev_x = x, .prev_y = y, .prev_z = z,
                     .vx = cosf(a) * sp, .vy = sinf(a) * sp * 0.6f,   /* sol vu en perspective : moins loin en y */
                     .vz = rand_range(tuning.splat_up_min, tuning.splat_up_max), .size = rand_range(1.5f, 3.5f) };
        k++;
    }
}

void splatter_update(SplatterPool *pool, const Room *room, float dt)
{
    for (int i = 0; i < MAX_BURSTS; i++) {
        Burst *b = &pool->bursts[i];
        if (b->active && (b->t += dt) >= BURST_TIME) b->active = false;
    }
    for (int i = 0; i < MAX_DROPS; i++) {
        Drop *d = &pool->drops[i];
        if (!d->active) continue;
        d->prev_x = d->x;
        d->prev_y = d->y;
        d->prev_z = d->z;
        d->x += d->vx * dt;
        d->y += d->vy * dt;
        d->vz -= tuning.splat_gravity * dt;
        d->z += d->vz * dt;
        if (d->z <= 0.0f) {
            /* Elle tombe : petite tache sur le sol (ramenée sur le sol si elle a volé jusqu'au mur). */
            float x = d->x, y = d->y;
            room_clamp_to_floor(room, d->size + 1.0f, d->size * 0.5f + 1.0f, &x, &y);
            add_stain(pool, x, y, d->size * 1.3f, d->color);
            d->active = false;
        }
    }
}

/* Petit générateur pour la forme d'une tache, toujours la même pour une graine. */
static float hash01(Uint32 *s)
{
    *s = *s * 1664525u + 1013904223u;
    return (float)(*s >> 8) / 16777216.0f;
}

static void draw_stain(SDL_Renderer *r, const Stain *s, Camera cam)
{
    float x = SDL_floorf(s->x - cam.x), y = SDL_floorf(s->y - cam.y);
    SDL_Color dark = with_alpha(DARK[s->color], 240), light = with_alpha(LIGHT[s->color], 240);   /* le parchemin transparaît à peine */
    Uint32 seed = s->seed;
    /* Gouttelettes autour de la tache principale (seulement pour les grandes). */
    int spots = s->r > 6.0f ? 4 + (int)(hash01(&seed) * 4.0f) : 0;
    for (int k = 0; k < spots; k++) {
        float a = hash01(&seed) * 2.0f * SDL_PI_F, d = s->r * (1.0f + hash01(&seed) * 0.9f);
        float rr = s->r * (0.12f + hash01(&seed) * 0.22f);
        draw_ellipse(r, x + cosf(a) * d, y + sinf(a) * d * 0.5f, rr, rr * 0.55f, dark);
    }
    draw_ellipse(r, x, y, s->r + 1.0f, s->r * 0.5f + 1.0f, dark);
    float ox = (hash01(&seed) - 0.5f) * s->r * 0.4f;
    draw_ellipse(r, x + ox, y, s->r * 0.8f, s->r * 0.38f, light);
}

static void lerp_drop(const Drop *d, float alpha, float *x, float *y, float *z)
{
    *x = d->prev_x + (d->x - d->prev_x) * alpha;
    *y = d->prev_y + (d->y - d->prev_y) * alpha;
    *z = d->prev_z + (d->z - d->prev_z) * alpha;
}

void splatter_render_ground(const SplatterPool *pool, SDL_Renderer *r, float alpha, Camera cam)
{
    for (int i = 0; i < MAX_STAINS; i++) {
        if (pool->stains[i].active) draw_stain(r, &pool->stains[i], cam);
    }
    for (int i = 0; i < MAX_DROPS; i++) {
        const Drop *d = &pool->drops[i];
        if (!d->active) continue;
        float x, y, z;
        lerp_drop(d, alpha, &x, &y, &z);
        draw_ellipse(r, x - cam.x, y - cam.y, d->size * 0.8f, d->size * 0.4f, (SDL_Color){ 60, 40, 30, 70 });
    }
}

void splatter_render_air(const SplatterPool *pool, SDL_Renderer *r, float alpha, Camera cam)
{
    for (int i = 0; i < MAX_BURSTS; i++) {
        const Burst *b = &pool->bursts[i];
        if (!b->active) continue;
        float u = SDL_clamp((b->t - (1.0f - alpha) / (float)TICK_HZ) / BURST_TIME, 0.0f, 1.0f);
        float x = b->x - cam.x, y = b->y - cam.y - b->z;
        float rad = (10.0f + 22.0f * u) * b->scale;
        Uint8 a = (Uint8)(230.0f * (1.0f - u));
        draw_ellipse(r, x, y, rad, rad * 0.8f, with_alpha(LIGHT[b->color], a));
        if (u < 0.4f) draw_ellipse(r, x, y, rad * 0.5f, rad * 0.4f, (SDL_Color){ 255, 240, 220, (Uint8)(255.0f * (1.0f - u / 0.4f)) });
    }
    for (int i = 0; i < MAX_DROPS; i++) {
        const Drop *d = &pool->drops[i];
        if (!d->active) continue;
        float x, y, z;
        lerp_drop(d, alpha, &x, &y, &z);
        float s = SDL_ceilf(d->size);
        draw_rect(r, SDL_floorf(x - cam.x - s * 0.5f), SDL_floorf(y - cam.y - z - s * 0.5f), s, s, LIGHT[d->color]);
    }
}
