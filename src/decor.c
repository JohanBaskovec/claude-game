#include "decor.h"
#include "sprite.h"
#include "font.h"

#include <math.h>
#include <string.h>

/*
 * Portage en C de docs/concept/outils_concept/room_base.py (géométrie des murs et des portes)
 * et page.py (mise en page du manuscrit). Tout est en coordonnées « page » du GDD.
 * Phase 12 (GDD §21) : sol en parchemin avec sa réglure, murs peints façon tables des canons,
 * trous dans le parchemin, petites choses oubliées sur la page et marginalia.
 */

/* ------------------------------------------------------------ double page (GDD §9) */
#define PAGE_W      ((float)FLOOR_COLS * CELL_W)   /* 5376 */
#define PAGE_H      ((float)FLOOR_ROWS * CELL_H)   /* 3600 */
#define BINDING_X   (PAGE_W * 0.5f)                /* reliure au milieu de la colonne 3 */
#define MARGIN_OUT  150.0f
#define MARGIN_IN   120.0f
#define LINE0       46.0f                           /* ligne de base de la ligne 0 */
#define PITCH       60.0f                           /* 6 lignes par cellule */
#define LINES       60                              /* lignes par page (la première et la dernière restent vides) */
#define STITCH_STEP 120.0f                          /* points de couture de la reliure */

/* ------------------------------------------------------------ murs et portes (room_base.py) */
#define BRICKS_H    6           /* arcades par cellule, murs du haut et du bas */
#define BRICKS_V    3           /* arcades par cellule, murs de côté */
#define DOOR_T0     0.14f       /* la porte commence un peu sous le chaperon */
#define DOOR_ARCH   0.40f       /* hauteur de l'arc (en fraction de la porte) */
#define DOOR_HALF_H 0.065f      /* demi-largeur de la porte, murs du haut et du bas (≈ 50 px au pied) */
#define DOOR_HALF_V 0.11f       /* idem murs de côté (≈ 40 px au pied) */
#define FRAME_GROW  0.30f       /* épaisseur de l'encadrement de claveaux */

#define TEX_MAX_W   (2 * (int)CELL_W + 2 * (int)WALL_SIDE)
#define TEX_MAX_H   (2 * (int)CELL_H + (int)WALL_TOP + (int)WALL_BOTTOM)
#define DOOR_BUF    (256 * 256)

static const float INK[3] = { 58, 40, 30 }, RED[3] = { 150, 52, 38 };
#define INK_OPACITY 0.5f

/* ------------------------------------------------------------ hachage et bruit */
static Uint32 hash3(int a, int b, int c)
{
    Uint32 h = (Uint32)a * 0x8da6b343u ^ (Uint32)b * 0xd8163841u ^ (Uint32)c * 0xcb1ab31fu;
    h ^= h >> 16; h *= 0x7feb352du;
    h ^= h >> 15; h *= 0x846ca68bu;
    h ^= h >> 16;
    return h;
}

static float hash01(int a, int b, int c)
{
    return (float)(hash3(a, b, c) >> 8) * (1.0f / 16777216.0f);
}

/* Bruit fractal (0,5·grain 48 px + 0,3·12 px + 0,2·3 px, comme fbm() du prototype),
 * précalculé une fois dans une table qui se répète tous les NOISE_N px. */
#define NOISE_N 384
static float fbm_table[NOISE_N * NOISE_N];
static bool  noise_ready;

enum { NZ_GRAIN, NZ_CHIP, NZ_WOOD, NZ_FLOOR, NZ_WEAR, NZ_COUNT };
static int    noise_off[NZ_COUNT][2];   /* chaque usage lit la table à un endroit différent */
static Uint32 floor_hash;               /* graine de l'étage, pour les petites choses de la page */

static int wrap(int v, int n)
{
    v %= n;
    return v < 0 ? v + n : v;
}

static float smooth(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

static void noise_build(void)
{
    static const int   cell[3] = { 48, 12, 3 };
    static const float weight[3] = { 0.5f, 0.3f, 0.2f };
    SDL_memset(fbm_table, 0, sizeof fbm_table);
    for (int o = 0; o < 3; o++) {
        int lat = NOISE_N / cell[o];
        for (int y = 0; y < NOISE_N; y++) {
            float fy = ((float)y + 0.5f) / (float)cell[o] - 0.5f;
            int iy = (int)floorf(fy);
            float ty = smooth(fy - (float)iy);
            int y0 = wrap(iy, lat), y1 = wrap(iy + 1, lat);
            for (int x = 0; x < NOISE_N; x++) {
                float fx = ((float)x + 0.5f) / (float)cell[o] - 0.5f;
                int ix = (int)floorf(fx);
                float tx = smooth(fx - (float)ix);
                int x0 = wrap(ix, lat), x1 = wrap(ix + 1, lat);
                float a = hash01(x0, y0, 100 + o), b = hash01(x1, y0, 100 + o);
                float c = hash01(x0, y1, 100 + o), d = hash01(x1, y1, 100 + o);
                float v = (a + (b - a) * tx) + ((c + (d - c) * tx) - (a + (b - a) * tx)) * ty;
                fbm_table[y * NOISE_N + x] += weight[o] * v;
            }
        }
    }
    noise_ready = true;
}

static float fbm(int which, float x, float y)
{
    int ix = wrap((int)floorf(x) + noise_off[which][0], NOISE_N);
    int iy = wrap((int)floorf(y) + noise_off[which][1], NOISE_N);
    return fbm_table[iy * NOISE_N + ix];
}

/* ------------------------------------------------------------ mise en page du manuscrit (page.py) */
typedef struct Chant {
    const char *title;
    const char *text;
} Chant;

/* Textes liturgiques latins du haut Moyen Âge (domaine public). « \xC2\xB7 » = point médian. */
static const Chant CHANTS[] = {
    { "IN NATIVITATE DOMINI \xC2\xB7 INTROITUS",
      "Puer natus est nobis et filius datus est nobis cuius imperium super humerum eius et vocabitur nomen eius "
      "magni consilii angelus. Cantate domino canticum novum quia mirabilia fecit." },
    { "HYMNUS \xC2\xB7 IN NATIVITATE S. IOHANNIS BAPTISTAE",
      "Ut queant laxis resonare fibris mira gestorum famuli tuorum solve polluti labii reatum sancte Iohannes. "
      "Nuntius celso veniens Olympo te patri magnum fore nasciturum nomen et vitae seriem gerendae ordine promit." },
    { "HYMNUS \xC2\xB7 IN FESTO PENTECOSTES",
      "Veni creator spiritus mentes tuorum visita imple superna gratia quae tu creasti pectora. "
      "Qui diceris paraclitus altissimi donum dei fons vivus ignis caritas et spiritalis unctio." },
    { "KYRIE", "Kyrie eleison. Christe eleison. Kyrie eleison." },
    { "HYMNUS \xC2\xB7 IN FESTIS BEATAE MARIAE",
      "Ave maris stella dei mater alma atque semper virgo felix caeli porta. "
      "Sumens illud ave gabrielis ore funda nos in pace mutans evae nomen." },
    { "GLORIA",
      "Gloria in excelsis deo et in terra pax hominibus bonae voluntatis. Laudamus te. Benedicimus te. "
      "Adoramus te. Glorificamus te. Gratias agimus tibi propter magnam gloriam tuam." },
    { "HYMNUS \xC2\xB7 TE DEUM",
      "Te deum laudamus te dominum confitemur. Te aeternum patrem omnis terra veneratur. "
      "Tibi omnes angeli tibi caeli et universae potestates." },
    { "AGNUS DEI", "Agnus dei qui tollis peccata mundi miserere nobis. Agnus dei qui tollis peccata mundi dona nobis pacem." },
};
#define CHANT_COUNT ((int)(sizeof CHANTS / sizeof CHANTS[0]))

typedef enum Neume {
    NEUME_PUNCTUM, NEUME_TRACTULUS, NEUME_VIRGA, NEUME_PES, NEUME_CLIVIS,
    NEUME_TORCULUS, NEUME_PORRECTUS, NEUME_CLIMACUS, NEUME_SCANDICUS, NEUME_QUILISMA,
} Neume;
static const float NEUME_W[] = { 5, 6, 7, 7, 8, 9, 9, 11, 10, 8 };
/* Tirage : la virga, le pes et la clivis sont deux fois plus fréquents. */
static const Uint8 NEUME_DRAW[] = {
    NEUME_PUNCTUM, NEUME_TRACTULUS, NEUME_VIRGA, NEUME_VIRGA, NEUME_PES, NEUME_PES, NEUME_CLIVIS, NEUME_CLIVIS,
    NEUME_TORCULUS, NEUME_PORRECTUS, NEUME_CLIMACUS, NEUME_SCANDICUS, NEUME_QUILISMA,
};

typedef enum OpKind { OP_SYL, OP_NEUME, OP_RUBRIC, OP_VERSAL, OP_CAP } OpKind;

/* Un élément posé sur la page : (x, y) en coordonnées page, comme les « ops » du prototype. */
typedef struct PageOp {
    float  x, y;
    Uint8  kind, neume;
    Uint32 text;    /* début du texte dans text_pool */
} PageOp;

#define MAX_OPS   40000
#define TEXT_POOL 131072
static PageOp ops[MAX_OPS];
static int    op_count;
static char   text_pool[TEXT_POOL];
static Uint32 text_used;

static void add_op(float x, float y, OpKind kind, int neume, const char *s, int len)
{
    if (op_count >= MAX_OPS) return;
    PageOp *op = &ops[op_count];
    *op = (PageOp){ x, y, (Uint8)kind, (Uint8)neume, 0 };
    if (s) {
        if (len < 0) len = (int)SDL_strlen(s);
        if (text_used + (Uint32)len + 1 > TEXT_POOL) return;
        op->text = text_used;
        SDL_memcpy(text_pool + text_used, s, (size_t)len);
        text_pool[text_used + (Uint32)len] = '\0';
        text_used += (Uint32)len + 1;
    }
    op_count++;
}

static bool is_vowel(char c)
{
    c = (char)SDL_tolower((unsigned char)c);
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y';
}

/*
 * Syllabation latine simplifiée (GDD §9) : une consonne seule va avec la voyelle suivante,
 * deux consonnes se séparent sauf muta cum liquida et ch, ph, th, gn ; diphtongues ae, oe, au ;
 * qu = une consonne ; i consonne au début d'un mot ou entre deux voyelles.
 * Remplit starts[] (début de chaque syllabe dans le mot) et renvoie le nombre de syllabes.
 */
#define MAX_UNITS 48
static int syllabify(const char *w, int n, int *starts)
{
    int ustart[MAX_UNITS], ulen[MAX_UNITS];
    bool uvowel[MAX_UNITS];
    int nu = 0;
    for (int i = 0; i < n && nu < MAX_UNITS;) {
        char c = (char)SDL_tolower((unsigned char)w[i]);
        char c1 = (i + 1 < n) ? (char)SDL_tolower((unsigned char)w[i + 1]) : '\0';
        int len = 1;
        bool vowel = false;
        if (c == 'q' && c1 == 'u') {
            len = 2;
        } else if (is_vowel(c)) {
            bool prev_v = i > 0 && is_vowel(w[i - 1]);
            bool next_v = c1 && is_vowel(c1);
            if ((c == 'a' && c1 == 'e') || (c == 'o' && c1 == 'e') || (c == 'a' && c1 == 'u')) {
                len = 2;
                vowel = true;
            } else {
                vowel = !(c == 'i' && next_v && (i == 0 || prev_v));   /* i consonne : iam, e-ius */
            }
        }
        ustart[nu] = i;
        ulen[nu] = len;
        uvowel[nu] = vowel;
        nu++;
        i += len;
    }
    int nuclei[MAX_UNITS], nn = 0;
    for (int k = 0; k < nu; k++) {
        if (uvowel[k]) nuclei[nn++] = k;
    }
    starts[0] = 0;
    if (nn <= 1) return 1;
    int count = 1;
    for (int k = 0; k + 1 < nn; k++) {
        int a = nuclei[k], b = nuclei[k + 1], cut;
        int ncons = b - a - 1;
        if (ncons == 0) {
            cut = b;
        } else if (ncons == 1) {
            cut = a + 1;
        } else {
            int u2 = b - 2, u1 = b - 1;   /* les deux dernières consonnes */
            char p = (char)SDL_tolower((unsigned char)w[ustart[u2] + ulen[u2] - 1]);
            char q = (char)SDL_tolower((unsigned char)w[ustart[u1]]);
            bool muta = ulen[u2] == 1 && SDL_strchr("bcdgptf", p) && ulen[u1] == 1 && (q == 'l' || q == 'r');
            bool pair = (q == 'h' && (p == 'c' || p == 'p' || p == 't')) || (p == 'g' && q == 'n');
            cut = (muta || pair) ? b - 2 : b - 1;
        }
        if (ustart[cut] > starts[count - 1]) starts[count++] = ustart[cut];
    }
    return count;
}

/* Trous de la double page (en cases de 48 x 40), pour que le texte les contourne. */
#define PAGE_TX (FLOOR_COLS * CELL_TILES_X)
#define PAGE_TY (FLOOR_ROWS * CELL_TILES_Y)
static Uint8 page_holes[PAGE_TY][PAGE_TX];

/* Si [x0, x1] sur la ligne de base `base` (neumes compris) touche un trou, renvoie le x
 * juste après ce trou ; sinon -1. */
static float hole_skip(float x0, float x1, float base)
{
    int ty0 = SDL_max((int)floorf((base - 46.0f) / TILE_H), 0), ty1 = SDL_min((int)floorf((base + 8.0f) / TILE_H), PAGE_TY - 1);
    int tx0 = SDL_max((int)floorf(x0 / TILE_W), 0), tx1 = SDL_min((int)floorf(x1 / TILE_W), PAGE_TX - 1);
    float skip = -1.0f;
    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            if (page_holes[ty][tx]) skip = SDL_max(skip, (float)(tx + 1) * TILE_W + 6.0f);
        }
    }
    return skip;
}

