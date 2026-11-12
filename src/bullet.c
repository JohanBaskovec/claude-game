#include "bullet.h"
#include "draw.h"

#include <math.h>

#define NOTE_BOX 8.0f   /* boîte de collision d'une note de taille 1, px */

void bullet_pool_clear(BulletPool *pool)
{
    for (int i = 0; i < MAX_BULLETS; i++) pool->items[i].active = false;
}

static Bullet *alloc(BulletPool *pool)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!pool->items[i].active) return &pool->items[i];
    }
    return NULL;
}

Bullet *bullet_spawn_note(BulletPool *pool, float x, float y, float vx, float vy, float range, float size, float height)
{
    Bullet *b = alloc(pool);
    if (!b) return NULL;
    *b = (Bullet){ .active = true, .kind = BULLET_NOTE, .x = x, .y = y, .prev_x = x, .prev_y = y,
                   .vx = vx, .vy = vy, .size = size, .height = height, .range = range };
    return b;
}

Bullet *bullet_spawn_ink(BulletPool *pool, float x, float y, float tx, float ty, float flight)
{
    Bullet *b = alloc(pool);
    if (!b) return NULL;
    *b = (Bullet){ .active = true, .kind = BULLET_INK, .x = x, .y = y, .prev_x = x, .prev_y = y,
                   .from_x = x, .from_y = y, .to_x = tx, .to_y = ty, .flight = SDL_max(flight, 0.05f) };
    return b;
}

static SDL_FRect note_box(const Bullet *b)
{
    float s = NOTE_BOX * b->size;
    return (SDL_FRect){ b->x - s * 0.5f, b->y - s * 0.5f, s, s };
}

bool bullet_update(Bullet *b, const Room *room, float dt)
{
    if (!b->active) return false;
    b->prev_x = b->x;
    b->prev_y = b->y;
    if (b->kind == BULLET_INK) {
        b->t += dt;
        float u = SDL_min(b->t / b->flight, 1.0f);
        b->x = b->from_x + (b->to_x - b->from_x) * u;
        b->y = b->from_y + (b->to_y - b->from_y) * u;
        if (u >= 1.0f) {
            b->active = false;
            return true;
        }
        return false;
    }
    float dx = b->vx * dt, dy = b->vy * dt;
    b->x += dx;
    b->y += dy;
    b->traveled += sqrtf(dx * dx + dy * dy);
    SDL_FRect box = note_box(b);
    SDL_FPoint pos = { b->x, b->y };
    if (b->traveled >= b->range || !SDL_PointInRectFloat(&pos, &room->bounds) || room_overlaps_wall(room, &box)) {
        b->active = false;
    }
    return false;
}

bool bullet_hits_box(const Bullet *b, const SDL_FRect *box)
{
    if (!b->active || b->kind != BULLET_NOTE) return false;
    SDL_FRect nb = note_box(b);
    return nb.x < box->x + box->w && box->x < nb.x + nb.w && nb.y < box->y + box->h && box->y < nb.y + nb.h;
}

/* Hauteur de la goutte au-dessus du sol : une parabole, plus haute pour les lancers lointains. */
static float ink_height(const Bullet *b, float u)
{
    float dx = b->to_x - b->from_x, dy = b->to_y - b->from_y;
    float top = 50.0f + sqrtf(dx * dx + dy * dy) * 0.3f;
    return 36.0f * (1.0f - u) + 4.0f * top * u * (1.0f - u);   /* part de la pointe de la plume */
}

static void interp(const Bullet *b, float alpha, Camera cam, float *x, float *y)
{
    *x = SDL_floorf(b->prev_x + (b->x - b->prev_x) * alpha - cam.x);
    *y = SDL_floorf(b->prev_y + (b->y - b->prev_y) * alpha - cam.y);
}

void bullet_render_shadow(const Bullet *b, SDL_Renderer *r, float alpha, Camera cam)
{
    if (!b->active) return;
    float x, y;
    interp(b, alpha, cam, &x, &y);
    if (b->kind == BULLET_NOTE) {
        draw_ellipse(r, x, y, 5.0f * b->size, 2.0f * b->size, (SDL_Color){ 60, 40, 30, 90 });
        return;
    }
    /* Goutte : son ombre à l'arrivée grandit et fonce pendant le vol, pour prévenir. */
    float u = SDL_clamp(b->t / b->flight, 0.0f, 1.0f);
    float rad = tuning.toxic_radius * (0.3f + 0.5f * u);
    draw_ellipse(r, b->to_x - cam.x, b->to_y - cam.y, rad, rad * 0.5f, (SDL_Color){ 30, 50, 30, (Uint8)(40 + 110 * u) });
    draw_ellipse(r, x, y, 6.0f, 2.5f, (SDL_Color){ 60, 40, 30, 90 });
}

/* Note carrée (punctum) rouge à contour sombre, pour la distinguer des notes d'Amadeus. */
static void draw_square_note(SDL_Renderer *r, float x, float y, float s)
{
    SDL_Color ink = { 40, 16, 14, 255 }, red = { 196, 56, 40, 255 }, light = { 240, 130, 100, 255 };
    float h = SDL_floorf(5.0f * s);
    draw_rect(r, x - h - 1.0f, y - h - 1.0f, 2.0f * h + 2.0f, 2.0f * h + 2.0f, ink);
    draw_rect(r, x - h, y - h, 2.0f * h, 2.0f * h, red);
    draw_rect(r, x - h + 1.0f, y - h + 1.0f, SDL_max(h * 0.6f, 1.0f), SDL_max(h * 0.4f, 1.0f), light);
    /* Hampe à droite, comme une virga. */
    draw_rect(r, x + h - 1.0f, y - h - 6.0f * s, SDL_max(SDL_floorf(1.5f * s), 1.0f) + 1.0f, 6.0f * s + 1.0f, ink);
}

void bullet_render(const Bullet *b, SDL_Renderer *r, float alpha, Camera cam)
{
    if (!b->active) return;
    float x, y;
    interp(b, alpha, cam, &x, &y);
    if (b->kind == BULLET_NOTE) {
        draw_square_note(r, x, y - b->height, b->size);
        return;
    }
    float u = SDL_clamp((b->t - (1.0f - alpha) / (float)TICK_HZ) / b->flight, 0.0f, 1.0f);
    float z = SDL_floorf(ink_height(b, u));
    draw_ellipse(r, x, y - z, 6.0f, 6.0f, (SDL_Color){ 16, 24, 18, 255 });
    draw_ellipse(r, x, y - z, 4.5f, 4.5f, (SDL_Color){ 40, 70, 44, 255 });
    draw_rect(r, x - 3.0f, y - z - 3.0f, 2.0f, 2.0f, (SDL_Color){ 140, 190, 120, 255 });
}

int bullet_count(const BulletPool *pool)
{
    int n = 0;
    for (int i = 0; i < MAX_BULLETS; i++) n += pool->items[i].active ? 1 : 0;
    return n;
}
