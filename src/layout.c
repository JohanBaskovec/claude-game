#include "layout.h"
#include "config.h"
#include "floor.h"

/* Colonne de cases de la reliure, en coordonnées page (x = 2688 = 56 cases de 48 px). */
#define BINDING_TILE ((FLOOR_COLS * CELL_TILES_X) / 2)

static bool cell_in(const FloorRoom *fr, int cx, int cy)
{
    for (int i = 0; i < fr->cell_count; i++) {
        if (fr->cell_x[i] == cx && fr->cell_y[i] == cy) return true;
    }
    return false;
}

bool tile_is_pit(int kind)
{
    return kind == TILE_HOLE || kind == TILE_BURN || kind == TILE_WORM;
}

bool tile_is_hole(int kind)
{
    return tile_is_pit(kind) || kind == TILE_SEWN;
}

bool obstacle_blocks_walk(int type)
{
    return type != OBST_NEEDLES;
}

bool obstacle_blocks_shots(int type)
{
    return type == OBST_BOOKS || type == OBST_INKWELL || type == OBST_CANDLE;
}

SDL_FRect layout_tile_rect(const RoomLayout *L, int tx, int ty)
{
    return (SDL_FRect){ (float)L->cx0 * CELL_TILES_X * TILE_W + (float)tx * TILE_W,
                        (float)L->cy0 * CELL_TILES_Y * TILE_H + (float)ty * TILE_H, TILE_W, TILE_H };
}

bool layout_tile_at(const RoomLayout *L, float x, float y, int *tx, int *ty)
{
    float lx = x - (float)L->cx0 * CELL_TILES_X * TILE_W, ly = y - (float)L->cy0 * CELL_TILES_Y * TILE_H;
    if (lx < 0.0f || ly < 0.0f) return false;
    *tx = (int)(lx / TILE_W);
    *ty = (int)(ly / TILE_H);
    return *tx < L->w && *ty < L->h;
}

static int obstacle_on(const RoomLayout *L, int tx, int ty, bool alive_only)
{
    for (int i = 0; i < L->obstacle_count; i++) {
        const Obstacle *o = &L->obstacles[i];
        if (o->tx == tx && o->ty == ty && (o->alive || !alive_only)) return i;
    }
    return -1;
}

static bool same_type_at(const RoomLayout *L, int tx, int ty, int type)
{
    int i = (tx >= 0 && ty >= 0) ? obstacle_on(L, tx, ty, true) : -1;
    return i >= 0 && L->obstacles[i].type == type;
}

bool layout_walkable(const RoomLayout *L, int tx, int ty)
{
    if (tx < 0 || ty < 0 || tx >= L->w || ty >= L->h) return false;
    int k = L->tiles[ty][tx];
    if (k != TILE_FLOOR && k != TILE_SEWN) return false;
    int o = obstacle_on(L, tx, ty, true);
    return o < 0 || !obstacle_blocks_walk(L->obstacles[o].type);
}

static SDL_FRect centered(SDL_FRect t, float w, float h)
{
    return (SDL_FRect){ t.x + (t.w - w) * 0.5f, t.y + (t.h - h) * 0.5f, w, h };
}

SDL_FRect obstacle_rect(const RoomLayout *L, const Obstacle *o)
{
    SDL_FRect t = layout_tile_rect(L, o->tx, o->ty);
    switch (o->type) {
    case OBST_BOOKS:    return centered(t, 40.0f, 28.0f);
    case OBST_INKWELL:  return centered(t, 26.0f, 18.0f);
    case OBST_CANDLE:   return centered(t, 18.0f, 14.0f);
    case OBST_NEEDLES:  return centered(t, 40.0f, 32.0f);
    case OBST_THREAD:   return (SDL_FRect){ t.x - 4.0f, t.y, 8.0f, t.h };   /* le long du pli, au bord gauche de la case */
    default: {
        /* Signet : horizontal, sauf s'il continue en haut ou en bas. */
        bool vert = same_type_at(L, o->tx, o->ty - 1, OBST_RIBBON) || same_type_at(L, o->tx, o->ty + 1, OBST_RIBBON);
        return vert ? (SDL_FRect){ t.x + t.w * 0.5f - 6.0f, t.y, 12.0f, t.h }
                    : (SDL_FRect){ t.x, t.y + t.h * 0.5f - 6.0f, t.w, 12.0f };
    }
    }
}