typedef struct Layout {
    float   x, x0, x1;
    int     line;
    int     res_line;          /* ligne dont une partie est prise par la grande lettrine */
    float   res_x0, res_x1;
    Uint64  rng;
} Layout;

static float baseline(const Layout *L)
{
    return LINE0 + (float)L->line * PITCH;
}

static void newline(Layout *L)
{
    L->line++;
    L->x = L->x0;
    if (L->line == L->res_line && L->x >= L->res_x0 && L->x < L->res_x1) L->x = L->res_x1;
}

/* Place pour un élément de largeur w : après la lettrine, à la ligne si besoin, et jamais sur un trou. */
static void fit(Layout *L, float w)
{
    for (int guard = 0; guard < 256; guard++) {
        if (L->line == L->res_line && L->x < L->res_x1 && L->x + w > L->res_x0) L->x = L->res_x1;
        if (L->x + w > L->x1) {
            newline(L);
            if (L->line >= LINES - 1) return;
            continue;
        }
        float skip = hole_skip(L->x, L->x + w, baseline(L));
        if (skip > L->x) {
            L->x = skip;
            continue;
        }
        return;
    }
}

static int rand_int(Layout *L, int lo, int hi)
{
    return lo + SDL_rand_r(&L->rng, hi - lo + 1);
}

/* Remplit une page de chants, à partir du chant *ci. Elle commence par une grande lettrine. */
static void layout_page(Layout *L, int *ci)
{
    bool first = true, full = false;
    while (!full && L->line < LINES - 1) {
        const Chant *c = &CHANTS[*ci % CHANT_COUNT];
        (*ci)++;
        /* Rubrique rouge */
        float w = font_text_width(FONT_RUBRIC, c->title) + 14.0f;
        fit(L, w);
        if (L->line >= LINES - 1) break;
        add_op(L->x, baseline(L), OP_RUBRIC, 0, c->title, -1);
        L->x += w;

        const char *p = c->text;
        bool first_word = true;
        while (*p && !full) {
            while (*p == ' ') p++;
            if (!*p) break;
            const char *word = p;
            while (*p && *p != ' ') p++;
            int len = (int)(p - word);

            if (first_word) {
                first_word = false;
                char letter[2] = { word[0], '\0' };
                word++;
                len--;
                if (first) {
                    /* Grande lettrine sur deux lignes, au début de la page. */
                    fit(L, 64.0f);
                    add_op(L->x, baseline(L) - 62.0f, OP_CAP, 0, letter, 1);
                    L->res_line = L->line + 1;
                    L->res_x0 = L->x - 4.0f;
                    L->res_x1 = L->x + 62.0f;
                    L->x += 58.0f;
                    first = false;
                } else {
                    /* Initiale rouge sur une ligne. */
                    w = font_text_width(FONT_VERSAL, letter) + 4.0f;
                    fit(L, w);
                    add_op(L->x, baseline(L), OP_VERSAL, 0, letter, 1);
                    L->x += w;
                }
            }
            if (len <= 0) continue;

            int starts[MAX_UNITS];
            int ns = syllabify(word, len, starts);
            for (int k = 0; k < ns; k++) {
                int end = (k + 1 < ns) ? starts[k + 1] : len;
                char syl[64];
                int sl = SDL_min(end - starts[k], (int)sizeof syl - 1);
                SDL_memcpy(syl, word + starts[k], (size_t)sl);
                syl[sl] = '\0';

                /* 1 à 3 neumes au-dessus de la syllabe (70 %, 22 %, 8 %). */
                int roll = SDL_rand_r(&L->rng, 100);
                int n = roll < 70 ? 1 : roll < 92 ? 2 : 3;
                Uint8 kinds[3];
                float nw = 0.0f;
                for (int q = 0; q < n; q++) {
                    kinds[q] = NEUME_DRAW[SDL_rand_r(&L->rng, (Sint32)sizeof NEUME_DRAW)];
                    nw += NEUME_W[kinds[q]] + 3.0f;
                }
                float tw = font_text_width(FONT_TEXT, syl);
                float box = SDL_max(nw, tw);
                float slot = box + (k < ns - 1 ? 5.0f : 13.0f);
                fit(L, slot);
                if (L->line >= LINES - 1) {
                    full = true;
                    break;
                }
                add_op(L->x + (box - tw) * 0.5f, baseline(L), OP_SYL, 0, syl, sl);
                float nx = L->x + (box - nw) * 0.5f;
                float ny = baseline(L) - 36.0f + (float)rand_int(L, -3, 3);
                for (int q = 0; q < n; q++) {
                    add_op(nx, ny + (float)rand_int(L, -2, 2), OP_NEUME, kinds[q], NULL, 0);
                    nx += NEUME_W[kinds[q]] + 3.0f;
                }
                L->x += slot;
            }
        }
        L->x += 10.0f;
    }
}

/* Le texte remplit la page de gauche, puis continue en haut de la page de droite. */
static void layout_double_page(Uint64 seed)
{
    op_count = 0;
    text_used = 0;
    if (!font_ready()) return;
    int ci = 0;
    Uint64 rng = seed ^ 0x6e65756d65ull;
    for (int page = 0; page < 2; page++) {
        Layout L = { 0 };
        L.x0 = page == 0 ? MARGIN_OUT : BINDING_X + MARGIN_IN;
        L.x1 = page == 0 ? BINDING_X - MARGIN_IN : PAGE_W - MARGIN_OUT;
        L.x = L.x0;
        L.line = 1;   /* la ligne 0 est la marge du haut */
        L.res_line = -1;
        L.rng = rng;
        layout_page(&L, &ci);
        rng = L.rng;
    }
}

int decor_page_ops(void)
{
    return op_count;
}

/* ------------------------------------------------------------ encre : tampons de couverture */
typedef struct Coverage {
    Uint8 *buf;
    int    w, h;
    float  ox, oy;   /* coin haut-gauche du tampon en coordonnées page */
} Coverage;

static void cov_put(const Coverage *c, int x, int y, float v)
{
    if (x < 0 || y < 0 || x >= c->w || y >= c->h || v <= 0.0f) return;
    Uint8 b = (Uint8)(SDL_min(v, 1.0f) * 255.0f);
    Uint8 *d = &c->buf[y * c->w + x];
    if (b > *d) *d = b;
}

/* Trait de plume arrondi de A à B, demi-épaisseur hw (coordonnées page). */
static void cov_segment(const Coverage *c, float ax, float ay, float bx, float by, float hw)
{
    ax -= c->ox; bx -= c->ox;
    ay -= c->oy; by -= c->oy;
    int x0 = (int)floorf(SDL_min(ax, bx) - hw - 1.0f), x1 = (int)ceilf(SDL_max(ax, bx) + hw + 1.0f);
    int y0 = (int)floorf(SDL_min(ay, by) - hw - 1.0f), y1 = (int)ceilf(SDL_max(ay, by) + hw + 1.0f);
    float dx = bx - ax, dy = by - ay, len2 = dx * dx + dy * dy;
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float px = (float)x + 0.5f - ax, py = (float)y + 0.5f - ay;
            float t = len2 > 0.0f ? SDL_clamp((px * dx + py * dy) / len2, 0.0f, 1.0f) : 0.0f;
            float ex = px - t * dx, ey = py - t * dy;
            cov_put(c, x, y, hw + 0.5f - sqrtf(ex * ex + ey * ey));
        }
    }
}

static void cov_polyline(const Coverage *c, float x, float y, const float (*pts)[2], int n)
{
    for (int i = 0; i + 1 < n; i++) {
        cov_segment(c, x + pts[i][0], y + pts[i][1], x + pts[i + 1][0], y + pts[i + 1][1], 0.75f);
    }
}

static void cov_dot(const Coverage *c, float x, float y)
{
    cov_segment(c, x, y, x, y, 1.6f - 0.5f);
}

/* Les neumes de Saint-Gall du prototype, dessinés depuis (x, y). */
static void draw_neume(const Coverage *c, int kind, float x, float y)
{
#define LINE(...) do { static const float p[][2] = { __VA_ARGS__ }; \
                       cov_polyline(c, x, y, p, (int)(sizeof p / sizeof p[0])); } while (0)
    switch (kind) {
    case NEUME_PUNCTUM:   cov_dot(c, x + 2, y + 6); break;
    case NEUME_TRACTULUS: LINE({ 0, 6 }, { 5, 5 }); break;
    case NEUME_VIRGA:     LINE({ 0, 11 }, { 4, 0 }, { 6, -1 }); break;
    case NEUME_PES:       LINE({ 0, 7 }, { 2, 10 }, { 5, 8 }, { 6, -1 }); break;
    case NEUME_CLIVIS:    LINE({ 0, 9 }, { 2, 0 }, { 5, -1 }, { 7, 11 }); break;
    case NEUME_TORCULUS:  LINE({ 0, 6 }, { 2, 9 }, { 4, 1 }, { 6, 0 }, { 8, 9 }); break;
    case NEUME_PORRECTUS: LINE({ 0, 0 }, { 4, 9 }, { 7, 9 }, { 8, -1 }); break;
    case NEUME_CLIMACUS:  LINE({ 0, 10 }, { 3, 0 }); cov_dot(c, x + 6, y + 4); cov_dot(c, x + 9, y + 9); break;
    case NEUME_SCANDICUS: cov_dot(c, x + 1, y + 10); cov_dot(c, x + 4, y + 6); LINE({ 6, 10 }, { 9, -1 }); break;
    default:              LINE({ 0, 8 }, { 1, 6 }, { 2, 8 }, { 3, 6 }, { 4, 8 }, { 7, -1 }); break;
    }
#undef LINE
}

