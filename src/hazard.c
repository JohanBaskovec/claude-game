#include "hazard.h"
#include "draw.h"

#include <math.h>

#define RUBBLE_TIME  0.6f     /* les gravats restent un peu, s */
#define DROP_HEIGHT  560.0f   /* la brique part d'au-dessus de l'écran, px */
#define VISIBLE_FALL 0.30f    /* on ne voit la brique que pendant la fin de sa chute, s */

void hazard_pool_clear(HazardPool *pool)
{
    for (int i = 0; i < MAX_HAZARDS; i++) pool->items[i].active = false;
}

Hazard *hazard_spawn_brick(HazardPool *pool, float x, float y, float fall_time)
{
    for (int i = 0; i < MAX_HAZARDS; i++) {
        Hazard *h = &pool->items[i];
        if (!h->active) {
            *h = (Hazard){ .active = true, .type = HAZARD_BRICK, .x = x, .y = y, .fall_time = SDL_max(fall_time, 0.05f) };
            return h;
        }
    }
    return NULL;
}

static Hazard *spawn_puddle(HazardPool *pool, HazardType type, float x, float y)
{
    for (int i = 0; i < MAX_HAZARDS; i++) {
        Hazard *h = &pool->items[i];
        if (!h->active) {
            *h = (Hazard){ .active = true, .type = type, .landed = true, .x = x, .y = y };
            return h;
        }
    }
    return NULL;
}

Hazard *hazard_spawn_ink(HazardPool *pool, float x, float y)
{
    return spawn_puddle(pool, HAZARD_INK, x, y);
}

Hazard *hazard_spawn_toxic(HazardPool *pool, float x, float y)
{
    return spawn_puddle(pool, HAZARD_TOXIC, x, y);
}

Hazard *hazard_spawn_wax(HazardPool *pool, float x, float y)
{
    return spawn_puddle(pool, HAZARD_WAX, x, y);
}

static float puddle_radius(HazardType type)
{
    return type == HAZARD_TOXIC ? tuning.toxic_radius : type == HAZARD_WAX ? tuning.wax_radius : tuning.ink_radius;
}

static float puddle_time(HazardType type)
{
    return type == HAZARD_TOXIC ? tuning.toxic_time : type == HAZARD_WAX ? 1e9f : tuning.ink_time;
}

bool hazard_in(const HazardPool *pool, HazardType type, float x, float y)
{
    float r = puddle_radius(type);
    for (int i = 0; i < MAX_HAZARDS; i++) {
        const Hazard *h = &pool->items[i];
        if (!h->active || h->type != type) continue;
        /* Flaque vue en perspective : ellipse deux fois plus large que haute. */
        float dx = (x - h->x) / r, dy = (y - h->y) / (r * 0.5f);
        if (dx * dx + dy * dy <= 1.0f) return true;
    }
    return false;
}

bool hazard_update(Hazard *h, float dt)
{
    if (!h->active) return false;
    h->timer += dt;
    if (h->type != HAZARD_BRICK) {
        if (h->timer >= puddle_time(h->type)) h->active = false;
        return false;
    }
    if (h->landed) {
        if (h->timer >= RUBBLE_TIME) h->active = false;
        return false;
    }
    if (h->timer >= h->fall_time - 1e-4f) {
        h->landed = true;
        h->timer = 0.0f;
        return true;
    }
    return false;
}

static void brick(SDL_Renderer *r, float x, float y, Uint8 a)
{
    draw_rect(r, x - 15, y - 9, 30, 18, (SDL_Color){ 70, 34, 26, a });
    draw_rect(r, x - 14, y - 8, 28, 16, (SDL_Color){ 168, 80, 56, a });
    draw_rect(r, x - 14, y - 1, 28, 2, (SDL_Color){ 120, 56, 40, a });
    draw_rect(r, x - 1, y - 8, 2, 7, (SDL_Color){ 120, 56, 40, a });
}

