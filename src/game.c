#include "game.h"
#include "audio.h"
#include "config.h"
#include "decor.h"
#include "describe.h"
#include "draw.h"
#include "minimap.h"
#include "obstacle.h"
#include "sprite.h"
#include "text.h"

#include <math.h>

static bool overlaps(const SDL_FRect *a, const SDL_FRect *b)
{
    return a->x < b->x + b->w && b->x < a->x + a->w &&
           a->y < b->y + b->h && b->y < a->y + a->h;
}

static Dir opposite(Dir d)
{
    switch (d) {
    case DIR_UP:    return DIR_DOWN;
    case DIR_DOWN:  return DIR_UP;
    case DIR_LEFT:  return DIR_RIGHT;
    default:        return DIR_LEFT;
    }
}

static Uint64 random_seed(void)
{
    return ((Uint64)SDL_rand_bits() << 32) | SDL_rand_bits();
}

static float rand_range(float a, float b)
{
    return a + SDL_randf() * (b - a);
}

static void place_player(Game *g, float cx, float cy)
{
    Player *p = &g->player;
    p->box.x = cx - p->box.w * 0.5f;
    p->box.y = cy - p->box.h * 0.5f;
    p->prev_x = p->box.x;
    p->prev_y = p->box.y;
}

void game_spawn_enemy(Game *g, EnemyType type)
{
    float px, py;
    player_center(&g->player, &px, &py);
    if (type == ENEMY_BOSS) {
        room_middle(&g->room, &px, &py);
        enemy_spawn(&g->enemies, ENEMY_BOSS, px, py);
        return;
    }
    enemy_spawn_away_from(&g->enemies, &g->room, type, px, py);
}

/* Un ennemi meurt : il explose en sang (vert pour le ver, encre pour la plume). GDD §18. */
static void enemy_died(Game *g, const Enemy *e)
{
    SplatColor color = e->type == ENEMY_WORM ? SPLAT_GREEN : (e->type == ENEMY_QUILL ? SPLAT_INK : SPLAT_BLOOD);
    bool boss = e->type == ENEMY_BOSS;
    float x = e->box.x + e->box.w * 0.5f, y = e->box.y + e->box.h * 0.5f;
    splatter_burst(&g->splatter, x, y, boss ? 60.0f : 24.0f, boss ? tuning.splat_boss_scale : 1.0f, color);
    audio_play(SOUND_SPLAT, boss ? tuning.splat_boss_pitch : 1.0f);
}

void game_kill_enemies(Game *g)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies.items[i];
        if (e->active && enemy_tangible(e)) enemy_died(g, e);   /* un ver caché ou un boss en l'air disparaît sans bruit */
    }
    enemy_pool_clear(&g->enemies);
}

/* Joueur sur un meuble (téléportation, meubles qui apparaissent) ou trop près pour ne pas
 * prendre son objet sans le vouloir : on le déplace jusqu'à un endroit dégagé, en dessous,
 * ou au-dessus si on bute sur le mur du bas. */
static void free_player_from_props(Game *g)
{
    float cx, cy;
    player_center(&g->player, &cx, &cy);
    for (int i = 0; i < 80; i++) {
        float y = cy + (i < 40 ? 8.0f * (float)i : -8.0f * (float)(i - 40));
        place_player(g, cx, y);
        SDL_FRect margin = { g->player.box.x - 24.0f, g->player.box.y - 24.0f,
                             g->player.box.w + 48.0f, g->player.box.h + 48.0f };
        bool blocked = false;
        for (int k = 0; k < g->room.prop_count; k++) blocked |= overlaps(&margin, &g->room.props[k]);
        if (!blocked && !room_blocks_walk(&g->room, &g->player.box)) return;
    }
    place_player(g, cx, cy);
}

static void collect(Game *g, PickupType type)
{
    PlayerStats *st = &g->player.stats;
    switch (type) {
    case PICKUP_BOMB: st->bombs = SDL_min(st->bombs + 1, 99); break;
    case PICKUP_COIN: st->coins = SDL_min(st->coins + 1, 99); break;
    default: {
        int k = type - PICKUP_KEY_SOL;
        st->keys[k] = SDL_min(st->keys[k] + 1, 99);
        break;
    }
    }
}

static void room_cleared(Game *g);

/* Entre dans une salle : construit sa géométrie, place le joueur, fait apparaître
 * les ennemis à la première visite et ferme les portes s'il y en a. */
static void enter_room(Game *g, int index, float entry_x, float entry_y)
{
    /* Un prix encore en vol est donné au joueur plutôt que perdu. */
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (g->pickups.items[i].active) collect(g, g->pickups.items[i].type);
    }

    FloorRoom *fr = &g->floor.rooms[index];
    room_build(&g->room, &g->floor, index);
    place_player(g, entry_x, entry_y);
    free_player_from_props(g);
    projectile_pool_clear(&g->shots);
    bullet_pool_clear(&g->bullets);
    enemy_pool_clear(&g->enemies);
    pickup_pool_clear(&g->pickups);
    bomb_pool_clear(&g->bombs);
    hazard_pool_clear(&g->hazards);
    splatter_clear(&g->splatter);
    g->trapdoor_armed = false;
    g->socle_armed = false;
    g->shake_timer = 0.0f;

    fr->visited = true;
    if (!fr->cleared) {
        if (fr->kind == ROOM_BOSS) {
            float mx, my;
            room_middle(&g->room, &mx, &my);
            enemy_spawn(&g->enemies, ENEMY_BOSS, mx, my);
        }
        for (int i = 0; i < fr->spawn_count; i++) {
            game_spawn_enemy(g, (EnemyType)fr->spawns[i]);
        }
    }
    bool empty = enemy_count(&g->enemies) == 0;
    room_set_doors_open(&g->room, empty, &g->player.box);
    if (empty && !fr->cleared) {
        fr->cleared = true;
        if (fr->kind == ROOM_NORMAL) room_cleared(g);   /* salle normale sans ennemis : récompense quand même */
    }
}

/* Tire `n` valeurs différentes dans pool[0..*count[ et les retire du tas. -1 quand il est vide. */
static int draw_from(int *pool, int *count, Uint64 *rng)
{
    if (*count == 0) return -1;
    int j = SDL_rand_r(rng, *count);
    int v = pool[j];
    pool[j] = pool[--*count];
    return v;
}

/* Objets et armes du trésor et de la salle du boss : tirés avec la graine de l'étage,
 * parmi les objets pas encore pris et les armes autres que celle qu'on tient.
 * Les deux salles ne proposent jamais la même chose. */
static void fill_offers(Game *g)
{
    Uint64 rng = g->floor.seed * 0x9E3779B97F4A7C15ull + 0x5EED;
    int items[ITEM_COUNT], ni = 0, weapons[WEAPON_COUNT], nw = 0;
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (!g->owned[i]) items[ni++] = i;
    }
    for (int w = 0; w < WEAPON_COUNT; w++) {
        if (w != WEAPON_VOICE && w != g->player.stats.weapon) weapons[nw++] = w;
    }
    int rooms[3] = { g->floor.treasure, g->floor.boss, g->floor.shop };
    for (int k = 0; k < 3; k++) {
        if (rooms[k] < 0) continue;
        FloorRoom *fr = &g->floor.rooms[rooms[k]];
        for (int i = 0; i < TREASURE_OFFERS; i++) fr->offers[i] = draw_from(items, &ni, &rng);
        if (fr->kind != ROOM_SHOP) fr->weapon_offer = draw_from(weapons, &nw, &rng);   /* pas d'arme au magasin */
    }
}

static void start_floor(Game *g, Uint64 seed)
{
    floor_generate(&g->floor, seed);
    fill_offers(g);
    pickup_pool_clear(&g->pickups);
    Room start;
    room_build(&start, &g->floor, g->floor.start);
    float cx, cy;
    room_center(&start, &cx, &cy);
    enter_room(g, g->floor.start, cx, cy);
    g->transition.active = false;
    hud_init(&g->hud, &g->player.stats);
    if (g->floor_number == 1) hud_show_era(&g->hud, text_get(TXT_ERA_1));   /* GDD §23 */
}

void game_new_run(Game *g, Uint64 seed)
{
    player_init(&g->player, 0.0f, 0.0f);
    for (int i = 0; i < ITEM_COUNT; i++) g->owned[i] = false;
    g->floor_number = 1;
    start_floor(g, seed);
}

