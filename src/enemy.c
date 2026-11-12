#include "enemy.h"
#include "draw.h"
#include "sprite.h"

#include <math.h>

#define BOSS_JUMP_HEIGHT 600.0f   /* il sort de l'écran par le haut, px */
#define BOSS_RISE_PART   0.35f    /* part du temps en l'air passée à monter... */
#define BOSS_FALL_PART   0.30f    /* ... et à retomber (entre les deux : hors de l'écran) */

static float rand_range(float a, float b)
{
    return a + SDL_randf() * (b - a);
}

void enemy_pool_clear(EnemyPool *pool)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        pool->items[i].active = false;
    }
}

/* PV et boîte de collision de chaque type. */
static void type_stats(EnemyType type, float *hp, float *w, float *h)
{
    *hp = tuning.baby_hp;
    *w = tuning.baby_hitbox_w;
    *h = tuning.baby_hitbox_h;
    switch (type) {
    case ENEMY_BOSS:   *hp = tuning.boss_hp; *w = tuning.boss_hitbox_w; *h = tuning.boss_hitbox_h; break;
    case ENEMY_MONK:   *hp = tuning.monk_hp; break;
    case ENEMY_QUILL:  *hp = tuning.quill_hp; *w = tuning.quill_hitbox_w; *h = tuning.quill_hitbox_h; break;
    case ENEMY_WORM:   *hp = tuning.worm_hp; *w = tuning.worm_hitbox_w; *h = tuning.worm_hitbox_h; break;
    case ENEMY_RINGER: *hp = tuning.ringer_hp; break;
    default: break;
    }
}

Enemy *enemy_spawn(EnemyPool *pool, EnemyType type, float x, float y)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &pool->items[i];
        if (e->active) continue;
        *e = (Enemy){ 0 };
        e->active = true;
        e->type = type;
        type_stats(type, &e->hp, &e->box.w, &e->box.h);
        e->max_hp = e->hp;
        e->box.x = x - e->box.w * 0.5f;
        e->box.y = y - e->box.h * 0.5f;
        e->prev_x = e->box.x;
        e->prev_y = e->box.y;
        e->anim_time = SDL_randf() * 10.0f;   /* pour qu'ils ne se dandinent pas en rythme */
        e->target_x = x;
        e->target_y = y;
        e->cry_timer = rand_range(tuning.baby_cry_min * 0.5f, tuning.baby_cry_max);   /* pas tous à l'entrée de la salle */
        switch (type) {
        case ENEMY_BOSS:
            e->state = BOSS_WANDER;
            e->state_dur = 1.0f;   /* petite pause avant de bouger */
            e->jump_timer = rand_range(tuning.boss_jump_min_delay, tuning.boss_jump_max_delay);
            break;
        case ENEMY_WORM:
            e->state = WORM_HIDDEN;
            e->state_dur = rand_range(0.5f, 1.5f);
            break;
        default:
            /* Les tireurs attendent un peu avant leur premier tir, et pas tous en même temps. */
            e->action_timer = rand_range(1.0f, 2.2f);
            break;
        }
        return e;
    }
    return NULL;
}

Enemy *enemy_spawn_away_from(EnemyPool *pool, const Room *room, EnemyType type, float avoid_x, float avoid_y)
{
    const float margin = 40.0f;
    float x = 0.0f, y = 0.0f;
    /* Quelques tirages au hasard ; à défaut, le point du sol le plus éloigné testé. */
    float best_d2 = -1.0f;
    for (int attempt = 0; attempt < 32; attempt++) {
        float cx, cy;
        room_random_floor_point(room, margin, &cx, &cy);
        float d2 = (cx - avoid_x) * (cx - avoid_x) + (cy - avoid_y) * (cy - avoid_y);
        if (d2 > best_d2) {
            best_d2 = d2;
            x = cx;
            y = cy;
        }
        if (d2 >= tuning.enemy_spawn_min_dist * tuning.enemy_spawn_min_dist) {
            break;
        }
    }
    return enemy_spawn(pool, type, x, y);
}

void enemy_center(const Enemy *e, float *x, float *y)
{
    *x = e->box.x + e->box.w * 0.5f;
    *y = e->box.y + e->box.h * 0.5f;
}

static float approach(float v, float target, float time_constant, float dt)
{
    if (time_constant <= 0.0f) {
        return target;
    }
    return v + (target - v) * (1.0f - expf(-dt / time_constant));
}

