#include "ui_text.h"

#include <math.h>

#define CACHE_SIZE 96   /* titres, boutons, descriptions, et les syllabes des moines en vol */
#define MAX_CHARS  96
#define BUF_W      1024
#define BUF_H      128

typedef struct Entry {
    SDL_Renderer *renderer;
    SDL_Texture  *tex;
    FontId        font;
    char          text[MAX_CHARS];
    int           w, h;
    Uint64        used;   /* dernier affichage, pour remplacer la plus ancienne */
} Entry;

static Entry  cache[CACHE_SIZE];
static Uint64 clock_count;
static Uint8  coverage[BUF_W * BUF_H];
static Uint32 pixels[BUF_W * BUF_H];

/* Texture du texte (créée au premier affichage), NULL si la police manque. */
static Entry *lookup(SDL_Renderer *r, FontId f, const char *utf8)
{
    Entry *oldest = &cache[0];
    for (int i = 0; i < CACHE_SIZE; i++) {
        Entry *e = &cache[i];
        if (e->tex && e->renderer == r && e->font == f && SDL_strcmp(e->text, utf8) == 0) {
            e->used = ++clock_count;
            return e;
        }
        if (!e->tex || e->used < oldest->used) oldest = e;
    }
    if (!font_ready() || SDL_strlen(utf8) >= MAX_CHARS) return NULL;

    int w = (int)ceilf(font_text_width(f, utf8)) + 4;
    int h = (int)ceilf(font_line_height(f)) + 4;
    if (w > BUF_W) w = BUF_W;
    if (h > BUF_H) h = BUF_H;
    SDL_memset(coverage, 0, (size_t)(w * h));
    font_draw(f, utf8, 2.0f, 2.0f, coverage, w, h);
    for (int i = 0; i < w * h; i++) {
        /* blanc, transparence = couverture (octets R, G, B, A en mémoire) */
        Uint8 *p = (Uint8 *)&pixels[i];
        p[0] = p[1] = p[2] = 255;
        p[3] = coverage[i];
    }

    if (oldest->tex) SDL_DestroyTexture(oldest->tex);
    oldest->tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
    if (!oldest->tex) return NULL;
    SDL_UpdateTexture(oldest->tex, NULL, pixels, w * 4);
    SDL_SetTextureBlendMode(oldest->tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(oldest->tex, SDL_SCALEMODE_NEAREST);
    oldest->renderer = r;
    oldest->font = f;
    SDL_strlcpy(oldest->text, utf8, sizeof oldest->text);
    oldest->w = w;
    oldest->h = h;
    oldest->used = ++clock_count;
    return oldest;
}

void ui_text_draw(SDL_Renderer *r, FontId f, const char *utf8, float x, float y, float align, SDL_Color c)
{
    Entry *e = lookup(r, f, utf8);
    if (!e) return;
    SDL_SetTextureColorMod(e->tex, c.r, c.g, c.b);
    SDL_SetTextureAlphaMod(e->tex, c.a);
    SDL_FRect dst = { SDL_floorf(x - (float)(e->w - 4) * align) - 2.0f, SDL_floorf(y) - 2.0f, (float)e->w, (float)e->h };
    SDL_RenderTexture(r, e->tex, NULL, &dst);
}

float ui_text_width(FontId f, const char *utf8)
{
    return font_text_width(f, utf8);
}

void ui_text_quit(void)
{
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].tex) SDL_DestroyTexture(cache[i].tex);
        cache[i] = (Entry){ 0 };
    }
}