/* Encre noire et rouge des éléments [from, to[ qui touchent la zone `ink`/`red` (même taille et même origine). */
static void draw_page_ink(const Coverage *ink, const Coverage *red, int from, int to)
{
    float x0 = ink->ox - 200.0f, x1 = ink->ox + (float)ink->w + 50.0f;
    float y0 = ink->oy - 120.0f, y1 = ink->oy + (float)ink->h + 120.0f;
    for (int i = from; i < to && i < op_count; i++) {
        const PageOp *op = &ops[i];
        if (op->x < x0 || op->x > x1 || op->y < y0 || op->y > y1) continue;
        const char *s = text_pool + op->text;
        float lx = op->x - ink->ox, ly = op->y - ink->oy;
        switch (op->kind) {
        case OP_SYL:    font_draw(FONT_TEXT, s, lx, ly - 17.0f, ink->buf, ink->w, ink->h); break;
        case OP_NEUME:
            draw_neume(ink, op->neume, op->x, op->y);
            if (hash3(i, 7, (int)(floor_hash & 0xffff)) % 89 == 0) {
                /* Neume raturé par le copiste : un gribouillis par-dessus. */
                float w = NEUME_W[op->neume];
                for (int k = 0; k < 4; k++) {
                    cov_segment(ink, op->x - 1.0f + w * (float)k / 4.0f, op->y + ((k & 1) ? 11.0f : -1.0f),
                                op->x - 1.0f + w * (float)(k + 1) / 4.0f, op->y + ((k & 1) ? -1.0f : 11.0f), 0.7f);
                }
            }
            break;
        case OP_RUBRIC: font_draw(FONT_RUBRIC, s, lx, ly - 11.0f, red->buf, red->w, red->h); break;
        case OP_VERSAL: font_draw(FONT_VERSAL, s, lx, ly - 35.0f, red->buf, red->w, red->h); break;
        default:        font_draw(FONT_CAP, s, lx, ly, red->buf, red->w, red->h); break;
        }
    }
}

/* ------------------------------------------------------------ géométrie des murs */

/*
 * Un pan de mur, sur le côté `side` d'une cellule. Comme dans le prototype, t va de 0
 * (bord extérieur, en haut du mur) à 1 (pied du mur, contre le sol), et le pan couvre
 * [lo(t), hi(t)[ le long du mur, avec lo(t) = lo + klo·(1 - t) :
 *  - coin saillant (coin de la salle) : le pan s'élargit vers l'extérieur, en onglet ;
 *  - mur qui continue dans la cellule voisine : bord droit ;
 *  - coin rentrant (salle en L) : le pan rétrécit vers l'extérieur.
 * vlo/vhi : même chose pour une salle 1x1, qui sert de repère aux portes.
 */
typedef struct WallSeg {
    Dir   side;
    float base, depth;      /* bord du sol ; profondeur du mur */
    float lo, hi, klo, khi;
    float vlo, vhi;
    int   bricks;           /* arcades par cellule */
    int   brick0, row_id;   /* indices dans le monde, pour que les arcades se raccordent d'une cellule à l'autre */
    int   door;             /* porte de ce pan (indice dans room->doors), -1 si aucune */
    float door_half;
} WallSeg;

typedef struct RoomCells {
    int n;
    int cx[ROOM_MAX_CELLS], cy[ROOM_MAX_CELLS];
} RoomCells;

static bool has_cell(const RoomCells *rc, int cx, int cy)
{
    for (int i = 0; i < rc->n; i++) {
        if (rc->cx[i] == cx && rc->cy[i] == cy) return true;
    }
    return false;
}

static const int NX[DIR_COUNT] = { [DIR_LEFT] = -1, [DIR_RIGHT] = 1 };
static const int NY[DIR_COUNT] = { [DIR_UP] = -1, [DIR_DOWN] = 1 };

/* Pente d'un bout de pan : -1 saillant, 0 continu, +1 rentrant (côté lo ; inversé côté hi). */
static int end_type(const RoomCells *rc, int cx, int cy, int ax, int ay, Dir side)
{
    if (!has_cell(rc, cx + ax, cy + ay)) return -1;
    if (!has_cell(rc, cx + ax + NX[side], cy + ay + NY[side])) return 0;
    return 1;
}

static int make_walls(const Room *room, const RoomCells *rc, WallSeg *out)
{
    int n = 0;
    for (int i = 0; i < rc->n; i++) {
        int cx = rc->cx[i], cy = rc->cy[i];
        SDL_FRect c = room->cells[i];
        for (int s = 0; s < DIR_COUNT; s++) {
            Dir side = (Dir)s;
            if (has_cell(rc, cx + NX[s], cy + NY[s])) continue;
            WallSeg *w = &out[n++];
            bool horiz = side == DIR_UP || side == DIR_DOWN;
            w->side = side;
            switch (side) {
            case DIR_UP:   w->base = c.y;       w->depth = WALL_TOP; break;
            case DIR_DOWN: w->base = c.y + c.h; w->depth = WALL_BOTTOM; break;
            case DIR_LEFT: w->base = c.x;       w->depth = WALL_SIDE; break;
            default:       w->base = c.x + c.w; w->depth = WALL_SIDE; break;
            }
            int ax = horiz ? 1 : 0, ay = horiz ? 0 : 1;
            int elo = end_type(rc, cx, cy, -ax, -ay, side);
            int ehi = end_type(rc, cx, cy, ax, ay, side);
            if (horiz) {
                w->lo = c.x;
                w->hi = c.x + c.w;
                w->klo = elo < 0 ? -WALL_SIDE : elo > 0 ? WALL_SIDE : 0.0f;
                w->khi = ehi < 0 ? WALL_SIDE : ehi > 0 ? -WALL_SIDE : 0.0f;
                w->vlo = -WALL_SIDE;
                w->vhi = WALL_SIDE;
                w->bricks = BRICKS_H;
                w->brick0 = cx * BRICKS_H;
                w->row_id = cy * 4 + s;
                w->door_half = DOOR_HALF_H;
            } else {
                /* Le mur qui rejoint un mur de côté est celui du haut ou du bas, selon le coin. */
                w->lo = c.y;
                w->hi = c.y + c.h;
                w->klo = elo < 0 ? -WALL_TOP : elo > 0 ? WALL_BOTTOM : 0.0f;
                w->khi = ehi < 0 ? WALL_BOTTOM : ehi > 0 ? -WALL_TOP : 0.0f;
                w->vlo = -WALL_TOP;
                w->vhi = WALL_BOTTOM;
                w->bricks = BRICKS_V;
                w->brick0 = cy * BRICKS_V;
                w->row_id = cx * 4 + s + 1000;
                w->door_half = DOOR_HALF_V;
            }
            w->door = -1;
            for (int k = 0; k < room->door_count; k++) {
                const RoomDoor *d = &room->doors[k];
                if (d->cell_x == cx && d->cell_y == cy && d->side == side) w->door = k;
            }
        }
    }
    return n;
}

/* t et position le long du mur pour le point (x, y) ; faux s'il n'est pas sur ce pan. */
static bool wall_coords(const WallSeg *w, float x, float y, float *t, float *along)
{
    switch (w->side) {
    case DIR_UP:   *t = (y - (w->base - w->depth)) / w->depth; *along = x; break;
    case DIR_DOWN: *t = ((w->base + w->depth) - y) / w->depth; *along = x; break;
    case DIR_LEFT: *t = (x - (w->base - w->depth)) / w->depth; *along = y; break;
    default:       *t = ((w->base + w->depth) - x) / w->depth; *along = y; break;
    }
    if (*t < 0.0f || *t > 1.0f) return false;
    float s = 1.0f - *t;
    return *along >= w->lo + w->klo * s && *along < w->hi + w->khi * s;
}

/* Coordonnées dans la porte : du de -1 à 1 en travers, dt de 0 (sommet de l'arc) à 1 (sol). */
static void door_coords(const WallSeg *w, float t, float along, float *du, float *dt)
{
    float s = 1.0f - t;
    float lo = w->lo + w->vlo * s, hi = w->hi + w->vhi * s;
    float u = (along - lo) / (hi - lo);
    *du = (u - 0.5f) / w->door_half;
    *dt = (t - DOOR_T0) / (1.0f - DOOR_T0);
}

static bool door_inside(float du, float dt, float g)
{
    if (dt < -g || fabsf(du) > 1.0f + g) return false;
    if (dt >= DOOR_ARCH) return true;
    float ay = (DOOR_ARCH - dt) / (DOOR_ARCH + g);
    return du * du + ay * ay <= (1.0f + g) * (1.0f + g);
}

/* ------------------------------------------------------------ couleurs */
static void put_rgb(Uint8 *o, float r, float g, float b, Uint8 a)
{
    o[0] = (Uint8)SDL_clamp(r, 0.0f, 255.0f);
    o[1] = (Uint8)SDL_clamp(g, 0.0f, 255.0f);
    o[2] = (Uint8)SDL_clamp(b, 0.0f, 255.0f);
    o[3] = a;
}

/* Couleurs de l'enluminure (GDD §21). */
static const float GOLD[3]  = { 206, 162, 74 };
static const float LAPIS[3] = { 44, 70, 138 };
static const float VERMIL[3] = { 166, 50, 38 };
static const float VERT[3]  = { 58, 108, 78 };
static const float PEARL[3] = { 236, 226, 198 };
static const float WALL_INK[3] = { 38, 26, 22 };

static void set3(float *c, const float *v)
{
    c[0] = v[0]; c[1] = v[1]; c[2] = v[2];
}

/*
 * Mur peint façon tables des canons : colonnes marbrées (chapiteau et base dorés) qui portent
 * des arcs ; fond bleu lapis sous les arcs, écoinçons vermillon, frise de perles en haut,
 * bord doré cerné d'encre. t va du bord extérieur (0) au pied du mur (1).
 */
static void wall_color(const WallSeg *w, float x, float y, float t, float along, Uint8 *o)
{
    float s = 1.0f - t;
    float lo = w->lo + w->klo * s, hi = w->hi + w->khi * s, len = hi - lo;
    float u = (along - lo) / len;
    float g = fbm(NZ_GRAIN, x, y);
    float du = 1.0f / len, dt = 1.0f / w->depth;   /* un pixel, en u et en t */
    float c[3];

    float cu = u * (float)w->bricks, ci = floorf(cu), fu = cu - ci;   /* position entre deux colonnes */
    float pu = du * (float)w->bricks;
    float col_d = SDL_min(fu, 1.0f - fu);                             /* distance à l'axe de la colonne */
    const float shaft = 0.075f;
    int column = (int)ci + w->brick0 + (fu > 0.5f ? 1 : 0);

    /* Arc entre deux colonnes : sommet à t = 0.30, retombées sur les chapiteaux à t = 0.44. */
    float a = 2.0f * fu - 1.0f;
    float arch_t = 0.30f + 0.14f * (1.0f - sqrtf(SDL_max(1.0f - a * a, 0.0f)));

    if (t < 0.05f) {
        set3(c, GOLD);                                                /* bord doré */
        if (t < dt * 1.5f || t > 0.05f - dt * 1.2f) set3(c, WALL_INK);
    } else if (t < 0.20f) {
        set3(c, VERMIL);                                              /* frise de perles */
        float pv = fmodf(u * (float)w->bricks * 4.0f, 1.0f), tv = (t - 0.125f) / 0.04f;
        float pr = (pv - 0.5f) * (pv - 0.5f) / 0.06f + tv * tv;
        if (pr < 1.0f) set3(c, PEARL);
        else if (pr < 1.6f) set3(c, WALL_INK);
        if (t > 0.20f - dt * 1.2f) set3(c, WALL_INK);
    } else if (col_d < shaft) {
        /* Colonne : marbre rouge ou vert, chapiteau et base dorés. */
        if (t < 0.27f || (t > 0.84f && t < 0.92f)) {
            set3(c, GOLD);
            if (fabsf(t - 0.27f) < dt || fabsf(t - 0.84f) < dt || fabsf(t - 0.92f) < dt) set3(c, WALL_INK);
        } else {
            const float *m = (column & 1) ? VERMIL : VERT;
            float vein = sinf(along * 0.35f + t * 9.0f + fbm(NZ_WOOD, x, y) * 8.0f);
            float k = 0.85f + 0.25f * vein * vein;
            c[0] = m[0] * k; c[1] = m[1] * k; c[2] = m[2] * k;
        }
        if (col_d > shaft - pu * 1.2f) set3(c, WALL_INK);
    } else if (t < arch_t) {
        set3(c, VERMIL);                                              /* écoinçon, avec un point d'or */
        float ex = (fu - 0.5f) * 12.0f, ey = (t - 0.245f) / dt / 6.0f;
        if (fabsf(fu - 0.5f) > 0.25f && ex * ex * 0.02f + ey * ey < 0.25f) set3(c, GOLD);
    } else if (t < arch_t + 0.035f) {
        set3(c, GOLD);                                                /* l'arc */
        if (t < arch_t + dt * 1.2f || t > arch_t + 0.035f - dt * 1.2f) set3(c, WALL_INK);
    } else if (t < 0.92f) {
        set3(c, LAPIS);                                               /* fond bleu, constellé */
        float k = 0.9f + 0.18f * fbm(NZ_FLOOR, x, y);
        c[0] *= k; c[1] *= k; c[2] *= k;
        float sx = floorf(x / 9.0f), sy = floorf(y / 9.0f);
        if (hash01((int)sx, (int)sy, 77) < 0.035f && fmodf(x, 9.0f) < 1.5f && fmodf(y, 9.0f) < 1.5f) set3(c, PEARL);
    } else {
        set3(c, VERT);                                                /* soubassement */
        if (t < 0.92f + dt * 1.2f || t > 1.0f - dt * 1.5f) set3(c, WALL_INK);
    }
    /* Peint sur le parchemin : un peu de grain, plus sombre vers le haut du mur. */
    float k = (0.9f + 0.18f * g) * (0.62f + 0.38f * t);
    put_rgb(o, c[0] * k, c[1] * k, c[2] * k, 255);
}