void game_init(Game *g, Uint64 seed)
{
    *g = (Game){ 0 };
    g->map_hover = -1;
    game_new_run(g, seed);
}

void game_next_floor(Game *g, Uint64 seed)
{
    PlayerStats kept = g->player.stats;
    player_init(&g->player, 0.0f, 0.0f);
    g->player.stats = kept;
    g->floor_number++;
    start_floor(g, seed);
}

Camera game_camera(const Game *g, float alpha)
{
    const Player *p = &g->player;
    float x = p->prev_x + (p->box.x - p->prev_x) * alpha + p->box.w * 0.5f;
    float y = p->prev_y + (p->box.y - p->prev_y) * alpha + p->box.h * 0.5f;
    return room_camera(&g->room, x, y);
}

static void start_transition(Game *g, const RoomDoor *door)
{
    Transition *t = &g->transition;
    t->active = true;
    t->t = 0.0f;
    t->dir = door->side;
    t->from = g->room;
    t->from_cam = game_camera(g, 1.0f);

    /* Cellule voisine, de l'autre côté de la porte, et la porte correspondante en face. */
    float dx, dy;
    dir_vector(door->side, &dx, &dy);
    int ncx = door->cell_x + (int)dx, ncy = door->cell_y + (int)dy;
    Room next;
    room_build(&next, &g->floor, door->target);
    const RoomDoor *entry = room_find_door(&next, ncx, ncy, opposite(door->side));
    float ex, ey;
    if (entry) {
        ex = entry->entry_x;
        ey = entry->entry_y;
    } else {
        room_center(&next, &ex, &ey);   /* ne devrait pas arriver */
    }
    enter_room(g, door->target, ex, ey);
}

/* Salle pas encore visitée, mais reliée à une salle visitée (et pas fermée à clef) : sombre sur la mini-carte. */
static bool reachable_unvisited(const Game *g, int room)
{
    const FloorRoom *fr = &g->floor.rooms[room];
    if (fr->visited || fr->locked) return false;
    for (int k = 0; k < fr->door_count; k++) {
        int t = fr->doors[k].target;
        if (t >= 0 && g->floor.rooms[t].visited) return true;
    }
    return false;
}

bool game_can_teleport(const Game *g, int room)
{
    if (room < 0 || room >= g->floor.room_count || g->transition.active) return false;
    if (g->reveal_map) return true;
    if (enemy_count(&g->enemies) > 0) return false;
    return g->floor.rooms[room].visited || reachable_unvisited(g, room);
}

/* Entrée dans une salle pas encore visitée « comme si on y avait marché par le plus court chemin »
 * (GDD §22) : le chemin part de la salle courante et passe par les salles visitées (l'étage est un
 * arbre, il est unique) ; on arrive par la porte de la dernière salle visitée la plus proche de
 * l'endroit où le chemin entre dans celle-ci. */
static bool walked_entry(const Game *g, int target, float *ex, float *ey)
{
    const Floor *f = &g->floor;
    int parent[FLOOR_MAX_ROOMS], queue[FLOOR_MAX_ROOMS], head = 0, tail = 0;
    for (int i = 0; i < f->room_count; i++) parent[i] = -2;
    parent[g->room.index] = -1;
    queue[tail++] = g->room.index;
    while (head < tail) {
        int r = queue[head++];
        const FloorRoom *fr = &f->rooms[r];
        for (int k = 0; k < fr->door_count; k++) {
            int t = fr->doors[k].target;
            if (t < 0 || parent[t] != -2 || f->rooms[t].locked) continue;
            if (!f->rooms[t].visited && t != target) continue;
            parent[t] = r;
            if (t != target) queue[tail++] = t;
        }
    }
    int from = target >= 0 ? parent[target] : -2;
    if (from < 0) return false;

    /* Point par lequel on arrive dans la salle `from`. */
    static Room near, dest;
    room_build(&near, f, from);
    float ax, ay;
    if (from == g->room.index) {
        player_center(&g->player, &ax, &ay);
    } else {
        room_center(&near, &ax, &ay);
        for (int k = 0; k < near.door_count; k++) {
            if (near.doors[k].target == parent[from]) { ax = near.doors[k].entry_x; ay = near.doors[k].entry_y; break; }
        }
    }
    /* Porte de `from` vers la cible la plus proche, puis la porte d'en face dans la cible. */
    const RoomDoor *best = NULL;
    float best_d = 1e30f;
    for (int k = 0; k < near.door_count; k++) {
        const RoomDoor *d = &near.doors[k];
        if (d->target != target) continue;
        float dd = (d->entry_x - ax) * (d->entry_x - ax) + (d->entry_y - ay) * (d->entry_y - ay);
        if (dd < best_d) { best_d = dd; best = d; }
    }
    if (!best) return false;
    float dx, dy;
    dir_vector(best->side, &dx, &dy);
    room_build(&dest, f, target);
    const RoomDoor *entry = room_find_door(&dest, best->cell_x + (int)dx, best->cell_y + (int)dy, opposite(best->side));
    if (!entry) return false;
    *ex = entry->entry_x;
    *ey = entry->entry_y;
    return true;
}

bool game_teleport(Game *g, int room, int cx, int cy)
{
    if (!game_can_teleport(g, room) || floor_room_at(&g->floor, cx, cy) != room) return false;
    float x = (cx + 0.5f) * CELL_W, y = (cy + 0.5f) * CELL_H;
    if (reachable_unvisited(g, room)) walked_entry(g, room, &x, &y);   /* sinon (debug) : au centre de la cellule */
    g->player.vx = g->player.vy = 0.0f;
    enter_room(g, room, x, y);
    return true;
}

static void fire(Game *g, PlayerFire f)
{
    Player *p = &g->player;
    /* Bâton de Lully : de temps en temps, le coup part dans le pied. */
    if (g->owned[ITEM_LULLY] && tuning.item_lully_foot_chance > 0 && SDL_rand(tuning.item_lully_foot_chance) == 0) {
        player_hurt(p, tuning.item_lully_foot_damage);
        return;
    }

    audio_sing(g->time, weapon_shot_interval(&p->stats));   /* la syllabe suivante, aussi longue que la cadence le permet */
    float dx = f.dx, dy = f.dy, cx, cy;
    player_center(p, &cx, &cy);
    ShotSpec s = weapon_shot(&p->stats, g->owned, f.charge);
    if (tuning.weapons[p->stats.weapon].lock_time > 0.0f) {
        p->lock_timer = SDL_max(p->lock_timer, tuning.weapons[p->stats.weapon].lock_time);
        p->vx = p->vy = 0.0f;   /* cloué net, sans glisser */
    }

    if (s.kind == SHOT_RING) {
        Projectile *pr = projectile_spawn_ring(&g->shots, cx, cy, s.range, s.damage);
        if (pr) pr->stun = s.stun;
        return;
    }
    if (s.kind == SHOT_BEAM) {
        Projectile *pr = projectile_spawn_beam(&g->shots, &g->room, cx, cy, dx, dy, s.damage);
        if (pr) pr->stun = s.stun;
        return;
    }

    /* Notes et ondes : en éventail autour de la direction du tir. */
    float base = atan2f(dy, dx);
    float step = s.count > 1 ? s.spread / (float)(s.count - 1) : 0.0f;
    for (int i = 0; i < s.count; i++) {
        float a = base + ((float)i - (float)(s.count - 1) * 0.5f) * step;
        float ux = cosf(a), uy = sinf(a);
        float vx = ux * s.speed + p->vx * tuning.shot_inherit;
        float vy = uy * s.speed + p->vy * tuning.shot_inherit;
        Projectile *pr = projectile_spawn(&g->shots, cx + ux * 8.0f, cy + uy * 8.0f, vx, vy, s.range, s.damage);
        if (!pr) break;
        pr->kind = s.kind;
        pr->size = s.size;
        pr->note = weapon_note_value(&p->stats);
        pr->stun = s.stun;
        pr->spectral = s.spectral;
        pr->pause = s.pause;
        pr->ink = s.ink;
    }
}

/* Flaque d'encre ramenée sur le sol (une note arrêtée par un mur finit dans le mur). */
static void drop_ink(Game *g, float x, float y)
{
    room_clamp_to_floor(&g->room, tuning.ink_radius + 2.0f, tuning.ink_radius * 0.5f + 2.0f, &x, &y);
    hazard_spawn_ink(&g->hazards, x, y);
}

