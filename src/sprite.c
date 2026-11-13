#include "sprite.h"
#include "sprite_data.h"

#include <math.h>

/*
 * Planches PNG dessinées par Johan (GDD §15) : cases de taille fixe, sans marge,
 * lues de gauche à droite. Si le fichier manque, on garde la grille de sprite_data.h.
 */
typedef enum SheetId {
    SHEET_NONE = -1,
    SHEET_AMA_HEAD,
    SHEET_AMA_BODY,
    SHEET_BABY,
    SHEET_MONK,
    SHEET_QUILL,
    SHEET_WORM,
    SHEET_WORM_HOLE,
    SHEET_RINGER,
    SHEET_BELL,
    SHEET_OBSTACLES,
    SHEET_PICKUPS,
    SHEET_COUNT
} SheetId;

static const struct { const char *file; int cell_w, cell_h; } SHEETS[SHEET_COUNT] = {
    /* Tête : lignes face, dos, profil droit ; colonnes repos, chante. */
    [SHEET_AMA_HEAD] = { "assets/sprites/amadeus/amadeus_head.png", 56, 48 },
    /* Corps : lignes face, dos, profil droit ; colonnes repos, pas 1, pas 2. */
    [SHEET_AMA_BODY] = { "assets/sprites/amadeus/amadeus_body.png", 40, 32 },
    /* Bébé : une ligne de trois images de marche, profil droit. */
    [SHEET_BABY]     = { "assets/sprites/baby/baby.png", 25, 56 },
    /* Ennemis de la phase 7 (planches provisoires) : une ligne, profil droit. */
    [SHEET_MONK]      = { "assets/sprites/monk/monk.png", 36, 60 },
    [SHEET_QUILL]     = { "assets/sprites/quill/quill.png", 32, 64 },
    [SHEET_WORM]      = { "assets/sprites/worm/worm.png", 32, 48 },
    [SHEET_WORM_HOLE] = { "assets/sprites/worm/worm_hole.png", 48, 24 },
    [SHEET_RINGER]    = { "assets/sprites/bellringer/bellringer.png", 36, 60 },
    [SHEET_BELL]      = { "assets/sprites/bellringer/bell.png", 28, 28 },
    /* Obstacles de la phase 12 (planche provisoire) : une case = une case du sol (48 px de large),
     * le centre de la case au sol est à 8 px du bas. */
    [SHEET_OBSTACLES] = { "assets/sprites/obstacles/obstacles.png", 48, 64 },
    /* Clefs et boîtes à musique de la phase 13 (planche provisoire) : objets centrés dans leur case,
     * le bas des boîtes à 4 px du bas de la case. */
    [SHEET_PICKUPS]   = { "assets/sprites/pickups/pickups.png", 48, 48 },
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
#define DEF_PNG(sheet, cell) { NULL, 0, NULL, sheet, cell }   /* sans grille : rien à dessiner si le PNG manque */
static const SpriteDef DEFS[SPR_COUNT] = {
    [SPR_AMA_HEAD_FRONT]       = DEF_SHEET(AMA_HEAD_FRONT, PALETTE_AMADEUS, SHEET_AMA_HEAD, 0),
    [SPR_AMA_HEAD_FRONT_SING]  = DEF_SHEET(AMA_HEAD_FRONT_SING, PALETTE_AMADEUS, SHEET_AMA_HEAD, 1),
    [SPR_AMA_HEAD_BACK]        = DEF_SHEET(AMA_HEAD_BACK, PALETTE_AMADEUS, SHEET_AMA_HEAD, 2),
    [SPR_AMA_HEAD_BACK_SING]   = DEF_SHEET(AMA_HEAD_BACK, PALETTE_AMADEUS, SHEET_AMA_HEAD, 3),
    [SPR_AMA_HEAD_SIDE]        = DEF_SHEET(AMA_HEAD_SIDE, PALETTE_AMADEUS, SHEET_AMA_HEAD, 4),
    [SPR_AMA_HEAD_SIDE_SING]   = DEF_SHEET(AMA_HEAD_SIDE_SING, PALETTE_AMADEUS, SHEET_AMA_HEAD, 5),
    [SPR_AMA_BODY_FRONT]       = DEF_SHEET(AMA_BODY_FRONT, PALETTE_AMADEUS, SHEET_AMA_BODY, 0),
    [SPR_AMA_BODY_FRONT_STEP1] = DEF_SHEET(AMA_BODY_FRONT_STEP1, PALETTE_AMADEUS, SHEET_AMA_BODY, 1),
    [SPR_AMA_BODY_FRONT_STEP2] = DEF_SHEET(AMA_BODY_FRONT_STEP2, PALETTE_AMADEUS, SHEET_AMA_BODY, 2),
    [SPR_AMA_BODY_BACK]        = DEF_SHEET(AMA_BODY_BACK, PALETTE_AMADEUS, SHEET_AMA_BODY, 3),
    [SPR_AMA_BODY_BACK_STEP1]  = DEF_SHEET(AMA_BODY_BACK_STEP1, PALETTE_AMADEUS, SHEET_AMA_BODY, 4),
    [SPR_AMA_BODY_BACK_STEP2]  = DEF_SHEET(AMA_BODY_BACK_STEP2, PALETTE_AMADEUS, SHEET_AMA_BODY, 5),
    [SPR_AMA_BODY_SIDE]        = DEF_SHEET(AMA_BODY_SIDE, PALETTE_AMADEUS, SHEET_AMA_BODY, 6),
    [SPR_AMA_BODY_SIDE_STEP1]  = DEF_SHEET(AMA_BODY_SIDE_STEP1, PALETTE_AMADEUS, SHEET_AMA_BODY, 7),
    [SPR_AMA_BODY_SIDE_STEP2]  = DEF_SHEET(AMA_BODY_SIDE_STEP2, PALETTE_AMADEUS, SHEET_AMA_BODY, 8),
    /* La grille n'a que deux temps : le troisième reprend le premier. */
    [SPR_BABY_0]               = DEF_SHEET(BABY_0, PALETTE_BABY, SHEET_BABY, 0),
    [SPR_BABY_1]               = DEF_SHEET(BABY_1, PALETTE_BABY, SHEET_BABY, 1),
    [SPR_BABY_2]               = DEF_SHEET(BABY_0, PALETTE_BABY, SHEET_BABY, 2),
    [SPR_MONK_0]          = DEF_PNG(SHEET_MONK, 0),
    [SPR_MONK_1]          = DEF_PNG(SHEET_MONK, 1),
    [SPR_MONK_SING]       = DEF_PNG(SHEET_MONK, 2),
    [SPR_QUILL_0]         = DEF_PNG(SHEET_QUILL, 0),
    [SPR_QUILL_1]         = DEF_PNG(SHEET_QUILL, 1),
    [SPR_QUILL_SPIT]      = DEF_PNG(SHEET_QUILL, 2),
    [SPR_WORM_HEAD]       = DEF_PNG(SHEET_WORM, 0),
    [SPR_WORM_HALF]       = DEF_PNG(SHEET_WORM, 1),
    [SPR_WORM_UP_0]       = DEF_PNG(SHEET_WORM, 2),
    [SPR_WORM_UP_1]       = DEF_PNG(SHEET_WORM, 3),
    [SPR_WORM_HOLE_BUMP]  = DEF_PNG(SHEET_WORM_HOLE, 0),
    [SPR_WORM_HOLE_CRACK] = DEF_PNG(SHEET_WORM_HOLE, 1),
    [SPR_WORM_HOLE]       = DEF_PNG(SHEET_WORM_HOLE, 2),
    [SPR_RINGER_0]        = DEF_PNG(SHEET_RINGER, 0),
    [SPR_RINGER_1]        = DEF_PNG(SHEET_RINGER, 1),
    [SPR_RINGER_RING]     = DEF_PNG(SHEET_RINGER, 2),
    [SPR_BELL]            = DEF_PNG(SHEET_BELL, 0),
    [SPR_BELL_LEFT]       = DEF_PNG(SHEET_BELL, 1),
    [SPR_BELL_RIGHT]      = DEF_PNG(SHEET_BELL, 2),
    [SPR_BOOKS_0]         = DEF_PNG(SHEET_OBSTACLES, 0),
    [SPR_BOOKS_1]         = DEF_PNG(SHEET_OBSTACLES, 1),
    [SPR_INKWELL]         = DEF_PNG(SHEET_OBSTACLES, 2),
    [SPR_CANDLE_0]        = DEF_PNG(SHEET_OBSTACLES, 3),
    [SPR_CANDLE_1]        = DEF_PNG(SHEET_OBSTACLES, 4),
    [SPR_NEEDLES_IN]      = DEF_PNG(SHEET_OBSTACLES, 5),
    [SPR_NEEDLES_TIPS]    = DEF_PNG(SHEET_OBSTACLES, 6),
    [SPR_NEEDLES_OUT]     = DEF_PNG(SHEET_OBSTACLES, 7),
    [SPR_KEY_SOL]         = DEF_PNG(SHEET_PICKUPS, 0),
    [SPR_KEY_FA]          = DEF_PNG(SHEET_PICKUPS, 1),
    [SPR_KEY_UT]          = DEF_PNG(SHEET_PICKUPS, 2),
    [SPR_BOX_GRAY]        = DEF_PNG(SHEET_PICKUPS, 3),
    [SPR_BOX_GRAY_OPEN]   = DEF_PNG(SHEET_PICKUPS, 4),
    [SPR_BOX_GOLD]        = DEF_PNG(SHEET_PICKUPS, 5),
    [SPR_BOX_GOLD_OPEN]   = DEF_PNG(SHEET_PICKUPS, 6),
    [SPR_KEY_SOL_SMALL]   = DEF_PNG(SHEET_PICKUPS, 7),
    [SPR_KEY_FA_SMALL]    = DEF_PNG(SHEET_PICKUPS, 8),
    [SPR_KEY_UT_SMALL]    = DEF_PNG(SHEET_PICKUPS, 9),
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
    if (!io) {
        SDL_Log("Planche absente : %s", path);
        return NULL;
    }
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

int sprite_ama_head_above_body(void)
{
    /* Grilles : tête de 45 lignes dont la ligne 0 est 2 px au-dessus du crâne, corps à y = 37. */
    return sheet_of(SPR_AMA_HEAD_FRONT) != SHEET_NONE ? SHEETS[SHEET_AMA_HEAD].cell_h - 8 : 39;
}

int sprite_width(SpriteId id)
{
    SheetId s = sheet_of(id);
    if (s != SHEET_NONE || !DEFS[id].rows) return SHEETS[DEFS[id].sheet].cell_w;
    return (int)SDL_strlen(DEFS[id].rows[0]);
}

int sprite_height(SpriteId id)
{
    SheetId s = sheet_of(id);
    if (s != SHEET_NONE || !DEFS[id].rows) return SHEETS[DEFS[id].sheet].cell_h;
    return DEFS[id].h;
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
    if (!d->rows) return sprite_height(id) - 1;
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
    if (!DEFS[id].rows) return;   /* planche absente et pas de grille de secours */
    if (!tex[id][k]) tex[id][k] = make_texture(r, id, white);
    if (!tex[id][k]) return;
    SDL_RenderTextureRotated(r, tex[id][k], NULL, &dst, 0.0, NULL, fm);
}