SDL_FRect pit_rect(const RoomLayout *L, int tx, int ty)
{
    const float inset = 7.0f;
    SDL_FRect r = layout_tile_rect(L, tx, ty);
    float x0 = r.x, y0 = r.y, x1 = r.x + r.w, y1 = r.y + r.h;
    if (tx <= 0 || !tile_is_pit(L->tiles[ty][tx - 1])) x0 += inset;
    if (tx + 1 >= L->w || !tile_is_pit(L->tiles[ty][tx + 1])) x1 -= inset;
    if (ty <= 0 || !tile_is_pit(L->tiles[ty - 1][tx])) y0 += inset;
    if (ty + 1 >= L->h || !tile_is_pit(L->tiles[ty + 1][tx])) y1 -= inset;
    return (SDL_FRect){ x0, y0, x1 - x0, y1 - y0 };
}

/* Règle d'accès, en comptant en plus la case (ex, ey) (-1 : aucune) et les cases de `pending`
 * (ou NULL) comme infranchissables. */
static bool connected_except(const RoomLayout *L, int ex, int ey, const TileMask *pending)
{
    static Uint8 seen[LAYOUT_H][LAYOUT_W];
    static Uint16 queue[LAYOUT_W * LAYOUT_H];
    SDL_memset(seen, 0, sizeof seen);
    int total = 0, start = -1;
    for (int y = 0; y < L->h; y++) {
        for (int x = 0; x < L->w; x++) {
            if (!layout_walkable(L, x, y) || (x == ex && y == ey) || (pending && (*pending)[y][x])) continue;
            total++;
            if (start < 0) start = y * LAYOUT_W + x;
        }
    }
    if (start < 0) return false;
    int head = 0, tail = 0, reached = 0;
    queue[tail++] = (Uint16)start;
    seen[start / LAYOUT_W][start % LAYOUT_W] = 1;
    static const int DX[4] = { 1, -1, 0, 0 }, DY[4] = { 0, 0, 1, -1 };
    while (head < tail) {
        int c = queue[head++], x = c % LAYOUT_W, y = c / LAYOUT_W;
        reached++;
        for (int k = 0; k < 4; k++) {
            int nx = x + DX[k], ny = y + DY[k];
            if (nx < 0 || ny < 0 || nx >= L->w || ny >= L->h || seen[ny][nx] || !layout_walkable(L, nx, ny)) continue;
            if ((nx == ex && ny == ey) || (pending && (*pending)[ny][nx])) continue;
            seen[ny][nx] = 1;
            queue[tail++] = (Uint16)(ny * LAYOUT_W + nx);
        }
    }
    return reached == total;
}

bool layout_connected(const RoomLayout *L)
{
    return connected_except(L, -1, -1, NULL);
}

int layout_hole_count(const RoomLayout *L)
{
    int n = 0;
    for (int y = 0; y < L->h; y++) {
        for (int x = 0; x < L->w; x++) n += tile_is_hole(L->tiles[y][x]) ? 1 : 0;
    }
    return n;
}

bool layout_can_worm_hole(const RoomLayout *L, int tx, int ty, const TileMask *pending)
{
    if (tx < 0 || ty < 0 || tx >= L->w || ty >= L->h) return false;
    if (L->tiles[ty][tx] != TILE_FLOOR || L->reserved[ty][tx] || (pending && (*pending)[ty][tx])) return false;
    if (obstacle_on(L, tx, ty, true) >= 0) return false;
    return connected_except(L, tx, ty, pending);
}

bool layout_add_worm_hole(RoomLayout *L, int tx, int ty, const TileMask *pending)
{
    if (!layout_can_worm_hole(L, tx, ty, pending)) return false;
    L->tiles[ty][tx] = TILE_WORM;
    L->worm_holes++;
    L->version++;
    return true;
}

