#include "game.h"
#include "config.h"
#include "minimap.h"

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

void game_spawn_baby(Game *g)
{
    float px, py;
    player_center(&g->player, &px, &py);
    enemy_spawn_baby_away_from(&g->enemies, &g->room, px, py);
}

void game_kill_enemies(Game *g)
{
    enemy_pool_clear(&g->enemies);
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
    /* Téléporté sur un lutrin (ou trop près pour ne pas prendre son objet sans le vouloir) :
     * on descend jusqu'à un endroit dégagé. */
    for (int i = 0; i < 40; i++) {
        SDL_FRect margin = { g->player.box.x - 24.0f, g->player.box.y - 24.0f,
                             g->player.box.w + 48.0f, g->player.box.h + 48.0f };
        bool blocked = false;
        for (int k = 0; k < g->room.prop_count; k++) blocked |= overlaps(&margin, &g->room.props[k]);
        if (!blocked && !room_overlaps_wall(&g->room, &g->player.box)) break;
        entry_y += 8.0f;
        place_player(g, entry_x, entry_y);
    }
    projectile_pool_clear(&g->shots);
    enemy_pool_clear(&g->enemies);
    pickup_pool_clear(&g->pickups);
    bomb_pool_clear(&g->bombs);
    hazard_pool_clear(&g->hazards);
    g->trapdoor_armed = false;
    g->shake_timer = 0.0f;

    fr->visited = true;
    if (!fr->cleared) {
        if (fr->kind == ROOM_BOSS) {
            float mx, my;
            room_middle(&g->room, &mx, &my);
            enemy_spawn_boss(&g->enemies, mx, my);
        }
        for (int i = 0; i < fr->babies; i++) {
            game_spawn_baby(g);
        }
    }
    bool empty = enemy_count(&g->enemies) == 0;
    if (empty) fr->cleared = true;
    room_set_doors_open(&g->room, empty, &g->player.box);
}

/* Objets de la salle du trésor : tirés avec la graine de l'étage, parmi ceux pas encore pris. */
static void fill_treasure(Game *g)
{
    if (g->floor.treasure < 0) return;
    Uint64 rng = g->floor.seed * 0x9E3779B97F4A7C15ull + 0x5EED;
    int pool[ITEM_COUNT], n = 0;
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (!g->owned[i]) pool[n++] = i;
    }
    FloorRoom *fr = &g->floor.rooms[g->floor.treasure];
    for (int k = 0; k < TREASURE_OFFERS; k++) {
        if (n == 0) {
            fr->offers[k] = ITEM_NONE;
            continue;
        }
        int j = SDL_rand_r(&rng, n);
        fr->offers[k] = pool[j];
        pool[j] = pool[--n];
    }
}

