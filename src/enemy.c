#include "enemy.h"
#include "draw.h"

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

Enemy *enemy_spawn_baby(EnemyPool *pool, float x, float y)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &pool->items[i];
        if (!e->active) {
            *e = (Enemy){ 0 };
            e->active = true;
            e->type = ENEMY_BABY;
            e->box.w = tuning.baby_hitbox_w;
            e->box.h = tuning.baby_hitbox_h;
            e->box.x = x - e->box.w * 0.5f;
            e->box.y = y - e->box.h * 0.5f;
            e->prev_x = e->box.x;
            e->prev_y = e->box.y;
            e->hp = e->max_hp = tuning.baby_hp;
            e->anim_time = SDL_randf() * 10.0f;   /* pour qu'ils ne se dandinent pas en rythme */
            return e;
        }
    }
    return NULL;
}

Enemy *enemy_spawn_boss(EnemyPool *pool, float x, float y)
{
    Enemy *e = enemy_spawn_baby(pool, x, y);
    if (!e) return NULL;
    e->type = ENEMY_BOSS;
    e->box.w = tuning.boss_hitbox_w;
    e->box.h = tuning.boss_hitbox_h;
    e->box.x = x - e->box.w * 0.5f;
    e->box.y = y - e->box.h * 0.5f;
    e->prev_x = e->box.x;
    e->prev_y = e->box.y;
    e->hp = e->max_hp = tuning.boss_hp;
    e->state = BOSS_WANDER;
    e->state_dur = 1.0f;   /* petite pause avant de bouger */
    e->target_x = x;
    e->target_y = y;
    e->jump_timer = rand_range(tuning.boss_jump_min_delay, tuning.boss_jump_max_delay);
    return e;
}

Enemy *enemy_spawn_baby_away_from(EnemyPool *pool, const Room *room, float avoid_x, float avoid_y)
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
    return enemy_spawn_baby(pool, x, y);
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
        if (!a->active || !enemy_tangible(a)) continue;
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
    if (len > 1.0f) {
        tx = dx / len * tuning.baby_speed;
        ty = dy / len * tuning.baby_speed;
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
        } else {
            tx = dx / len * tuning.boss_speed;
            ty = dy / len * tuning.boss_speed;
        }
        float px = e->box.x, py = e->box.y;
        move(e, room, tx, ty, tuning.baby_accel_time, dt);
        /* Coincé contre un obstacle : il change d'idée. */
        if (len >= 10.0f && e->state_dur <= 0.0f && fabsf(e->box.x - px) + fabsf(e->box.y - py) < 0.01f) {
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

void enemy_update_all(EnemyPool *pool, const Room *room, float target_x, float target_y, float dt)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &pool->items[i];
        if (!e->active) continue;

        e->prev_x = e->box.x;
        e->prev_y = e->box.y;
        e->anim_time += dt;
        if (e->hurt_timer > 0.0f) e->hurt_timer -= dt;

        if (e->type == ENEMY_BOSS) {
            boss_update(e, room, dt);
        } else {
            baby_update(e, room, target_x, target_y, dt);
        }
    }
    separate(pool, room);
}

bool enemy_tangible(const Enemy *e)
{
    return e->active && !(e->type == ENEMY_BOSS && e->state == BOSS_AIR);
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
    if (e->type != ENEMY_BOSS) {   /* le Gros Bébé est trop lourd pour reculer */
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

static void fill(SDL_Renderer *r, float x, float y, float w, float h, SDL_Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_FRect rect = { SDL_floorf(x), SDL_floorf(y), w, h };
    SDL_RenderFillRect(r, &rect);
}

/*
 * Placeholder du bébé médiéval (GDD §4) : proportions d'adulte (petite tête,
 * corps raide dans son lange), carnation ocre, nimbe doré, visage de vieillard.
 */
static void baby_render(const Enemy *e, SDL_Renderer *r, float alpha, Camera cam)
{
    float x = e->prev_x + (e->box.x - e->prev_x) * alpha - cam.x;
    float y = e->prev_y + (e->box.y - e->prev_y) * alpha - cam.y;
    float cx = x + e->box.w * 0.5f;
    float feet = y + e->box.h;
    float bob = (SDL_fmodf(e->anim_time, 0.5f) < 0.25f) ? 0.0f : -2.0f;   /* dandinement */

    /* Quand il est touché, il clignote en blanc. */
    bool flash = e->hurt_timer > 0.0f;
    SDL_Color white  = { 255, 255, 255, 255 };
    SDL_Color nimbus = flash ? white : (SDL_Color){ 222, 178, 52, 255 };
    SDL_Color cross  = flash ? white : (SDL_Color){ 176, 52, 40, 255 };
    SDL_Color skin   = flash ? white : (SDL_Color){ 196, 150, 96, 255 };
    SDL_Color green  = flash ? white : (SDL_Color){ 136, 132, 84, 255 };   /* verdaccio */
    SDL_Color cloth  = flash ? white : (SDL_Color){ 236, 228, 206, 255 };
    SDL_Color fold   = flash ? white : (SDL_Color){ 196, 186, 160, 255 };
    SDL_Color ink    = flash ? white : (SDL_Color){ 60, 36, 24, 255 };

    float top = feet - 50.0f + bob;
    /* Nimbe crucifère derrière la tête */
    fill(r, cx - 11.0f, top - 2.0f, 22.0f, 20.0f, nimbus);
    fill(r, cx - 1.0f, top - 2.0f, 2.0f, 6.0f, cross);
    fill(r, cx - 11.0f, top + 7.0f, 5.0f, 2.0f, cross);
    fill(r, cx + 6.0f, top + 7.0f, 5.0f, 2.0f, cross);
    /* Petite tête chauve de vieillard */
    fill(r, cx - 7.0f, top + 3.0f, 14.0f, 14.0f, skin);
    fill(r, cx - 7.0f, top + 12.0f, 14.0f, 5.0f, green);
    fill(r, cx - 5.0f, top + 8.0f, 3.0f, 2.0f, ink);    /* regard en coin */
    fill(r, cx + 2.0f, top + 8.0f, 3.0f, 2.0f, ink);
    fill(r, cx - 3.0f, top + 13.0f, 6.0f, 1.0f, ink);   /* bouche pincée */
    /* Corps raide dans son lange */
    fill(r, cx - 9.0f, top + 17.0f, 18.0f, 28.0f, cloth);
    fill(r, cx - 9.0f, top + 24.0f, 18.0f, 2.0f, fold);
    fill(r, cx - 9.0f, top + 32.0f, 18.0f, 2.0f, fold);
    fill(r, cx - 9.0f, top + 40.0f, 18.0f, 2.0f, fold);
    /* Main qui bénit, deux doigts levés */
    fill(r, cx + 9.0f, top + 18.0f, 4.0f, 6.0f, skin);
    fill(r, cx + 9.0f, top + 14.0f, 1.0f, 4.0f, skin);
    fill(r, cx + 11.0f, top + 14.0f, 1.0f, 4.0f, skin);
    /* Pieds */
    fill(r, cx - 7.0f, feet - 5.0f + bob, 5.0f, 4.0f, skin);
    fill(r, cx + 2.0f, feet - 5.0f - bob, 5.0f, 4.0f, skin);
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
    if (e->type == ENEMY_BOSS) {
        boss_render(e, r, alpha, cam);
    } else {
        baby_render(e, r, alpha, cam);
    }
}