bool layout_worm_spot(const RoomLayout *L, float x, float y, const TileMask *pending, int *tx, int *ty)
{
    static Uint8 tried[LAYOUT_H][LAYOUT_W];
    SDL_memset(tried, 0, sizeof tried);
    /* Cases de la plus proche à la plus lointaine ; la règle d'accès n'est vérifiée que sur les meilleures.
     * D'abord une case où percer un nouveau trou ; un trou de ver déjà percé seulement s'il n'y en a plus. */
    for (int attempt = 0; attempt < 2 * LAYOUT_W * LAYOUT_H; attempt++) {
        int want = attempt < LAYOUT_W * LAYOUT_H ? TILE_FLOOR : TILE_WORM;
        if (attempt == LAYOUT_W * LAYOUT_H) SDL_memset(tried, 0, sizeof tried);
        int bx = -1, by = -1;
        float best = 1e30f;
        for (int cy = 0; cy < L->h; cy++) {
            for (int cx = 0; cx < L->w; cx++) {
                if (tried[cy][cx] || L->tiles[cy][cx] != want || L->reserved[cy][cx] || (pending && (*pending)[cy][cx])) continue;
                SDL_FRect t = layout_tile_rect(L, cx, cy);
                float dx = t.x + t.w * 0.5f - x, dy = t.y + t.h * 0.5f - y, d = dx * dx + dy * dy;
                if (d < best) { best = d; bx = cx; by = cy; }
            }
        }
        if (bx < 0) {
            if (want == TILE_WORM) return false;
            attempt = LAYOUT_W * LAYOUT_H - 1;   /* plus de case libre : on passe aux trous existants */
            continue;
        }
        tried[by][bx] = 1;
        if (L->tiles[by][bx] == TILE_WORM || layout_can_worm_hole(L, bx, by, pending)) {
            *tx = bx;
            *ty = by;
            return true;
        }
    }
    return false;
}

/* ------------------------------------------------------------ tirage */

static void reserve(RoomLayout *L, int x0, int y0, int x1, int y1)
{
    for (int y = SDL_max(y0, 0); y <= y1 && y < L->h; y++) {
        for (int x = SDL_max(x0, 0); x <= x1 && x < L->w; x++) L->reserved[y][x] = 1;
    }
}

typedef enum Feature { F_HOLE, F_BOOKS, F_INKWELL, F_CANDLE, F_NEEDLES, F_RIBBON, F_THREAD, F_COUNT } Feature;

#define MAX_FEATURE_TILES 32

typedef struct Shape {
    int n;
    int x[MAX_FEATURE_TILES], y[MAX_FEATURE_TILES];
} Shape;

static void shape_add(Shape *s, int x, int y)
{
    for (int i = 0; i < s->n; i++) {
        if (s->x[i] == x && s->y[i] == y) return;
    }
    if (s->n < MAX_FEATURE_TILES) {
        s->x[s->n] = x;
        s->y[s->n] = y;
        s->n++;
    }
}

static int rnd(Uint64 *rng, int n)
{
    return n > 0 ? SDL_rand_r(rng, n) : 0;
}

/* Forme de base d'un élément, à partir de la case (ax, ay). */
static void base_shape(Feature f, int ax, int ay, Uint64 *rng, Shape *s)
{
    s->n = 0;
    switch (f) {
    case F_HOLE: {
        static const int SHAPES[5][4][2] = {
            { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },   /* 1 case */
            { { 0, 0 }, { 1, 0 }, { 1, 0 }, { 1, 0 } },   /* 2 x 1 */
            { { 0, 0 }, { 0, 1 }, { 0, 1 }, { 0, 1 } },   /* 1 x 2 */
            { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } },   /* 2 x 2 */
            { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 0, 1 } },   /* en L */
        };
        int k = rnd(rng, 5);
        for (int i = 0; i < 4; i++) shape_add(s, ax + SHAPES[k][i][0], ay + SHAPES[k][i][1]);
        break;
    }
    case F_BOOKS:
        shape_add(s, ax, ay);
        if (rnd(rng, 2)) shape_add(s, ax + 1, ay);
        break;
    case F_NEEDLES: {
        int k = rnd(rng, 3);
        int len = 2 + rnd(rng, 2);
        for (int i = 0; i < len; i++) shape_add(s, k == 1 ? ax : ax + i, k == 1 ? ay + i : ay);
        if (k == 2) { shape_add(s, ax, ay + 1); shape_add(s, ax + 1, ay + 1); }
        break;
    }
    case F_RIBBON: {
        bool vert = rnd(rng, 3) == 0;
        int len = (vert ? 2 : 3) + rnd(rng, 3);
        for (int i = 0; i < len; i++) shape_add(s, vert ? ax : ax + i, vert ? ay + i : ay);
        break;
    }
    case F_THREAD: {
        int len = 2 + rnd(rng, 2);
        for (int i = 0; i < len; i++) shape_add(s, ax, ay + i);
        break;
    }
    default:
        shape_add(s, ax, ay);
        break;
    }
}