/* Les ennemis qui se chevauchent s'écartent, pour ne pas se fondre en un seul. */
static void separate(EnemyPool *pool, const Room *room)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *a = &pool->items[i];
        if (!a->active || !enemy_tangible(a) || a->type == ENEMY_WORM) continue;   /* le ver reste dans son trou */
        for (int j = i + 1; j < MAX_ENEMIES; j++) {
            Enemy *b = &pool->items[j];
            if (!b->active || !enemy_tangible(b)) continue;
            float ox = SDL_min(a->box.x + a->box.w, b->box.x + b->box.w) - SDL_max(a->box.x, b->box.x);
            float oy = SDL_min(a->box.y + a->box.h, b->box.y + b->box.h) - SDL_max(a->box.y, b->box.y);
            if (ox <= 0.0f || oy <= 0.0f) continue;
            /* On sépare sur l'axe où le chevauchement est le plus petit. */
            float acx = a->box.x + a->box.w * 0.5f, bcx = b->box.x + b->box.w * 0.5f;
            float acy = a->box.y + a->box.h * 0.5f, bcy = b->box.y + b->box.h * 0.5f;
            if (ox < oy) {
                float s = (acx < bcx) ? -0.5f : 0.5f;
                room_move_box(room, &a->box, s * ox, 0.0f);
                room_move_box(room, &b->box, -s * ox, 0.0f);
            } else {
                float s = (acy < bcy) ? -0.5f : 0.5f;
                room_move_box(room, &a->box, 0.0f, s * oy);
                room_move_box(room, &b->box, 0.0f, -s * oy);
            }
        }
    }
}

static void move(Enemy *e, const Room *room, float tx, float ty, float accel_time, float dt)
{
    e->vx = approach(e->vx, tx, accel_time, dt);
    e->vy = approach(e->vy, ty, accel_time, dt);
    int blocked = room_move_box(room, &e->box, e->vx * dt, e->vy * dt);
    if (blocked & ROOM_BLOCKED_X) e->vx = 0.0f;
    if (blocked & ROOM_BLOCKED_Y) e->vy = 0.0f;
}

/* Bébé : fonce vers le joueur, avec de l'inertie. */
static void baby_update(Enemy *e, const Room *room, float target_x, float target_y, float dt)
{
    float dx = target_x - (e->box.x + e->box.w * 0.5f);
    float dy = target_y - (e->box.y + e->box.h * 0.5f);
    float len = sqrtf(dx * dx + dy * dy);
    float tx = 0.0f, ty = 0.0f;
    float speed = tuning.baby_speed * (e->slow_timer > 0.0f ? tuning.ink_slow : 1.0f);
    if (len > 1.0f && e->stun_timer <= 0.0f) {
        tx = dx / len * speed;
        ty = dy / len * speed;
        /* Un peu de marge : il ne se retourne pas sans cesse quand le joueur est juste au-dessus ou en dessous. */
        if (dx < -4.0f) e->face_left = true;
        else if (dx > 4.0f) e->face_left = false;
    }
    move(e, room, tx, ty, tuning.baby_accel_time, dt);
}

static void boss_set_state(Enemy *e, BossState s)
{
    e->state = s;
    e->state_time = 0.0f;
    e->state_dur = 0.0f;
}

/* Gros Bébé : errance au hasard, et de temps en temps un saut vers un point au hasard. */
static void boss_update(Enemy *e, const Room *room, float dt)
{
    e->landed = false;
    e->state_time += dt;

    switch (e->state) {
    case BOSS_WANDER: {
        e->jump_timer -= dt;
        float dx = e->target_x - (e->box.x + e->box.w * 0.5f);
        float dy = e->target_y - (e->box.y + e->box.h * 0.5f);
        float len = sqrtf(dx * dx + dy * dy);
        float tx = 0.0f, ty = 0.0f;
        if (e->state_dur > 0.0f) {
            /* En pause ; state_dur = temps de pause restant. */
            e->state_dur -= dt;
            if (e->state_dur <= 0.0f) room_random_floor_point(room, 80.0f, &e->target_x, &e->target_y);
        } else if (len < 10.0f) {
            e->state_dur = rand_range(0.3f, 1.0f);
        } else if (e->stun_timer <= 0.0f) {
            float speed = tuning.boss_speed * (e->slow_timer > 0.0f ? tuning.ink_slow : 1.0f);
            tx = dx / len * speed;
            ty = dy / len * speed;
        }
        float px = e->box.x, py = e->box.y;
        move(e, room, tx, ty, tuning.baby_accel_time, dt);
        /* Coincé contre un obstacle : il change d'idée. */
        if (len >= 10.0f && e->state_dur <= 0.0f && e->stun_timer <= 0.0f && fabsf(e->box.x - px) + fabsf(e->box.y - py) < 0.01f) {
            room_random_floor_point(room, 80.0f, &e->target_x, &e->target_y);
        }
        if (e->jump_timer <= 0.0f) {
            boss_set_state(e, BOSS_CROUCH);
            e->vx = e->vy = 0.0f;
        }
        break;
    }
    case BOSS_CROUCH:
        if (e->state_time >= tuning.boss_crouch_time - 1e-4f) {
            /* Il décolle : on le place tout de suite au point d'atterrissage, où son ombre va grandir. */
            e->jump_from_x = e->box.x + e->box.w * 0.5f;
            e->jump_from_y = e->box.y + e->box.h;
            float lx, ly;
            room_random_floor_point(room, 70.0f, &lx, &ly);
            e->box.x = e->prev_x = lx - e->box.w * 0.5f;
            e->box.y = e->prev_y = ly - e->box.h * 0.5f;
            boss_set_state(e, BOSS_AIR);
        }
        break;
    case BOSS_AIR:
        if (e->state_time >= tuning.boss_air_time - 1e-4f) {
            boss_set_state(e, BOSS_RECOVER);
            e->landed = true;
        }
        break;
    case BOSS_RECOVER:
        if (e->state_time >= tuning.boss_recover_time - 1e-4f) {
            boss_set_state(e, BOSS_WANDER);
            e->jump_timer = rand_range(tuning.boss_jump_min_delay, tuning.boss_jump_max_delay);
            room_random_floor_point(room, 80.0f, &e->target_x, &e->target_y);
        }
        break;
    }
    if (e->state != BOSS_WANDER) e->vx = e->vy = 0.0f;
}

