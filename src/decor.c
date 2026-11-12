#include "decor.h"
#include "font.h"

#include <math.h>
#include <string.h>

/*
 * Portage en C de docs/concept/outils_concept/room_base.py (murs, portes, dalles)
 * et page.py (mise en page du manuscrit). Tout est en coordonnées « page » du GDD.
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
#define COURSES     5           /* rangs de briques dans la profondeur du mur */
#define BRICKS_H    20          /* briques par cellule, murs du haut et du bas */
#define BRICKS_V    10          /* briques par cellule, murs de côté */
#define DOOR_T0     0.14f       /* la porte commence un peu sous le chaperon */
#define DOOR_ARCH   0.40f       /* hauteur de l'arc (en fraction de la porte) */
#define DOOR_HALF_H 0.065f      /* demi-largeur de la porte, murs du haut et du bas (≈ 50 px au pied) */
#define DOOR_HALF_V 0.11f       /* idem murs de côté (≈ 40 px au pied) */
#define FRAME_GROW  0.30f       /* épaisseur de l'encadrement de claveaux */
#define SLAB_W      72.0f       /* dalles du sol */
#define SLAB_H      46.0f

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
static Uint32 floor_hash;               /* graine de l'étage, pour les dalles et les briques */

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

static void fit(Layout *L, float w)
{
    if (L->line == L->res_line && L->x < L->res_x1 && L->x + w > L->res_x0) L->x = L->res_x1;
    if (L->x + w > L->x1) newline(L);
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
        case OP_NEUME:  draw_neume(ink, op->neume, op->x, op->y); break;
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
    int   bricks;           /* briques par cellule */
    int   brick0, row_id;   /* indices dans le monde, pour que les briques se raccordent d'une cellule à l'autre */
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

/* Briques en perspective : biseaux clairs et sombres, joints, éclats, chaperon au sommet. */
static void brick_color(const WallSeg *w, float x, float y, float t, float along, Uint8 *o)
{
    float s = 1.0f - t;
    float lo = w->lo + w->klo * s, hi = w->hi + w->khi * s, len = hi - lo;
    float u = (along - lo) / len;
    float g = fbm(NZ_GRAIN, x, y);

    if (t < 0.075f) {   /* chaperon */
        float k = 0.85f + 0.3f * g;
        put_rgb(o, 46 * k, 37 * k, 34 * k, 255);
        return;
    }
    if (t < 0.095f) {   /* arête */
        put_rgb(o, 92, 80, 70, 255);
        return;
    }
    float ct = t * COURSES, ci = floorf(ct), fv = ct - ci;
    float wv = u * (float)w->bricks + 0.5f * (float)((int)ci & 1);
    float bi = floorf(wv), fu = wv - bi;
    float mu = 2.0f * (float)w->bricks / len, mv = 2.0f * COURSES / w->depth;
    int brick = (int)bi + w->brick0, course = (int)ci;
    float bh = hash01(brick, course, w->row_id ^ (int)(floor_hash & 0xffff));
    float r = 112 + bh * 26 - 13, gr = 98 + bh * 22 - 11, b = 86 + bh * 18 - 9;
    float k = 0.86f + 0.28f * g;
    bool mortar = fu < mu || fv < mv;
    if (mortar) {
        float m = 0.9f + 0.2f * g;
        r = 54 * m; gr = 44 * m; b = 39 * m;
    } else {
        if (fv < mv * 2.4f || fu < mu * 1.6f) k *= 1.13f;            /* biseau éclairé */
        else if (fv > 1.0f - mv * 1.6f || fu > 1.0f - mu * 1.2f) k *= 0.8f;   /* biseau dans l'ombre */
        if (hash01(brick, course, w->row_id * 7 + 3) < 0.08f && fbm(NZ_CHIP, x, y) > 0.62f) k *= 0.78f;   /* éclat */
        r *= k; gr *= k; b *= k;
    }
    float depth = 0.5f + 0.5f * t;   /* plus sombre vers le haut du mur */
    put_rgb(o, r * depth, gr * depth, b * depth, 255);
}

/* Encadrement : claveaux rayonnants dans l'arc, blocs droits en dessous. */
static void frame_color(const WallSeg *w, float x, float y, float t, float du, float dt, Uint8 *o)
{
    float theta = atan2f((DOOR_ARCH - dt) / DOOR_ARCH, du);
    float seg = theta / (SDL_PI_F / 9.0f);
    float g = fbm(NZ_GRAIN, x, y);
    float k = (0.85f + 0.3f * g) * (0.92f + 0.14f * hash01((int)floorf(seg), (int)floorf(dt * 6.0f), w->row_id));
    float r = 150 * k, gr = 140 * k, b = 128 * k;
    bool joint_arch = dt < DOOR_ARCH && fabsf(seg - roundf(seg)) < 0.07f;
    bool joint_jamb = dt >= DOOR_ARCH && fabsf(dt * 6.0f - roundf(dt * 6.0f)) < 0.05f;
    if (joint_arch || joint_jamb) { r = 70; gr = 60; b = 54; }
    float shade = 0.62f + 0.38f * t;
    put_rgb(o, r * shade, gr * shade, b * shade, 255);
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

typedef struct RoomTex {
    Uint64       gen;
    SDL_Texture *tex;
    int          w, h;
    int          door_count;
    SDL_Texture *door_tex[ROOM_MAX_DOORS];
    SDL_FRect    door_rect[ROOM_MAX_DOORS];   /* coordonnées page */
} RoomTex;

static RoomTex       cache[FLOOR_MAX_ROOMS];
static SDL_Renderer *cache_renderer;
static Uint64        generation = 1;

static Uint8 pixels[TEX_MAX_W * TEX_MAX_H * 4];
static Uint8 labels[TEX_MAX_W * TEX_MAX_H];
static Uint8 ink_buf[TEX_MAX_W * TEX_MAX_H];
static Uint8 red_buf[TEX_MAX_W * TEX_MAX_H];
static Uint8 door_px[DOOR_BUF * 4];

static void free_tex(RoomTex *rt)
{
    if (rt->tex) SDL_DestroyTexture(rt->tex);
    for (int i = 0; i < rt->door_count; i++) {
        if (rt->door_tex[i]) SDL_DestroyTexture(rt->door_tex[i]);
    }
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

/* Dalles du sol, ombre des murs, vignette, creux de la reliure. */
static void floor_color(const RoomCells *rc, const SDL_FRect *fb, float x, float y, int cx, int cy, Uint8 *o)
{
    float row = floorf(y / SLAB_H);
    float off = ((int)row & 1) ? SLAB_W * 0.5f : 0.0f;
    float col = floorf((x + off) / SLAB_W);
    float sv = hash01((int)col, (int)row, (int)(floor_hash >> 8));
    float g = fbm(NZ_GRAIN, x, y);
    float k = (0.94f + 0.12f * g) * (0.985f + 0.03f * fbm(NZ_FLOOR, x, y));
    float r = (158 + sv * 14 - 7) * k, gr = (145 + sv * 12 - 6) * k, b = (126 + sv * 10 - 5) * k;
    float my = y - row * SLAB_H, mx = (x + off) - col * SLAB_W;
    if (my < 2.0f || mx < 2.0f) { r = 132; gr = 120; b = 105; }    /* joints */
    else if (my < 3.0f) { r *= 1.05f; gr *= 1.05f; b *= 1.05f; }   /* arête éclairée */

    float f = 1.0f;
    if (!has_cell(rc, cx, cy - 1)) f -= SDL_max(26.0f - (y - (float)cy * CELL_H), 0.0f) * 0.012f;
    if (!has_cell(rc, cx - 1, cy)) f -= SDL_max(14.0f - (x - (float)cx * CELL_W), 0.0f) * 0.012f;
    if (!has_cell(rc, cx + 1, cy)) f -= SDL_max(14.0f - ((float)(cx + 1) * CELL_W - x), 0.0f) * 0.012f;
    float nx = (x - (fb->x + fb->w * 0.5f)) / (fb->w * 0.5f + WALL_SIDE);
    float ny = (y - (fb->y + fb->h * 0.5f)) / (fb->h * 0.5f);
    f -= 0.07f * (nx * nx + ny * ny);
    float d = x - BINDING_X;   /* creux de la reliure */
    if (fabsf(d) < 300.0f) f -= 0.34f * expf(-(d / 16.0f) * (d / 16.0f)) + 0.12f * expf(-(d / 90.0f) * (d / 90.0f));
    put_rgb(o, r * f, gr * f, b * f, 255);
}

/*
 * Construction d'une salle, découpée en tranches (quelques lignes de pixels à la fois)
 * pour pouvoir dessiner les salles voisines à l'avance sans faire sauter d'image.
 */
typedef enum BuildPhase { PH_SHADE, PH_OUTLINE, PH_INK, PH_BLEND, PH_FINISH } BuildPhase;
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
            brick_color(w, x, y, t, along, o);
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
            if (j->cursor >= j->H) { j->phase = op_count > 0 ? PH_INK : PH_FINISH; j->cursor = 0; }
            break;
        case PH_INK: {
            Coverage ink = { ink_buf, j->W, j->H, j->ox, j->oy }, red = { red_buf, j->W, j->H, j->ox, j->oy };
            draw_page_ink(&ink, &red, j->cursor, j->cursor + INK_CHUNK);
            j->cursor += INK_CHUNK;
            if (j->cursor >= op_count) { j->phase = PH_BLEND; j->cursor = 0; }
            break;
        }
        case PH_BLEND:
            blend_row(j, j->cursor++);
            if (j->cursor >= j->H) { j->phase = PH_FINISH; j->cursor = 0; }
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

void decor_set_floor(Uint64 seed)
{
    static bool   has_floor;
    static Uint64 current;
    if (has_floor && seed == current) return;   /* même graine = même étage */
    has_floor = true;
    current = seed;
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
    if (room->doors_open) return;
    for (int d = 0; d < rt->door_count; d++) {
        if (!rt->door_tex[d]) continue;
        SDL_FRect dr = rt->door_rect[d];
        dr.x = floorf(dr.x - cam.x);
        dr.y = floorf(dr.y - cam.y);
        SDL_RenderTexture(r, rt->door_tex[d], NULL, &dr);
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
