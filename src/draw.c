#include "draw.h"

#include <math.h>
#include <string.h>

static SDL_FColor fcolor(SDL_Color c)
{
    return (SDL_FColor){ c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f };
}

void draw_rect(SDL_Renderer *r, float x, float y, float w, float h, SDL_Color c)
{
    SDL_SetRenderDrawBlendMode(r, c.a < 255 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_FRect rect = { SDL_floorf(x), SDL_floorf(y), w, h };
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void draw_ellipse(SDL_Renderer *r, float cx, float cy, float rx, float ry, SDL_Color c)
{
    enum { SEG = 20 };
    SDL_Vertex v[SEG + 1];
    int idx[SEG * 3];
    SDL_FColor fc = fcolor(c);
    v[0] = (SDL_Vertex){ { cx, cy }, fc, { 0, 0 } };
    for (int i = 0; i < SEG; i++) {
        float a = (float)i / SEG * 2.0f * SDL_PI_F;
        v[i + 1] = (SDL_Vertex){ { cx + cosf(a) * rx, cy + sinf(a) * ry }, fc, { 0, 0 } };
        idx[i * 3] = 0;
        idx[i * 3 + 1] = i + 1;
        idx[i * 3 + 2] = (i + 1) % SEG + 1;
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(r, NULL, v, SEG + 1, idx, SEG * 3);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void draw_line(SDL_Renderer *r, float ax, float ay, float bx, float by, float width, SDL_Color c)
{
    float dx = bx - ax, dy = by - ay;
    float len = sqrtf(dx * dx + dy * dy);
    if (len <= 0.0f) return;
    float nx = -dy / len * width * 0.5f, ny = dx / len * width * 0.5f;
    SDL_FColor fc = fcolor(c);
    SDL_Vertex v[4] = {
        { { ax + nx, ay + ny }, fc, { 0, 0 } }, { { bx + nx, by + ny }, fc, { 0, 0 } },
        { { bx - nx, by - ny }, fc, { 0, 0 } }, { { ax - nx, ay - ny }, fc, { 0, 0 } },
    };
    int idx[6] = { 0, 1, 2, 0, 2, 3 };
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(r, NULL, v, 4, idx, 6);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* Quadrilatère plein (a, b, c, d dans l'ordre du contour). */
static void quad(SDL_Renderer *r, float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy, SDL_Color col)
{
    SDL_FColor fc = fcolor(col);
    SDL_Vertex v[4] = {
        { { ax, ay }, fc, { 0, 0 } }, { { bx, by }, fc, { 0, 0 } },
        { { cx, cy }, fc, { 0, 0 } }, { { dx, dy }, fc, { 0, 0 } },
    };
    int idx[6] = { 0, 1, 2, 0, 2, 3 };
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(r, NULL, v, 4, idx, 6);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void draw_metronome(SDL_Renderer *r, float cx, float cy, float s, float angle)
{
    SDL_Color ink = { 40, 24, 16, 255 }, wood = { 166, 100, 50, 255 }, face = { 240, 226, 192, 255 };
    SDL_Color metal = { 50, 50, 60, 255 }, weight = { 220, 180, 50, 255 };
    /* Pyramide de bois tronquée, contour sombre. */
    quad(r, cx - 4 * s, cy - 10 * s, cx + 4 * s, cy - 10 * s, cx + 8.5f * s, cy + 8 * s, cx - 8.5f * s, cy + 8 * s, ink);
    quad(r, cx - 3 * s, cy - 9 * s, cx + 3 * s, cy - 9 * s, cx + 7.2f * s, cy + 7 * s, cx - 7.2f * s, cy + 7 * s, wood);
    /* Cadran clair */
    quad(r, cx - 1.6f * s, cy - 7 * s, cx + 1.6f * s, cy - 7 * s, cx + 4 * s, cy + 3.5f * s, cx - 4 * s, cy + 3.5f * s, face);
    /* Socle */
    draw_rect(r, cx - 9 * s, cy + 6 * s, 18 * s, 3 * s, ink);
    /* Balancier : pivot en bas du cadran, il oscille vers le haut. */
    angle = SDL_clamp(angle, -0.45f, 0.45f);
    float px = cx, py = cy + 3 * s, len = 10 * s;
    draw_line(r, px, py, px + sinf(angle) * len, py - cosf(angle) * len, SDL_max(1.2f * s, 1.0f), metal);
    float wx = px + sinf(angle) * len * 0.65f, wy = py - cosf(angle) * len * 0.65f;
    draw_rect(r, wx - 1.5f * s, wy - 1.2f * s, 3 * s, 2.4f * s, weight);
}

void draw_crown(SDL_Renderer *r, float cx, float cy, float s)
{
    SDL_Color gold = { 236, 190, 52, 255 }, shade = { 176, 128, 30, 255 }, gem = { 200, 40, 60, 255 };
    draw_rect(r, cx - 7 * s, cy + 1 * s, 14 * s, 5 * s, gold);
    draw_rect(r, cx - 7 * s, cy + 5 * s, 14 * s, 1 * s, shade);
    /* Trois pointes */
    draw_rect(r, cx - 7 * s, cy - 5 * s, 3 * s, 6 * s, gold);
    draw_rect(r, cx - 1.5f * s, cy - 6 * s, 3 * s, 7 * s, gold);
    draw_rect(r, cx + 4 * s, cy - 5 * s, 3 * s, 6 * s, gold);
    draw_rect(r, cx - 1 * s, cy + 2 * s, 2 * s, 2 * s, gem);
}

/* Chiffres 3x5 : une ligne de 3 bits par rangée, de haut en bas. */
static const Uint8 DIGITS[10][5] = {
    { 7, 5, 5, 5, 7 }, { 2, 6, 2, 2, 7 }, { 7, 1, 7, 4, 7 }, { 7, 1, 7, 1, 7 }, { 5, 5, 7, 1, 1 },
    { 7, 4, 7, 1, 7 }, { 7, 4, 7, 5, 7 }, { 7, 1, 1, 1, 1 }, { 7, 5, 7, 5, 7 }, { 7, 5, 7, 1, 7 },
};

float draw_number(SDL_Renderer *r, float x, float y, int value, int min_digits, float s, SDL_Color c)
{
    char buf[16];
    SDL_snprintf(buf, sizeof buf, "%0*d", min_digits, value < 0 ? 0 : value);
    float cx = x;
    for (const char *p = buf; *p; p++) {
        const Uint8 *d = DIGITS[*p - '0'];
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 3; col++) {
                if (d[row] & (4 >> col)) draw_rect(r, cx + col * s, y + row * s, s, s, c);
            }
        }
        cx += 4 * s;
    }
    return cx - x - s;
}

/* Remplace les lettres accentuées UTF-8 courantes du français par leur lettre de base. */
static void fold_ascii(const char *in, char *out, size_t size)
{
    static const char *from[] = { "à", "â", "ä", "é", "è", "ê", "ë", "î", "ï", "ô", "ö", "ù", "û", "ü", "ç",
                                  "À", "Â", "É", "È", "Ê", "Î", "Ô", "Ù", "Û", "Ç", "œ", "Œ", "’", "×", "« ", " »", "«", "»", "…", "—", "\u00a0" };
    static const char *to[]   = { "a", "a", "a", "e", "e", "e", "e", "i", "i", "o", "o", "u", "u", "u", "c",
                                  "A", "A", "E", "E", "E", "I", "O", "U", "U", "C", "oe", "OE", "'", "x", "\"", "\"", "\"", "\"", "...", "-", " " };
    size_t o = 0;
    while (*in && o + 3 < size) {
        bool done = false;
        for (size_t k = 0; k < sizeof from / sizeof from[0]; k++) {
            size_t n = strlen(from[k]);
            if (strncmp(in, from[k], n) == 0) {
                for (const char *t = to[k]; *t && o + 1 < size; t++) out[o++] = *t;
                in += n;
                done = true;
                break;
            }
        }
        if (!done) out[o++] = *in++;
    }
    out[o] = '\0';
}

float draw_debug_text_width(const char *utf8, float scale)
{
    char buf[256];
    fold_ascii(utf8, buf, sizeof buf);
    return (float)strlen(buf) * 8.0f * scale;
}

void draw_debug_text(SDL_Renderer *r, float x, float y, const char *utf8, float scale, SDL_Color c)
{
    char buf[256];
    fold_ascii(utf8, buf, sizeof buf);
    float sx, sy;
    SDL_GetRenderScale(r, &sx, &sy);
    SDL_SetRenderScale(r, sx * scale, sy * scale);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDebugText(r, x / scale, y / scale, buf);
    SDL_SetRenderScale(r, sx, sy);
}

void draw_crosshair(SDL_Renderer *r, float cx, float cy)
{
    /* Quatre traits roses (couleur des notes d'Amadeus) cernés de sombre, et un point au centre. */
    SDL_Color dark = { 25, 18, 30, 255 }, pink = { 255, 70, 168, 255 };
    cx = SDL_floorf(cx);
    cy = SDL_floorf(cy);
    for (int pass = 0; pass < 2; pass++) {
        SDL_Color c = pass ? pink : dark;
        float o = pass ? 0.0f : 1.0f;   /* le contour déborde d'un pixel */
        draw_rect(r, cx - 8.0f - o, cy - 1.0f - o, 5.0f + 2.0f * o, 2.0f + 2.0f * o, c);
        draw_rect(r, cx + 3.0f - o, cy - 1.0f - o, 5.0f + 2.0f * o, 2.0f + 2.0f * o, c);
        draw_rect(r, cx - 1.0f - o, cy - 8.0f - o, 2.0f + 2.0f * o, 5.0f + 2.0f * o, c);
        draw_rect(r, cx - 1.0f - o, cy + 3.0f - o, 2.0f + 2.0f * o, 5.0f + 2.0f * o, c);
        draw_rect(r, cx - 1.0f - o, cy - 1.0f - o, 2.0f + 2.0f * o, 2.0f + 2.0f * o, c);
    }
}