/* Encadrement : claveaux peints, vermillon et blanc en alternance, joints à l'encre. */
static void frame_color(const WallSeg *w, float x, float y, float t, float du, float dt, Uint8 *o)
{
    (void)w;
    float theta = atan2f((DOOR_ARCH - dt) / DOOR_ARCH, du);
    float seg = theta / (SDL_PI_F / 9.0f);
    float g = fbm(NZ_GRAIN, x, y);
    int block = dt < DOOR_ARCH ? (int)floorf(seg) : 100 + (int)floorf(dt * 6.0f);
    const float *c = (block & 1) ? VERMIL : PEARL;
    float r = c[0], gr = c[1], b = c[2];
    bool joint_arch = dt < DOOR_ARCH && fabsf(seg - roundf(seg)) < 0.06f;
    bool joint_jamb = dt >= DOOR_ARCH && fabsf(dt * 6.0f - roundf(dt * 6.0f)) < 0.04f;
    if (joint_arch || joint_jamb) { r = WALL_INK[0]; gr = WALL_INK[1]; b = WALL_INK[2]; }
    float k = (0.9f + 0.16f * g) * (0.66f + 0.34f * t);
    put_rgb(o, r * k, gr * k, b * k, 255);
}

/* Porte ouverte : le passage plonge dans le noir. */
static void passage_color(float dt, Uint8 *o)
{
    float k = 0.35f + 0.65f * SDL_clamp(dt, 0.0f, 1.0f);
    put_rgb(o, 26 * k, 20 * k, 19 * k, 255);
}

/* Porte fermée : planches, veinure, ferrures rivetées, heurtoir, ombre sous l'arc. */
static void wood_color(const WallSeg *w, float x, float y, float t, float du, float dt, Uint8 *o)
{
    float plank = (du + 1.0f) * 2.5f, pf = plank - floorf(plank);
    float streak = sinf(du * 55.0f + fbm(NZ_WOOD, x, y) * 9.0f) * 0.5f + 0.5f;
    float k = 0.82f + 0.22f * streak + 0.1f * hash01((int)floorf(plank), w->row_id, 5);
    float r = 118 * k, g = 72 * k, b = 38 * k;
    if (pf < 0.07f || pf > 0.95f) { r = 62; g = 36; b = 20; }
    bool strap = (dt > 0.36f && dt < 0.43f) || (dt > 0.80f && dt < 0.87f);
    if (strap) {
        r = 62; g = 60; b = 68;
        if ((dt > 0.42f && dt < 0.43f) || dt > 0.86f) { r = 40; g = 38; b = 44; }
        float rivet_row = dt > 0.6f ? 0.835f : 0.395f;
        if (fabsf(pf - 0.5f) < 0.1f && fabsf(rivet_row - dt) < 0.014f) { r = 150; g = 146; b = 150; }
    }
    float ex = (du - 0.42f) / 0.16f, ey = (dt - 0.62f) / 0.085f;
    float rr = sqrtf(ex * ex + ey * ey);   /* heurtoir */
    if (rr > 0.72f && rr < 0.82f) { r = 130; g = 126; b = 132; }
    else if (rr > 0.72f && rr < 1.0f) { r = 70; g = 66; b = 72; }
    float shade = (0.55f + 0.45f * SDL_clamp(dt / 0.35f, 0.0f, 1.0f)) * (0.62f + 0.38f * t);
    put_rgb(o, r * shade, g * shade, b * shade, 255);
}


/* ------------------------------------------------------------ textures des salles */
enum { L_VOID, L_FLOOR, L_WALL, L_FRAME, L_WOOD };
#define LABEL_KIND(l) ((l) & 15)
#define LABEL_DOOR(l) ((l) >> 4)

#define MAX_HOLE_TEX (LAYOUT_W * LAYOUT_H)   /* un trou de ver par case au pire */

typedef struct RoomTex {
    Uint64       gen;
    SDL_Texture *tex;
    int          w, h;
    int          door_count;
    SDL_Texture *door_tex[ROOM_MAX_DOORS];
    SDL_FRect    door_rect[ROOM_MAX_DOORS];   /* coordonnées page */
    bool         holes_built;                 /* trous du parchemin, posés par-dessus le sol */
    int          holes_version;               /* RoomLayout.version au moment où ils ont été dessinés */
    int          hole_count;
    SDL_Texture *hole_tex[MAX_HOLE_TEX];
    SDL_FRect    hole_rect[MAX_HOLE_TEX];
    Uint8        worm_drawn[LAYOUT_H][LAYOUT_W];   /* trous de ver déjà dans une texture */
} RoomTex;

static RoomTex       cache[FLOOR_MAX_ROOMS];
static SDL_Renderer *cache_renderer;
static Uint64        generation = 1;

static Uint8 pixels[TEX_MAX_W * TEX_MAX_H * 4];
static Uint8 labels[TEX_MAX_W * TEX_MAX_H];
static Uint8 ink_buf[TEX_MAX_W * TEX_MAX_H];
static Uint8 red_buf[TEX_MAX_W * TEX_MAX_H];
static Uint8 door_px[DOOR_BUF * 4];

static void free_holes(RoomTex *rt)
{
    for (int i = 0; i < rt->hole_count; i++) {
        if (rt->hole_tex[i]) SDL_DestroyTexture(rt->hole_tex[i]);
    }
    rt->hole_count = 0;
    rt->holes_built = false;
    SDL_memset(rt->worm_drawn, 0, sizeof rt->worm_drawn);
}

static void free_tex(RoomTex *rt)
{
    if (rt->tex) SDL_DestroyTexture(rt->tex);
    for (int i = 0; i < rt->door_count; i++) {
        if (rt->door_tex[i]) SDL_DestroyTexture(rt->door_tex[i]);
    }
    free_holes(rt);
    *rt = (RoomTex){ 0 };
}