/* Regarde vers le joueur, avec un peu de marge pour ne pas se retourner sans cesse. */
static void face(Enemy *e, float dx)
{
    if (dx < -4.0f) e->face_left = true;
    else if (dx > 4.0f) e->face_left = false;
}

static float slowed(const Enemy *e, float speed)
{
    return speed * (e->slow_timer > 0.0f ? tuning.ink_slow : 1.0f);
}

/* Tireurs : s'approche au-delà de `far`, recule en deçà de `near`, sinon reste sur place. */
static void keep_distance(Enemy *e, const Room *room, float px, float py, float speed, float near, float far, float dt)
{
    float cx, cy;
    enemy_center(e, &cx, &cy);
    float dx = px - cx, dy = py - cy, len = sqrtf(dx * dx + dy * dy);
    float tx = 0.0f, ty = 0.0f;
    if (len > 1.0f && e->stun_timer <= 0.0f) {
        float k = len > far ? 1.0f : (len < near ? -1.0f : 0.0f);
        tx = dx / len * slowed(e, speed) * k;
        ty = dy / len * slowed(e, speed) * k;
        face(e, dx);
    }
    move(e, room, tx, ty, tuning.baby_accel_time, dt);
}

/* Moine chanteur : rafales de notes visées sur le joueur, en gardant ses distances. */
static void monk_update(Enemy *e, const Room *room, float px, float py, float dt)
{
    keep_distance(e, room, px, py, tuning.monk_speed, tuning.monk_near, tuning.monk_far, dt);
    if (e->stun_timer > 0.0f) return;
    e->action_timer -= dt;
    if (e->action_timer <= 1e-4f) {
        if (e->burst_left <= 0) e->burst_left = SDL_max(tuning.monk_burst, 1);
        e->fire = true;
        e->sing_timer = tuning.enemy_sing_time;
        e->burst_left--;
        e->action_timer = e->burst_left > 0 ? tuning.monk_note_interval : rand_range(tuning.monk_pause_min, tuning.monk_pause_max);
    }
}

/* Plume géante : petits bonds autour du joueur ; s'arrête, se penche et crache une goutte d'encre. */
static void quill_update(Enemy *e, const Room *room, float px, float py, float dt)
{
    float cx, cy;
    enemy_center(e, &cx, &cy);
    face(e, px - cx);
    if (e->stun_timer > 0.0f) {
        move(e, room, 0.0f, 0.0f, tuning.baby_accel_time, dt);
        return;
    }
    if (e->state == SHOOTER_WINDUP) {
        move(e, room, 0.0f, 0.0f, 0.05f, dt);
        if (e->state_time >= tuning.quill_windup - 1e-4f) {
            e->fire = true;
            e->state = SHOOTER_MOVE;
            e->state_time = 0.0f;
            e->action_timer = rand_range(tuning.quill_spit_min, tuning.quill_spit_max);
        }
        return;
    }
    float dx = e->target_x - cx, dy = e->target_y - cy, len = sqrtf(dx * dx + dy * dy);
    if (len < 10.0f) {
        /* Nouveau point au hasard près du joueur. */
        e->target_x = px + rand_range(-150.0f, 150.0f);
        e->target_y = py + rand_range(-90.0f, 90.0f);
        room_clamp_to_floor(room, 40.0f, 40.0f, &e->target_x, &e->target_y);
        dx = e->target_x - cx;
        dy = e->target_y - cy;
        len = sqrtf(dx * dx + dy * dy);
    }
    float tx = 0.0f, ty = 0.0f;
    if (len > 1.0f) {
        tx = dx / len * slowed(e, tuning.quill_speed);
        ty = dy / len * slowed(e, tuning.quill_speed);
    }
    float ox = e->box.x, oy = e->box.y;
    move(e, room, tx, ty, tuning.baby_accel_time, dt);
    if (len >= 10.0f && fabsf(e->box.x - ox) + fabsf(e->box.y - oy) < 0.01f) {
        e->target_x = cx;   /* coincée : elle choisira un autre point au prochain pas */
        e->target_y = cy;
    }
    e->action_timer -= dt;
    if (e->action_timer <= 1e-4f) {
        e->state = SHOOTER_WINDUP;
        e->state_time = 0.0f;
    }
}

static void worm_set_state(Enemy *e, WormState s, float dur)
{
    e->state = s;
    e->state_time = 0.0f;
    e->state_dur = dur;
    if (s == WORM_WARN) e->dig = true;
}