/* Tirs contre ennemis. Une note disparaît au premier ennemi touché ; les ondes et le rayon
 * traversent et touchent chaque ennemi une seule fois. */
/* --- Obstacles (GDD §21) --------------------------------------------------------- */

static RoomLayout *layout_of(Game *g)
{
    return &g->floor.rooms[g->room.index].layout;
}

/* Un obstacle se casse (ou une chandelle s'éteint) : éclats, flaque, parfois des royalties. */
static void break_obstacle(Game *g, Obstacle *o)
{
    RoomLayout *L = layout_of(g);
    float x, y;
    obstacle_center(L, o, &x, &y);
    o->alive = false;
    switch (o->type) {
    case OBST_BOOKS:    splatter_burst(&g->splatter, x, y, 14.0f, 0.8f, SPLAT_PAPER); break;
    case OBST_CANDLE:   hazard_spawn_wax(&g->hazards, x, y + 2.0f); break;
    case OBST_INKWELL:
        splatter_burst(&g->splatter, x, y, 10.0f, 0.6f, SPLAT_INK);
        hazard_spawn_ink(&g->hazards, x, y + 4.0f);
        if (SDL_randf() < tuning.inkwell_coin_chance) pickup_spawn(&g->pickups, PICKUP_COIN, x, y + 10.0f);
        break;
    default: break;
    }
}

/* Tirs contre obstacles : un encrier perd des points, une chandelle s'éteint. */
static void resolve_shots_vs_obstacles(Game *g)
{
    RoomLayout *L = layout_of(g);
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *pr = &g->shots.items[i];
        bool stopped = pr->stopped;
        pr->stopped = false;
        if ((!pr->active && !stopped) || pr->spectral || pr->kind == SHOT_BEAM) continue;   /* au-dessus des obstacles */
        for (int k = 0; k < L->obstacle_count; k++) {
            Obstacle *o = &L->obstacles[k];
            Uint64 bit = (Uint64)1 << k;
            if (!o->alive || (pr->obstacle_hits & bit)) continue;
            if (o->type != OBST_INKWELL && o->type != OBST_CANDLE) continue;
            SDL_FRect b = obstacle_rect(L, o);
            b.x -= 1.0f;
            b.y -= 1.0f;
            b.w += 2.0f;
            b.h += 2.0f;
            if (!projectile_hits_box(pr, &b)) continue;
            pr->obstacle_hits |= bit;
            o->hp -= o->type == OBST_CANDLE ? o->hp : pr->damage;
            if (o->hp <= 1e-4f) break_obstacle(g, o);
            if (!projectile_pierces(pr)) {
                pr->active = false;
                break;
            }
        }
    }
}

/* Le métronome casse tout ce qui se casse dans son rayon. */
static void explode_obstacles(Game *g, float bx, float by)
{
    RoomLayout *L = layout_of(g);
    for (int k = 0; k < L->obstacle_count; k++) {
        Obstacle *o = &L->obstacles[k];
        if (!o->alive || !obstacle_blocks_shots(o->type)) continue;
        float x, y;
        obstacle_center(L, o, &x, &y);
        float r = tuning.bomb_radius + 18.0f;
        if ((x - bx) * (x - bx) + (y - by) * (y - by) <= r * r) break_obstacle(g, o);
    }
}

/* Chandelles allumées (brûlent au contact) et aiguilles sorties (piquent). */
static void resolve_obstacles_vs_player(Game *g)
{
    const RoomLayout *L = layout_of(g);
    SDL_FRect pb = g->player.box;
    for (int k = 0; k < L->obstacle_count; k++) {
        const Obstacle *o = &L->obstacles[k];
        if (!o->alive) continue;
        SDL_FRect b = obstacle_rect(L, o);
        if (o->type == OBST_CANDLE) {
            SDL_FRect near = { b.x - 3.0f, b.y - 3.0f, b.w + 6.0f, b.h + 6.0f };
            if (overlaps(&near, &pb)) player_hurt(&g->player, tuning.candle_damage);
        } else if (o->type == OBST_NEEDLES && needle_stage(o, g->time) == 2 && overlaps(&b, &pb)) {
            player_hurt(&g->player, tuning.needle_damage);
        }
    }
}

/* Le ver perce la surface : il laisse toujours un trou dans le parchemin (il a choisi une case où la
 * règle d'accès le permet, layout_worm_spot). Ceux qui se tenaient là sont poussés sur le bord. */
static void worm_hole(Game *g, const Enemy *w)
{
    RoomLayout *L = layout_of(g);
    float cx, cy;
    enemy_center(w, &cx, &cy);
    int tx, ty;
    static TileMask pending;   /* les autres vers qui vont sortir : leurs trous comptent déjà */
    enemy_worm_tiles(&g->enemies, L, w, pending);
    if (!layout_tile_at(L, cx, cy, &tx, &ty) || !layout_add_worm_hole(L, tx, ty, (const TileMask *)&pending)) return;
    if (room_blocks_walk(&g->room, &g->player.box)) {
        float px, py;
        player_center(&g->player, &px, &py);
        room_find_free_point(&g->room, g->player.box.w, g->player.box.h, &px, &py);
        place_player(g, px, py);
    }
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies.items[i];
        if (!e->active || e->type == ENEMY_WORM || !room_blocks_walk(&g->room, &e->box)) continue;
        float ex, ey;
        enemy_center(e, &ex, &ey);
        room_find_free_point(&g->room, e->box.w, e->box.h, &ex, &ey);
        e->box.x = e->prev_x = ex - e->box.w * 0.5f;
        e->box.y = e->prev_y = ey - e->box.h * 0.5f;
    }
}

static void resolve_shots_vs_enemies(Game *g)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *pr = &g->shots.items[i];
        if (!pr->active) continue;
        for (int j = 0; j < MAX_ENEMIES; j++) {
            Enemy *e = &g->enemies.items[j];
            Uint32 bit = (Uint32)1 << j;
            if (!enemy_tangible(e) || (pr->hits & bit) || !projectile_hits_box(pr, &e->box)) continue;

            /* Poussée dans le sens du tir (vers l'extérieur pour la cloche). */
            float kx = pr->dir_x, ky = pr->dir_y;
            if (pr->kind == SHOT_RING) {
                float ex = e->box.x + e->box.w * 0.5f - pr->x, ey = e->box.y + e->box.h * 0.5f - pr->y;
                float len = sqrtf(ex * ex + ey * ey);
                kx = len > 0.0f ? ex / len : 0.0f;
                ky = len > 0.0f ? ey / len : 1.0f;
            }
            if (pr->stun > 0.0f) enemy_stun(e, pr->stun);
            if (enemy_hurt(e, pr->damage, kx * tuning.shot_knockback, ky * tuning.shot_knockback)) enemy_died(g, e);
            if (projectile_pierces(pr)) {
                pr->hits |= bit;
                continue;
            }
            pr->active = false;
            if (pr->ink) drop_ink(g, pr->x, pr->y);
            break;
        }
    }
    /* Notes arrêtées par un mur ou au bout de leur portée. */
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *pr = &g->shots.items[i];
        if (pr->ink_drop) {
            drop_ink(g, pr->x, pr->y);
            pr->ink_drop = false;
        }
    }
}

