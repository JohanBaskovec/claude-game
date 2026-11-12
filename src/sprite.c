#include "sprite.h"
#include "sprite_data.h"

#include <math.h>

/*
 * Planches PNG dessinées par Johan (GDD §15) : cases de taille fixe, sans marge,
 * lues de gauche à droite. Si le fichier manque, on garde la grille de sprite_data.h.
 */
typedef enum SheetId {
    SHEET_NONE = -1,
    SHEET_BABY,
    SHEET_COUNT
} SheetId;

static const struct { const char *file; int cell_w, cell_h; } SHEETS[SHEET_COUNT] = {
    [SHEET_BABY] = { "assets/sprites/baby/baby_sprite_sheet.png", 25, 56 },
};

typedef struct SpriteDef {
    const char *const  *rows;      /* grille de secours */
    int                 h;
    const PaletteEntry *palette;
    SheetId             sheet;     /* planche PNG, ou SHEET_NONE */
    int                 cell;      /* case dans la planche */
} SpriteDef;

#define DEF(grid, pal) { grid, (int)(sizeof grid / sizeof grid[0]), pal, SHEET_NONE, 0 }
#define DEF_SHEET(grid, pal, sheet, cell) { grid, (int)(sizeof grid / sizeof grid[0]), pal, sheet, cell }
static const SpriteDef DEFS[SPR_COUNT] = {
    [SPR_AMA_HEAD_FRONT]       = DEF(AMA_HEAD_FRONT, PALETTE_AMADEUS),
    [SPR_AMA_HEAD_FRONT_SING]  = DEF(AMA_HEAD_FRONT_SING, PALETTE_AMADEUS),
    [SPR_AMA_HEAD_BACK]        = DEF(AMA_HEAD_BACK, PALETTE_AMADEUS),
    [SPR_AMA_HEAD_SIDE]        = DEF(AMA_HEAD_SIDE, PALETTE_AMADEUS),
    [SPR_AMA_HEAD_SIDE_SING]   = DEF(AMA_HEAD_SIDE_SING, PALETTE_AMADEUS),
    [SPR_AMA_BODY_FRONT]       = DEF(AMA_BODY_FRONT, PALETTE_AMADEUS),
    [SPR_AMA_BODY_FRONT_STEP1] = DEF(AMA_BODY_FRONT_STEP1, PALETTE_AMADEUS),
    [SPR_AMA_BODY_FRONT_STEP2] = DEF(AMA_BODY_FRONT_STEP2, PALETTE_AMADEUS),
    [SPR_AMA_BODY_BACK]        = DEF(AMA_BODY_BACK, PALETTE_AMADEUS),
    [SPR_AMA_BODY_BACK_STEP1]  = DEF(AMA_BODY_BACK_STEP1, PALETTE_AMADEUS),
    [SPR_AMA_BODY_BACK_STEP2]  = DEF(AMA_BODY_BACK_STEP2, PALETTE_AMADEUS),
    [SPR_AMA_BODY_SIDE]        = DEF(AMA_BODY_SIDE, PALETTE_AMADEUS),
    [SPR_AMA_BODY_SIDE_STEP1]  = DEF(AMA_BODY_SIDE_STEP1, PALETTE_AMADEUS),
    [SPR_AMA_BODY_SIDE_STEP2]  = DEF(AMA_BODY_SIDE_STEP2, PALETTE_AMADEUS),
    /* La grille n'a que deux temps : le troisième reprend le premier. */
    [SPR_BABY_0]               = DEF_SHEET(BABY_0, PALETTE_BABY, SHEET_BABY, 0),
    [SPR_BABY_1]               = DEF_SHEET(BABY_1, PALETTE_BABY, SHEET_BABY, 1),
    [SPR_BABY_2]               = DEF_SHEET(BABY_0, PALETTE_BABY, SHEET_BABY, 2),
};

#define MAX_SPRITE_PIXELS (64 * 64)

