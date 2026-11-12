#include "game.h"
#include "config.h"

#include <math.h>

static bool overlaps(const SDL_FRect *a, const SDL_FRect *b)
{
    return a->x < b->x + b->w && b->x < a->x + a->w &&
           a->y < b->y + b->h && b->y < a->y + a->h;
}

static void place_player_at_center(Game *g)
{
    player_init(&g->player, ROOM_FLOOR_X + ROOM_FLOOR_W * 0.5f, ROOM_FLOOR_Y + ROOM_FLOOR_H * 0.5f);
}

void game_spawn_baby(Game *g)
{
    float px, py;
    player_center(&g->player, &px, &py);
    enemy_spawn_baby_away_from(&g->enemies, &g->room, px, py);
}

void game_init(Game *g)
{
    room_init(&g->room);
    place_player_at_center(g);
    projectile_pool_clear(&g->shots);
    enemy_pool_clear(&g->enemies);
    game_spawn_baby(g);
    hud_init(&g->hud, &g->player.stats);
    g->deaths = 0;
}

void game_restart(Game *g)
{
    PlayerStats kept = g->player.stats;
    place_player_at_center(g);
    g->player.stats = kept;
    g->player.stats.hp = kept.max_hp;
    projectile_pool_clear(&g->shots);
    enemy_pool_clear(&g->enemies);
    game_spawn_baby(g);
    hud_init(&g->hud, &g->player.stats);
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
        game_restart(g);
        return;
    }
    hud_update(&g->hud, &g->player.stats, dt);
}

void game_render(const Game *g, SDL_Renderer *r, float alpha)
{
    room_render(&g->room, r);

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
        enemy_render(order[i], r, alpha);
    }
    player_render(p, r, alpha);
    for (; i < n; i++) {
        enemy_render(order[i], r, alpha);
    }

    /* Les notes volent au-dessus de tout le monde. */
    projectile_render_all(&g->shots, r, alpha);

    hud_render(&g->hud, r, &g->player.stats);
}