/* Ver de livre : caché, il choisit l'endroit où est le joueur ; le sol s'y soulève, puis il jaillit. */
static void worm_update(Enemy *e, const Room *room, float px, float py, float dt)
{
    e->vx = e->vy = 0.0f;
    float cx, cy;
    enemy_center(e, &cx, &cy);
    face(e, px - cx);
    /* Étourdi dehors, il y reste. */
    if (e->stun_timer > 0.0f && e->state == WORM_UP) e->state_time -= dt;
    if (e->state_time < e->state_dur - 1e-4f) return;

    switch (e->state) {
    case WORM_HIDDEN: {
        float a = SDL_randf() * 2.0f * SDL_PI_F, d = SDL_randf() * tuning.worm_aim_jitter;
        float x = px + cosf(a) * d, y = py + sinf(a) * d;
        room_clamp_to_floor(room, e->box.w * 0.5f + 16.0f, e->box.h * 0.5f + 16.0f, &x, &y);
        e->box.x = e->prev_x = x - e->box.w * 0.5f;
        e->box.y = e->prev_y = y - e->box.h * 0.5f;
        worm_set_state(e, WORM_WARN, tuning.worm_warn_time);
        break;
    }
    case WORM_WARN:   worm_set_state(e, WORM_EMERGE, tuning.worm_emerge_time); break;
    case WORM_EMERGE: worm_set_state(e, WORM_UP, tuning.worm_up_time); break;
    case WORM_UP:     worm_set_state(e, WORM_DIVE, tuning.worm_dive_time); break;
    default:          worm_set_state(e, WORM_HIDDEN, rand_range(tuning.worm_hidden_min, tuning.worm_hidden_max)); break;
    }
}

/* Sonneur de cloche : avance lentement ; sa cloche se balance, puis sonne un éventail de grosses notes. */
static void ringer_update(Enemy *e, const Room *room, float px, float py, float dt)
{
    bool windup = e->state == SHOOTER_WINDUP;
    keep_distance(e, room, px, py, windup ? 0.0f : tuning.ringer_speed, tuning.ringer_keep, tuning.ringer_keep + 90.0f, dt);
    if (e->stun_timer > 0.0f) return;
    if (windup) {
        if (e->state_time >= tuning.ringer_windup - 1e-4f) {
            e->fire = true;
            e->sing_timer = 0.35f;   /* la cloche vibre encore un peu */
            e->state = SHOOTER_MOVE;
            e->state_time = 0.0f;
            e->action_timer = rand_range(tuning.ringer_ring_min, tuning.ringer_ring_max);
        }
        return;
    }
    e->action_timer -= dt;
    if (e->action_timer <= 1e-4f) {
        e->state = SHOOTER_WINDUP;
        e->state_time = 0.0f;
    }
}

void enemy_update_all(EnemyPool *pool, const Room *room, float target_x, float target_y, float dt)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &pool->items[i];
        if (!e->active) continue;

        e->prev_x = e->box.x;
        e->prev_y = e->box.y;
        e->anim_time += dt;
        if (e->hurt_timer > 0.0f) e->hurt_timer -= dt;
        if (e->stun_timer > 0.0f) e->stun_timer -= dt;
        if (e->slow_timer > 0.0f) e->slow_timer -= dt;
        if (e->sing_timer > 0.0f) e->sing_timer -= dt;
        e->fire = false;
        e->dig = false;
        e->cry = false;
        if ((e->type == ENEMY_BABY || e->type == ENEMY_BOSS) && (e->cry_timer -= dt) <= 0.0f) {
            e->cry = true;
            e->cry_timer = rand_range(tuning.baby_cry_min, tuning.baby_cry_max);
        }

        switch (e->type) {
        case ENEMY_BOSS:   boss_update(e, room, dt); break;
        case ENEMY_MONK:   monk_update(e, room, target_x, target_y, dt); break;
        case ENEMY_QUILL:  e->state_time += dt; quill_update(e, room, target_x, target_y, dt); break;
        case ENEMY_WORM:   e->state_time += dt; worm_update(e, room, target_x, target_y, dt); break;
        case ENEMY_RINGER: e->state_time += dt; ringer_update(e, room, target_x, target_y, dt); break;
        default:           baby_update(e, room, target_x, target_y, dt); break;
        }
    }
    separate(pool, room);
}

void enemy_stun(Enemy *e, float seconds)
{
    e->stun_timer = SDL_max(e->stun_timer, seconds);
}

void enemy_slow(Enemy *e, float seconds)
{
    e->slow_timer = SDL_max(e->slow_timer, seconds);
}

bool enemy_tangible(const Enemy *e)
{
    if (!e->active) return false;
    if (e->type == ENEMY_BOSS) return e->state != BOSS_AIR;
    if (e->type == ENEMY_WORM) return e->state != WORM_HIDDEN && e->state != WORM_WARN;
    return true;
}

int enemy_contact_damage(const Enemy *e)
{
    return e->type == ENEMY_BOSS ? tuning.boss_contact_damage : tuning.baby_contact_damage;
}

const Enemy *enemy_find_boss(const EnemyPool *pool)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (pool->items[i].active && pool->items[i].type == ENEMY_BOSS) return &pool->items[i];
    }
    return NULL;
}