/* Un ennemi tire (GDD §16) : notes du moine, éventail du sonneur, goutte d'encre de la plume. */
static void enemy_fire(Game *g, Enemy *e, float px, float py)
{
    float ex, ey;
    enemy_center(e, &ex, &ey);
    float dx = px - ex, dy = py - ey, len = sqrtf(dx * dx + dy * dy);
    float ux = len > 0.0f ? dx / len : 0.0f, uy = len > 0.0f ? dy / len : 1.0f;
    switch (e->type) {
    case ENEMY_MONK: {
        /* Il chante la syllabe suivante de son chant, et le tir en est le neume (GDD §25). */
        if (e->chant < 0 || e->syllable >= audio_chant_length(e->chant)) {
            int n = audio_chant_count(), c = SDL_rand(n);
            if (n > 1 && c == e->chant) c = (c + 1) % n;   /* fini : un autre chant */
            e->chant = (Sint16)c;
            e->syllable = 0;
        }
        int c = e->chant, sy = e->syllable++;
        const char *prev = sy > 0 ? audio_syllable_notes(c, sy - 1) : "";
        audio_monk_sing(c, sy, audio_syllable_duration(tuning.monk_note_interval));
        Bullet *b = bullet_spawn_note(&g->bullets, ex + ux * 10.0f, ey + uy * 10.0f, ux * tuning.monk_note_speed,
                                      uy * tuning.monk_note_speed, tuning.monk_note_range, 1.0f, tuning.enemy_note_height);
        if (b) bullet_set_syllable(b, audio_syllable_text(c, sy), audio_syllable_notes(c, sy), prev[0] ? prev[SDL_strlen(prev) - 1] : 0);
        break;
    }
    case ENEMY_RINGER: {
        audio_play(SOUND_BELL, 1.0f);
        int n = SDL_max(tuning.ringer_notes, 1);
        float base = atan2f(uy, ux), step = n > 1 ? tuning.ringer_spread / (float)(n - 1) : 0.0f;
        for (int k = 0; k < n; k++) {
            float a = base + ((float)k - (float)(n - 1) * 0.5f) * step;
            bullet_spawn_note(&g->bullets, ex + cosf(a) * 12.0f, ey + sinf(a) * 12.0f,
                              cosf(a) * tuning.ringer_note_speed, sinf(a) * tuning.ringer_note_speed,
                              tuning.ringer_note_range, tuning.ringer_note_size, tuning.enemy_note_height + 24.0f);
        }
        break;
    }
    case ENEMY_QUILL: {
        audio_play(SOUND_INK_SPIT, 1.0f);
        float tx = px, ty = py;
        room_clamp_to_floor(&g->room, tuning.toxic_radius, tuning.toxic_radius * 0.5f, &tx, &ty);
        bullet_spawn_ink(&g->bullets, ex, ey, tx, ty, tuning.ink_blob_flight);
        break;
    }
    default:
        break;
    }
}

/* Tirs ennemis : avancent, blessent le joueur au contact ; une goutte d'encre laisse une flaque toxique. */
static void update_bullets(Game *g, float dt)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &g->bullets.items[i];
        if (bullet_update(b, &g->room, dt)) hazard_spawn_toxic(&g->hazards, b->to_x, b->to_y);
        if (bullet_hits_box(b, &g->player.box)) {
            player_hurt(&g->player, tuning.enemy_shot_damage);
            b->active = false;
        }
    }
    /* Encre toxique sous les pieds du joueur. */
    float fx = g->player.box.x + g->player.box.w * 0.5f, fy = g->player.box.y + g->player.box.h * 0.5f;
    if (hazard_in(&g->hazards, HAZARD_TOXIC, fx, fy)) player_hurt(&g->player, tuning.toxic_damage);
}

/* Ennemis contre joueur : dégâts au contact, sauf pendant l'invincibilité. */
static void resolve_enemies_vs_player(Game *g)
{
    for (int j = 0; j < MAX_ENEMIES; j++) {
        Enemy *e = &g->enemies.items[j];
        if (enemy_tangible(e) && overlaps(&e->box, &g->player.box)) {
            if (player_hurt(&g->player, enemy_contact_damage(e))) {
                return;   /* un seul coup par pas ; l'invincibilité protège ensuite */
            }
        }
    }
}

static void shake(Game *g, float duration)
{
    g->shake_timer = SDL_max(g->shake_timer, duration);
}

/* Le Gros Bébé retombe : l'écran tremble et des briques tombent du plafond. */
static void boss_landed(Game *g)
{
    shake(g, tuning.shake_time);
    for (int i = 0; i < tuning.boss_bricks; i++) {
        float x, y;
        room_random_floor_point(&g->room, 30.0f, &x, &y);
        hazard_spawn_brick(&g->hazards, x, y, rand_range(tuning.brick_fall_min, tuning.brick_fall_max));
    }
}

/* Explosion d'un métronome : dégâts et poussée aux ennemis, dégâts au joueur s'il est dedans. */
static void explode(Game *g, const Bomb *b)
{
    audio_play(SOUND_EXPLOSION, 1.0f);
    explode_obstacles(g, b->x, b->y);
    float R = tuning.bomb_radius;
    for (int j = 0; j < MAX_ENEMIES; j++) {
        Enemy *e = &g->enemies.items[j];
        if (!enemy_tangible(e)) continue;
        float dx = e->box.x + e->box.w * 0.5f - b->x, dy = e->box.y + e->box.h * 0.5f - b->y;
        float d = sqrtf(dx * dx + dy * dy);
        float reach = R + SDL_min(e->box.w, e->box.h) * 0.5f;
        if (d > reach) continue;
        float kx = d > 0.0f ? dx / d : 0.0f, ky = d > 0.0f ? dy / d : 1.0f;
        if (enemy_hurt(e, tuning.bomb_damage, kx * tuning.bomb_knockback, ky * tuning.bomb_knockback)) enemy_died(g, e);
    }
    float px, py;
    player_center(&g->player, &px, &py);
    float pd = sqrtf((px - b->x) * (px - b->x) + (py - b->y) * (py - b->y));
    if (pd <= R + SDL_min(g->player.box.w, g->player.box.h) * 0.5f) {
        player_hurt(&g->player, tuning.bomb_player_damage);
    }
    shake(g, tuning.shake_time * 0.6f);
}

static void take_item(Game *g, ItemId id)
{
    g->owned[id] = true;
    item_apply(id, &g->player.stats);
    hud_show_banner(&g->hud, item_name(id), item_desc(id));
}

static void take_weapon(Game *g, FloorRoom *fr)
{
    Player *p = &g->player;
    int old = p->stats.weapon;
    p->stats.weapon = fr->weapon_offer;
    fr->weapon_offer = old;            /* l'ancienne arme reste sur le socle */
    p->charge = 0.0f;
    p->charging = false;
    p->fire_cooldown = 0.0f;
    hud_show_banner(&g->hud, weapon_name(p->stats.weapon), weapon_desc(p->stats.weapon));
}

/* Lutrins : en toucher un prend son objet, les autres disparaissent.
 * Socle d'arme : le toucher échange les armes ; il faut s'en éloigner avant de recommencer. */
static void resolve_offers(Game *g)
{
    FloorRoom *fr = &g->floor.rooms[g->room.index];
    bool on_socle = false;
    for (int i = 0; i < g->room.prop_count; i++) {
        SDL_FRect reach = g->room.props[i];
        reach.x -= 6.0f;
        reach.y -= 6.0f;
        reach.w += 12.0f;
        reach.h += 12.0f;
        if (!overlaps(&reach, &g->player.box)) continue;

        if (g->room.prop_kind[i] == PROP_WEAPON) {
            on_socle = true;
            if (g->socle_armed && fr->weapon_offer >= 0) {
                take_weapon(g, fr);
                g->socle_armed = false;
            }
        } else if (g->room.prop_kind[i] == PROP_HEAL) {
            /* Magasin : le soin, s'il reste à vendre, si on peut payer et si la vie n'est pas pleine. */
            PlayerStats *st = &g->player.stats;
            if (!fr->heal_sold && st->coins >= tuning.shop_price && st->hp < st->max_hp) {
                st->coins -= tuning.shop_price;
                st->hp = SDL_min(st->hp + tuning.shop_heal, st->max_hp);
                fr->heal_sold = true;
                SDL_snprintf(g->heal_text, sizeof g->heal_text, text_get(TXT_SHOP_HEAL_DESC), tuning.shop_heal);
                hud_show_banner(&g->hud, text_get(TXT_SHOP_HEAL_NAME), g->heal_text);
                audio_play(SOUND_PICKUP_COIN, 0.8f);
            }
        } else if (i < TREASURE_OFFERS && fr->offers[i] >= 0) {
            if (fr->kind == ROOM_SHOP) {
                /* Magasin : on paie, l'objet acheté disparaît, les autres restent. */
                if (g->player.stats.coins < tuning.shop_price) continue;
                g->player.stats.coins -= tuning.shop_price;
                take_item(g, (ItemId)fr->offers[i]);
                fr->offers[i] = ITEM_NONE;
                audio_play(SOUND_PICKUP_COIN, 0.8f);
                continue;
            }
            take_item(g, (ItemId)fr->offers[i]);
            for (int k = 0; k < TREASURE_OFFERS; k++) fr->offers[k] = ITEM_NONE;
        }
    }
    if (!on_socle) g->socle_armed = true;
}

