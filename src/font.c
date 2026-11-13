#include "font.h"

#include <math.h>

/* stb_truetype : un seul fichier, domaine public. Ses avertissements ne nous concernent pas. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x, u) ((void)(u), SDL_malloc(x))
#define STBTT_free(x, u)   ((void)(u), SDL_free(x))
#define STBTT_assert(x)    SDL_assert(x)
#include "third_party/stb_truetype.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/* Caractères rastérisés : ASCII, Latin-1 (accents du français), et quelques signes typographiques. */
#define ASCII_FIRST  32
#define ASCII_LAST   126
#define LATIN1_FIRST 0xA0
#define LATIN1_LAST  0xFF
#define ASCII_SLOTS  (ASCII_LAST - ASCII_FIRST + 1)
#define LATIN1_SLOTS (LATIN1_LAST - LATIN1_FIRST + 1)
static const Uint32 EXTRA_CPS[] = { 0x2018, 0x2019, 0x201C, 0x201D, 0x2013, 0x2014, 0x2026 };   /* ‘ ’ “ ” – — … */
#define EXTRA_SLOTS  ((int)SDL_arraysize(EXTRA_CPS))
#define SLOTS        (ASCII_SLOTS + LATIN1_SLOTS + EXTRA_SLOTS)
#define ATLAS_SIZE   (4 * 1024 * 1024)

typedef struct Glyph {
    int   index;          /* glyphe dans la police */
    int   x0, y0, w, h;   /* boîte du bitmap par rapport à la plume et à la ligne de base */
    float advance;
    Uint32 offset;        /* début du bitmap dans l'atlas */
} Glyph;

static const char *const FILES[2] = { "assets/fonts/Junicode-Medium.ttf", "assets/fonts/Junicode-Bold.ttf" };
/* lining : chiffres alignés (« 1012 ») au lieu des chiffres elzéviriens de Junicode (« ɪoɪ2 »), pour l'interface. */
static const struct { int file; float size; bool lining; } SPECS[FONT_COUNT] = {
    [FONT_TEXT]   = { 0, 22.0f, false },
    [FONT_RUBRIC] = { 0, 13.0f, false },
    [FONT_VERSAL] = { 1, 40.0f, false },
    [FONT_CAP]    = { 1, 92.0f, false },
    [FONT_TITLE]  = { 1, 60.0f, true },
    [FONT_UI]     = { 0, 30.0f, true },
};

static void          *data[2];
static stbtt_fontinfo info[2];
static float          scale[FONT_COUNT], ascent[FONT_COUNT], descent[FONT_COUNT];
static Glyph          glyphs[FONT_COUNT][SLOTS];
static Uint8          atlas[ATLAS_SIZE];
static Uint32         atlas_used;
static bool           ready;

static void *load_file(const char *name)
{
    char path[1024];
    const char *base = SDL_GetBasePath();
    SDL_snprintf(path, sizeof path, "%s%s", base ? base : "", name);
    SDL_IOStream *io = SDL_IOFromFile(path, "rb");
    if (!io) {
        SDL_Log("Police introuvable : %s (%s)", path, SDL_GetError());
        return NULL;
    }
    return SDL_LoadFile_IO(io, NULL, true);
}

/* Décode un caractère UTF-8 et avance le pointeur. */
static Uint32 next_cp(const char **s)
{
    const Uint8 *p = (const Uint8 *)*s;
    Uint32 c = p[0];
    int n = 1;
    if (c >= 0xF0 && p[1] && p[2] && p[3]) { c = ((c & 7u) << 18) | ((p[1] & 63u) << 12) | ((p[2] & 63u) << 6) | (p[3] & 63u); n = 4; }
    else if (c >= 0xE0 && p[1] && p[2])    { c = ((c & 15u) << 12) | ((p[1] & 63u) << 6) | (p[2] & 63u); n = 3; }
    else if (c >= 0xC0 && p[1])            { c = ((c & 31u) << 6) | (p[1] & 63u); n = 2; }
    *s += n;
    return c;
}

static int slot_of(Uint32 cp)
{
    if (cp >= ASCII_FIRST && cp <= ASCII_LAST) return (int)(cp - ASCII_FIRST);
    if (cp >= LATIN1_FIRST && cp <= LATIN1_LAST) return ASCII_SLOTS + (int)(cp - LATIN1_FIRST);
    for (int i = 0; i < EXTRA_SLOTS; i++) {
        if (EXTRA_CPS[i] == cp) return ASCII_SLOTS + LATIN1_SLOTS + i;
    }
    return '?' - ASCII_FIRST;
}

static Uint32 slot_cp(int slot)
{
    if (slot < ASCII_SLOTS) return (Uint32)(slot + ASCII_FIRST);
    if (slot < ASCII_SLOTS + LATIN1_SLOTS) return (Uint32)(slot - ASCII_SLOTS + LATIN1_FIRST);
    return EXTRA_CPS[slot - ASCII_SLOTS - LATIN1_SLOTS];
}

/*
 * Glyphe nommé `name` dans la table `post` (format 2), 0 s'il n'existe pas.
 * stb_truetype n'applique pas les fonctions OpenType : les chiffres alignés (« one.lf »)
 * se retrouvent par leur nom.
 */