static SDL_Texture *upload(SDL_Renderer *r, const Uint8 *px, int w, int h, bool blend)
{
    SDL_Texture *t = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
    if (!t) {
        SDL_Log("Texture du décor impossible : %s", SDL_GetError());
        return NULL;
    }
    SDL_UpdateTexture(t, NULL, px, w * 4);
    SDL_SetTextureScaleMode(t, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(t, blend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    return t;
}

/* Parchemin : peau crème au grain fin, follicules, taches, réglure à la pointe sèche
 * et piqûres dans la marge ; ombre des murs, bords de page et creux de la reliure. */
static void floor_color(const RoomCells *rc, const SDL_FRect *fb, float x, float y, int cx, int cy, Uint8 *o)
{
    float g = fbm(NZ_GRAIN, x, y);
    float blot = fbm(NZ_FLOOR, x * 0.25f, y * 0.25f);   /* grandes taches irrégulières */
    float k = (0.95f + 0.08f * g) * (0.93f + 0.1f * blot);
    bool right_page = x >= BINDING_X;
    /* Côté chair (page de gauche) un peu plus blanc, côté poil (droite) un peu plus jaune. */
    float r = (right_page ? 216 : 220) * k, gr = (right_page ? 196 : 202) * k, b = (right_page ? 152 : 164) * k;
    b -= 10.0f * (1.0f - blot);                          /* les taches tirent vers le jaune */
    if (hash01((int)floorf(x / 3.0f), (int)floorf(y / 3.0f), 91) < 0.010f) { r *= 0.86f; gr *= 0.84f; b *= 0.8f; }   /* follicule */

    /* Réglure : sillon fin sous chaque ligne de texte, et deux lignes verticales qui encadrent la colonne. */
    float x0 = right_page ? BINDING_X + MARGIN_IN : MARGIN_OUT, x1 = right_page ? PAGE_W - MARGIN_OUT : BINDING_X - MARGIN_IN;
    float first = LINE0 + PITCH, last = LINE0 + (float)(LINES - 2) * PITCH;
    float ly = fmodf(y - LINE0 + PITCH * 4.0f, PITCH);
    if (x > x0 - 10.0f && x < x1 + 10.0f && y > first - 2.0f && y < last + 2.0f) {
        if (ly >= 3.0f && ly < 4.0f) { r *= 0.93f; gr *= 0.92f; b *= 0.9f; }          /* sillon */
        else if (ly >= 4.0f && ly < 5.0f) { r *= 1.03f; gr *= 1.03f; b *= 1.03f; }   /* son bord éclairé */
    }
    for (int v = 0; v < 2; v++) {
        float vx = v == 0 ? x0 - 8.0f : x1 + 8.0f;
        if (fabsf(x - vx) < 0.5f && y > first - 50.0f && y < last + 12.0f) { r *= 0.92f; gr *= 0.91f; b *= 0.89f; }
    }
    /* Piqûres dans la marge extérieure, une par ligne : elles guidaient la réglure. */
    float prick_x = right_page ? PAGE_W - 46.0f : 46.0f;
    float dy = fmodf(y - LINE0 - 3.5f + PITCH * 4.0f, PITCH);
    if (dy > PITCH * 0.5f) dy -= PITCH;
    float pd = (x - prick_x) * (x - prick_x) + dy * dy;
    if (y > first - 6.0f && y < last + 6.0f) {
        if (pd < 1.2f) { r = 92; gr = 72; b = 54; }
        else if (pd < 3.0f) { r *= 0.9f; gr *= 0.88f; b *= 0.86f; }
    }

    float f = 1.0f;
    if (!has_cell(rc, cx, cy - 1)) f -= SDL_max(26.0f - (y - (float)cy * CELL_H), 0.0f) * 0.010f;
    if (!has_cell(rc, cx - 1, cy)) f -= SDL_max(14.0f - (x - (float)cx * CELL_W), 0.0f) * 0.010f;
    if (!has_cell(rc, cx + 1, cy)) f -= SDL_max(14.0f - ((float)(cx + 1) * CELL_W - x), 0.0f) * 0.010f;
    float nx = (x - (fb->x + fb->w * 0.5f)) / (fb->w * 0.5f + WALL_SIDE);
    float ny = (y - (fb->y + fb->h * 0.5f)) / (fb->h * 0.5f);
    f -= 0.05f * (nx * nx + ny * ny);
    /* Bords de la double page, un peu plus sombres et plus roux (le doigt du lecteur, la poussière). */
    float edge = SDL_min(SDL_min(x, PAGE_W - x), SDL_min(y, PAGE_H - y));
    f -= 0.12f * expf(-edge / 70.0f);
    float d = x - BINDING_X;   /* creux de la reliure */
    if (fabsf(d) < 300.0f) f -= 0.30f * expf(-(d / 16.0f) * (d / 16.0f)) + 0.12f * expf(-(d / 90.0f) * (d / 90.0f));
    put_rgb(o, r * f, gr * f, b * f, 255);
}

/*
 * Construction d'une salle, découpée en tranches (quelques lignes de pixels à la fois)
 * pour pouvoir dessiner les salles voisines à l'avance sans faire sauter d'image.
 */
typedef enum BuildPhase { PH_SHADE, PH_OUTLINE, PH_INK, PH_MARGINALIA, PH_BLEND, PH_DETAILS, PH_FINISH } BuildPhase;
#define INK_CHUNK 512   /* éléments de la page examinés par tranche */

typedef struct BuildJob {
    bool       active;
    Uint64     gen;
    Room       room;
    BuildPhase phase;
    int        cursor;
    int        W, H;
    float      ox, oy;
    RoomCells  rc;
    WallSeg    walls[ROOM_MAX_CELLS * 4];
    int        nw;
    SDL_FRect  fb;          /* rectangle englobant le sol */
    int        door_seg[ROOM_MAX_DOORS];
    int        bx0[ROOM_MAX_DOORS], by0[ROOM_MAX_DOORS], bx1[ROOM_MAX_DOORS], by1[ROOM_MAX_DOORS];
} BuildJob;

static BuildJob job;
static const int DX4[4] = { 1, -1, 0, 0 }, DY4[4] = { 0, 0, 1, -1 };

static void job_start(const Room *room)
{
    if (!noise_ready) noise_build();
    BuildJob *j = &job;
    j->active = true;
    j->gen = generation;
    j->room = *room;
    j->phase = PH_SHADE;
    j->cursor = 0;
    j->W = SDL_min((int)room->bounds.w, TEX_MAX_W);
    j->H = SDL_min((int)room->bounds.h, TEX_MAX_H);
    j->ox = room->bounds.x;
    j->oy = room->bounds.y;
    j->rc.n = room->cell_count;
    for (int i = 0; i < room->cell_count; i++) {
        j->rc.cx[i] = (int)floorf(room->cells[i].x / CELL_W + 0.5f);
        j->rc.cy[i] = (int)floorf(room->cells[i].y / CELL_H + 0.5f);
    }
    j->nw = make_walls(room, &j->rc, j->walls);
    j->fb = (SDL_FRect){ j->ox + WALL_SIDE, j->oy + WALL_TOP, room->bounds.w - 2.0f * WALL_SIDE,
                         room->bounds.h - WALL_TOP - WALL_BOTTOM };
    for (int d = 0; d < ROOM_MAX_DOORS; d++) {
        j->door_seg[d] = -1;
        j->bx0[d] = j->by0[d] = 1 << 20;
        j->bx1[d] = j->by1[d] = -1;
    }
    for (int k = 0; k < j->nw; k++) {
        if (j->walls[k].door >= 0) j->door_seg[j->walls[k].door] = k;
    }
    SDL_memset(ink_buf, 0, (size_t)(j->W * j->H));
    SDL_memset(red_buf, 0, (size_t)(j->W * j->H));
}

/* 1) Sol, murs, encadrements et passages d'une ligne de pixels. */
static void shade_row(BuildJob *j, int py)
{
    float y = j->oy + (float)py + 0.5f;
    int cy = (int)floorf(y / CELL_H);
    for (int px = 0; px < j->W; px++) {
        float x = j->ox + (float)px + 0.5f;
        int i = py * j->W + px;
        Uint8 *o = &pixels[i * 4];
        int cx = (int)floorf(x / CELL_W);
        if (has_cell(&j->rc, cx, cy)) {
            labels[i] = L_FLOOR;
            floor_color(&j->rc, &j->fb, x, y, cx, cy, o);
            continue;
        }
        labels[i] = L_VOID;   /* hors de la salle (creux d'une salle en L) : noir */
        put_rgb(o, 0, 0, 0, 255);
        for (int k = 0; k < j->nw; k++) {
            const WallSeg *w = &j->walls[k];
            float t, along;
            if (!wall_coords(w, x, y, &t, &along)) continue;
            labels[i] = L_WALL;
            if (w->door >= 0) {
                int d = w->door;
                float du, dt;
                door_coords(w, t, along, &du, &dt);
                if (door_inside(du, dt, 0.0f)) {
                    labels[i] = (Uint8)(L_WOOD | (d << 4));
                    passage_color(dt, o);
                    j->bx0[d] = SDL_min(j->bx0[d], px);
                    j->by0[d] = SDL_min(j->by0[d], py);
                    j->bx1[d] = SDL_max(j->bx1[d], px);
                    j->by1[d] = SDL_max(j->by1[d], py);
                    break;
                }
                if (door_inside(du, dt, FRAME_GROW)) {
                    labels[i] = (Uint8)(L_FRAME | (d << 4));
                    frame_color(w, x, y, t, du, dt, o);
                    break;
                }
            }
            wall_color(w, x, y, t, along, o);
            break;
        }
    }
}

/* 2) Contours des portes (encadrement puis bois), comme outline() du prototype. */
static void outline_row(BuildJob *j, int py)
{
    for (int px = 0; px < j->W; px++) {
        int i = py * j->W + px, kind = LABEL_KIND(labels[i]);
        if (kind != L_FRAME && kind != L_WOOD) continue;
        bool door_edge = false, wood_edge = false;
        for (int n = 0; n < 4; n++) {
            int qx = px + DX4[n], qy = py + DY4[n];
            if (qx < 0 || qy < 0 || qx >= j->W || qy >= j->H) continue;
            int qk = LABEL_KIND(labels[qy * j->W + qx]);
            if (qk != L_FRAME && qk != L_WOOD) door_edge = true;
            if (qk != L_WOOD) wood_edge = true;
        }
        if (kind == L_WOOD && wood_edge) put_rgb(&pixels[i * 4], 34, 22, 16, 255);
        else if (door_edge) put_rgb(&pixels[i * 4], 24, 18, 18, 255);
    }
}

/* 4) Encre usée, à moitié transparente, sur le sol seulement (le texte passe sous les murs). */
static void blend_row(BuildJob *j, int py)
{
    float y = j->oy + (float)py + 0.5f;
    for (int px = 0; px < j->W; px++) {
        int i = py * j->W + px;
        if (labels[i] != L_FLOOR || (ink_buf[i] == 0 && red_buf[i] == 0)) continue;
        float x = j->ox + (float)px + 0.5f;
        float fine = hash01((int)floorf(x * 0.5f), (int)floorf(y * 0.5f), 43);
        float wear = SDL_clamp(0.55f + 0.6f * fbm(NZ_WEAR, x, y) - 0.2f * fine, 0.0f, 1.0f);
        Uint8 *o = &pixels[i * 4];
        for (int pass = 0; pass < 2; pass++) {
            const float *col = pass == 0 ? INK : RED;
            float a = (float)(pass == 0 ? ink_buf[i] : red_buf[i]) / 255.0f * INK_OPACITY * wear;
            for (int ch = 0; ch < 3; ch++) o[ch] = (Uint8)((float)o[ch] * (1.0f - a) + col[ch] * a);
        }
    }
}

/* Fils de couture de la reliure, tous les STITCH_STEP px : un point de fil clair, son ombre et ses deux trous. */
static void draw_stitches(BuildJob *j)
{
    int bxp = (int)floorf(BINDING_X - j->ox);
    if (bxp < 2 || bxp + 2 >= j->W) return;
    for (float sy = STITCH_STEP * 0.5f; sy < PAGE_H; sy += STITCH_STEP) {
        int py0 = (int)floorf(sy - j->oy);
        for (int dy = -14; dy <= 14; dy++) {
            int py = py0 + dy;
            if (py < 0 || py >= j->H) continue;
            for (int dx = -2; dx <= 2; dx++) {
                int i = py * j->W + bxp + dx;
                if (labels[i] != L_FLOOR) continue;
                Uint8 *o = &pixels[i * 4];
                bool hole = dy <= -13 || dy >= 13;
                if (hole && dx >= -1 && dx <= 0) put_rgb(o, 40, 30, 26, 255);
                else if (hole) continue;
                else if (dx == -1) put_rgb(o, 224, 208, 170, 255);   /* fil, côté éclairé */
                else if (dx == 0) put_rgb(o, 186, 166, 128, 255);
                else if (dx == 1) put_rgb(o, o[0] * 0.6f, o[1] * 0.6f, o[2] * 0.6f, 255);   /* ombre du fil */
            }
        }
    }
}

/* ------------------------------------------------------------ trous du parchemin (GDD §21) */
#define HOLE_MARGIN  12      /* place autour d'un trou pour son ombre ou sa brûlure, px */
#define HOLE_CHUNK_W 256     /* un grand trou est découpé en morceaux de cette taille */
#define HOLE_CHUNK_H 200
static Uint8 hole_px[HOLE_CHUNK_W * HOLE_CHUNK_H * 4];

/* Distance signée d'un point à un rectangle aux coins arrondis (négative dedans). */
static float sd_round_box(float px, float py, float x0, float y0, float x1, float y1, float r)
{
    float hx = (x1 - x0) * 0.5f - r, hy = (y1 - y0) * 0.5f - r;
    float qx = fabsf(px - (x0 + x1) * 0.5f) - hx, qy = fabsf(py - (y0 + y1) * 0.5f) - hy;
    float ox = SDL_max(qx, 0.0f), oy = SDL_max(qy, 0.0f);
    return sqrtf(ox * ox + oy * oy) + SDL_min(SDL_max(qx, qy), 0.0f) - r;
}

static bool tile_is(const RoomLayout *L, int tx, int ty, int kind)
{
    return tx >= 0 && ty >= 0 && tx < L->w && ty < L->h && L->tiles[ty][tx] == kind;
}

/* Distance signée au trou de genre `kind` près du point (coordonnées page) : union des cases,
 * en retrait sur les bords libres, collées aux cases voisines du même genre. */
static float hole_sd(const RoomLayout *L, int kind, float x, float y, int solo_x, int solo_y)
{
    float ox = (float)L->cx0 * CELL_TILES_X * TILE_W, oy = (float)L->cy0 * CELL_TILES_Y * TILE_H;
    int tx = (int)floorf((x - ox) / TILE_W), ty = (int)floorf((y - oy) / TILE_H);
    float best = 1e9f;
    for (int ny = ty - 1; ny <= ty + 1; ny++) {
        for (int nx = tx - 1; nx <= tx + 1; nx++) {
            if (!tile_is(L, nx, ny, kind) || (solo_x >= 0 && (nx != solo_x || ny != solo_y))) continue;
            SDL_FRect t = layout_tile_rect(L, nx, ny);
            float d;
            if (kind == TILE_WORM) {
                float ex = (x - (t.x + t.w * 0.5f)) / 17.0f, ey = (y - (t.y + t.h * 0.5f)) / 14.0f;
                d = (sqrtf(ex * ex + ey * ey) - 1.0f) * 14.0f;
            } else {
                bool l = tile_is(L, nx - 1, ny, kind), r = tile_is(L, nx + 1, ny, kind);
                bool u = tile_is(L, nx, ny - 1, kind), dn = tile_is(L, nx, ny + 1, kind);
                const float ix = 5.0f, iy = 5.0f, rad = 9.0f;
                d = sd_round_box(x, y, t.x + (l ? -rad - 2.0f : ix), t.y + (u ? -rad - 2.0f : iy),
                                 t.x + t.w - (r ? -rad - 2.0f : ix), t.y + t.h - (dn ? -rad - 2.0f : iy), rad);
            }
            best = SDL_min(best, d);
        }
    }
    return best;
}

static void put_rgba(Uint8 *o, float r, float g, float b, float a)
{
    put_rgb(o, r, g, b, (Uint8)SDL_clamp(a * 255.0f, 0.0f, 255.0f));
}

/* Un pixel d'un trou : fond noir, tranche du parchemin au bord du fond, bord déchiré, ombre ou brûlure. */
static void hole_pixel(const RoomLayout *L, int kind, float x, float y, int solo_x, int solo_y, Uint8 *o)
{
    float amp = kind == TILE_HOLE ? 7.0f : kind == TILE_BURN ? 4.0f : kind == TILE_WORM ? 3.0f : 2.0f;
    float n = (fbm(NZ_WEAR, x * 3.1f, y * 3.1f) - 0.5f) * amp + (hash01((int)x, (int)y, 61) - 0.5f) * amp * 0.35f;
    /* Ondulation large : le papier se déchire en festons irréguliers. */
    float wave = kind == TILE_HOLE ? 12.0f : kind == TILE_BURN ? 8.0f : 0.0f;
    n += (fbm(NZ_WEAR, x * 0.8f + 157.0f, y * 0.8f + 91.0f) - 0.5f) * wave;
    float d = hole_sd(L, kind, x, y, solo_x, solo_y) + n;
    put_rgba(o, 0, 0, 0, 0.0f);
    if (d < 0.0f) {
        /* Dedans : le noir, avec la tranche du parchemin vue en haut (fausse perspective). */
        float up = hole_sd(L, kind, x, y - 4.0f, solo_x, solo_y) + n;
        if (up >= 0.0f) put_rgba(o, 132, 108, 80, 1.0f);
        else put_rgba(o, 22, 16, 15, 1.0f);
        return;
    }
    switch (kind) {
    case TILE_BURN:
        if (d < 2.5f) put_rgba(o, 38, 22, 14, 1.0f);                                   /* bord carbonisé */
        else if (d < 12.0f) {
            float a = 1.0f - (d - 2.5f) / 9.5f;
            put_rgba(o, 112, 64, 30, 0.85f * a * a);                                   /* roussi */
        }
        if (d < 3.0f && hash01((int)x, (int)y, 62) < 0.07f) put_rgba(o, 214, 108, 40, 1.0f);   /* braises */
        break;
    case TILE_WORM:
        if (d < 2.0f) put_rgba(o, 124, 100, 72, 1.0f);                                 /* bord grignoté */
        else if (d < 10.0f && hash01((int)floorf(x / 2.0f), (int)floorf(y / 2.0f), 63) < 0.05f) put_rgba(o, 96, 74, 52, 0.9f);   /* sciure */
        else if (d < 4.0f) put_rgba(o, 40, 26, 20, 0.35f * (1.0f - (d - 2.0f) / 2.0f));
        break;
    default:
        if (d < 2.0f) put_rgba(o, 244, 234, 208, 1.0f);                                /* fibres du bord déchiré */
        else if (d < 6.0f) put_rgba(o, 40, 26, 20, 0.30f * (1.0f - (d - 2.0f) / 4.0f)); /* ombre : le bord se soulève */
        break;
    }
}

/* Trou recousu (GDD §22) : on ne voit plus le trou, seulement la couture. Une fente refermée un peu
 * ondulée, le papier légèrement fripé de part et d'autre, des points de fil en biais et les trous de
 * l'aiguille. La couture suit les cases recousues voisines (horizontale, ou verticale). */
static void sewn_pixel(const RoomLayout *L, float x, float y, Uint8 *o)
{
    float ox = (float)L->cx0 * CELL_TILES_X * TILE_W, oy = (float)L->cy0 * CELL_TILES_Y * TILE_H;
    int tx = (int)floorf((x - ox) / TILE_W), ty = (int)floorf((y - oy) / TILE_H);
    float best = 1e9f, perp = 0.0f, along = 0.0f;
    for (int ny = ty - 1; ny <= ty + 1; ny++) {
        for (int nx = tx - 1; nx <= tx + 1; nx++) {
            if (!tile_is(L, nx, ny, TILE_SEWN)) continue;
            SDL_FRect t = layout_tile_rect(L, nx, ny);
            bool vert = tile_is(L, nx, ny - 1, TILE_SEWN) || tile_is(L, nx, ny + 1, TILE_SEWN);
            float a = vert ? y : x, b = vert ? x : y;   /* le long de la couture, et en travers */
            float a0 = vert ? t.y : t.x, len = vert ? t.h : t.w, mid = vert ? t.x + t.w * 0.5f : t.y + t.h * 0.5f;
            bool before = vert ? tile_is(L, nx, ny - 1, TILE_SEWN) : tile_is(L, nx - 1, ny, TILE_SEWN);
            bool after = vert ? tile_is(L, nx, ny + 1, TILE_SEWN) : tile_is(L, nx + 1, ny, TILE_SEWN);
            float lo = a0 + (before ? 0.0f : 7.0f), hi = a0 + len - (after ? 0.0f : 7.0f);
            float pp = b - mid - 1.3f * sinf(a * 0.21f) - 0.8f * sinf(a * 0.53f + 1.0f);
            float out = a < lo ? lo - a : a > hi ? a - hi : 0.0f;
            float dist = sqrtf(out * out + pp * pp);
            if (dist < best) { best = dist; perp = out > 0.0f ? 99.0f : pp; along = a; }
        }
    }
    put_rgba(o, 0, 0, 0, 0.0f);
    if (best > 8.0f) return;
    float ap = fabsf(perp);
    if (ap < 6.0f) {   /* papier fripé : un côté plus clair, l'autre dans l'ombre */
        float k = 1.0f - ap / 6.0f;
        if (perp < 0.0f) put_rgba(o, 246, 236, 210, 0.22f * k);
        else put_rgba(o, 70, 50, 34, 0.18f * k);
    }
    if (ap < 0.9f) put_rgba(o, 92, 68, 50, 0.85f);                      /* la fente refermée */
    float ph = fmodf(along + perp * 0.6f + 700.0f, 7.0f);
    if (ap < 4.2f && ph < 1.5f) {                                       /* point de fil, en biais */
        bool lit = ph < 0.75f;
        put_rgba(o, lit ? 232 : 182, lit ? 214 : 158, lit ? 168 : 112, 1.0f);
    } else if (ap >= 4.2f && ap < 5.4f && ph < 1.6f) {
        put_rgba(o, 74, 54, 42, 0.9f);                                  /* trou de l'aiguille */
    }
}

static bool point_on_floor(const Room *room, float x, float y)
{
    SDL_FPoint p = { x, y };
    for (int i = 0; i < room->cell_count; i++) {
        if (SDL_PointInRectFloat(&p, &room->cells[i])) return true;
    }
    return false;
}

/* Textures d'un trou dont les cases vont de (x0, y0) à (x1, y1) (découpé en morceaux s'il est grand).
 * solo_x >= 0 : seulement cette case (trou de ver, toujours seul). */
static void build_hole(RoomTex *rt, const Room *room, SDL_Renderer *rend, int kind,
                       int x0, int y0, int x1, int y1, int solo_x, int solo_y)
{
    const RoomLayout *L = room->layout;
    SDL_FRect a = layout_tile_rect(L, x0, y0), b = layout_tile_rect(L, x1, y1);
    int bx0 = (int)a.x - HOLE_MARGIN, by0 = (int)a.y - HOLE_MARGIN;
    int bx1 = (int)(b.x + b.w) + HOLE_MARGIN, by1 = (int)(b.y + b.h) + HOLE_MARGIN;
    for (int cy = by0; cy < by1; cy += HOLE_CHUNK_H) {
        for (int cx = bx0; cx < bx1; cx += HOLE_CHUNK_W) {
            int w = SDL_min(HOLE_CHUNK_W, bx1 - cx), h = SDL_min(HOLE_CHUNK_H, by1 - cy);
            bool any = false;
            for (int py = 0; py < h; py++) {
                for (int px = 0; px < w; px++) {
                    Uint8 *o = &hole_px[(py * w + px) * 4];
                    float x = (float)(cx + px) + 0.5f, y = (float)(cy + py) + 0.5f;
                    put_rgba(o, 0, 0, 0, 0.0f);
                    if (!point_on_floor(room, x, y)) continue;
                    if (kind == TILE_SEWN) sewn_pixel(L, x, y, o);
                    else hole_pixel(L, kind, x, y, solo_x, solo_y, o);
                    any |= o[3] != 0;
                }
            }
            if (!any || rt->hole_count >= MAX_HOLE_TEX) continue;
            rt->hole_tex[rt->hole_count] = upload(rend, hole_px, w, h, true);
            rt->hole_rect[rt->hole_count++] = (SDL_FRect){ (float)cx, (float)cy, (float)w, (float)h };
        }
    }
}

/* Textures des trous de la salle : une par groupe de cases du même genre, une par trou de ver.
 * Déjà construites : on ajoute seulement les nouveaux trous de ver (les autres ne changent pas en jeu). */
static void build_holes(RoomTex *rt, const Room *room, SDL_Renderer *rend)
{
    const RoomLayout *L = room->layout;
    bool full = !rt->holes_built;
    if (full) free_holes(rt);
    rt->holes_built = true;
    if (!L) return;
    rt->holes_version = L->version;
    static Uint8 seen[LAYOUT_H][LAYOUT_W];
    static int qx[LAYOUT_W * LAYOUT_H], qy[LAYOUT_W * LAYOUT_H];
    SDL_memset(seen, 0, sizeof seen);
    for (int sy = 0; sy < L->h; sy++) {
        for (int sx = 0; sx < L->w; sx++) {
            int kind = L->tiles[sy][sx];
            if (kind == TILE_WORM) {
                if (rt->worm_drawn[sy][sx]) continue;
                rt->worm_drawn[sy][sx] = 1;
                build_hole(rt, room, rend, kind, sx, sy, sx, sy, sx, sy);
                continue;
            }
            if (!full || !tile_is_hole(kind) || seen[sy][sx]) continue;
            /* Groupe de cases du même genre (voisines, diagonales comprises). */
            int head = 0, tail = 0, x0 = sx, y0 = sy, x1 = sx, y1 = sy;
            qx[tail] = sx; qy[tail++] = sy; seen[sy][sx] = 1;
            while (head < tail) {
                int cx = qx[head], cy = qy[head++];
                x0 = SDL_min(x0, cx); y0 = SDL_min(y0, cy); x1 = SDL_max(x1, cx); y1 = SDL_max(y1, cy);
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = cx + dx, ny = cy + dy;
                        if (tile_is(L, nx, ny, kind) && !seen[ny][nx]) {
                            seen[ny][nx] = 1;
                            qx[tail] = nx; qy[tail++] = ny;
                        }
                    }
                }
            }
            build_hole(rt, room, rend, kind, x0, y0, x1, y1, -1, -1);
        }
    }
}

