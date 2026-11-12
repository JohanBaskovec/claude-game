#ifndef FUGUE_LAYOUT_H
#define FUGUE_LAYOUT_H

#include <SDL3/SDL.h>
#include <stdbool.h>

/*
 * Disposition d'une salle (GDD §21) : grille de cases de 48 x 40 px (16 x 9 par cellule),
 * avec les trous du parchemin et les obstacles. Tirée au hasard avec la graine de l'étage
 * (floor_generate), puis modifiée en jeu (obstacles cassés, trous du ver de livre) :
 * elle fait partie du plan de l'étage, donc tout reste en place quand on revient.
 *
 * Règle d'accès : toutes les cases où l'on peut marcher sont reliées entre elles
 * (donc chaque porte mène à chaque porte, et aucun ennemi n'est enfermé). Les obstacles
 * comptent comme infranchissables dans cette vérification, même ceux qui se cassent.
 */
#define TILE_W        48.0f
#define TILE_H        40.0f
#define CELL_TILES_X  16
#define CELL_TILES_Y  9
#define LAYOUT_W      (2 * CELL_TILES_X)   /* salle de 2 x 2 cellules au plus */
#define LAYOUT_H      (2 * CELL_TILES_Y)
#define MAX_OBSTACLES 48

typedef enum TileKind {
    TILE_VOID,    /* hors de la salle (creux d'une salle en L) */
    TILE_FLOOR,
    TILE_HOLE,    /* trou déchiré */
    TILE_BURN,    /* brûlure de chandelle */
    TILE_WORM,    /* trou rond laissé par un ver de livre */
    TILE_SEWN,    /* trou recousu : on peut marcher dessus */
} TileKind;

typedef enum ObstacleType {
    OBST_BOOKS,      /* pile de livres : se casse au métronome */
    OBST_INKWELL,    /* encrier : se casse aux tirs, flaque d'encre */
    OBST_CANDLE,     /* chandelle : brûle au contact, un tir l'éteint */
    OBST_NEEDLES,    /* aiguilles du relieur : sortent par intermittence, on marche dessus */
    OBST_RIBBON,     /* signet : muret bas, les tirs passent au-dessus */
    OBST_THREAD,     /* fils de la reliure : idem, dans la colonne de la reliure */
    OBST_TYPE_COUNT
} ObstacleType;

typedef struct Obstacle {
    Uint8 type;      /* ObstacleType */
    Uint8 tx, ty;    /* case */
    Uint8 variant;   /* lettre, forme de la pile, groupe des aiguilles... */
    bool  alive;
    float hp;
} Obstacle;

typedef struct RoomLayout {
    int      cx0, cy0;                    /* cellule en haut à gauche : la case (0, 0) commence à son coin */
    int      w, h;                        /* taille utile de la grille, en cases */
    Uint8    tiles[LAYOUT_H][LAYOUT_W];   /* TileKind */
    Uint8    reserved[LAYOUT_H][LAYOUT_W];/* devant les portes, centre des cellules : toujours libre */
    int      obstacle_count;
    Obstacle obstacles[MAX_OBSTACLES];
    int      worm_holes;                  /* trous laissés par les vers (pas de limite) */
    int      version;                     /* +1 à chaque trou ajouté en jeu (le décor se met à jour) */
    bool     dark;                        /* salle sombre (GDD §21) */
} RoomLayout;

/* Une case sur deux états (cases prises par les vers...). */
typedef Uint8 TileMask[LAYOUT_H][LAYOUT_W];

struct FloorRoom;

/* Tire la disposition d'une salle. Seules les salles normales ont des trous et des obstacles. */
void layout_generate(RoomLayout *L, const struct FloorRoom *fr, Uint64 *rng);

/* Rectangle de la case en coordonnées page. */
SDL_FRect layout_tile_rect(const RoomLayout *L, int tx, int ty);
/* Case qui contient le point (coordonnées page). Faux hors de la grille. */
bool layout_tile_at(const RoomLayout *L, float x, float y, int *tx, int *ty);

/* Vrai pour un trou où l'on ne peut pas marcher (pas le trou recousu). */
bool tile_is_pit(int kind);
/* Vrai pour tout trou dessiné dans le parchemin (y compris le trou recousu). */
bool tile_is_hole(int kind);

/* On peut marcher sur la case (sol ou trou recousu, sans obstacle vivant qui bloque). */
bool layout_walkable(const RoomLayout *L, int tx, int ty);

/* Ce qui bloque la marche, et ce qui arrête les tirs (obstacles hauts). */
bool obstacle_blocks_walk(int type);
bool obstacle_blocks_shots(int type);
/* Boîte de collision d'un obstacle (coordonnées page). */
SDL_FRect obstacle_rect(const RoomLayout *L, const Obstacle *o);
/* Boîte de collision d'un trou : collée aux trous voisins, en retrait ailleurs. */
SDL_FRect pit_rect(const RoomLayout *L, int tx, int ty);

/* Vrai si toutes les cases où l'on peut marcher sont reliées (règle d'accès). */
bool layout_connected(const RoomLayout *L);

/* Vrai si le ver de livre peut percer un trou dans la case : sol libre, pas réservé, sans obstacle,
 * et la règle d'accès tient avec ce trou et ceux qui vont être percés (`pending` : cases où d'autres
 * vers vont sortir, ou NULL). */
bool layout_can_worm_hole(const RoomLayout *L, int tx, int ty, const TileMask *pending);
/* Perce le trou (si layout_can_worm_hole). */
bool layout_add_worm_hole(RoomLayout *L, int tx, int ty, const TileMask *pending);
/* Case où le ver sort, la plus proche du point où il peut percer un nouveau trou (il creuse toujours
 * en sortant) ; s'il n'y en a plus, un trou de ver déjà percé. `pending` (ou NULL) : cases déjà prises
 * par d'autres vers, comptées comme des trous. Faux s'il n'y a aucune case. */
bool layout_worm_spot(const RoomLayout *L, float x, float y, const TileMask *pending, int *tx, int *ty);

int layout_hole_count(const RoomLayout *L);

#endif
