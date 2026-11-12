#include "floor.h"
#include "config.h"

/* Cellules de chaque forme, relatives à son coin haut-gauche. */
static const int SHAPE_SIZE[SHAPE_COUNT] = { 1, 2, 2, 4, 3, 3, 3, 3 };
static const int SHAPE_CELLS[SHAPE_COUNT][ROOM_MAX_CELLS][2] = {
    [SHAPE_1x1]     = { { 0, 0 } },
    [SHAPE_2x1]     = { { 0, 0 }, { 1, 0 } },
    [SHAPE_1x2]     = { { 0, 0 }, { 0, 1 } },
    [SHAPE_2x2]     = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } },
    [SHAPE_L_NO_TL] = { { 1, 0 }, { 0, 1 }, { 1, 1 } },
    [SHAPE_L_NO_TR] = { { 0, 0 }, { 0, 1 }, { 1, 1 } },
    [SHAPE_L_NO_BL] = { { 0, 0 }, { 1, 0 }, { 1, 1 } },
    [SHAPE_L_NO_BR] = { { 0, 0 }, { 1, 0 }, { 0, 1 } },
};

static const int DIR_DX[DIR_COUNT] = { [DIR_UP] = 0, [DIR_DOWN] = 0, [DIR_LEFT] = -1, [DIR_RIGHT] = 1 };
static const int DIR_DY[DIR_COUNT] = { [DIR_UP] = -1, [DIR_DOWN] = 1, [DIR_LEFT] = 0, [DIR_RIGHT] = 0 };

const char *floor_shape_name(RoomShape shape)
{
    static const char *names[SHAPE_COUNT] = { "1x1", "2x1", "1x2", "2x2", "L", "L", "L", "L" };
    return (shape >= 0 && shape < SHAPE_COUNT) ? names[shape] : "?";
}

int floor_room_at(const Floor *f, int cx, int cy)
{
    if (cx < 0 || cy < 0 || cx >= FLOOR_COLS || cy >= FLOOR_ROWS) {
        return -1;
    }
    return f->grid[cy][cx];
}

bool floor_room_discovered(const Floor *f, int index)
{
    const FloorRoom *r = &f->rooms[index];
    if (r->visited) {
        return true;
    }
    for (int i = 0; i < r->door_count; i++) {
        if (f->rooms[r->doors[i].target].visited) {
            return true;
        }
    }
    return false;
}

static void clear(Floor *f)
{
    f->room_count = 0;
    for (int y = 0; y < FLOOR_ROWS; y++) {
        for (int x = 0; x < FLOOR_COLS; x++) {
            f->grid[y][x] = -1;
        }
    }
}

/*
 * Essaie de poser une salle de forme `shape` dont le coin haut-gauche est (ox, oy).
 * Règle façon TBOI : la nouvelle salle ne touche qu'une seule salle existante,
 * son parent ; on évite ainsi les boucles. parent < 0 : pas de voisin exigé.
 */
static int try_place(Floor *f, RoomShape shape, int ox, int oy, int parent)
{
    if (f->room_count >= FLOOR_MAX_ROOMS) {
        return -1;
    }
    int n = SHAPE_SIZE[shape];
    for (int i = 0; i < n; i++) {
        int cx = ox + SHAPE_CELLS[shape][i][0], cy = oy + SHAPE_CELLS[shape][i][1];
        if (cx < 0 || cy < 0 || cx >= FLOOR_COLS || cy >= FLOOR_ROWS || f->grid[cy][cx] != -1) {
            return -1;
        }
    }
    bool touches_parent = false;
    for (int i = 0; i < n; i++) {
        int cx = ox + SHAPE_CELLS[shape][i][0], cy = oy + SHAPE_CELLS[shape][i][1];
        for (int d = 0; d < DIR_COUNT; d++) {
            int r = floor_room_at(f, cx + DIR_DX[d], cy + DIR_DY[d]);
            if (r < 0) continue;
            if (r != parent) return -1;
            touches_parent = true;
        }
    }
    if (parent >= 0 && !touches_parent) {
        return -1;
    }

    int idx = f->room_count++;
    FloorRoom *room = &f->rooms[idx];
    *room = (FloorRoom){ 0 };
    room->shape = shape;
    room->kind = ROOM_NORMAL;
    room->cell_count = n;
    for (int i = 0; i < TREASURE_OFFERS; i++) room->offers[i] = -1;
    for (int i = 0; i < n; i++) {
        room->cell_x[i] = ox + SHAPE_CELLS[shape][i][0];
        room->cell_y[i] = oy + SHAPE_CELLS[shape][i][1];
        f->grid[room->cell_y[i]][room->cell_x[i]] = idx;
    }
    return idx;
}