/* ------------------------------------------------------------ petites choses sur la page (GDD §21) */

static void blend_px(BuildJob *j, float x, float y, float r, float g, float b, float a)
{
    int px = (int)floorf(x - j->ox), py = (int)floorf(y - j->oy);
    if (px < 0 || py < 0 || px >= j->W || py >= j->H || a <= 0.0f) return;
    int i = py * j->W + px;
    if (labels[i] != L_FLOOR) return;
    Uint8 *o = &pixels[i * 4];
    a = SDL_min(a, 1.0f);
    o[0] = (Uint8)((float)o[0] * (1.0f - a) + r * a);
    o[1] = (Uint8)((float)o[1] * (1.0f - a) + g * a);
    o[2] = (Uint8)((float)o[2] * (1.0f - a) + b * a);
}

static void blend_disc(BuildJob *j, float cx, float cy, float rx, float ry, float r, float g, float b, float a)
{
    for (float y = floorf(cy - ry); y <= cy + ry; y += 1.0f) {
        for (float x = floorf(cx - rx); x <= cx + rx; x += 1.0f) {
            float ex = (x + 0.5f - cx) / rx, ey = (y + 0.5f - cy) / ry;
            if (ex * ex + ey * ey <= 1.0f) blend_px(j, x, y, r, g, b, a);
        }
    }
}