bool enemy_hurt(Enemy *e, float damage, float kx, float ky)
{
    if (!enemy_tangible(e)) return false;
    e->hp -= damage;
    e->hurt_timer = tuning.enemy_hurt_flash;
    if (e->type != ENEMY_BOSS && e->type != ENEMY_WORM) {   /* trop lourd, ou planté dans son trou */
        e->vx += kx;
        e->vy += ky;
    }
    if (e->hp <= 1e-4f) {
        e->active = false;
        return true;
    }
    return false;
}

int enemy_count(const EnemyPool *pool)
{
    int n = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        n += pool->items[i].active ? 1 : 0;
    }
    return n;
}

float enemy_feet_y(const Enemy *e, float alpha)
{
    return e->prev_y + (e->box.y - e->prev_y) * alpha + e->box.h;
}

/*
 * Bébé médiéval (GDD §4) : visage de vieillard, auréole, couche. Il marche en trois
 * images et clignote en blanc quand il est touché. Le sprite regarde à droite.
 */
static void baby_render(const Enemy *e, SDL_Renderer *r, float alpha, Camera cam)
{
    float x = e->prev_x + (e->box.x - e->prev_x) * alpha - cam.x;
    float y = e->prev_y + (e->box.y - e->prev_y) * alpha - cam.y;
    float cx = SDL_floorf(x + e->box.w * 0.5f);
    float feet = SDL_floorf(y + e->box.h);
    /* Marche : images 0-1-2-1 (aller-retour) ou 0-1-2 (boucle). */
    static const SpriteId PINGPONG[4] = { SPR_BABY_0, SPR_BABY_1, SPR_BABY_2, SPR_BABY_1 };
    int steps = tuning.baby_walk_pingpong ? 4 : 3;
    int step = (int)(e->anim_time / SDL_max(tuning.baby_walk_frame, 0.01f)) % steps;
    SpriteId id = PINGPONG[step];
    float top = feet - (float)(sprite_bottom(id) + 1);
    sprite_draw(r, id, cx - (float)(sprite_width(id) / 2), top, e->face_left, e->hurt_timer > 0.0f);
}

/* Position à l'écran : centre et pieds, interpolés. */
static void screen_pos(const Enemy *e, float alpha, Camera cam, float *cx, float *feet)
{
    *cx = SDL_floorf(e->prev_x + (e->box.x - e->prev_x) * alpha + e->box.w * 0.5f - cam.x);
    *feet = SDL_floorf(e->prev_y + (e->box.y - e->prev_y) * alpha + e->box.h - cam.y);
}

/* Case de planche posée par son bas sur les pieds, `lift` px plus haut. */
static void draw_on_feet(const Enemy *e, SDL_Renderer *r, SpriteId id, float cx, float feet, float lift, bool flip)
{
    float top = feet - (float)sprite_height(id) - SDL_floorf(lift);
    sprite_draw(r, id, cx - (float)(sprite_width(id) / 2), top, flip, e->hurt_timer > 0.0f);
}

static bool walking(const Enemy *e)
{
    return fabsf(e->vx) + fabsf(e->vy) > 8.0f;
}

static int walk_frame(const Enemy *e, float period)
{
    return walking(e) ? (int)(e->anim_time / period) % 2 : 0;
}

static void monk_render(const Enemy *e, SDL_Renderer *r, float alpha, Camera cam)
{
    float cx, feet;
    screen_pos(e, alpha, cam, &cx, &feet);
    SpriteId id = e->sing_timer > 0.0f ? SPR_MONK_SING : (SpriteId)(SPR_MONK_0 + walk_frame(e, 0.22f));
    draw_on_feet(e, r, id, cx, feet, 0.0f, e->face_left);
}

/* La plume flotte un peu au-dessus du sol et sautille quand elle avance. */
static float quill_lift(const Enemy *e)
{
    float lift = 3.0f + 2.0f * sinf(e->anim_time * 3.0f);
    if (walking(e)) lift += fabsf(sinf(e->anim_time * 9.0f)) * 6.0f;
    return lift;
}

static void quill_render(const Enemy *e, SDL_Renderer *r, float alpha, Camera cam)
{
    float cx, feet;
    screen_pos(e, alpha, cam, &cx, &feet);
    SpriteId id = e->state == SHOOTER_WINDUP ? SPR_QUILL_SPIT : (SpriteId)(SPR_QUILL_0 + (int)(e->anim_time / 0.3f) % 2);
    draw_on_feet(e, r, id, cx, feet, quill_lift(e), e->face_left);
}