typedef enum Symmetry { SYM_NONE, SYM_X, SYM_XY } Symmetry;

/* Essaie de poser un élément (et ses copies symétriques). Refusé s'il casse la règle d'accès. */
static bool try_feature(RoomLayout *L, Feature f, Symmetry sym, int binding_tx, Uint64 *rng)
{
    int ax, ay;
    if (f == F_THREAD) {
        if (binding_tx < 0) return false;
        ax = binding_tx;
        ay = rnd(rng, L->h);
        sym = SYM_NONE;
    } else {
        ax = rnd(rng, L->w);
        ay = rnd(rng, L->h);
    }
    Shape base, all = { 0 };
    base_shape(f, ax, ay, rng, &base);
    for (int i = 0; i < base.n; i++) shape_add(&all, base.x[i], base.y[i]);   /* d'abord la forme de base */
    for (int i = 0; i < base.n; i++) {
        int x = base.x[i], y = base.y[i];
        if (sym >= SYM_X) shape_add(&all, L->w - 1 - x, y);
        if (sym == SYM_XY) {
            shape_add(&all, x, L->h - 1 - y);
            shape_add(&all, L->w - 1 - x, L->h - 1 - y);
        }
    }
    /* Une copie qui tombe hors de la salle (salle en L) est oubliée ; le reste doit être libre. */
    Shape ok = { 0 };
    for (int i = 0; i < all.n; i++) {
        int x = all.x[i], y = all.y[i];
        if (x < 0 || y < 0 || x >= L->w || y >= L->h) {
            if (i < base.n) return false;
            continue;
        }
        if (L->tiles[y][x] == TILE_VOID) {
            if (i < base.n) return false;
            continue;
        }
        if (L->tiles[y][x] != TILE_FLOOR || L->reserved[y][x] || obstacle_on(L, x, y, false) >= 0) return false;
        shape_add(&ok, x, y);
    }
    if (ok.n == 0) return false;

    int saved_count = L->obstacle_count;
    if (f == F_HOLE) {
        int roll = rnd(rng, 100);
        int kind = roll < 55 ? TILE_HOLE : roll < 80 ? TILE_BURN : TILE_SEWN;
        for (int i = 0; i < ok.n; i++) L->tiles[ok.y[i]][ok.x[i]] = (Uint8)kind;
    } else {
        static const Uint8 TYPE[F_COUNT] = {
            [F_BOOKS] = OBST_BOOKS, [F_INKWELL] = OBST_INKWELL,
            [F_CANDLE] = OBST_CANDLE, [F_NEEDLES] = OBST_NEEDLES, [F_RIBBON] = OBST_RIBBON, [F_THREAD] = OBST_THREAD,
        };
        if (L->obstacle_count + ok.n > MAX_OBSTACLES) return false;
        int group = rnd(rng, 3);
        for (int i = 0; i < ok.n; i++) {
            Obstacle *o = &L->obstacles[L->obstacle_count++];
            *o = (Obstacle){ TYPE[f], (Uint8)ok.x[i], (Uint8)ok.y[i], (Uint8)rnd(rng, 8), true, 0.0f };
            if (f == F_NEEDLES) o->variant = (Uint8)group;   /* un groupe sort en même temps */
            if (f == F_INKWELL) o->hp = tuning.inkwell_hp;
            if (f == F_CANDLE) o->hp = 1.0f;
        }
    }
    if (layout_connected(L)) return true;
    /* Refusé : on remet tout comme avant. */
    if (f == F_HOLE) {
        for (int i = 0; i < ok.n; i++) L->tiles[ok.y[i]][ok.x[i]] = TILE_FLOOR;
    }
    L->obstacle_count = saved_count;
    return false;
}