/* Sceau de cire écrasé sur la page, avec ses deux queues de parchemin : une goutte aux bords
 * irréguliers, un bourrelet en relief éclairé d'en haut à gauche, et l'empreinte de la matrice
 * (grènetis et croix pattée) au milieu. Plat : purement décoratif. */
/* Relief du sceau (px, coordonnées relatives au centre) : bourrelet au bord, champ creux,
 * grènetis de 12 perles et croix pattée en relief. Renvoie la hauteur, ou -1 hors de la cire. */
static float seal_height(float px, float py, float R, float ph)
{
    float a = atan2f(py, px);
    float edge = R * (1.0f + 0.07f * sinf(a * 3.0f + ph) + 0.05f * sinf(a * 7.0f + ph * 2.0f));
    float r = sqrtf(px * px + py * py), t = r / edge;
    if (t > 1.0f) return -1.0f;
    if (t > 0.70f) return 3.0f * sinf(SDL_PI_F * (t - 0.70f) / 0.30f);
    float u = px / (edge * 0.70f), v = py / (edge * 0.70f), h = 0.0f;
    /* Perles. */
    float ru = sqrtf(u * u + v * v), au = atan2f(v, u) + 7.0f * SDL_PI_F;
    float k = SDL_fmodf(au, SDL_PI_F / 6.0f) - SDL_PI_F / 12.0f;
    float bx = ru * sinf(k), by = ru * cosf(k) - 0.80f, bd = sqrtf(bx * bx + by * by) / 0.10f;
    if (bd < 1.0f) h = SDL_max(h, 1.6f * (1.0f - bd * bd));
    /* Croix pattée : distance douce à la forme. */
    float ax = fabsf(u), ay = fabsf(v), m = SDL_max(ax, ay), n = SDL_min(ax, ay);
    float arm = 0.12f + 0.20f * m / 0.60f;
    float sd = SDL_max(m - 0.60f, n - arm);
    if (sd < 0.08f) h = SDL_max(h, 2.0f * SDL_clamp((0.08f - sd) / 0.16f, 0.0f, 1.0f));
    return h;
}

static void draw_seal(BuildJob *j, float x, float y, Uint32 seed)
{
    static const float WAX[3][3] = { { 170, 30, 28 }, { 64, 104, 54 }, { 132, 86, 46 } };   /* rouge, vert, cire brune */
    int c = (seed >> 3) % 10 < 6 ? 0 : (seed >> 3) % 10 < 8 ? 1 : 2;
    float R = 15.0f + (float)((seed >> 7) % 3);
    float ph = (float)(seed % 628) / 100.0f;
    /* Queues de parchemin qui sortent dessous, un peu écartées. */
    for (int q = 0; q < 2; q++) {
        float dir = q ? 0.35f : -0.25f, len = R + 12.0f + (float)((seed >> (12 + q)) & 7);
        for (float t = 0.0f; t < len; t += 0.5f) {
            float cx = x + sinf(dir) * t + (q ? 3.0f : -3.0f), cy = y + cosf(dir) * t;
            bool notch = t > len - 4.0f;
            for (float w = -2.5f; w <= 2.5f; w += 1.0f) {
                if (notch && fabsf(w) < (t - (len - 4.0f)) * 0.7f) continue;   /* bout en queue d'aronde */
                float shade = w < -1.5f ? 0.82f : 1.0f;
                blend_px(j, cx + w, cy, 224 * shade, 210 * shade, 172 * shade, 1.0f);
            }
            blend_px(j, cx + 3.5f, cy + 1.0f, 60, 40, 30, 0.25f);
        }
    }
    for (float py = -R - 4.0f; py <= R + 4.0f; py += 1.0f) {
        for (float px = -R - 4.0f; px <= R + 4.0f; px += 1.0f) {
            float cx = px + 0.5f, cy = py + 0.5f;
            float h = seal_height(cx, cy, R, ph);
            if (h < 0.0f) {
                if (seal_height(cx - 1.5f, cy - 2.0f, R, ph) >= 0.0f) blend_px(j, x + px, y + py, 50, 30, 24, 0.35f);   /* ombre portée */
                continue;
            }
            /* Lumière d'en haut à gauche : pente de la hauteur. */
            float hx = SDL_max(seal_height(cx + 0.7f, cy, R, ph), 0.0f) - SDL_max(seal_height(cx - 0.7f, cy, R, ph), 0.0f);
            float hy = SDL_max(seal_height(cx, cy + 0.7f, R, ph), 0.0f) - SDL_max(seal_height(cx, cy - 0.7f, R, ph), 0.0f);
            float lit = SDL_clamp((hx * 0.6f + hy * 0.8f) * 0.32f, -0.55f, 0.55f) - 0.08f + h * 0.07f;
            float k = 1.0f + lit;
            float rr = WAX[c][0] * k, gg = WAX[c][1] * k, bb = WAX[c][2] * k;
            if (lit > 0.38f) { rr += 46; gg += 38; bb += 32; }       /* reflet */
            blend_px(j, x + px, y + py, SDL_min(rr, 255.0f), SDL_min(gg, 255.0f), SDL_min(bb, 255.0f), 1.0f);
        }
    }
}

typedef enum Detail { DET_FLY, DET_FLOWER, DET_HAIR, DET_WINE, DET_FINGER, DET_COUNT } Detail;

static void draw_detail(BuildJob *j, Detail kind, float x, float y, Uint32 seed)
{
    float ph = (float)(seed & 255) / 40.0f;
    switch (kind) {
    case DET_FLY:   /* mouche écrasée entre les pages */
        blend_disc(j, x + 3.0f, y + 1.0f, 5.0f, 1.6f, 92, 34, 28, 0.35f);
        blend_disc(j, x - 3.0f, y - 2.0f, 3.2f, 2.0f, 130, 130, 140, 0.35f);
        blend_disc(j, x + 3.0f, y - 2.0f, 3.2f, 2.0f, 130, 130, 140, 0.35f);
        blend_disc(j, x, y, 2.5f, 2.0f, 30, 24, 20, 0.9f);
        for (int k = 0; k < 6; k++) blend_px(j, x - 3.0f + (float)(k % 3) * 3.0f, y + ((k < 3) ? -3.0f : 3.0f), 30, 24, 20, 0.7f);
        break;
    case DET_FLOWER: {   /* fleur séchée, pressée et fanée */
        for (int k = 0; k < 24; k++) {
            float t = (float)k / 23.0f;
            blend_px(j, x + t * 6.0f + sinf(t * 3.0f + ph) * 2.0f, y + t * 22.0f, 104, 112, 64, 0.7f);
        }
        blend_disc(j, x + 5.0f, y + 12.0f, 4.0f, 1.8f, 96, 116, 62, 0.55f);
        blend_disc(j, x - 1.0f, y + 16.0f, 4.0f, 1.8f, 96, 116, 62, 0.55f);
        for (int k = 0; k < 5; k++) {
            float a = (float)k * 1.2566f + ph;
            blend_disc(j, x + cosf(a) * 4.0f, y + sinf(a) * 3.4f, 3.2f, 2.6f, 148, 104, 150, 0.55f);
        }
        blend_disc(j, x, y, 1.6f, 1.4f, 190, 160, 70, 0.75f);
        break;
    }
    case DET_HAIR:   /* un cheveu */
        for (int k = 0; k < 60; k++) {
            float t = (float)k / 59.0f;
            blend_px(j, x + t * 44.0f, y + sinf(t * 5.0f + ph) * 6.0f + t * 4.0f, 54, 40, 30, 0.5f);
        }
        break;
    case DET_WINE: {   /* tache de vin en anneau */
        float R = 12.0f + (float)(seed % 7);
        for (float yy = -R; yy <= R; yy += 1.0f) {
            for (float xx = -R; xx <= R; xx += 1.0f) {
                float d = sqrtf(xx * xx + (yy / 0.8f) * (yy / 0.8f)) / R;
                if (d >= 1.0f) continue;
                float edge = SDL_clamp((d - 0.82f) / 0.18f, 0.0f, 1.0f);
                blend_px(j, x + xx, y + yy, 126, 44, 56, 0.08f + 0.28f * edge * (1.0f - edge) * 4.0f);
            }
        }
        break;
    }
    default:   /* empreinte de doigt à l'encre */
        for (float yy = -10.0f; yy <= 10.0f; yy += 1.0f) {
            for (float xx = -8.0f; xx <= 8.0f; xx += 1.0f) {
                float e = sqrtf((xx / 7.0f) * (xx / 7.0f) + (yy / 9.0f) * (yy / 9.0f));
                if (e < 1.0f && sinf(e * 19.0f + fbm(NZ_WEAR, x + xx, y + yy) * 3.0f) > 0.45f) {
                    blend_px(j, x + xx, y + yy, 58, 40, 30, 0.3f * (1.0f - e * 0.6f));
                }
            }
        }
        break;
    }
}

/* Une ou deux petites choses par cellule, au hasard (même graine = mêmes choses). */
static void draw_details(BuildJob *j)
{
    for (int i = 0; i < j->rc.n; i++) {
        int cx = j->rc.cx[i], cy = j->rc.cy[i];
        Uint32 h = hash3(cx, cy, (int)(floor_hash & 0xffffff));
        int n = (int)(h % 5) < 2 ? 1 : ((h % 5) == 2 ? 2 : 0);
        for (int k = 0; k < n; k++) {
            Uint32 hk = hash3(cx * 31 + k, cy * 17, (int)(floor_hash >> 4));
            float x = (float)cx * CELL_W + 60.0f + (float)(hk % 640);
            float y = (float)cy * CELL_H + 40.0f + (float)((hk >> 10) % 270);
            draw_detail(j, (Detail)((hk >> 20) % DET_COUNT), x, y, hk);
        }
        /* Un sceau de temps en temps, collé là par un chancelier distrait. */
        Uint32 hs = hash3(cx * 7 + 3, cy * 13 + 1, (int)(floor_hash >> 8));
        if (hs % 100 < 40) {
            float x = (float)cx * CELL_W + 70.0f + (float)(hs % 620);
            float y = (float)cy * CELL_H + 50.0f + (float)((hs >> 12) % 240);
            draw_seal(j, x, y, hs >> 4);
        }
    }
}

/* Marginalia dans les marges extérieures : un lapin armé ou un escargot chevalier, à la plume. */
static void cov_ellipse(const Coverage *c, float cx, float cy, float rx, float ry, int n)
{
    for (int k = 0; k < n; k++) {
        float a0 = (float)k / (float)n * 2.0f * SDL_PI_F, a1 = (float)(k + 1) / (float)n * 2.0f * SDL_PI_F;
        cov_segment(c, cx + cosf(a0) * rx, cy + sinf(a0) * ry, cx + cosf(a1) * rx, cy + sinf(a1) * ry, 0.7f);
    }
}

static void draw_rabbit(const Coverage *ink, const Coverage *red, float x, float y)
{
    cov_ellipse(ink, x, y - 12.0f, 12.0f, 9.0f, 18);                     /* corps */
    cov_ellipse(ink, x + 13.0f, y - 23.0f, 6.0f, 5.0f, 12);              /* tête */
    const float ears[2][4][2] = { { { 10, -27 }, { 8, -42 }, { 11, -44 }, { 13, -28 } },
                                  { { 15, -27 }, { 17, -41 }, { 20, -41 }, { 18, -27 } } };
    for (int e = 0; e < 2; e++) {
        for (int k = 0; k < 3; k++) cov_segment(ink, x + ears[e][k][0], y + ears[e][k][1], x + ears[e][k + 1][0], y + ears[e][k + 1][1], 0.7f);
    }
    cov_ellipse(ink, x - 12.0f, y - 14.0f, 3.0f, 3.0f, 8);              /* queue */
    cov_segment(ink, x - 6.0f, y - 4.0f, x - 9.0f, y, 0.7f);
    cov_segment(ink, x + 6.0f, y - 4.0f, x + 9.0f, y, 0.7f);
    cov_segment(ink, x + 17.0f, y - 17.0f, x + 32.0f, y - 36.0f, 0.8f);  /* épée */
    cov_segment(red, x + 20.0f, y - 25.0f, x + 25.0f, y - 19.0f, 1.0f);  /* garde */
    cov_segment(red, x + 15.0f, y - 24.0f, x + 15.0f, y - 24.0f, 1.2f);  /* œil rouge */
}