/* Le ver sort de son trou : la case est posée sur le trou, le bas du corps reste dedans. */
static void worm_render(const Enemy *e, SDL_Renderer *r, float alpha, Camera cam)
{
    float cx, feet;
    screen_pos(e, alpha, cam, &cx, &feet);
    float u = e->state_dur > 0.0f ? e->state_time / e->state_dur : 1.0f;
    SpriteId id;
    switch (e->state) {
    case WORM_EMERGE: id = u < 0.5f ? SPR_WORM_HEAD : SPR_WORM_HALF; break;
    case WORM_UP:     id = (SpriteId)(SPR_WORM_UP_0 + (int)(e->anim_time / 0.2f) % 2); break;
    case WORM_DIVE:   id = u < 0.5f ? SPR_WORM_HALF : SPR_WORM_HEAD; break;
    default:          return;   /* sous le sol */
    }
    draw_on_feet(e, r, id, cx, feet - 4.0f, 0.0f, e->face_left);   /* bas de la case au milieu du trou */
}

static void worm_render_ground(const Enemy *e, SDL_Renderer *r, float alpha, Camera cam)
{
    if (e->state == WORM_HIDDEN) return;
    float cx, feet;
    screen_pos(e, alpha, cam, &cx, &feet);
    SpriteId id = SPR_WORM_HOLE;
    float shake = 0.0f;
    if (e->state == WORM_WARN) {
        bool late = e->state_time >= e->state_dur * 0.5f;
        id = late ? SPR_WORM_HOLE_CRACK : SPR_WORM_HOLE_BUMP;
        shake = SDL_roundf(sinf(e->anim_time * 70.0f) * (late ? 1.5f : 0.8f));   /* le sol tremble */
    }
    sprite_draw(r, id, cx - (float)(sprite_width(id) / 2) + shake, feet + 6.0f - (float)sprite_height(id), false, false);
}

static void ringer_render(const Enemy *e, SDL_Renderer *r, float alpha, Camera cam)
{
    float cx, feet;
    screen_pos(e, alpha, cam, &cx, &feet);
    bool windup = e->state == SHOOTER_WINDUP;
    SpriteId body = (windup || e->sing_timer > 0.0f) ? SPR_RINGER_RING : (SpriteId)(SPR_RINGER_0 + walk_frame(e, 0.26f));
    draw_on_feet(e, r, body, cx, feet, 0.0f, e->face_left);
    /* Cloche en lévitation au-dessus de la tête : elle se balance avant de sonner, puis vibre. */
    SpriteId bell = SPR_BELL;
    if (windup)                  bell = (SpriteId)(SPR_BELL_LEFT + (int)(e->state_time / 0.09f) % 2);
    else if (e->sing_timer > 0.0f) bell = (SpriteId)(SPR_BELL_LEFT + (int)(e->sing_timer / 0.04f) % 2);
    float bob = SDL_roundf(sinf(e->anim_time * 2.5f) * 2.0f);
    float bell_bottom = feet - (float)sprite_height(body) + 4.0f + bob;
    sprite_draw(r, bell, cx - (float)(sprite_width(bell) / 2), bell_bottom - (float)sprite_height(bell), false, e->hurt_timer > 0.0f);
}

/* Hauteur au-dessus des pieds où tournent les étoiles d'étourdissement. */
static float stun_height(const Enemy *e)
{
    switch (e->type) {
    case ENEMY_BOSS:   return 150.0f;
    case ENEMY_MONK:   return 64.0f;
    case ENEMY_QUILL:  return 72.0f;
    case ENEMY_WORM:   return 48.0f;
    case ENEMY_RINGER: return 92.0f;
    default:           return 58.0f;
    }
}

/* Le Gros Bébé est dessiné dans un repère (cx, feet) étiré par (sx, sy) :
 * x vers la droite, y vers le haut depuis les pieds. */
typedef struct BossFrame {
    float cx, feet, sx, sy;
    bool  flash;
} BossFrame;

static void bf(SDL_Renderer *r, const BossFrame *f, float x, float y, float w, float h, SDL_Color c)
{
    if (f->flash) c = (SDL_Color){ 255, 255, 255, c.a };
    draw_rect(r, f->cx + x * f->sx, f->feet - (y + h) * f->sy, w * f->sx, h * f->sy, c);
}

static void bf_ellipse(SDL_Renderer *r, const BossFrame *f, float x, float y, float rx, float ry, SDL_Color c)
{
    if (f->flash) c = (SDL_Color){ 255, 255, 255, c.a };
    draw_ellipse(r, f->cx + x * f->sx, f->feet - y * f->sy, rx * f->sx, ry * f->sy, c);
}

/* Temps passé dans l'état, interpolé pour le rendu. */
static float boss_render_time(const Enemy *e, float alpha)
{
    return SDL_max(e->state_time - (1.0f - alpha) / (float)TICK_HZ, 0.0f);
}

/* Position (pieds) et hauteur du boss en l'air, ou false s'il est hors de l'écran. */
static bool boss_air_pos(const Enemy *e, float alpha, float *x, float *y, float *z)
{
    float T = SDL_max(tuning.boss_air_time, 0.01f);
    float u = boss_render_time(e, alpha) / T;
    if (u < BOSS_RISE_PART) {
        float v = 1.0f - u / BOSS_RISE_PART;
        *x = e->jump_from_x;
        *y = e->jump_from_y;
        *z = BOSS_JUMP_HEIGHT * (1.0f - v * v);   /* monte en ralentissant */
        return true;
    }
    if (u > 1.0f - BOSS_FALL_PART) {
        float w = (u - (1.0f - BOSS_FALL_PART)) / BOSS_FALL_PART;
        *x = e->box.x + e->box.w * 0.5f;
        *y = e->box.y + e->box.h;
        *z = BOSS_JUMP_HEIGHT * (1.0f - w * w);   /* retombe en accélérant */
        return true;
    }
    return false;
}