static Feature pick_feature(bool dark, bool binding, Uint64 *rng)
{
    int weight[F_COUNT] = {
        [F_HOLE] = 4, [F_BOOKS] = 4, [F_INKWELL] = 3,
        [F_CANDLE] = dark ? 4 : 2, [F_NEEDLES] = 2, [F_RIBBON] = 1, [F_THREAD] = binding ? 3 : 0,
    };
    int total = 0;
    for (int i = 0; i < F_COUNT; i++) total += weight[i];
    int r = rnd(rng, total);
    for (int i = 0; i < F_COUNT; i++) {
        if (r < weight[i]) return (Feature)i;
        r -= weight[i];
    }
    return F_BOOKS;
}

void layout_generate(RoomLayout *L, const FloorRoom *fr, Uint64 *rng)
{
    SDL_memset(L, 0, sizeof *L);
    int minx = 1 << 20, miny = 1 << 20, maxx = -1, maxy = -1;
    for (int i = 0; i < fr->cell_count; i++) {
        minx = SDL_min(minx, fr->cell_x[i]);
        miny = SDL_min(miny, fr->cell_y[i]);
        maxx = SDL_max(maxx, fr->cell_x[i]);
        maxy = SDL_max(maxy, fr->cell_y[i]);
    }
    L->cx0 = minx;
    L->cy0 = miny;
    L->w = (maxx - minx + 1) * CELL_TILES_X;
    L->h = (maxy - miny + 1) * CELL_TILES_Y;
    for (int y = 0; y < L->h; y++) {
        for (int x = 0; x < L->w; x++) {
            L->tiles[y][x] = cell_in(fr, minx + x / CELL_TILES_X, miny + y / CELL_TILES_Y) ? TILE_FLOOR : TILE_VOID;
        }
    }
    /* Devant chaque porte, et au centre de chaque cellule (prix, trappe, téléportation) : libre. */
    for (int i = 0; i < fr->cell_count; i++) {
        int lx = (fr->cell_x[i] - minx) * CELL_TILES_X, ly = (fr->cell_y[i] - miny) * CELL_TILES_Y;
        reserve(L, lx + 7, ly + 4, lx + 8, ly + 4);
    }
    for (int k = 0; k < fr->door_count; k++) {
        const FloorDoor *d = &fr->doors[k];
        int lx = (d->cell_x - minx) * CELL_TILES_X, ly = (d->cell_y - miny) * CELL_TILES_Y;
        switch (d->side) {
        case DIR_UP:    reserve(L, lx + 6, ly, lx + 9, ly + 2); break;
        case DIR_DOWN:  reserve(L, lx + 6, ly + 6, lx + 9, ly + 8); break;
        case DIR_LEFT:  reserve(L, lx, ly + 3, lx + 2, ly + 5); break;
        default:        reserve(L, lx + 13, ly + 3, lx + 15, ly + 5); break;
        }
    }
    if (fr->kind != ROOM_NORMAL) return;

    int binding_tx = BINDING_TILE - minx * CELL_TILES_X;
    if (binding_tx <= 0 || binding_tx >= L->w) binding_tx = -1;
    L->dark = fr->depth >= 2 && (float)rnd(rng, 1000) < tuning.dark_room_chance * 1000.0f;
    int span = SDL_max(tuning.layout_features_max - tuning.layout_features_min, 0);
    int target = fr->cell_count * (tuning.layout_features_min + rnd(rng, span + 1));
    Symmetry sym = (float)rnd(rng, 1000) < tuning.layout_symmetry * 1000.0f ? (rnd(rng, 3) ? SYM_X : SYM_XY) : SYM_NONE;

    int placed = 0;
    /* Une salle sombre a toujours des chandelles. */
    for (int attempt = 0; L->dark && placed < 2 && attempt < 60; attempt++) {
        if (try_feature(L, F_CANDLE, sym, binding_tx, rng)) placed++;
    }
    for (int attempt = 0; placed < target && attempt < target * 8; attempt++) {
        if (try_feature(L, pick_feature(L->dark, binding_tx >= 0, rng), sym, binding_tx, rng)) placed++;
    }
}