void hazard_render_ground(const Hazard *h, SDL_Renderer *r, Camera cam)
{
    if (!h->active) return;
    float x = h->x - cam.x, y = h->y - cam.y;
    if (h->type == HAZARD_INK) {
        /* La flaque apparaît vite et s'efface à la fin. */
        float u = SDL_min(h->timer / 0.1f, 1.0f) * SDL_clamp((tuning.ink_time - h->timer) / 0.5f, 0.0f, 1.0f);
        float ir = tuning.ink_radius * (0.6f + 0.4f * SDL_min(h->timer / 0.1f, 1.0f));
        draw_ellipse(r, x, y, ir, ir * 0.5f, (SDL_Color){ 26, 30, 64, (Uint8)(200.0f * u) });
        draw_ellipse(r, x - ir * 0.3f, y - ir * 0.1f, ir * 0.25f, ir * 0.1f, (SDL_Color){ 90, 100, 150, (Uint8)(120.0f * u) });
        return;
    }
    if (h->type == HAZARD_WAX) {
        /* Cire blanche figée, en coulures autour du point où était la chandelle, avec un reflet. */
        float ir = tuning.wax_radius * (0.5f + 0.5f * SDL_min(h->timer / 0.25f, 1.0f));
        draw_ellipse(r, x, y, ir + 1.0f, ir * 0.5f + 1.0f, (SDL_Color){ 170, 150, 110, 200 });
        draw_ellipse(r, x, y, ir, ir * 0.5f, (SDL_Color){ 238, 228, 196, 235 });
        for (int k = 0; k < 5; k++) {
            float a = (float)k * 1.26f + h->x * 0.1f;
            draw_ellipse(r, x + cosf(a) * ir * 0.85f, y + sinf(a) * ir * 0.42f, ir * 0.3f, ir * 0.16f, (SDL_Color){ 238, 228, 196, 235 });
        }
        draw_ellipse(r, x - ir * 0.3f, y - ir * 0.12f, ir * 0.35f, ir * 0.1f, (SDL_Color){ 255, 252, 236, 220 });
        return;
    }
    if (h->type == HAZARD_TOXIC) {
        /* Encre verdâtre qui bouillonne : des bulles naissent et éclatent. */
        float T = tuning.toxic_time;
        float u = SDL_min(h->timer / 0.12f, 1.0f) * SDL_clamp((T - h->timer) / 0.6f, 0.0f, 1.0f);
        float ir = tuning.toxic_radius * (0.5f + 0.5f * SDL_min(h->timer / 0.12f, 1.0f));
        draw_ellipse(r, x, y, ir + 2.0f, ir * 0.5f + 1.0f, (SDL_Color){ 14, 22, 14, (Uint8)(210.0f * u) });
        draw_ellipse(r, x, y, ir, ir * 0.5f, (SDL_Color){ 36, 62, 34, (Uint8)(220.0f * u) });
        draw_ellipse(r, x - ir * 0.35f, y - ir * 0.12f, ir * 0.3f, ir * 0.1f, (SDL_Color){ 96, 140, 70, (Uint8)(150.0f * u) });
        for (int k = 0; k < 4; k++) {
            float ph = SDL_fmodf(h->timer * 1.3f + (float)k * 0.37f, 1.0f);   /* cycle de vie d'une bulle */
            float bx = x + sinf((float)k * 2.4f + h->x) * ir * 0.6f;
            float by = y + cosf((float)k * 1.7f + h->y) * ir * 0.25f;
            float br = 1.0f + 3.0f * ph;
            draw_ellipse(r, bx, by - br * 0.5f, br, br * 0.8f, (SDL_Color){ 130, 180, 90, (Uint8)(200.0f * u * (1.0f - ph * ph)) });
        }
        return;
    }
    float rad = tuning.brick_radius;
    if (!h->landed) {
        /* L'ombre grandit et fonce à l'approche de l'impact. */
        float u = SDL_clamp(h->timer / h->fall_time, 0.0f, 1.0f);
        Uint8 a = (Uint8)(50.0f + 120.0f * u);
        draw_ellipse(r, x, y, rad * (0.35f + 0.65f * u), rad * 0.5f * (0.35f + 0.65f * u), (SDL_Color){ 30, 20, 16, a });
        return;
    }
    /* Gravats qui s'effacent. */
    Uint8 a = (Uint8)(255.0f * SDL_clamp(1.0f - h->timer / RUBBLE_TIME, 0.0f, 1.0f));
    draw_rect(r, x - 14, y - 4, 10, 6, (SDL_Color){ 168, 80, 56, a });
    draw_rect(r, x + 3, y - 2, 9, 5, (SDL_Color){ 150, 70, 48, a });
    draw_rect(r, x - 4, y + 3, 7, 4, (SDL_Color){ 168, 80, 56, a });
    draw_ellipse(r, x, y, rad, rad * 0.45f, (SDL_Color){ 200, 190, 170, (Uint8)(a / 3) });
}

void hazard_render_air(const Hazard *h, SDL_Renderer *r, Camera cam)
{
    if (!h->active || h->landed || h->type != HAZARD_BRICK) return;
    float left = h->fall_time - h->timer;
    if (left > VISIBLE_FALL) return;
    float u = left / VISIBLE_FALL;   /* 1 en haut, 0 à l'impact */
    float z = DROP_HEIGHT * u * u;   /* chute accélérée */
    brick(r, h->x - cam.x, h->y - cam.y - 9.0f - z, 255);
}

int hazard_count(const HazardPool *pool)
{
    int n = 0;
    for (int i = 0; i < MAX_HAZARDS; i++) n += pool->items[i].active ? 1 : 0;
    return n;
}