/* ------------------------------------------------------------ boîtes à musique (GDD §22) */

static SDL_FRect box_rect(const MusicBox *b)
{
    return (SDL_FRect){ b->x - 18.0f, b->y - 14.0f, 36.0f, 16.0f };
}

static void add_box(Game *g, float x, float y, bool golden)
{
    FloorRoom *fr = &g->floor.rooms[g->room.index];
    if (fr->box_count >= ROOM_MAX_BOXES) return;
    fr->boxes[fr->box_count++] = (MusicBox){ x, y, golden, false, -1, false };
    g->box_cooldown = 0.5f;
}

void game_spawn_box(Game *g)
{
    float px, py;
    player_center(&g->player, &px, &py);
    float x = px + 90.0f, y = py;
    room_find_free_point(&g->room, 40.0f, 20.0f, &x, &y);
    add_box(g, x, y, SDL_rand(2) == 0);
}

static int weighted_pick(const int *weight, int n)
{
    int total = 0;
    for (int i = 0; i < n; i++) total += SDL_max(weight[i], 0);
    if (total <= 0) return 0;
    int r = SDL_rand(total);
    for (int i = 0; i < n; i++) {
        if (r < SDL_max(weight[i], 0)) return i;
        r -= SDL_max(weight[i], 0);
    }
    return 0;
}

/* Objet rare d'une boîte dorée : pas encore pris, et proposé nulle part ailleurs sur l'étage. */
static int pick_box_item(const Game *g)
{
    int pool[ITEM_COUNT], n = 0;
    for (int i = 0; i < ITEM_COUNT; i++) {
        bool taken = g->owned[i];
        for (int r = 0; r < g->floor.room_count && !taken; r++) {
            const FloorRoom *fr = &g->floor.rooms[r];
            for (int k = 0; k < TREASURE_OFFERS; k++) taken |= fr->offers[k] == i;
            for (int k = 0; k < fr->box_count; k++) taken |= fr->boxes[k].item == i;
        }
        if (!taken) pool[n++] = i;
    }
    return n > 0 ? pool[SDL_rand(n)] : -1;
}

/* La boîte s'ouvre : mélodie, et son contenu se répand autour. */
static void open_box(Game *g, MusicBox *b)
{
    b->open = true;
    audio_play(b->golden ? SOUND_BOX_GOLD : SOUND_BOX_GRAY, 1.0f);
    if (b->golden && SDL_randf() < tuning.box_gold_item_chance) {
        b->item = pick_box_item(g);
        if (b->item >= 0) return;
    }
    int lo = b->golden ? tuning.box_gold_min : tuning.box_gray_min;
    int hi = b->golden ? tuning.box_gold_max : tuning.box_gray_max;
    int n = lo + SDL_rand(SDL_max(hi - lo + 1, 1));
    for (int i = 0; i < n; i++) {
        int c = weighted_pick(tuning.box_content_weight, 3);
        PickupType type = c == 0 ? PICKUP_COIN : c == 1 ? PICKUP_BOMB : (PickupType)(PICKUP_KEY_SOL + SDL_rand(KEY_COUNT));
        /* direction au hasard, retirée tant que le point d'arrivée (vitesse / frottement de pickup.c) n'est pas du sol libre */
        float vx = 0.0f, vy = 0.0f;
        for (int tries = 0; tries < 12; tries++) {
            float a = SDL_randf() * 2.0f * SDL_PI_F, sp = tuning.box_spill_speed * (0.6f + 0.4f * SDL_randf());
            float tx = cosf(a) * sp, ty = sinf(a) * sp * 0.7f;
            SDL_FRect land = { b->x + tx / tuning.box_spill_friction - 8.0f, b->y + 4.0f + ty / tuning.box_spill_friction - 8.0f, 16.0f, 16.0f };
            if (!room_blocks_walk(&g->room, &land)) { vx = tx; vy = ty; break; }
        }
        pickup_spill(&g->pickups, type, b->x, b->y + 4.0f, vx, vy, tuning.box_spill_delay + 0.05f * (float)i);
    }
}

/* Boîtes touchées : la grise s'ouvre, la dorée avec une clef de fa ; l'objet d'une boîte ouverte se prend. */
static void resolve_boxes(Game *g)
{
    if (g->box_cooldown > 0.0f) return;
    FloorRoom *fr = &g->floor.rooms[g->room.index];
    for (int i = 0; i < fr->box_count; i++) {
        MusicBox *b = &fr->boxes[i];
        SDL_FRect r = box_rect(b);
        if (!overlaps(&r, &g->player.box)) { b->touched = false; continue; }
        if (b->open) {
            /* l'objet se prend en revenant sur la boîte, pas dans l'élan qui l'a ouverte */
            if (b->item >= 0 && !b->touched) {
                take_item(g, (ItemId)b->item);
                b->item = -1;
            }
            b->touched = true;
            continue;
        }
        b->touched = true;
        if (b->golden) {
            if (g->player.stats.keys[KEY_FA] <= 0) continue;
            g->player.stats.keys[KEY_FA]--;
            audio_play(SOUND_UNLOCK, 1.0f);
        }
        open_box(g, b);
    }
}

/* Portes fermées à clef : les toucher avec la bonne clef les ouvre pour de bon. */
static void resolve_locked_doors(Game *g)
{
    FloorRoom *fr = &g->floor.rooms[g->room.index];
    for (int i = 0; i < g->room.door_count; i++) {
        const RoomDoor *d = &g->room.doors[i];
        if (!d->locked) continue;
        SDL_FRect reach = { d->opening.x - 6.0f, d->opening.y - 6.0f, d->opening.w + 12.0f, d->opening.h + 12.0f };
        if (!overlaps(&reach, &g->player.box) || g->player.stats.keys[d->lock_key] <= 0) continue;
        g->player.stats.keys[d->lock_key]--;
        FloorRoom *locked = fr->locked ? fr : &g->floor.rooms[d->target];
        locked->locked = false;
        room_refresh_locks(&g->room, &g->floor);
        audio_play(SOUND_UNLOCK, 1.0f);
        return;
    }
}

void game_give_next_item(Game *g)
{
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (!g->owned[i]) {
            take_item(g, (ItemId)i);
            return;
        }
    }
}

/* Salle vidée (GDD §22) : les obstacles qu'on casse en tirant se cassent tout seuls, toujours une
 * récompense au centre, et la trappe si c'était le boss. */
static void room_cleared(Game *g)
{
    FloorRoom *fr = &g->floor.rooms[g->room.index];
    RoomLayout *L = layout_of(g);
    for (int i = 0; i < L->obstacle_count; i++) {
        Obstacle *o = &L->obstacles[i];
        if (o->alive && (o->type == OBST_INKWELL || o->type == OBST_CANDLE)) break_obstacle(g, o);
    }
    float x, y;
    room_middle(&g->room, &x, &y);
    if (fr->kind == ROOM_BOSS) {
        room_open_trapdoor(&g->room);
        room_place_offers(&g->room, true);
        free_player_from_props(g);
        g->trapdoor_armed = false;
        g->socle_armed = false;
        x -= 170.0f;   /* à côté de la trappe et des lutrins */
        y += 20.0f;
    }
    RewardKind k = (RewardKind)weighted_pick(tuning.reward_weight, REWARD_COUNT);
    switch (k) {
    case REWARD_COIN:     pickup_spawn(&g->pickups, PICKUP_COIN, x, y); break;
    case REWARD_BOMB:     pickup_spawn(&g->pickups, PICKUP_BOMB, x, y); break;
    case REWARD_KEY_SOL:  pickup_spawn(&g->pickups, PICKUP_KEY_SOL, x, y); break;
    case REWARD_KEY_FA:   pickup_spawn(&g->pickups, PICKUP_KEY_FA, x, y); break;
    case REWARD_KEY_UT:   pickup_spawn(&g->pickups, PICKUP_KEY_UT, x, y); break;
    case REWARD_BOX_GRAY: add_box(g, x, y, false); break;
    default:              add_box(g, x, y, true); break;
    }
}

/* Direction du tir (GDD §17) : la flèche enfoncée, sinon le clic gauche vers le curseur ;
 * (0, 0) si on ne tire pas. On vise depuis la hauteur de vol des notes, pour qu'elles passent sous le curseur. */
