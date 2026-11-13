#include "bullet.h"
#include "draw.h"
#include "ui_text.h"

#include <math.h>

#define NOTE_BOX 8.0f   /* boîte de collision d'une note de taille 1, px */

/*
 * Neumes de Saint-Gall tirés par les moines et les sonneurs (GDD §24) : les tracés du décor
 * (decor.c, draw_neume), en unités d'environ 10 x 12, la ligne de base en bas.
 */
typedef struct NeumeShape {
    int   points;
    float p[6][2];
    int   dots;
    float d[2][2];
} NeumeShape;
static const NeumeShape NEUMES[] = {
    { 3, { { 0, 11 }, { 4, 0 }, { 6, -1 } }, 0, { { 0 } } },                                   /* virga */
    { 4, { { 0, 7 }, { 2, 10 }, { 5, 8 }, { 6, -1 } }, 0, { { 0 } } },                         /* pes */
    { 4, { { 0, 9 }, { 2, 0 }, { 5, -1 }, { 7, 11 } }, 0, { { 0 } } },                         /* clivis */
    { 5, { { 0, 6 }, { 2, 9 }, { 4, 1 }, { 6, 0 }, { 8, 9 } }, 0, { { 0 } } },                 /* torculus */
    { 4, { { 0, 0 }, { 4, 9 }, { 7, 9 }, { 8, -1 } }, 0, { { 0 } } },                          /* porrectus */
    { 2, { { 0, 10 }, { 3, 0 } }, 2, { { 6, 4 }, { 9, 9 } } },                                 /* climacus */
    { 2, { { 6, 10 }, { 9, -1 } }, 2, { { 1, 10 }, { 4, 6 } } },                               /* scandicus */
    { 6, { { 0, 8 }, { 1, 6 }, { 2, 8 }, { 3, 6 }, { 4, 8 }, { 7, -1 } }, 0, { { 0 } } },     /* quilisma */
    { 0, { { 0 } }, 1, { { 2, 6 } } },                                                         /* punctum */
    { 2, { { 0, 6 }, { 5, 5 } }, 0, { { 0 } } },                                               /* tractulus */
    { 0, { { 0 } }, 2, { { 1, 6 }, { 5, 6 } } },                                               /* distropha */
};
enum { N_VIRGA, N_PES, N_CLIVIS, N_TORCULUS, N_PORRECTUS, N_CLIMACUS, N_SCANDICUS, N_QUILISMA,
       N_PUNCTUM, N_TRACTULUS, N_DISTROPHA };
#define NEUME_SHAPES ((int)SDL_arraysize(NEUMES))
#define NEUME_RANDOM 8   /* sonneurs : tirés parmi les huit premiers (les plus lisibles) */

static int sgn(int v) { return (v > 0) - (v < 0); }

/* Neume d'un groupe de 1 à 3 notes, d'après la mélodie (prev : note d'avant, 0 si aucune). */
static Uint8 neume_of(const char *g, int n, char prev)
{
    if (n == 1) {
        if (!prev || g[0] == prev) return prev ? N_TRACTULUS : N_PUNCTUM;
        return g[0] > prev ? N_VIRGA : N_PUNCTUM;
    }
    int d1 = sgn(g[1] - g[0]);
    if (n == 2) return d1 > 0 ? N_PES : d1 < 0 ? N_CLIVIS : N_DISTROPHA;
    int d2 = sgn(g[2] - g[1]);
    if (d1 > 0 && d2 < 0) return N_TORCULUS;
    if (d1 < 0 && d2 > 0) return N_PORRECTUS;
    if (d1 >= 0 && d2 >= 0) return d1 == 0 && d2 == 0 ? N_DISTROPHA : N_SCANDICUS;
    return N_CLIMACUS;
}