/*
 * Placeholder du Gros Bébé : un bébé médiéval géant et très laid (GDD §13).
 * Tête de vieillard chauve à trois poils, monosourcil, verrue, deux dents,
 * nimbe terni et cabossé, lange crasseux.
 */
static void boss_body(SDL_Renderer *r, const BossFrame *f, float anim)
{
    SDL_Color ink = { 50, 30, 20, 255 }, nimbus = { 176, 146, 58, 255 }, nimbus_d = { 128, 102, 40, 255 };
    SDL_Color skin = { 196, 150, 96, 255 }, green = { 136, 132, 84, 255 }, cloth = { 222, 208, 176, 255 };
    SDL_Color fold = { 180, 164, 130, 255 }, stain = { 160, 140, 80, 255 }, red = { 170, 60, 50, 255 };
    SDL_Color tooth = { 230, 220, 170, 255 }, wart = { 150, 110, 70, 255 };
    float bob = SDL_fmodf(anim, 0.7f) < 0.35f ? 0.0f : 2.0f;

    /* Nimbe terni et fêlé */
    bf_ellipse(r, f, 0, 100 + bob, 40, 36, ink);
    bf_ellipse(r, f, 0, 100 + bob, 37, 33, nimbus);
    bf_ellipse(r, f, 0, 100 + bob, 30, 26, nimbus_d);
    bf(r, f, 22, 118 + bob, 3, 14, ink);
    bf(r, f, 25, 112 + bob, 3, 8, ink);
    /* Lange crasseux */
    bf(r, f, -33, 4, 66, 64, ink);
    bf(r, f, -31, 5, 62, 62, cloth);
    for (int k = 0; k < 4; k++) bf(r, f, -31, 14 + k * 13, 62, 3, fold);
    bf(r, f, -20, 22, 12, 8, stain);
    bf(r, f, 10, 40, 9, 6, stain);
    /* Bras potelés : une main qui bénit, l'autre qui gratte le ventre */
    bf(r, f, -44, 36 + bob, 14, 22, ink);
    bf(r, f, -43, 37 + bob, 12, 20, skin);
    bf(r, f, 30, 44 - bob, 14, 22, ink);
    bf(r, f, 31, 45 - bob, 12, 20, skin);
    bf(r, f, 33, 66 - bob, 3, 8, skin);
    bf(r, f, 38, 66 - bob, 3, 8, skin);
    /* Pieds */
    bf(r, f, -26, 0, 18, 9, ink);
    bf(r, f, -25, 1, 16, 7, skin);
    bf(r, f, 8, 0, 18, 9, ink);
    bf(r, f, 9, 1, 16, 7, skin);
    /* Grosse tête de vieillard */
    float hy = 66 + bob;
    bf(r, f, -28, hy, 56, 50, ink);
    bf(r, f, -27, hy + 1, 54, 48, skin);
    bf(r, f, -27, hy + 1, 54, 20, green);   /* verdaccio */
    /* Trois poils */
    bf(r, f, -8, hy + 49, 2, 8, ink);
    bf(r, f, 1, hy + 49, 2, 11, ink);
    bf(r, f, 9, hy + 49, 2, 6, ink);
    /* Monosourcil froncé et petits yeux méchants (un plus gros que l'autre) */
    bf(r, f, -20, hy + 34, 40, 4, ink);
    bf(r, f, -17, hy + 27, 8, 6, (SDL_Color){ 240, 230, 200, 255 });
    bf(r, f, -14, hy + 28, 4, 4, ink);
    bf(r, f, 9, hy + 28, 6, 4, (SDL_Color){ 240, 230, 200, 255 });
    bf(r, f, 11, hy + 29, 3, 3, ink);
    /* Gros nez et verrue */
    bf(r, f, -5, hy + 16, 10, 12, ink);
    bf(r, f, -4, hy + 17, 8, 10, (SDL_Color){ 210, 130, 96, 255 });
    bf_ellipse(r, f, 17, hy + 18, 3.5f, 3.5f, wart);
    bf(r, f, 17, hy + 21, 1, 3, ink);
    /* Bouche de travers, deux dents */
    bf(r, f, -14, hy + 6, 26, 6, ink);
    bf(r, f, -12, hy + 7, 22, 4, red);
    bf(r, f, -8, hy + 8, 5, 4, tooth);
    bf(r, f, 4, hy + 8, 4, 3, tooth);
}