/* Pose une salle qui contient la cellule (cx, cy), en essayant les placements dans un ordre aléatoire. */
static int place_covering(Floor *f, Uint64 *rng, RoomShape shape, int cx, int cy, int parent)
{
    int n = SHAPE_SIZE[shape];
    int start = SDL_rand_r(rng, n);
    for (int k = 0; k < n; k++) {
        int i = (start + k) % n;
        int idx = try_place(f, shape, cx - SHAPE_CELLS[shape][i][0], cy - SHAPE_CELLS[shape][i][1], parent);
        if (idx >= 0) return idx;
    }
    return -1;
}

static void compute_doors(Floor *f)
{
    for (int r = 0; r < f->room_count; r++) {
        FloorRoom *room = &f->rooms[r];
        room->door_count = 0;
        for (int i = 0; i < room->cell_count; i++) {
            for (int d = 0; d < DIR_COUNT; d++) {
                int other = floor_room_at(f, room->cell_x[i] + DIR_DX[d], room->cell_y[i] + DIR_DY[d]);
                if (other >= 0 && other != r && room->door_count < ROOM_MAX_DOORS) {
                    room->doors[room->door_count++] = (FloorDoor){ room->cell_x[i], room->cell_y[i], (Dir)d, other };
                }
            }
        }
    }
}

/* Profondeur de chaque salle (parcours en largeur depuis le départ). */
static void compute_depths(Floor *f)
{
    int queue[FLOOR_MAX_ROOMS], head = 0, tail = 0;
    for (int r = 0; r < f->room_count; r++) f->rooms[r].depth = -1;
    f->rooms[f->start].depth = 0;
    queue[tail++] = f->start;
    while (head < tail) {
        int r = queue[head++];
        for (int i = 0; i < f->rooms[r].door_count; i++) {
            int t = f->rooms[r].doors[i].target;
            if (f->rooms[t].depth < 0) {
                f->rooms[t].depth = f->rooms[r].depth + 1;
                queue[tail++] = t;
            }
        }
    }
}

static int distinct_neighbours(const FloorRoom *room)
{
    int seen[ROOM_MAX_DOORS], n = 0;
    for (int i = 0; i < room->door_count; i++) {
        bool dup = false;
        for (int k = 0; k < n; k++) dup |= (seen[k] == room->doors[i].target);
        if (!dup) seen[n++] = room->doors[i].target;
    }
    return n;
}

