#include "enemy.h"

#include <math.h>

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
            e->hp = tuning.baby_hp;
            e->anim_time = SDL_randf() * 10.0f;   /* pour qu'ils ne se dandinent pas en rythme */
            return e;
        }
    }
    return NULL;
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
        if (!a->active) continue;
        for (int j = i + 1; j < MAX_ENEMIES; j++) {
            Enemy *b = &pool->items[j];
            if (!b->active) continue;
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

void enemy_update_all(EnemyPool *pool, const Room *room, float target_x, float target_y, float dt)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &pool->items[i];
        if (!e->active) continue;

        e->prev_x = e->box.x;
        e->prev_y = e->box.y;
        e->anim_time += dt;
        if (e->hurt_timer > 0.0f) e->hurt_timer -= dt;

        /* Bébé : fonce vers le joueur, avec de l'inertie. */
        float dx = target_x - (e->box.x + e->box.w * 0.5f);
        float dy = target_y - (e->box.y + e->box.h * 0.5f);
        float len = sqrtf(dx * dx + dy * dy);
        float tx = 0.0f, ty = 0.0f;
        if (len > 1.0f) {
            tx = dx / len * tuning.baby_speed;
            ty = dy / len * tuning.baby_speed;
        }
        e->vx = approach(e->vx, tx, tuning.baby_accel_time, dt);
        e->vy = approach(e->vy, ty, tuning.baby_accel_time, dt);

        int blocked = room_move_box(room, &e->box, e->vx * dt, e->vy * dt);
        if (blocked & ROOM_BLOCKED_X) e->vx = 0.0f;
        if (blocked & ROOM_BLOCKED_Y) e->vy = 0.0f;
    }
    separate(pool, room);
}

bool enemy_hurt(Enemy *e, float damage, float kx, float ky)
{
    e->hp -= damage;
    e->hurt_timer = tuning.enemy_hurt_flash;
    e->vx += kx * tuning.shot_knockback;
    e->vy += ky * tuning.shot_knockback;
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
void enemy_render(const Enemy *e, SDL_Renderer *r, float alpha, Camera cam)
{
    float x = e->prev_x + (e->box.x - e->prev_x) * alpha - cam.x;
    float y = e->prev_y + (e->box.y - e->prev_y) * alpha - cam.y;
    float cx = x + e->box.w * 0.5f;
    float feet = y + e->box.h;
    float bob = (SDL_fmodf(e->anim_time, 0.5f) < 0.25f) ? 0.0f : -2.0f;   /* dandinement */

    /* Quand il est touché, il clignote en blanc. */
    bool flash = e->hurt_timer > 0.0f;
    SDL_Color white  = { 255, 255, 255, 255 };
    SDL_Color shadow = { 150, 138, 116, 255 };
    SDL_Color nimbus = flash ? white : (SDL_Color){ 222, 178, 52, 255 };
    SDL_Color cross  = flash ? white : (SDL_Color){ 176, 52, 40, 255 };
    SDL_Color skin   = flash ? white : (SDL_Color){ 196, 150, 96, 255 };
    SDL_Color green  = flash ? white : (SDL_Color){ 136, 132, 84, 255 };   /* verdaccio */
    SDL_Color cloth  = flash ? white : (SDL_Color){ 236, 228, 206, 255 };
    SDL_Color fold   = flash ? white : (SDL_Color){ 196, 186, 160, 255 };
    SDL_Color ink    = flash ? white : (SDL_Color){ 60, 36, 24, 255 };

    fill(r, cx - 15.0f, feet - 5.0f, 30.0f, 6.0f, shadow);

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