static int glyph_by_name(const stbtt_fontinfo *fi, const char *name)
{
    Uint8 *d = fi->data;
    Uint32 post = stbtt__find_table(fi->data, (Uint32)fi->fontstart, "post");
    if (!post || ttULONG(d + post) != 0x00020000u) return 0;
    int count = ttUSHORT(d + post + 32);
    Uint8 *index = d + post + 34;
    const Uint8 *str = index + 2 * count;
    size_t len = SDL_strlen(name);
    /* rang du nom parmi les chaînes Pascal qui suivent (au plus une par glyphe) */
    int rank = -1;
    for (int k = 0; k < count && rank < 0; k++) {
        int n = str[0];
        if ((size_t)n == len && SDL_memcmp(str + 1, name, len) == 0) rank = k;
        str += 1 + n;
    }
    if (rank < 0) return 0;
    for (int gi = 0; gi < count; gi++) {
        if (ttUSHORT(index + 2 * gi) == 258 + rank) return gi;
    }
    return 0;
}

bool font_init(void)
{
    if (ready) return true;
    for (int i = 0; i < 2; i++) {
        data[i] = load_file(FILES[i]);
        if (!data[i] || !stbtt_InitFont(&info[i], data[i], stbtt_GetFontOffsetForIndex(data[i], 0))) {
            SDL_Log("Police illisible : %s", FILES[i]);
            font_quit();
            return false;
        }
    }
    atlas_used = 0;
    for (int f = 0; f < FONT_COUNT; f++) {
        const stbtt_fontinfo *fi = &info[SPECS[f].file];
        /* Taille Pillow = taille du cadratin en pixels. */
        scale[f] = stbtt_ScaleForMappingEmToPixels(fi, SPECS[f].size);
        int asc, desc, gap;
        stbtt_GetFontVMetrics(fi, &asc, &desc, &gap);
        ascent[f] = (float)asc * scale[f];
        descent[f] = (float)desc * scale[f];
        for (int s = 0; s < SLOTS; s++) {
            Glyph *g = &glyphs[f][s];
            g->index = stbtt_FindGlyphIndex(fi, (int)slot_cp(s));
            Uint32 cp = slot_cp(s);
            if (SPECS[f].lining && cp >= '0' && cp <= '9') {
                static const char *const DIGITS[10] = { "zero.lf", "one.lf", "two.lf", "three.lf", "four.lf",
                                                        "five.lf", "six.lf", "seven.lf", "eight.lf", "nine.lf" };
                int lf = glyph_by_name(fi, DIGITS[cp - '0']);
                if (lf > 0) g->index = lf;
            }
            int adv, lsb, x1, y1;
            stbtt_GetGlyphHMetrics(fi, g->index, &adv, &lsb);
            g->advance = (float)adv * scale[f];
            stbtt_GetGlyphBitmapBox(fi, g->index, scale[f], scale[f], &g->x0, &g->y0, &x1, &y1);
            g->w = x1 - g->x0;
            g->h = y1 - g->y0;
            Uint32 size = (Uint32)(g->w * g->h);
            if (g->w <= 0 || g->h <= 0 || atlas_used + size > ATLAS_SIZE) {
                if (g->w > 0 && g->h > 0) SDL_Log("Atlas de la police plein : glyphe %u perdu", (unsigned)slot_cp(s));
                g->w = g->h = 0;
                continue;
            }
            g->offset = atlas_used;
            stbtt_MakeGlyphBitmap(fi, atlas + atlas_used, g->w, g->h, g->w, scale[f], scale[f], g->index);
            atlas_used += size;
        }
    }
    ready = true;
    return true;
}

void font_quit(void)
{
    for (int i = 0; i < 2; i++) {
        SDL_free(data[i]);
        data[i] = NULL;
    }
    ready = false;
}

bool font_ready(void)
{
    return ready;
}

float font_ascent(FontId f)
{
    return ready ? ascent[f] : 0.0f;
}

float font_line_height(FontId f)
{
    return ready ? ascent[f] - descent[f] : 0.0f;
}

float font_text_width(FontId f, const char *utf8)
{
    if (!ready) return 0.0f;
    const stbtt_fontinfo *fi = &info[SPECS[f].file];
    float w = 0.0f;
    int prev = -1;
    while (*utf8) {
        const Glyph *g = &glyphs[f][slot_of(next_cp(&utf8))];
        if (prev >= 0) w += (float)stbtt_GetGlyphKernAdvance(fi, prev, g->index) * scale[f];
        w += g->advance;
        prev = g->index;
    }
    return w;
}

void font_draw(FontId f, const char *utf8, float x, float top, Uint8 *buf, int w, int h)
{
    if (!ready) return;
    const stbtt_fontinfo *fi = &info[SPECS[f].file];
    int baseline = (int)floorf(top + ascent[f] + 0.5f);
    float pen = x;
    int prev = -1;
    while (*utf8) {
        const Glyph *g = &glyphs[f][slot_of(next_cp(&utf8))];
        if (prev >= 0) pen += (float)stbtt_GetGlyphKernAdvance(fi, prev, g->index) * scale[f];
        prev = g->index;
        int gx = (int)floorf(pen + 0.5f) + g->x0, gy = baseline + g->y0;
        pen += g->advance;
        if (gx >= w || gy >= h || gx + g->w <= 0 || gy + g->h <= 0) continue;
        const Uint8 *src = atlas + g->offset;
        for (int j = 0; j < g->h; j++) {
            int y = gy + j;
            if (y < 0 || y >= h) continue;
            for (int i = 0; i < g->w; i++) {
                int xx = gx + i;
                if (xx < 0 || xx >= w) continue;
                Uint8 v = src[j * g->w + i];
                Uint8 *d = &buf[y * w + xx];
                if (v > *d) *d = v;
            }
        }
    }
}