/* Une tentative de génération. Renvoie false si l'étage ne convient pas. */
static bool attempt(Floor *f, Uint64 *rng)
{
    clear(f);
    f->boss = f->treasure = -1;
    int target = tuning.floor_min_rooms + SDL_rand_r(rng, tuning.floor_max_rooms - tuning.floor_min_rooms + 1);

    f->start = try_place(f, SHAPE_1x1, FLOOR_START_X, FLOOR_START_Y, -1);

    /* Les salles sont ajoutées en fin de liste : la parcourir dans l'ordre revient
     * à un parcours en largeur depuis le départ. Plusieurs passes si besoin. */
    for (int pass = 0; pass < 10 && f->room_count < target; pass++) {
        for (int r = 0; r < f->room_count && f->room_count < target; r++) {
            const FloorRoom *room = &f->rooms[r];
            for (int i = 0; i < room->cell_count && f->room_count < target; i++) {
                int d0 = SDL_rand_r(rng, DIR_COUNT);
                for (int k = 0; k < DIR_COUNT && f->room_count < target; k++) {
                    int d = (d0 + k) % DIR_COUNT;
                    int cx = room->cell_x[i] + DIR_DX[d], cy = room->cell_y[i] + DIR_DY[d];
                    if (cx < 0 || cy < 0 || cx >= FLOOR_COLS || cy >= FLOOR_ROWS || f->grid[cy][cx] != -1) continue;
                    if (SDL_randf_r(rng) < 0.5f) continue;   /* comme TBOI : on abandonne une fois sur deux */

                    int idx = -1;
                    if (SDL_randf_r(rng) < tuning.floor_big_room_chance) {
                        RoomShape big = (RoomShape)(SHAPE_2x1 + SDL_rand_r(rng, SHAPE_COUNT - SHAPE_2x1));
                        idx = place_covering(f, rng, big, cx, cy, r);
                    }
                    if (idx < 0) place_covering(f, rng, SHAPE_1x1, cx, cy, r);
                    room = &f->rooms[r];
                }
            }
        }
    }
    if (f->room_count < tuning.floor_min_rooms) return false;

    int big = 0;
    for (int r = 0; r < f->room_count; r++) big += (f->rooms[r].cell_count > 1);
    if (big < tuning.floor_min_big_rooms) return false;

    compute_doors(f);
    compute_depths(f);

    /* Salle du boss : le cul-de-sac le plus éloigné du départ, pas collé au départ. */
    f->boss = -1;
    for (int r = 0; r < f->room_count; r++) {
        if (r == f->start || distinct_neighbours(&f->rooms[r]) != 1 || f->rooms[r].depth < 2) continue;
        if (f->boss < 0 || f->rooms[r].depth > f->rooms[f->boss].depth) f->boss = r;
    }
    if (f->boss < 0) return false;

    /* Salle du trésor : un autre cul-de-sac, d'une seule cellule, tiré au hasard. */
    int candidates[FLOOR_MAX_ROOMS], nc = 0;
    for (int r = 0; r < f->room_count; r++) {
        if (r == f->start || r == f->boss || f->rooms[r].cell_count != 1) continue;
        if (distinct_neighbours(&f->rooms[r]) == 1) candidates[nc++] = r;
    }
    if (nc == 0) return false;
    f->treasure = candidates[SDL_rand_r(rng, nc)];

    for (int r = 0; r < f->room_count; r++) {
        FloorRoom *room = &f->rooms[r];
        if (r == f->start) {
            room->kind = ROOM_START;
            room->babies = 0;
        } else if (r == f->boss) {
            room->kind = ROOM_BOSS;
            room->babies = tuning.boss_room_babies;
        } else if (r == f->treasure) {
            room->kind = ROOM_TREASURE;
            room->babies = 0;
        } else {
            room->babies = SDL_rand_r(rng, tuning.room_max_babies + 1)
                         + tuning.room_babies_per_extra_cell * (room->cell_count - 1);
        }
    }
    return true;
}

/* Mélange la graine (splitmix64). Sans cela, le générateur de SDL donne des
 * premiers tirages presque identiques pour les petites graines (1, 2, 3...). */
static Uint64 mix_seed(Uint64 x)
{
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

void floor_generate(Floor *f, Uint64 seed)
{
    Uint64 rng = mix_seed(seed);
    f->seed = seed;
    for (int tries = 0; tries < 1000; tries++) {
        if (attempt(f, &rng)) {
            return;
        }
    }
    /* Ne devrait jamais arriver : on garde la dernière tentative, en la rendant jouable. */
    SDL_Log("floor_generate : aucun étage valide pour la graine %llu", (unsigned long long)seed);
    compute_doors(f);
    compute_depths(f);
    if (f->boss < 0) f->boss = f->start;
}