static SDL_Renderer *owner;                 /* les textures appartiennent à ce renderer */
static SDL_Texture  *tex[SPR_COUNT][2];     /* [normal, blanc] */

/* Planches : images en RGBA32 (normale et toute blanche), chargées une fois. */
static bool          sheets_loaded;
static SDL_Surface  *sheet_img[SHEET_COUNT][2];
static SDL_Texture  *sheet_tex[SHEET_COUNT][2];

static SDL_Surface *load_png(const char *name)
{
    char path[1024];
    const char *base = SDL_GetBasePath();
    SDL_snprintf(path, sizeof path, "%s%s", base ? base : "", name);
    SDL_IOStream *io = SDL_IOFromFile(path, "rb");
    if (!io) return NULL;
    SDL_Surface *raw = SDL_LoadPNG_IO(io, true);
    if (!raw) {
        SDL_Log("Planche illisible : %s (%s)", path, SDL_GetError());
        return NULL;
    }
    SDL_Surface *s = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(raw);
    return s;
}

static void load_sheets(void)
{
    if (sheets_loaded) return;
    sheets_loaded = true;
    for (int i = 0; i < SHEET_COUNT; i++) {
        SDL_Surface *s = load_png(SHEETS[i].file);
        if (!s) continue;
        if (s->w < SHEETS[i].cell_w || s->h < SHEETS[i].cell_h) {
            SDL_Log("Planche trop petite : %s", SHEETS[i].file);
            SDL_DestroySurface(s);
            continue;
        }
        /* Version blanche pour l'éclair de dégât : on garde l'alpha. */
        SDL_Surface *w = SDL_DuplicateSurface(s);
        if (w) {
            for (int y = 0; y < w->h; y++) {
                Uint8 *row = (Uint8 *)w->pixels + y * w->pitch;
                for (int x = 0; x < w->w; x++) row[x * 4] = row[x * 4 + 1] = row[x * 4 + 2] = 255;
            }
        }
        sheet_img[i][0] = s;
        sheet_img[i][1] = w;
    }
}

/* Planche utilisée par ce sprite si elle est chargée et contient sa case, sinon SHEET_NONE. */
static SheetId sheet_of(SpriteId id)
{
    load_sheets();
    SheetId s = DEFS[id].sheet;
    if (s == SHEET_NONE || !sheet_img[s][0]) return SHEET_NONE;
    int per_row = sheet_img[s][0]->w / SHEETS[s].cell_w;
    int rows = sheet_img[s][0]->h / SHEETS[s].cell_h;
    return DEFS[id].cell < per_row * rows ? s : SHEET_NONE;
}

static SDL_Rect cell_rect(SheetId s, int cell)
{
    int per_row = sheet_img[s][0]->w / SHEETS[s].cell_w;
    return (SDL_Rect){ (cell % per_row) * SHEETS[s].cell_w, (cell / per_row) * SHEETS[s].cell_h,
                       SHEETS[s].cell_w, SHEETS[s].cell_h };
}

int sprite_width(SpriteId id)
{
    SheetId s = sheet_of(id);
    return s != SHEET_NONE ? SHEETS[s].cell_w : (int)SDL_strlen(DEFS[id].rows[0]);
}

int sprite_height(SpriteId id)
{
    SheetId s = sheet_of(id);
    return s != SHEET_NONE ? SHEETS[s].cell_h : DEFS[id].h;
}

int sprite_bottom(SpriteId id)
{
    SheetId s = sheet_of(id);
    if (s != SHEET_NONE) {
        const SDL_Surface *img = sheet_img[s][0];
        SDL_Rect c = cell_rect(s, DEFS[id].cell);
        for (int y = c.h - 1; y >= 0; y--) {
            const Uint8 *row = (const Uint8 *)img->pixels + (c.y + y) * img->pitch + c.x * 4;
            for (int x = 0; x < c.w; x++) {
                if (row[x * 4 + 3] >= 128) return y;
            }
        }
        return 0;
    }
    const SpriteDef *d = &DEFS[id];
    for (int y = d->h - 1; y >= 0; y--) {
        for (const char *c = d->rows[y]; *c; c++) {
            if (*c != '.') return y;
        }
    }
    return 0;
}