static void aim_direction(const Game *g, const Input *in, float *ax, float *ay)
{
    *ax = *ay = 0.0f;
    if (in->shoot != DIR_NONE) {
        dir_vector(in->shoot, ax, ay);
        return;
    }
    if (!in->aim_mouse) return;
    Camera cam = game_camera(g, 1.0f);
    float px, py;
    player_center(&g->player, &px, &py);
    float dx = in->mouse_x + cam.x - px, dy = in->mouse_y + cam.y - (py - tuning.shot_height);
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) {
        dir_vector(g->player.look, ax, ay);   /* curseur sur la bouche : droit devant */
        return;
    }
    *ax = dx / len;
    *ay = dy / len;
}

static void step_inner(Game *g, const Input *in, float dt)
{
    g->time += dt;
    if (g->shake_timer > 0.0f) g->shake_timer -= dt;
    if (g->cry_cooldown > 0.0f) g->cry_cooldown -= dt;
    if (g->box_cooldown > 0.0f) g->box_cooldown -= dt;

    /* Pendant le glissement entre deux salles, le jeu est en pause. */
    if (g->transition.active) {
        float dur = tuning.room_transition_time;
        g->transition.t += (dur > 0.0f) ? dt / dur : 1.0f;
        if (g->transition.t >= 1.0f) g->transition.active = false;
        hud_update(&g->hud, &g->player.stats, dt);
        return;
    }

    float aim_x, aim_y;
    aim_direction(g, in, &aim_x, &aim_y);
    g->player.slipping = hazard_in(&g->hazards, HAZARD_WAX, g->player.box.x + g->player.box.w * 0.5f,
                                   g->player.box.y + g->player.box.h * 0.5f);
    PlayerFire shot = player_update(&g->player, in, aim_x, aim_y, &g->room, weapon_fire_rate(&g->player.stats),
                                    weapon_charge_time(&g->player.stats), dt);
    float px, py;
    player_center(&g->player, &px, &py);

    /* Poser un métronome à ses pieds. */
    if (in->bomb && g->player.stats.bombs > 0 && bomb_spawn(&g->bombs, px, py)) {
        g->player.stats.bombs--;
    }

    room_update_nav(&g->room, px, py);   /* les ennemis contournent obstacles et trous */
    enemy_update_all(&g->enemies, &g->room, px, py, dt);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies.items[i];
        if (!e->active) continue;
        if (e->surfaced) worm_hole(g, e);
        if (e->landed) boss_landed(g);
        if (e->fire) enemy_fire(g, e, px, py);
        if (e->dig) audio_play(SOUND_DIG, 1.0f);
        if (e->cry && g->cry_cooldown <= 0.0f) {   /* de temps en temps, et jamais deux bébés à la fois */
            audio_play(SDL_rand(2) ? SOUND_BABY_CRY : SOUND_BABY_SOB, e->type == ENEMY_BOSS ? tuning.boss_cry_pitch : 1.0f);
            g->cry_cooldown = tuning.baby_cry_gap;
        }
    }
    projectile_update_all(&g->shots, &g->room, dt);
    resolve_shots_vs_obstacles(g);
    update_bullets(g, dt);
    splatter_update(&g->splatter, &g->room, dt);

    pickup_update_all(&g->pickups, px, py, dt);
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickup *pk = &g->pickups.items[i];
        if (pickup_reached(pk, px, py)) {
            collect(g, pk->type);
            audio_play(pk->type == PICKUP_BOMB ? SOUND_PICKUP_BOMB : pk->type == PICKUP_COIN ? SOUND_PICKUP_COIN : SOUND_KEY, 1.0f);
            pk->active = false;
        }
    }

    for (int i = 0; i < MAX_BOMBS; i++) {
        Bomb *b = &g->bombs.items[i];
        bool boom = bomb_update(b, dt);
        if (b->tick) audio_play(b->tick == 1 ? SOUND_TICK : SOUND_TOCK, 1.0f);
        if (boom) explode(g, b);
    }
    for (int i = 0; i < MAX_HAZARDS; i++) {
        Hazard *h = &g->hazards.items[i];
        if (hazard_update(h, dt) && h->type == HAZARD_BRICK) {
            float d2 = (px - h->x) * (px - h->x) + (py - h->y) * (py - h->y);
            if (d2 <= tuning.brick_radius * tuning.brick_radius) player_hurt(&g->player, tuning.brick_damage);
        }
    }

    /* Encre : les ennemis qui marchent dedans sont ralentis. */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies.items[i];
        if (enemy_tangible(e) && hazard_in(&g->hazards, HAZARD_INK, e->box.x + e->box.w * 0.5f, e->box.y + e->box.h)) {
            enemy_slow(e, 0.15f);
        }
    }

    resolve_shots_vs_enemies(g);
    resolve_enemies_vs_player(g);
    resolve_obstacles_vs_player(g);
    if (shot.fire) {
        fire(g, shot);
        resolve_shots_vs_enemies(g);   /* la cloche et l'orgue frappent dès le tir */
        resolve_shots_vs_obstacles(g);
    }
    resolve_offers(g);
    resolve_boxes(g);
    resolve_locked_doors(g);

    if (g->player.stats.hp <= 0) {
        g->deaths++;
        game_new_run(g, random_seed());
        return;
    }

    /* Portes sans serrure : ouvertes si et seulement si la salle est vide. */
    bool empty = enemy_count(&g->enemies) == 0;
    FloorRoom *fr = &g->floor.rooms[g->room.index];
    if (empty && !fr->cleared) {
        fr->cleared = true;
        room_cleared(g);
    }
    room_set_doors_open(&g->room, empty, &g->player.box);

    /* Trappe : il faut en être sorti une fois (on peut apparaître dessus). */
    player_center(&g->player, &px, &py);
    if (g->room.trapdoor) {
        float dx = px - g->room.trapdoor_x, dy = py - g->room.trapdoor_y;
        bool inside = dx * dx + dy * dy <= tuning.trapdoor_radius * tuning.trapdoor_radius;
        if (!inside) {
            g->trapdoor_armed = true;
        } else if (g->trapdoor_armed) {
            game_next_floor(g, random_seed());
            return;
        }
    }

    if (g->room.doors_open) {
        const RoomDoor *door = room_door_at(&g->room, px, py);
        if (door) {
            start_transition(g, door);
        }
    }

    hud_update(&g->hud, &g->player.stats, dt);
}

/* Objet posé le plus proche d'Amadeus, à décrire dans un cadre (GDD §24). Faux s'il n'y en a pas. */
static bool nearby_description(const Game *g, Description *d)
{
    const FloorRoom *fr = &g->floor.rooms[g->room.index];
    const PlayerStats *st = &g->player.stats;
    float px, py;
    player_center(&g->player, &px, &py);
    float best = tuning.describe_distance * tuning.describe_distance;
    int prop = -1, box = -1;
    for (int i = 0; i < g->room.prop_count; i++) {
        PropKind k = g->room.prop_kind[i];
        bool full = (k == PROP_WEAPON && fr->weapon_offer >= 0) || (k == PROP_HEAL && !fr->heal_sold) ||
                    (k == PROP_LUTRIN && i < TREASURE_OFFERS && fr->offers[i] >= 0);
        if (!full) continue;
        const SDL_FRect *b = &g->room.props[i];
        float dx = b->x + b->w * 0.5f - px, dy = b->y + b->h * 0.5f - py;
        if (dx * dx + dy * dy < best) { best = dx * dx + dy * dy; prop = i; }
    }
    for (int i = 0; i < fr->box_count; i++) {
        const MusicBox *b = &fr->boxes[i];
        if (!b->open || b->item < 0) continue;
        float dx = b->x - px, dy = b->y - py;
        if (dx * dx + dy * dy < best) { best = dx * dx + dy * dy; box = i; prop = -1; }
    }
    if (box >= 0) {
        describe_item(d, (ItemId)fr->boxes[box].item);
        return true;
    }
    if (prop < 0) return false;
    PropKind k = g->room.prop_kind[prop];
    if (k == PROP_WEAPON) describe_weapon(d, (WeaponId)fr->weapon_offer);
    else if (k == PROP_HEAL) describe_heal(d);
    else describe_item(d, (ItemId)fr->offers[prop]);
    if (fr->kind == ROOM_SHOP) describe_price(d, tuning.shop_price, st->coins, k == PROP_HEAL && st->hp >= st->max_hp);
    return true;
}

