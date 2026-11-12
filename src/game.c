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

/* Entre dans une salle : construit sa géométrie, place le joueur, fait apparaître
 * les ennemis à la première visite et ferme les portes s'il y en a. */
static void enter_room(Game *g, int index, float entry_x, float entry_y)
{
    FloorRoom *fr = &g->floor.rooms[index];
    room_build(&g->room, &g->floor, index);
    place_player(g, entry_x, entry_y);
    projectile_pool_clear(&g->shots);
    enemy_pool_clear(&g->enemies);

    fr->visited = true;
    if (!fr->cleared) {
        for (int i = 0; i < fr->babies; i++) {
            game_spawn_baby(g);
        }
    }
    bool empty = enemy_count(&g->enemies) == 0;
    if (empty) fr->cleared = true;
    room_set_doors_open(&g->room, empty, &g->player.box);
}

static void start_floor(Game *g, Uint64 seed)
{
    floor_generate(&g->floor, seed);
    Room start;
    room_build(&start, &g->floor, g->floor.start);
    float cx, cy;
    room_center(&start, &cx, &cy);
    enter_room(g, g->floor.start, cx, cy);
    g->transition.active = false;
    hud_init(&g->hud, &g->player.stats);
}

void game_init(Game *g, Uint64 seed)
{
    player_init(&g->player, 0.0f, 0.0f);
    g->deaths = 0;
    g->reveal_map = false;
    g->map_hover = -1;
    start_floor(g, seed);
}

void game_new_floor(Game *g, Uint64 seed)
{
    PlayerStats kept = g->player.stats;
    player_init(&g->player, 0.0f, 0.0f);
    g->player.stats = kept;
    g->player.stats.hp = kept.max_hp;
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
            if (!e->active || !overlaps(&sb, &e->box)) continue;
            float len = sqrtf(pr->vx * pr->vx + pr->vy * pr->vy);
            float kx = len > 0.0f ? pr->vx / len : 0.0f;
            float ky = len > 0.0f ? pr->vy / len : 0.0f;
            enemy_hurt(e, pr->damage, kx, ky);
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
        if (e->active && overlaps(&e->box, &g->player.box)) {
            if (player_hurt(&g->player, tuning.baby_contact_damage)) {
                return;   /* un seul coup par pas ; l'invincibilité protège ensuite */
            }
        }
    }
}

void game_step(Game *g, const Input *in, float dt)
{
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
    enemy_update_all(&g->enemies, &g->room, px, py, dt);
    projectile_update_all(&g->shots, &g->room, dt);

    resolve_shots_vs_enemies(g);
    resolve_enemies_vs_player(g);

    if (shot != DIR_NONE) {
        fire(g, shot);
    }

    if (g->player.stats.hp <= 0) {
        g->deaths++;
        game_new_floor(g, ((Uint64)SDL_rand_bits() << 32) | SDL_rand_bits());
        return;
    }

    /* Portes sans serrure : ouvertes si et seulement si la salle est vide. */
    bool empty = enemy_count(&g->enemies) == 0;
    if (empty) g->floor.rooms[g->room.index].cleared = true;
    room_set_doors_open(&g->room, empty, &g->player.box);

    if (g->room.doors_open) {
        player_center(&g->player, &px, &py);
        const RoomDoor *door = room_door_at(&g->room, px, py);
        if (door) {
            start_transition(g, door);
        }
    }

    hud_update(&g->hud, &g->player.stats, dt);
}

/* La salle courante et ses occupants, vus par la caméra `cam`. */
static void render_room_contents(const Game *g, SDL_Renderer *r, float alpha, Camera cam)
{
    room_render(&g->room, r, cam);

    /* Personnages triés par profondeur : celui dont les pieds sont le plus haut est dessiné d'abord. */
    const Enemy *order[MAX_ENEMIES];
    int n = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy *e = &g->enemies.items[i];
        if (!e->active) continue;
        int k = n++;
        while (k > 0 && enemy_feet_y(order[k - 1], alpha) > enemy_feet_y(e, alpha)) {
            order[k] = order[k - 1];
            k--;
        }
        order[k] = e;
    }
    const Player *p = &g->player;
    float player_feet = p->prev_y + (p->box.y - p->prev_y) * alpha + p->box.h;
    int i = 0;
    for (; i < n && enemy_feet_y(order[i], alpha) <= player_feet; i++) {
        enemy_render(order[i], r, alpha, cam);
    }
    player_render(p, r, alpha, cam);
    for (; i < n; i++) {
        enemy_render(order[i], r, alpha, cam);
    }

    /* Les notes volent au-dessus de tout le monde. */
    projectile_render_all(&g->shots, r, alpha, cam);
}

void game_render(const Game *g, SDL_Renderer *r, float alpha)
{
    Camera cam = game_camera(g, alpha);
    const Transition *t = &g->transition;

    if (t->active) {
        /* Les deux salles glissent ensemble d'un écran entier, avec un léger lissage. */
        float e = t->t * t->t * (3.0f - 2.0f * t->t);
        float dx, dy;
        dir_vector(t->dir, &dx, &dy);
        float sx = dx * GAME_W, sy = dy * GAME_H;
        Camera old_cam = { t->from_cam.x + sx * e, t->from_cam.y + sy * e };
        Camera new_cam = { cam.x - sx * (1.0f - e), cam.y - sy * (1.0f - e) };
        room_render(&t->from, r, old_cam);
        render_room_contents(g, r, alpha, new_cam);
    } else {
        render_room_contents(g, r, alpha, cam);
    }

    hud_render(&g->hud, r, &g->player.stats);
    int hover = game_can_teleport(g, g->map_hover) ? g->map_hover : -1;
    minimap_render(&g->floor, g->room.index, r, g->reveal_map, hover);
}
