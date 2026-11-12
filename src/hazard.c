#include "hazard.h"
#include "draw.h"

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

Hazard *hazard_spawn_ink(HazardPool *pool, float x, float y)
{
    for (int i = 0; i < MAX_HAZARDS; i++) {
        Hazard *h = &pool->items[i];
        if (!h->active) {
            *h = (Hazard){ .active = true, .type = HAZARD_INK, .landed = true, .x = x, .y = y };
            return h;
        }
    }
    return NULL;
}

bool hazard_in_ink(const HazardPool *pool, float x, float y)
{
    float r = tuning.ink_radius;
    for (int i = 0; i < MAX_HAZARDS; i++) {
        const Hazard *h = &pool->items[i];
        if (!h->active || h->type != HAZARD_INK) continue;
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
    if (h->type == HAZARD_INK) {
        if (h->timer >= tuning.ink_time) h->active = false;
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