static void start_floor(Game *g, Uint64 seed)
{
    floor_generate(&g->floor, seed);
    fill_treasure(g);
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

static void fire(Game *g, Dir dir)
{
    /* Bâton de Lully : de temps en temps, le coup part dans le pied. */
    if (g->owned[ITEM_LULLY] && tuning.item_lully_foot_chance > 0 && SDL_rand(tuning.item_lully_foot_chance) == 0) {
        player_hurt(&g->player, tuning.item_lully_foot_damage);
        return;
    }

    float dx, dy, cx, cy;
    dir_vector(dir, &dx, &dy);
    player_center(&g->player, &cx, &cy);

    const PlayerStats *st = &g->player.stats;
    float vx = dx * st->shot_speed + g->player.vx * tuning.shot_inherit;
    float vy = dy * st->shot_speed + g->player.vy * tuning.shot_inherit;
    projectile_spawn(&g->shots, cx + dx * 8.0f, cy + dy * 8.0f, vx, vy, st->shot_range, st->damage);
}

static SDL_FRect shot_box(const Projectile *pr)
{
    float s = tuning.shot_size;
    return (SDL_FRect){ pr->x - s * 0.5f, pr->y - s * 0.5f, s, s };
}

/* Notes contre ennemis : la note disparaît, l'ennemi perd des PV et recule. */
static void resolve_shots_vs_enemies(Game *g)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *pr = &g->shots.items[i];
        if (!pr->active) continue;
        SDL_FRect sb = shot_box(pr);
        for (int j = 0; j < MAX_ENEMIES; j++) {
            Enemy *e = &g->enemies.items[j];
            if (!enemy_tangible(e) || !overlaps(&sb, &e->box)) continue;
            float len = sqrtf(pr->vx * pr->vx + pr->vy * pr->vy);
            float kx = len > 0.0f ? pr->vx / len : 0.0f;
            float ky = len > 0.0f ? pr->vy / len : 0.0f;
            enemy_hurt(e, pr->damage, kx * tuning.shot_knockback, ky * tuning.shot_knockback);
            pr->active = false;
            break;
        }
    }
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
    float R = tuning.bomb_radius;
    for (int j = 0; j < MAX_ENEMIES; j++) {
        Enemy *e = &g->enemies.items[j];
        if (!enemy_tangible(e)) continue;
        float dx = e->box.x + e->box.w * 0.5f - b->x, dy = e->box.y + e->box.h * 0.5f - b->y;
        float d = sqrtf(dx * dx + dy * dy);
        float reach = R + SDL_min(e->box.w, e->box.h) * 0.5f;
        if (d > reach) continue;
        float kx = d > 0.0f ? dx / d : 0.0f, ky = d > 0.0f ? dy / d : 1.0f;
        enemy_hurt(e, tuning.bomb_damage, kx * tuning.bomb_knockback, ky * tuning.bomb_knockback);
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

/* Salle du trésor : toucher un lutrin prend son objet, les autres disparaissent. */
static void resolve_treasure(Game *g)
{
    FloorRoom *fr = &g->floor.rooms[g->room.index];
    if (fr->kind != ROOM_TREASURE) return;
    for (int i = 0; i < g->room.prop_count; i++) {
        if (fr->offers[i] < 0) continue;
        SDL_FRect reach = g->room.props[i];
        reach.x -= 6.0f;
        reach.y -= 6.0f;
        reach.w += 12.0f;
        reach.h += 12.0f;
        if (!overlaps(&reach, &g->player.box)) continue;
        take_item(g, (ItemId)fr->offers[i]);
        for (int k = 0; k < TREASURE_OFFERS; k++) fr->offers[k] = ITEM_NONE;
        return;
    }
}

/* Salle vidée : prix au centre, et la trappe si c'était le boss. */
static void room_cleared(Game *g)
{
    FloorRoom *fr = &g->floor.rooms[g->room.index];
    if (fr->kind == ROOM_BOSS) {
        room_open_trapdoor(&g->room);
        g->trapdoor_armed = false;
    }
    if (SDL_randf() < tuning.reward_chance) {
        float x, y;
        room_middle(&g->room, &x, &y);
        pickup_spawn(&g->pickups, SDL_rand(2) == 0 ? PICKUP_BOMB : PICKUP_COIN, x, y);
    }
}

void game_step(Game *g, const Input *in, float dt)
{
    g->time += dt;
    if (g->shake_timer > 0.0f) g->shake_timer -= dt;

    /* Pendant le glissement entre deux salles, le jeu est en pause. */
    if (g->transition.active) {
        float dur = tuning.room_transition_time;
        g->transition.t += (dur > 0.0f) ? dt / dur : 1.0f;
        if (g->transition.t >= 1.0f) g->transition.active = false;
        hud_update(&g->hud, &g->player.stats, dt);
        return;
    }

    Dir shot = player_update(&g->player, in, &g->room, dt);
    float px, py;
    player_center(&g->player, &px, &py);

    /* Poser un métronome à ses pieds. */
    if (in->bomb && g->player.stats.bombs > 0 && bomb_spawn(&g->bombs, px, py)) {
        g->player.stats.bombs--;
    }

    enemy_update_all(&g->enemies, &g->room, px, py, dt);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g->enemies.items[i].active && g->enemies.items[i].landed) boss_landed(g);
    }
    projectile_update_all(&g->shots, &g->room, dt);

    pickup_update_all(&g->pickups, px, py, dt);
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickup *pk = &g->pickups.items[i];
        if (pickup_reached(pk, px, py)) {
            collect(g, pk->type);
            pk->active = false;
        }
    }

    for (int i = 0; i < MAX_BOMBS; i++) {
        if (bomb_update(&g->bombs.items[i], dt)) explode(g, &g->bombs.items[i]);
    }
    for (int i = 0; i < MAX_HAZARDS; i++) {
        Hazard *h = &g->hazards.items[i];
        if (hazard_update(h, dt)) {
            float d2 = (px - h->x) * (px - h->x) + (py - h->y) * (py - h->y);
            if (d2 <= tuning.brick_radius * tuning.brick_radius) player_hurt(&g->player, tuning.brick_damage);
        }
    }

    resolve_shots_vs_enemies(g);
    resolve_enemies_vs_player(g);
    if (shot != DIR_NONE) {
        fire(g, shot);
    }
    resolve_treasure(g);

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
typedef enum DrawKind { DRAW_PLAYER, DRAW_ENEMY, DRAW_LUTRIN, DRAW_BOMB, DRAW_PICKUP } DrawKind;
typedef struct DrawItem {
    float    y;
    DrawKind kind;
    int      index;
} DrawItem;
#define MAX_DRAW_ITEMS (1 + MAX_ENEMIES + ROOM_MAX_PROPS + MAX_BOMBS + MAX_PICKUPS)