static void boss_render(const Enemy *e, SDL_Renderer *r, float alpha, Camera cam)
{
    BossFrame f = { 0, 0, 1.0f, 1.0f, e->hurt_timer > 0.0f };
    float t = boss_render_time(e, alpha);
    float x = e->prev_x + (e->box.x - e->prev_x) * alpha;
    float y = e->prev_y + (e->box.y - e->prev_y) * alpha;
    f.cx = x + e->box.w * 0.5f - cam.x;
    f.feet = y + e->box.h - cam.y;

    switch (e->state) {
    case BOSS_CROUCH: {
        /* Il s'écrase sur lui-même avant de sauter. */
        float u = SDL_min(t / SDL_max(tuning.boss_crouch_time, 0.01f), 1.0f);
        f.sy = 1.0f - 0.25f * u;
        f.sx = 1.0f + 0.15f * u;
        break;
    }
    case BOSS_AIR: {
        float ax, ay, az;
        if (!boss_air_pos(e, alpha, &ax, &ay, &az)) return;
        f.cx = ax - cam.x;
        f.feet = ay - cam.y - az;
        f.sy = 1.12f;   /* étiré par la vitesse */
        f.sx = 0.92f;
        break;
    }
    case BOSS_RECOVER: {
        /* Écrasé par l'atterrissage, puis il se redresse. */
        float u = SDL_min(t / SDL_max(tuning.boss_recover_time, 0.01f), 1.0f);
        f.sy = 0.7f + 0.3f * u;
        f.sx = 1.25f - 0.25f * u;
        break;
    }
    default:
        break;
    }
    boss_body(r, &f, e->anim_time);
}

void enemy_render_shadow(const Enemy *e, SDL_Renderer *r, float alpha, Camera cam)
{
    SDL_Color shadow = { 60, 40, 30, 90 };
    float x = e->prev_x + (e->box.x - e->prev_x) * alpha + e->box.w * 0.5f - cam.x;
    float feet = e->prev_y + (e->box.y - e->prev_y) * alpha + e->box.h - cam.y;

    if (e->type == ENEMY_WORM) {
        worm_render_ground(e, r, alpha, cam);
        return;
    }
    if (e->type == ENEMY_QUILL) {
        float k = 1.0f - quill_lift(e) / 30.0f;   /* plus petite quand elle saute */
        draw_ellipse(r, x, feet - 2.0f, 11.0f * k, 3.0f * k, shadow);
        return;
    }
    if (e->type != ENEMY_BOSS) {
        draw_ellipse(r, x, feet - 2.0f, 15.0f, 4.0f, shadow);
        return;
    }
    if (e->state != BOSS_AIR) {
        draw_ellipse(r, x, feet - 4.0f, 46.0f, 12.0f, shadow);
        return;
    }
    /* En l'air : l'ombre de départ rétrécit, celle d'arrivée grandit et fonce. */
    float u = SDL_clamp(boss_render_time(e, alpha) / SDL_max(tuning.boss_air_time, 0.01f), 0.0f, 1.0f);
    if (u < BOSS_RISE_PART) {
        float k = 1.0f - u / BOSS_RISE_PART;
        draw_ellipse(r, e->jump_from_x - cam.x, e->jump_from_y - 4.0f - cam.y, 46.0f * k, 12.0f * k, shadow);
    }
    float s = 0.25f + 0.75f * u;
    draw_ellipse(r, x, feet - 4.0f, 52.0f * s, 14.0f * s, (SDL_Color){ 30, 20, 16, (Uint8)(40 + 140 * u) });
}

void enemy_render(const Enemy *e, SDL_Renderer *r, float alpha, Camera cam)
{
    switch (e->type) {
    case ENEMY_BOSS:   boss_render(e, r, alpha, cam); break;
    case ENEMY_MONK:   monk_render(e, r, alpha, cam); break;
    case ENEMY_QUILL:  quill_render(e, r, alpha, cam); break;
    case ENEMY_WORM:   worm_render(e, r, alpha, cam); break;
    case ENEMY_RINGER: ringer_render(e, r, alpha, cam); break;
    default:           baby_render(e, r, alpha, cam); break;
    }
    float x = e->prev_x + (e->box.x - e->prev_x) * alpha + e->box.w * 0.5f - cam.x;
    float feet = e->prev_y + (e->box.y - e->prev_y) * alpha + e->box.h - cam.y;
    /* Ralenti : de l'encre colle à ses pieds. */
    if (e->slow_timer > 0.0f && enemy_tangible(e)) {
        draw_ellipse(r, x, feet - 2.0f, e->box.w * 0.55f, 4.0f, (SDL_Color){ 26, 30, 64, 200 });
    }
    /* Étourdi : trois étoiles tournent au-dessus de sa tête. */
    if (e->stun_timer > 0.0f && enemy_tangible(e)) {
        float top = feet - stun_height(e);
        for (int k = 0; k < 3; k++) {
            float a = e->anim_time * tuning.stun_star_hz * 2.0f * SDL_PI_F + (float)k * 2.0f * SDL_PI_F / 3.0f;
            float sx = x + cosf(a) * 14.0f, sy = top + sinf(a) * 4.0f;
            draw_rect(r, sx - 1.0f, sy - 3.0f, 2.0f, 6.0f, (SDL_Color){ 255, 220, 60, 255 });
            draw_rect(r, sx - 3.0f, sy - 1.0f, 6.0f, 2.0f, (SDL_Color){ 255, 220, 60, 255 });
        }
    }
}