void game_step(Game *g, const Input *in, float dt)
{
    /* Un dégât, quelle qu'en soit la source (ou la mort, qui relance une partie) : Amadeus crie. */
    int hp0 = g->player.stats.hp, deaths0 = g->deaths;
    step_inner(g, in, dt);
    if (g->player.stats.hp < hp0 || g->deaths != deaths0) audio_play(SOUND_HURT, 1.0f);
}

/* Ce qui est trié par profondeur : on dessine d'abord ce dont le pied est le plus haut. */
typedef enum DrawKind { DRAW_PLAYER, DRAW_ENEMY, DRAW_LUTRIN, DRAW_BOMB, DRAW_PICKUP, DRAW_OBSTACLE, DRAW_BOX } DrawKind;
typedef struct DrawItem {
    float    y;
    DrawKind kind;
    int      index;
} DrawItem;
#define MAX_DRAW_ITEMS (1 + MAX_ENEMIES + ROOM_MAX_PROPS + MAX_BOMBS + MAX_PICKUPS + MAX_OBSTACLES + ROOM_MAX_BOXES)

static void push_item(DrawItem *items, int *n, float y, DrawKind kind, int index)
{
    int k = (*n)++;
    while (k > 0 && items[k - 1].y > y) {
        items[k] = items[k - 1];
        k--;
    }
    items[k] = (DrawItem){ y, kind, index };
}

/* Soin du magasin : un petit soufflet de crescendo rouge, comme la barre de vie. */
static void render_heal_icon(SDL_Renderer *r, float cx, float cy)
{
    SDL_FColor red = { 0.80f, 0.20f, 0.17f, 1.0f }, light = { 0.93f, 0.89f, 0.80f, 1.0f };
    SDL_Vertex v[6] = {
        { { cx - 16.0f, cy }, light, { 0, 0 } }, { { cx + 16.0f, cy - 11.0f }, light, { 0, 0 } }, { { cx + 16.0f, cy + 11.0f }, light, { 0, 0 } },
        { { cx - 12.0f, cy }, red, { 0, 0 } },   { { cx + 13.0f, cy - 8.0f }, red, { 0, 0 } },    { { cx + 13.0f, cy + 8.0f }, red, { 0, 0 } },
    };
    SDL_RenderGeometry(r, NULL, v, 6, NULL, 0);
}

/* Prix d'une chose du magasin : la couronne et le nombre de royalties, sous le lutrin. */
static void render_price(SDL_Renderer *r, float cx, float y, bool affordable)
{
    SDL_Color c = affordable ? (SDL_Color){ 60, 40, 30, 255 } : (SDL_Color){ 150, 40, 34, 255 };
    float w = 14.0f + 2.0f * 4.0f * 2.0f;
    draw_crown(r, cx - w * 0.5f + 5.0f, y + 5.0f, 0.9f);
    draw_number(r, cx - w * 0.5f + 14.0f, y, tuning.shop_price, 2, 2.0f, c);
}

static void render_prop(const Game *g, SDL_Renderer *r, int i, Camera cam)
{
    room_render_prop(&g->room, i, r, cam);
    const FloorRoom *fr = &g->floor.rooms[g->room.index];
    const SDL_FRect *p = &g->room.props[i];
    float bob = sinf(g->time * 3.0f + (float)i) * 3.0f;
    float x = p->x + p->w * 0.5f - cam.x;
    bool shop = fr->kind == ROOM_SHOP, rich = g->player.stats.coins >= tuning.shop_price;
    if (g->room.prop_kind[i] == PROP_WEAPON) {
        if (fr->weapon_offer >= 0) weapon_render_icon((WeaponId)fr->weapon_offer, r, x, p->y + p->h - 62.0f + bob - cam.y, 1.0f);
    } else if (g->room.prop_kind[i] == PROP_HEAL) {
        if (!fr->heal_sold) {
            render_heal_icon(r, x, p->y + p->h - 90.0f + bob - cam.y);
            render_price(r, x, p->y + p->h + 10.0f - cam.y, rich);
        }
    } else if (i < TREASURE_OFFERS && fr->offers[i] >= 0) {
        item_render((ItemId)fr->offers[i], r, x, p->y + p->h - 98.0f + bob - cam.y);
        if (shop) render_price(r, x, p->y + p->h + 10.0f - cam.y, rich);
    }
}

/* Boîte à musique, et l'objet qui flotte au-dessus d'une boîte dorée ouverte. */
static void render_box(const Game *g, const MusicBox *b, SDL_Renderer *r, Camera cam)
{
    float x = b->x - cam.x, y = b->y - cam.y;
    draw_ellipse(r, x + 1.0f, y, 21.0f, 5.0f, (SDL_Color){ 60, 40, 30, 110 });
    SpriteId id = b->golden ? (b->open ? SPR_BOX_GOLD_OPEN : SPR_BOX_GOLD) : (b->open ? SPR_BOX_GRAY_OPEN : SPR_BOX_GRAY);
    sprite_draw(r, id, SDL_floorf(x - 24.0f), SDL_floorf(y - 44.0f), false, false);
    if (b->open && b->item >= 0) item_render((ItemId)b->item, r, x, y - 62.0f + sinf(g->time * 3.0f) * 3.0f);
}

/* La salle courante et ses occupants, vus par la caméra `cam`. */
static void render_room_contents(const Game *g, SDL_Renderer *r, float alpha, Camera cam)
{
    room_render(&g->room, r, cam);
    const RoomLayout *L = g->room.layout;
    for (int i = 0; L && i < L->obstacle_count; i++) {
        if (obstacle_flat(L->obstacles[i].type)) obstacle_render(L, &L->obstacles[i], r, cam, g->time);
    }
    splatter_render_ground(&g->splatter, r, alpha, cam);

    /* Au sol : ombres des briques et des ennemis. */
    for (int i = 0; i < MAX_HAZARDS; i++) hazard_render_ground(&g->hazards.items[i], r, cam);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g->enemies.items[i].active) enemy_render_shadow(&g->enemies.items[i], r, alpha, cam);
    }
    for (int i = 0; i < MAX_BULLETS; i++) bullet_render_shadow(&g->bullets.items[i], r, alpha, cam);

    DrawItem items[MAX_DRAW_ITEMS];
    int n = 0;
    const Player *p = &g->player;
    push_item(items, &n, p->prev_y + (p->box.y - p->prev_y) * alpha + p->box.h, DRAW_PLAYER, 0);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g->enemies.items[i].active) push_item(items, &n, enemy_feet_y(&g->enemies.items[i], alpha), DRAW_ENEMY, i);
    }
    for (int i = 0; i < g->room.prop_count; i++) {
        push_item(items, &n, g->room.props[i].y + g->room.props[i].h, DRAW_LUTRIN, i);
    }
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (g->bombs.items[i].active) push_item(items, &n, g->bombs.items[i].y, DRAW_BOMB, i);
    }
    for (int i = 0; i < MAX_PICKUPS; i++) {
        if (g->pickups.items[i].active) push_item(items, &n, pickup_feet_y(&g->pickups.items[i], alpha), DRAW_PICKUP, i);
    }
    for (int i = 0; L && i < L->obstacle_count; i++) {
        const Obstacle *o = &L->obstacles[i];
        if (o->alive && !obstacle_flat(o->type)) push_item(items, &n, obstacle_feet_y(L, o), DRAW_OBSTACLE, i);
    }
    const FloorRoom *fr = &g->floor.rooms[g->room.index];
    for (int i = 0; i < fr->box_count; i++) push_item(items, &n, fr->boxes[i].y, DRAW_BOX, i);
    for (int k = 0; k < n; k++) {
        int i = items[k].index;
        switch (items[k].kind) {
        case DRAW_PLAYER: player_render(p, r, alpha, cam); break;
        case DRAW_ENEMY:  enemy_render(&g->enemies.items[i], r, alpha, cam); break;
        case DRAW_LUTRIN: render_prop(g, r, i, cam); break;
        case DRAW_BOMB:   bomb_render(&g->bombs.items[i], r, cam); break;
        case DRAW_PICKUP: pickup_render(&g->pickups.items[i], r, alpha, cam); break;
        case DRAW_OBSTACLE: obstacle_render(L, &L->obstacles[i], r, cam, g->time); break;
        case DRAW_BOX:      render_box(g, &fr->boxes[i], r, cam); break;
        }
    }

    /* Par-dessus : briques qui tombent, notes, explosions. */
    for (int i = 0; i < MAX_HAZARDS; i++) hazard_render_air(&g->hazards.items[i], r, cam);
    projectile_render_all(&g->shots, r, alpha, cam);
    for (int i = 0; i < MAX_BULLETS; i++) bullet_render(&g->bullets.items[i], r, alpha, cam);
    splatter_render_air(&g->splatter, r, alpha, cam);
    for (int i = 0; i < MAX_BOMBS; i++) bomb_render_explosion(&g->bombs.items[i], r, cam);
}