static void push_item(DrawItem *items, int *n, float y, DrawKind kind, int index)
{
    int k = (*n)++;
    while (k > 0 && items[k - 1].y > y) {
        items[k] = items[k - 1];
        k--;
    }
    items[k] = (DrawItem){ y, kind, index };
}

static void render_lutrin(const Game *g, SDL_Renderer *r, int i, Camera cam)
{
    room_render_lutrin(&g->room, i, r, cam);
    const FloorRoom *fr = &g->floor.rooms[g->room.index];
    if (fr->kind != ROOM_TREASURE || fr->offers[i] < 0) return;
    const SDL_FRect *p = &g->room.props[i];
    float bob = sinf(g->time * 3.0f + (float)i) * 3.0f;
    item_render((ItemId)fr->offers[i], r, p->x + p->w * 0.5f - cam.x, p->y + p->h - 98.0f + bob - cam.y);
}

/* La salle courante et ses occupants, vus par la caméra `cam`. */
static void render_room_contents(const Game *g, SDL_Renderer *r, float alpha, Camera cam)
{
    room_render(&g->room, r, cam);

    /* Au sol : ombres des briques et des ennemis. */
    for (int i = 0; i < MAX_HAZARDS; i++) hazard_render_ground(&g->hazards.items[i], r, cam);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g->enemies.items[i].active) enemy_render_shadow(&g->enemies.items[i], r, alpha, cam);
    }

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
    for (int k = 0; k < n; k++) {
        int i = items[k].index;
        switch (items[k].kind) {
        case DRAW_PLAYER: player_render(p, r, alpha, cam); break;
        case DRAW_ENEMY:  enemy_render(&g->enemies.items[i], r, alpha, cam); break;
        case DRAW_LUTRIN: render_lutrin(g, r, i, cam); break;
        case DRAW_BOMB:   bomb_render(&g->bombs.items[i], r, cam); break;
        case DRAW_PICKUP: pickup_render(&g->pickups.items[i], r, alpha, cam); break;
        }
    }

    /* Par-dessus : briques qui tombent, notes, explosions. */
    for (int i = 0; i < MAX_HAZARDS; i++) hazard_render_air(&g->hazards.items[i], r, cam);
    projectile_render_all(&g->shots, r, alpha, cam);
    for (int i = 0; i < MAX_BOMBS; i++) bomb_render_explosion(&g->bombs.items[i], r, cam);
}

void game_render(const Game *g, SDL_Renderer *r, float alpha)
{
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
        for (int i = 0; i < t->from.prop_count; i++) room_render_lutrin(&t->from, i, r, old_cam);
        render_room_contents(g, r, alpha, new_cam);
    } else {
        render_room_contents(g, r, alpha, cam);
    }

    hud_render(&g->hud, r, &g->player.stats);
    const Enemy *boss = enemy_find_boss(&g->enemies);
    if (boss && !t->active) hud_render_boss(r, boss->hp, boss->max_hp);
    int hover = game_can_teleport(g, g->map_hover) ? g->map_hover : -1;
    minimap_render(&g->floor, g->room.index, r, g->reveal_map, hover);
}