void bullet_set_syllable(Bullet *b, const char *text, const char *notes, char prev)
{
    int n = SDL_min((int)SDL_strlen(notes), 9);
    /* groupes d'au plus trois notes : 4 = 2+2, 5 = 3+2, 7 = 3+2+2... */
    int sizes[3], groups = 0, left = n;
    while (left > 0 && groups < 3) {
        int k = left == 4 ? 2 : SDL_min(left, 3);
        sizes[groups++] = k;
        left -= k;
    }
    b->neume_count = 0;
    const char *p = notes;
    for (int i = 0; i < groups; i++) {
        b->neumes[b->neume_count++] = neume_of(p, sizes[i], prev);
        prev = p[sizes[i] - 1];
        p += sizes[i];
    }
    if (b->neume_count == 0) {
        b->neumes[0] = N_PUNCTUM;
        b->neume_count = 1;
    }
    b->text = text;
}

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
    b->neumes[0] = (Uint8)SDL_rand(NEUME_RANDOM);
    b->neume_count = 1;
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

static void shape_span(const NeumeShape *n, float *minx, float *maxx)
{
    *minx = 1e9f;
    *maxx = -1e9f;
    for (int i = 0; i < n->points; i++) { *minx = SDL_min(*minx, n->p[i][0]); *maxx = SDL_max(*maxx, n->p[i][0]); }
    for (int i = 0; i < n->dots; i++)   { *minx = SDL_min(*minx, n->d[i][0]); *maxx = SDL_max(*maxx, n->d[i][0]); }
}

/* Neumes à l'encre rouge cernés de noir, à la suite, centrés en (x, y) ; s = taille.
 * Deux passes (contour, puis encre) ; la syllabe éventuelle est écrite dessous. */
static void draw_neume_shot(SDL_Renderer *r, float x, float y, float s, const Bullet *b)
{
    const float k = 1.65f * s, gap = 3.0f;
    float total = 0.0f, minx[3], maxx[3];
    for (int g = 0; g < b->neume_count; g++) {
        shape_span(&NEUMES[b->neumes[g] % NEUME_SHAPES], &minx[g], &maxx[g]);
        total += maxx[g] - minx[g] + (g > 0 ? gap : 0.0f);
    }
    const SDL_Color layers[2] = { { 40, 14, 12, 255 }, { 200, 52, 36, 255 } };
    const float widths[2] = { 2.8f * s + 3.0f, 2.8f * s };
    float oy = y - 5.0f * k;
    for (int pass = 0; pass < 2; pass++) {
        float w = widths[pass], ox = x - total * 0.5f * k;
        SDL_Color c = layers[pass];
        for (int g = 0; g < b->neume_count; g++) {
            const NeumeShape *n = &NEUMES[b->neumes[g] % NEUME_SHAPES];
            float bx = ox - minx[g] * k;
            for (int i = 0; i + 1 < n->points; i++) {
                draw_line(r, bx + n->p[i][0] * k, oy + n->p[i][1] * k, bx + n->p[i + 1][0] * k, oy + n->p[i + 1][1] * k, w, c);
            }
            for (int i = 0; i < n->points; i++) draw_ellipse(r, bx + n->p[i][0] * k, oy + n->p[i][1] * k, w * 0.5f, w * 0.5f, c);
            for (int i = 0; i < n->dots; i++) draw_ellipse(r, bx + n->d[i][0] * k, oy + n->d[i][1] * k, w * 0.75f, w * 0.75f, c);
            ox += (maxx[g] - minx[g] + gap) * k;
        }
    }
    if (b->text && b->text[0]) {
        /* la syllabe sous le neume, à l'encre, détourée de parchemin pour rester lisible */
        float top = oy + 12.0f * k;
        const SDL_Color halo = { 240, 228, 200, 220 }, ink = { 34, 22, 18, 255 };
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                if (dx || dy) ui_text_draw(r, FONT_SMALL, b->text, x + (float)dx, top + (float)dy, 0.5f, halo);
        ui_text_draw(r, FONT_SMALL, b->text, x, top, 0.5f, ink);
    }
}

void bullet_render(const Bullet *b, SDL_Renderer *r, float alpha, Camera cam)
{
    if (!b->active) return;
    float x, y;
    interp(b, alpha, cam, &x, &y);
    if (b->kind == BULLET_NOTE) {
        draw_neume_shot(r, x, y - b->height, b->size, b);
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