/* ------------------------------------------------------------ salles sombres (GDD §21) */
#define DARK_STEP   16                       /* maille du voile noir, px écran */
#define DARK_COLS   (GAME_W / DARK_STEP + 1)
#define DARK_ROWS   (GAME_H / DARK_STEP + 2)
#define MAX_LIGHTS  (MAX_PROJECTILES + MAX_OBSTACLES + 1)

typedef struct Light { float x, y, r; } Light;

/* Voile noir sur la salle (coordonnées écran), percé autour des lumières : un maillage
 * dont chaque sommet a sa propre transparence, que la carte graphique lisse entre les sommets. */
static void render_darkness(SDL_Renderer *r, const Room *room, Camera cam, const Light *lights, int n)
{
    static SDL_Vertex verts[DARK_COLS * DARK_ROWS];
    static int idx[(DARK_COLS - 1) * (DARK_ROWS - 1) * 6];
    float x0 = SDL_max(floorf(room->bounds.x - cam.x), 0.0f), y0 = SDL_max(floorf(room->bounds.y - cam.y), 0.0f);
    float x1 = SDL_min(room->bounds.x + room->bounds.w - cam.x, (float)GAME_W);
    float y1 = SDL_min(room->bounds.y + room->bounds.h - cam.y, (float)GAME_H);
    if (x1 <= x0 || y1 <= y0) return;
    int cols = SDL_min((int)ceilf((x1 - x0) / DARK_STEP) + 1, DARK_COLS);
    int rows = SDL_min((int)ceilf((y1 - y0) / DARK_STEP) + 1, DARK_ROWS);
    for (int j = 0; j < rows; j++) {
        for (int i = 0; i < cols; i++) {
            float x = SDL_min(x0 + (float)(i * DARK_STEP), x1), y = SDL_min(y0 + (float)(j * DARK_STEP), y1);
            float lit = 0.0f;
            for (int k = 0; k < n; k++) {
                float dx = x - lights[k].x, dy = (y - lights[k].y) * 1.25f;   /* halo un peu aplati */
                float d2 = dx * dx + dy * dy, r2 = lights[k].r * lights[k].r;
                if (d2 >= r2) continue;
                float t = 1.0f - sqrtf(d2 / r2);
                lit = SDL_max(lit, t * t * (3.0f - 2.0f * t));
            }
            SDL_Vertex *v = &verts[j * cols + i];
            v->position = (SDL_FPoint){ x, y };
            v->color = (SDL_FColor){ 0.03f, 0.02f, 0.04f, tuning.darkness * (1.0f - lit) };
            v->tex_coord = (SDL_FPoint){ 0.0f, 0.0f };
        }
    }
    int m = 0;
    for (int j = 0; j + 1 < rows; j++) {
        for (int i = 0; i + 1 < cols; i++) {
            int a = j * cols + i, b = a + 1, c = a + cols, d = c + 1;
            idx[m++] = a; idx[m++] = b; idx[m++] = c;
            idx[m++] = b; idx[m++] = d; idx[m++] = c;
        }
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(r, NULL, verts, rows * cols, idx, m);
}

/* Chandelles allumées de la salle, qui vacillent. */
static int candle_lights(const RoomLayout *L, Camera cam, float time, Light *out, int n)
{
    for (int i = 0; L && i < L->obstacle_count && n < MAX_LIGHTS; i++) {
        const Obstacle *o = &L->obstacles[i];
        if (!o->alive || o->type != OBST_CANDLE) continue;
        float x, y;
        obstacle_center(L, o, &x, &y);
        float flicker = 1.0f + 0.06f * sinf(time * 13.0f + (float)i * 2.1f) + 0.04f * sinf(time * 31.0f + (float)i);
        out[n++] = (Light){ x - cam.x, y - 30.0f - cam.y, tuning.light_candle * flicker };
    }
    return n;
}

/* Lumières de la salle courante : Amadeus, les chandelles, les notes qui volent. */
static void render_room_darkness(const Game *g, SDL_Renderer *r, float alpha, Camera cam)
{
    if (!g->room.layout || !g->room.layout->dark) return;
    static Light lights[MAX_LIGHTS];
    int n = 0;
    const Player *p = &g->player;
    float px = p->prev_x + (p->box.x - p->prev_x) * alpha + p->box.w * 0.5f;
    float py = p->prev_y + (p->box.y - p->prev_y) * alpha;
    lights[n++] = (Light){ px - cam.x, py - 16.0f - cam.y, tuning.light_player };
    n = candle_lights(g->room.layout, cam, g->time, lights, n);
    for (int i = 0; i < MAX_PROJECTILES && n < MAX_LIGHTS; i++) {
        const Projectile *pr = &g->shots.items[i];
        if (!pr->active || pr->kind != SHOT_NOTE) continue;
        float x = pr->prev_x + (pr->x - pr->prev_x) * alpha, y = pr->prev_y + (pr->y - pr->prev_y) * alpha;
        lights[n++] = (Light){ x - cam.x, y - tuning.shot_height - cam.y, tuning.light_note * pr->size };
    }
    render_darkness(r, &g->room, cam, lights, n);
}

void game_render(const Game *g, SDL_Renderer *r, float alpha)
{
    decor_set_floor(&g->floor);
    Camera cam = game_camera(g, alpha);
    const Transition *t = &g->transition;

    if (g->shake_timer > 0.0f && tuning.shake_time > 0.0f) {
        float k = SDL_min(g->shake_timer / tuning.shake_time, 1.0f) * tuning.shake_amplitude;
        cam.x += SDL_roundf(sinf(g->time * 97.0f) * k);
        cam.y += SDL_roundf(cosf(g->time * 71.0f) * k);
    }

    if (t->active) {
        /* Les deux salles glissent ensemble d'un écran entier, avec un léger lissage. */
        float e = t->t * t->t * (3.0f - 2.0f * t->t);
        float dx, dy;
        dir_vector(t->dir, &dx, &dy);
        float sx = dx * GAME_W, sy = dy * GAME_H;
        Camera old_cam = { t->from_cam.x + sx * e, t->from_cam.y + sy * e };
        Camera new_cam = { cam.x - sx * (1.0f - e), cam.y - sy * (1.0f - e) };
        room_render(&t->from, r, old_cam);
        for (int i = 0; i < t->from.prop_count; i++) room_render_prop(&t->from, i, r, old_cam);
        const RoomLayout *FL = t->from.layout;
        for (int i = 0; FL && i < FL->obstacle_count; i++) obstacle_render(FL, &FL->obstacles[i], r, old_cam, g->time);
        render_room_contents(g, r, alpha, new_cam);
        if (FL && FL->dark) {
            static Light from_lights[MAX_LIGHTS];
            int n = candle_lights(FL, old_cam, g->time, from_lights, 0);
            render_darkness(r, &t->from, old_cam, from_lights, n);
        }
        render_room_darkness(g, r, alpha, new_cam);
    } else {
        render_room_contents(g, r, alpha, cam);
        render_room_darkness(g, r, alpha, cam);
    }

    if (!g->backdrop) {
        hud_render(&g->hud, r, &g->player.stats);
        const Enemy *boss = enemy_find_boss(&g->enemies);
        if (boss && !t->active) hud_render_boss(r, boss->hp, boss->max_hp);
        int hover = game_can_teleport(g, g->map_hover) ? g->map_hover : -1;
        minimap_render(&g->floor, g->room.index, r, g->reveal_map, hover);
        Description desc;
        if (!t->active && nearby_description(g, &desc)) describe_render(&desc, r);
    }

    /* Hors des glissements, on dessine à l'avance une salle voisine par image. */
    if (!t->active) decor_prefetch(&g->floor, g->room.index, r);
}