static bool lookup(const PaletteEntry *pal, char key, Uint8 *rgb)
{
    for (; pal->key; pal++) {
        if (pal->key == key) {
            rgb[0] = pal->r;
            rgb[1] = pal->g;
            rgb[2] = pal->b;
            return true;
        }
    }
    return false;
}

static SDL_Texture *make_texture(SDL_Renderer *r, SpriteId id, bool white)
{
    static Uint8 px[MAX_SPRITE_PIXELS * 4];
    const SpriteDef *d = &DEFS[id];
    int w = sprite_width(id), h = d->h;
    if (w * h > MAX_SPRITE_PIXELS) return NULL;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Uint8 *o = &px[(y * w + x) * 4];
            char c = d->rows[y][x];
            Uint8 rgb[3];
            if (c == '.' || !lookup(d->palette, c, rgb)) {
                o[0] = o[1] = o[2] = o[3] = 0;
                continue;
            }
            o[0] = white ? 255 : rgb[0];
            o[1] = white ? 255 : rgb[1];
            o[2] = white ? 255 : rgb[2];
            o[3] = 255;
        }
    }
    SDL_Texture *t = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
    if (!t) return NULL;
    SDL_UpdateTexture(t, NULL, px, w * 4);
    SDL_SetTextureScaleMode(t, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    return t;
}

static void drop_textures(void)
{
    for (int i = 0; i < SPR_COUNT; i++) {
        for (int k = 0; k < 2; k++) {
            if (tex[i][k]) SDL_DestroyTexture(tex[i][k]);
            tex[i][k] = NULL;
        }
    }
    for (int i = 0; i < SHEET_COUNT; i++) {
        for (int k = 0; k < 2; k++) {
            if (sheet_tex[i][k]) SDL_DestroyTexture(sheet_tex[i][k]);
            sheet_tex[i][k] = NULL;
        }
    }
    owner = NULL;
}

void sprite_quit(void)
{
    drop_textures();
    for (int i = 0; i < SHEET_COUNT; i++) {
        for (int k = 0; k < 2; k++) {
            SDL_DestroySurface(sheet_img[i][k]);
            sheet_img[i][k] = NULL;
        }
    }
    sheets_loaded = false;
}

void sprite_draw(SDL_Renderer *r, SpriteId id, float x, float y, bool flip, bool white)
{
    if (id < 0 || id >= SPR_COUNT) return;
    if (r != owner) {
        drop_textures();
        owner = r;
    }
    int k = white ? 1 : 0;
    SDL_FRect dst = { floorf(x), floorf(y), (float)sprite_width(id), (float)sprite_height(id) };
    SDL_FlipMode fm = flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SheetId s = sheet_of(id);
    if (s != SHEET_NONE) {
        if (!sheet_tex[s][k] && sheet_img[s][k]) {
            sheet_tex[s][k] = SDL_CreateTextureFromSurface(r, sheet_img[s][k]);
            if (sheet_tex[s][k]) {
                SDL_SetTextureScaleMode(sheet_tex[s][k], SDL_SCALEMODE_NEAREST);
                SDL_SetTextureBlendMode(sheet_tex[s][k], SDL_BLENDMODE_BLEND);
            }
        }
        if (!sheet_tex[s][k]) return;
        SDL_Rect c = cell_rect(s, DEFS[id].cell);
        SDL_FRect src = { (float)c.x, (float)c.y, (float)c.w, (float)c.h };
        SDL_RenderTextureRotated(r, sheet_tex[s][k], &src, &dst, 0.0, NULL, fm);
        return;
    }
    if (!tex[id][k]) tex[id][k] = make_texture(r, id, white);
    if (!tex[id][k]) return;
    SDL_RenderTextureRotated(r, tex[id][k], NULL, &dst, 0.0, NULL, fm);
}
