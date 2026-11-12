#include "game.h"
#include "audio.h"
#include "config.h"
#include "decor.h"
#include "minimap.h"
#include "obstacle.h"

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
    if (type == PICKUP_BOMB) st->bombs = SDL_min(st->bombs + 1, 99);
    else                     st->coins = SDL_min(st->coins + 1, 99);
}

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
    if (empty) fr->cleared = true;
    room_set_doors_open(&g->room, empty, &g->player.box);
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
    int rooms[2] = { g->floor.treasure, g->floor.boss };
    for (int k = 0; k < 2; k++) {
        if (rooms[k] < 0) continue;
        FloorRoom *fr = &g->floor.rooms[rooms[k]];
        for (int i = 0; i < TREASURE_OFFERS; i++) fr->offers[i] = draw_from(items, &ni, &rng);
        fr->weapon_offer = draw_from(weapons, &nw, &rng);
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

bool game_can_teleport(const Game *g, int room)
{
    if (room < 0 || room >= g->floor.room_count || g->transition.active) return false;
    if (g->reveal_map) return true;
    return g->floor.rooms[room].visited && enemy_count(&g->enemies) == 0;
}

bool game_teleport(Game *g, int room, int cx, int cy)
{
    if (!game_can_teleport(g, room) || floor_room_at(&g->floor, cx, cy) != room) return false;
    float x = (cx + 0.5f) * CELL_W, y = (cy + 0.5f) * CELL_H;
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

    audio_sing(g->time);   /* Amadeus chante la syllabe suivante du chant en cours */
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
static void enemy_fire(Game *g, const Enemy *e, float px, float py)
{
    float ex, ey;
    enemy_center(e, &ex, &ey);
    float dx = px - ex, dy = py - ey, len = sqrtf(dx * dx + dy * dy);
    float ux = len > 0.0f ? dx / len : 0.0f, uy = len > 0.0f ? dy / len : 1.0f;
    switch (e->type) {
    case ENEMY_MONK:
        audio_play(SOUND_MONK_NOTE, 1.0f);
        bullet_spawn_note(&g->bullets, ex + ux * 10.0f, ey + uy * 10.0f, ux * tuning.monk_note_speed, uy * tuning.monk_note_speed,
                          tuning.monk_note_range, 1.0f, tuning.enemy_note_height);
        break;
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
        } else if (i < TREASURE_OFFERS && fr->offers[i] >= 0) {
            take_item(g, (ItemId)fr->offers[i]);
            for (int k = 0; k < TREASURE_OFFERS; k++) fr->offers[k] = ITEM_NONE;
        }
    }
    if (!on_socle) g->socle_armed = true;
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

/* Salle vidée : prix au centre, et la trappe si c'était le boss. */
static void room_cleared(Game *g)
{
    FloorRoom *fr = &g->floor.rooms[g->room.index];
    if (fr->kind == ROOM_BOSS) {
        room_open_trapdoor(&g->room);
        room_place_offers(&g->room, true);
        free_player_from_props(g);
        g->trapdoor_armed = false;
        g->socle_armed = false;
    }
    if (SDL_randf() < tuning.reward_chance) {
        float x, y;
        room_middle(&g->room, &x, &y);
        pickup_spawn(&g->pickups, SDL_rand(2) == 0 ? PICKUP_BOMB : PICKUP_COIN, x, y);
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

void game_step(Game *g, const Input *in, float dt)
{
    g->time += dt;
    if (g->shake_timer > 0.0f) g->shake_timer -= dt;
    if (g->cry_cooldown > 0.0f) g->cry_cooldown -= dt;

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
        const Enemy *e = &g->enemies.items[i];
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
            audio_play(pk->type == PICKUP_BOMB ? SOUND_PICKUP_BOMB : SOUND_PICKUP_COIN, 1.0f);
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

/* Ce qui est trié par profondeur : on dessine d'abord ce dont le pied est le plus haut. */
typedef enum DrawKind { DRAW_PLAYER, DRAW_ENEMY, DRAW_LUTRIN, DRAW_BOMB, DRAW_PICKUP, DRAW_OBSTACLE } DrawKind;
typedef struct DrawItem {
    float    y;
    DrawKind kind;
    int      index;
} DrawItem;
#define MAX_DRAW_ITEMS (1 + MAX_ENEMIES + ROOM_MAX_PROPS + MAX_BOMBS + MAX_PICKUPS + MAX_OBSTACLES)

static void push_item(DrawItem *items, int *n, float y, DrawKind kind, int index)
{
    int k = (*n)++;
    while (k > 0 && items[k - 1].y > y) {
        items[k] = items[k - 1];
        k--;
    }
    items[k] = (DrawItem){ y, kind, index };
}

static void render_prop(const Game *g, SDL_Renderer *r, int i, Camera cam)
{
    room_render_prop(&g->room, i, r, cam);
    const FloorRoom *fr = &g->floor.rooms[g->room.index];
    const SDL_FRect *p = &g->room.props[i];
    float bob = sinf(g->time * 3.0f + (float)i) * 3.0f;
    float x = p->x + p->w * 0.5f - cam.x;
    if (g->room.prop_kind[i] == PROP_WEAPON) {
        if (fr->weapon_offer >= 0) weapon_render_icon((WeaponId)fr->weapon_offer, r, x, p->y + p->h - 62.0f + bob - cam.y, 1.0f);
    } else if (i < TREASURE_OFFERS && fr->offers[i] >= 0) {
        item_render((ItemId)fr->offers[i], r, x, p->y + p->h - 98.0f + bob - cam.y);
    }
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
    for (int k = 0; k < n; k++) {
        int i = items[k].index;
        switch (items[k].kind) {
        case DRAW_PLAYER: player_render(p, r, alpha, cam); break;
        case DRAW_ENEMY:  enemy_render(&g->enemies.items[i], r, alpha, cam); break;
        case DRAW_LUTRIN: render_prop(g, r, i, cam); break;
        case DRAW_BOMB:   bomb_render(&g->bombs.items[i], r, cam); break;
        case DRAW_PICKUP: pickup_render(&g->pickups.items[i], r, alpha, cam); break;
        case DRAW_OBSTACLE: obstacle_render(L, &L->obstacles[i], r, cam, g->time); break;
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

    hud_render(&g->hud, r, &g->player.stats);
    const Enemy *boss = enemy_find_boss(&g->enemies);
    if (boss && !t->active) hud_render_boss(r, boss->hp, boss->max_hp);
    int hover = game_can_teleport(g, g->map_hover) ? g->map_hover : -1;
    minimap_render(&g->floor, g->room.index, r, g->reveal_map, hover);

    /* Hors des glissements, on dessine à l'avance une salle voisine par image. */
    if (!t->active) decor_prefetch(&g->floor, g->room.index, r);
}