static void draw_snail_knight(const Coverage *ink, const Coverage *red, float x, float y)
{
    float py = y - 16.0f;
    for (int k = 1; k < 40; k++) {                                       /* coquille en spirale */
        float a0 = (float)(k - 1) * 0.45f, a1 = (float)k * 0.45f;
        float r0 = 1.0f + (float)(k - 1) * 0.28f, r1 = 1.0f + (float)k * 0.28f;
        cov_segment(ink, x + cosf(a0) * r0, py + sinf(a0) * r0, x + cosf(a1) * r1, py + sinf(a1) * r1, 0.7f);
    }
    cov_segment(ink, x - 16.0f, y, x + 18.0f, y, 0.7f);                  /* pied */
    cov_segment(ink, x + 18.0f, y, x + 22.0f, y - 12.0f, 0.7f);          /* tête */
    cov_segment(ink, x + 22.0f, y - 12.0f, x + 20.0f, y - 20.0f, 0.6f);  /* cornes */
    cov_segment(ink, x + 22.0f, y - 12.0f, x + 26.0f, y - 19.0f, 0.6f);
    cov_segment(ink, x + 22.0f, y - 8.0f, x + 46.0f, y - 22.0f, 0.8f);   /* lance */
    cov_segment(red, x + 38.0f, y - 18.0f, x + 46.0f, y - 25.0f, 1.0f);  /* fanion */
    cov_segment(red, x + 38.0f, y - 18.0f, x + 40.0f, y - 26.0f, 1.0f);
}

static void draw_marginalia(BuildJob *j)
{
    Coverage ink = { ink_buf, j->W, j->H, j->ox, j->oy }, red = { red_buf, j->W, j->H, j->ox, j->oy };
    for (int i = 0; i < j->rc.n; i++) {
        int cx = j->rc.cx[i], cy = j->rc.cy[i];
        if (cx != 0 && cx != FLOOR_COLS - 1) continue;
        Uint32 h = hash3(cx, cy, (int)(floor_hash & 0xffff) + 5);
        if (h % 3 == 0) continue;
        float x = cx == 0 ? 52.0f : PAGE_W - 98.0f;
        float y = (float)cy * CELL_H + 90.0f + (float)((h >> 8) % 200);
        if ((h >> 4) & 1) draw_rabbit(&ink, &red, x, y);
        else draw_snail_knight(&ink, &red, x, y);
    }
}

/* 5) Texture de la salle, et le bois de chaque porte dans une petite texture posée par-dessus. */
static void job_finish(BuildJob *j, SDL_Renderer *rend)
{
    draw_stitches(j);
    RoomTex *rt = &cache[j->room.index];
    free_tex(rt);
    rt->tex = upload(rend, pixels, j->W, j->H, false);
    rt->w = j->W;
    rt->h = j->H;
    rt->gen = j->gen;
    rt->door_count = j->room.door_count;
    for (int d = 0; d < j->room.door_count; d++) {
        int k = j->door_seg[d];
        int bw = j->bx1[d] - j->bx0[d] + 1, bh = j->by1[d] - j->by0[d] + 1;
        if (k < 0 || bw <= 0 || bh <= 0 || bw * bh > DOOR_BUF) continue;
        const WallSeg *w = &j->walls[k];
        for (int dy = 0; dy < bh; dy++) {
            for (int dx = 0; dx < bw; dx++) {
                int px = j->bx0[d] + dx, py = j->by0[d] + dy, li = py * j->W + px;
                Uint8 *o = &door_px[(dy * bw + dx) * 4];
                if (LABEL_KIND(labels[li]) != L_WOOD || LABEL_DOOR(labels[li]) != d) {
                    put_rgb(o, 0, 0, 0, 0);
                    continue;
                }
                bool edge = false;
                for (int n = 0; n < 4; n++) {
                    int qx = px + DX4[n], qy = py + DY4[n];
                    if (qx < 0 || qy < 0 || qx >= j->W || qy >= j->H) continue;
                    if (LABEL_KIND(labels[qy * j->W + qx]) != L_WOOD) edge = true;
                }
                if (edge) {
                    put_rgb(o, 34, 22, 16, 255);
                    continue;
                }
                float x = j->ox + (float)px + 0.5f, y = j->oy + (float)py + 0.5f, t, along, du, dt;
                wall_coords(w, x, y, &t, &along);
                door_coords(w, t, along, &du, &dt);
                wood_color(w, x, y, t, du, dt, o);
            }
        }
        rt->door_tex[d] = upload(rend, door_px, bw, bh, true);
        rt->door_rect[d] = (SDL_FRect){ j->ox + (float)j->bx0[d], j->oy + (float)j->by0[d], (float)bw, (float)bh };
    }
    j->active = false;
}

/* Avance la construction jusqu'à `deadline` (en ns, 0 = jusqu'au bout). Renvoie vrai si elle est finie. */
static bool job_run(SDL_Renderer *rend, Uint64 deadline)
{
    BuildJob *j = &job;
    while (j->active) {
        switch (j->phase) {
        case PH_SHADE:
            shade_row(j, j->cursor++);
            if (j->cursor >= j->H) { j->phase = PH_OUTLINE; j->cursor = 0; }
            break;
        case PH_OUTLINE:
            outline_row(j, j->cursor++);
            if (j->cursor >= j->H) { j->phase = op_count > 0 ? PH_INK : PH_MARGINALIA; j->cursor = 0; }
            break;
        case PH_INK: {
            Coverage ink = { ink_buf, j->W, j->H, j->ox, j->oy }, red = { red_buf, j->W, j->H, j->ox, j->oy };
            draw_page_ink(&ink, &red, j->cursor, j->cursor + INK_CHUNK);
            j->cursor += INK_CHUNK;
            if (j->cursor >= op_count) { j->phase = PH_MARGINALIA; j->cursor = 0; }
            break;
        }
        case PH_MARGINALIA:
            draw_marginalia(j);
            j->phase = PH_BLEND;
            break;
        case PH_DETAILS:
            draw_details(j);
            j->phase = PH_FINISH;
            break;
        case PH_BLEND:
            blend_row(j, j->cursor++);
            if (j->cursor >= j->H) { j->phase = PH_DETAILS; j->cursor = 0; }
            break;
        default:
            job_finish(j, rend);
            return true;
        }
        if (deadline && SDL_GetTicksNS() >= deadline) return !j->active;
    }
    return true;
}

static void build_room(const Room *room, SDL_Renderer *rend)
{
    if (!job.active || job.room.index != room->index || job.gen != generation) job_start(room);
    job_run(rend, 0);
}

/* ------------------------------------------------------------ interface */
#define PREFETCH_BUDGET_NS (3 * SDL_NS_PER_MS)   /* temps donné par image au dessin des salles voisines */

static void free_all(void)
{
    for (int i = 0; i < FLOOR_MAX_ROOMS; i++) free_tex(&cache[i]);
    job.active = false;
}

void decor_init(void)
{
    font_init();
    if (!noise_ready) noise_build();
}

void decor_quit(void)
{
    free_all();
    cache_renderer = NULL;
    font_quit();
}

void decor_set_floor(const Floor *floor)
{
    static bool   has_floor;
    static Uint64 current;
    Uint64 seed = floor->seed;
    if (has_floor && seed == current) return;   /* même graine = même étage */
    has_floor = true;
    current = seed;
    /* Trous du plan, en coordonnées de la double page : le texte les contournera. */
    SDL_memset(page_holes, 0, sizeof page_holes);
    for (int i = 0; i < floor->room_count; i++) {
        const RoomLayout *L = &floor->rooms[i].layout;
        for (int ty = 0; ty < L->h; ty++) {
            for (int tx = 0; tx < L->w; tx++) {
                int px = L->cx0 * CELL_TILES_X + tx, py = L->cy0 * CELL_TILES_Y + ty;
                if (tile_is_pit(L->tiles[ty][tx]) && px < PAGE_TX && py < PAGE_TY) page_holes[py][px] = 1;   /* le texte passe sur les coutures */
            }
        }
    }
    free_all();
    generation++;
    Uint64 s = seed;
    floor_hash = (Uint32)(s ^ (s >> 32));
    for (int i = 0; i < NZ_COUNT; i++) {
        noise_off[i][0] = SDL_rand_r(&s, NOISE_N);
        noise_off[i][1] = SDL_rand_r(&s, NOISE_N);
    }
    layout_double_page(seed);
}

static bool built(int index)
{
    return cache[index].tex && cache[index].gen == generation;
}

static void use_renderer(SDL_Renderer *r)
{
    if (r != cache_renderer) {
        free_all();
        cache_renderer = r;
    }
}

void decor_render_room(const Room *room, SDL_Renderer *r, Camera cam)
{
    if (room->index < 0 || room->index >= FLOOR_MAX_ROOMS) return;
    use_renderer(r);
    if (!built(room->index)) build_room(room, r);
    const RoomTex *rt = &cache[room->index];
    if (!rt->tex) return;
    SDL_FRect dst = { floorf(room->bounds.x - cam.x), floorf(room->bounds.y - cam.y), (float)rt->w, (float)rt->h };
    SDL_RenderTexture(r, rt->tex, NULL, &dst);
    /* Trous du parchemin : redessinés quand un ver en a percé un nouveau. */
    if (!rt->holes_built || (room->layout && rt->holes_version != room->layout->version)) {
        build_holes(&cache[room->index], room, r);
    }
    for (int i = 0; i < rt->hole_count; i++) {
        SDL_FRect hr = rt->hole_rect[i];
        hr.x = floorf(hr.x - cam.x);
        hr.y = floorf(hr.y - cam.y);
        SDL_RenderTexture(r, rt->hole_tex[i], NULL, &hr);
    }
    for (int d = 0; d < rt->door_count && d < room->door_count; d++) {
        if (!rt->door_tex[d] || !room_door_closed(room, d)) continue;
        SDL_FRect dr = rt->door_rect[d];
        dr.x = floorf(dr.x - cam.x);
        dr.y = floorf(dr.y - cam.y);
        SDL_RenderTexture(r, rt->door_tex[d], NULL, &dr);
        if (room->doors[d].locked) {
            /* Porte fermée à clef : la clef qu'il faut, posée sur le bois (GDD §22). */
            SpriteId key = (SpriteId)(SPR_KEY_SOL + room->doors[d].lock_key);
            const SDL_FRect *o = &room->doors[d].opening;
            float kx = floorf(o->x + o->w * 0.5f - cam.x - sprite_width(key) * 0.5f);
            float ky = floorf(o->y + o->h * 0.5f - cam.y - sprite_height(key) * 0.5f);
            sprite_draw(r, key, kx, ky, false, false);
        }
    }
}

void decor_prefetch(const Floor *floor, int room_index, SDL_Renderer *r)
{
    if (!r || room_index < 0 || room_index >= floor->room_count) return;
    use_renderer(r);
    Uint64 deadline = SDL_GetTicksNS() + PREFETCH_BUDGET_NS;
    if (job.active && job.gen == generation && !built(job.room.index)) {
        job_run(r, deadline);
        return;
    }
    const FloorRoom *fr = &floor->rooms[room_index];
    for (int k = 0; k < fr->door_count; k++) {
        int t = fr->doors[k].target;
        if (t < 0 || t >= FLOOR_MAX_ROOMS || built(t)) continue;
        static Room next;
        room_build(&next, floor, t);
        job_start(&next);
        job_run(r, deadline);
        return;
    }
}
