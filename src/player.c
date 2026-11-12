#include "player.h"
#include "config.h"
#include "draw.h"
#include "sprite.h"

#include <math.h>

PlayerStats player_start_stats(void)
{
    return (PlayerStats){
        .hp         = tuning.start_hp,
        .max_hp     = tuning.start_max_hp,
        .speed      = tuning.start_speed,
        .fire_rate  = tuning.start_fire_rate,
        .damage     = tuning.start_damage,
        .shot_speed = tuning.start_shot_speed,
        .shot_range = tuning.start_shot_range,
        .bombs      = tuning.start_bombs,
        .coins      = tuning.start_coins,
        .weapon     = WEAPON_VOICE,
    };
}

void player_init(Player *p, float center_x, float center_y)
{
    p->stats = player_start_stats();
    p->box.w = tuning.player_hitbox_w;
    p->box.h = tuning.player_hitbox_h;
    p->box.x = center_x - p->box.w * 0.5f;
    p->box.y = center_y - p->box.h * 0.5f;
    p->prev_x = p->box.x;
    p->prev_y = p->box.y;
    p->vx = p->vy = 0.0f;
    p->look = DIR_DOWN;
    p->fire_cooldown = 0.0f;
    p->sing_timer = 0.0f;
    p->invuln_timer = 0.0f;
    p->charge = 0.0f;
    p->charging = false;
    p->charge_x = 0.0f;
    p->charge_y = 1.0f;
    p->lock_timer = 0.0f;
    p->walk_phase = 0.0f;
}

/* Rapproche v de target, avec une constante de temps (0 = immédiat). */
static float approach(float v, float target, float time_constant, float dt)
{
    if (time_constant <= 0.0f) {
        return target;
    }
    float k = 1.0f - expf(-dt / time_constant);
    return v + (target - v) * k;
}

/* Direction la plus proche parmi les 4 (pour choisir le sprite de la tête). */
static Dir nearest_dir(float x, float y)
{
    if (fabsf(x) >= fabsf(y)) return x > 0.0f ? DIR_RIGHT : DIR_LEFT;
    return y > 0.0f ? DIR_DOWN : DIR_UP;
}

PlayerFire player_update(Player *p, const Input *in, float aim_x, float aim_y, const Room *room,
                         float fire_rate, float charge_time, float dt)
{
    bool shooting = aim_x != 0.0f || aim_y != 0.0f;
    p->prev_x = p->box.x;
    p->prev_y = p->box.y;

    /* Cloué sur place (orgue) : les commandes de déplacement sont ignorées. */
    float mx = in->move_x, my = in->move_y;
    if (p->lock_timer > 0.0f) {
        p->lock_timer -= dt;
        mx = my = 0.0f;
    }

    /* Déplacement avec accélération / freinage. */
    bool moving = mx != 0.0f || my != 0.0f;
    float tc = moving ? tuning.player_accel_time : tuning.player_decel_time;
    p->vx = approach(p->vx, mx * p->stats.speed, tc, dt);
    p->vy = approach(p->vy, my * p->stats.speed, tc, dt);
    if (fabsf(p->vx) < 1.0f && mx == 0.0f) p->vx = 0.0f;
    if (fabsf(p->vy) < 1.0f && my == 0.0f) p->vy = 0.0f;

    int blocked = room_move_box(room, &p->box, p->vx * dt, p->vy * dt);
    /* Contre un mur, la vitesse sur cet axe tombe à zéro. */
    if (blocked & ROOM_BLOCKED_X) p->vx = 0.0f;
    if (blocked & ROOM_BLOCKED_Y) p->vy = 0.0f;

    /* Animation de marche : elle avance avec la distance parcourue, et revient au repos à l'arrêt. */
    float moved = sqrtf((p->box.x - p->prev_x) * (p->box.x - p->prev_x) + (p->box.y - p->prev_y) * (p->box.y - p->prev_y));
    if (moved > 0.2f && tuning.player_step_dist > 0.0f) {
        p->walk_phase = SDL_fmodf(p->walk_phase + moved / tuning.player_step_dist, 4.0f);
    } else {
        p->walk_phase = 0.0f;
    }

    /* Direction de la tête : le tir prime, sinon le déplacement. */
    if (shooting) {
        p->look = nearest_dir(aim_x, aim_y);
    } else if (moving) {
        p->look = nearest_dir(mx, my);
    }

    if (p->sing_timer > 0.0f) {
        p->sing_timer -= dt;
    }
    if (p->invuln_timer > 0.0f) {
        p->invuln_timer -= dt;
    }

    PlayerFire fired = { false, 0.0f, 0.0f, 1.0f };

    /* Arme à charger : on charge en tenant une flèche ou le clic, le tir part au relâchement,
     * dans la dernière direction visée. */
    if (charge_time > 0.0f) {
        if (shooting) {
            if (p->fire_cooldown <= 1e-4f) {
                p->charging = true;
                p->charge = SDL_min(p->charge + dt / charge_time, 1.0f);
                p->charge_x = aim_x;
                p->charge_y = aim_y;
            }
        } else if (p->charging) {
            fired.fire = true;
            fired.dx = p->charge_x;
            fired.dy = p->charge_y;
            fired.charge = p->charge;
            p->charging = false;
            p->charge = 0.0f;
            p->fire_cooldown = tuning.flute_recover;
            p->sing_timer = tuning.sing_duration;
        }
        p->fire_cooldown = SDL_max(p->fire_cooldown - dt, 0.0f);
        return fired;
    }
    p->charging = false;
    p->charge = 0.0f;

    /* Cadence de tir. Petite tolérance : évite qu'une erreur d'arrondi décale un tir d'une image. */
    if (shooting && p->fire_cooldown <= 1e-4f) {
        /* += plutôt que = : la cadence reste exacte quel que soit le pas. */
        p->fire_cooldown += 1.0f / fire_rate;
        p->sing_timer = tuning.sing_duration;
        fired.fire = true;
        fired.dx = aim_x;
        fired.dy = aim_y;
    }
    p->fire_cooldown -= dt;
    if (p->fire_cooldown < 0.0f && (!shooting || p->fire_cooldown < -dt)) {
        p->fire_cooldown = 0.0f; /* pas de « réserve » de tirs accumulée à l'arrêt */
    }
    return fired;
}

void player_center(const Player *p, float *x, float *y)
{
    *x = p->box.x + p->box.w * 0.5f;
    *y = p->box.y + p->box.h * 0.5f;
}

bool player_hurt(Player *p, int damage)
{
    /* Petite tolérance : évite qu'une erreur d'arrondi rallonge l'invincibilité d'une image. */
    if (p->invuln_timer > 1e-4f || p->stats.hp <= 0) {
        return false;
    }
    p->stats.hp -= damage;
    if (p->stats.hp < 0) {
        p->stats.hp = 0;
    }
    p->invuln_timer = tuning.player_invuln_time;
    return true;
}

/* Sprite de la direction : de dos, de face ou de profil (le profil regarde à droite). */
static SpriteId facing(Dir d, SpriteId back, SpriteId front, SpriteId side)
{
    return d == DIR_UP ? back : d == DIR_DOWN ? front : side;
}

/*
 * Amadeus (GDD §4) : le corps regarde où il marche, la tête où il tire (comme dans TBOI).
 * Les pieds sont posés sur le bas de la boîte de collision.
 */
void player_render(const Player *p, SDL_Renderer *renderer, float alpha, Camera cam)
{
    float x = p->prev_x + (p->box.x - p->prev_x) * alpha - cam.x;
    float y = p->prev_y + (p->box.y - p->prev_y) * alpha - cam.y;
    float cx = SDL_floorf(x + p->box.w * 0.5f);
    float feet = SDL_floorf(y + p->box.h);

    /* Clignotement pendant l'invincibilité : une demi-période sur deux, invisible. */
    if (p->invuln_timer > 0.0f) {
        int phase = (int)(p->invuln_timer * tuning.player_blink_hz * 2.0f);
        if (phase % 2 == 1) {
            return;
        }
    }

    draw_ellipse(renderer, cx, feet - 2.0f, 17.0f, 5.0f, (SDL_Color){ 40, 28, 22, 90 });   /* ombre */

    /* Corps : direction du déplacement (sinon celle de la tête), deux temps de marche. */
    bool walking = p->walk_phase > 0.0f;
    Dir body = p->look;
    if (walking && (p->vx != 0.0f || p->vy != 0.0f)) {
        body = fabsf(p->vx) > fabsf(p->vy) ? (p->vx > 0.0f ? DIR_RIGHT : DIR_LEFT) : (p->vy > 0.0f ? DIR_DOWN : DIR_UP);
    }
    int beat = (int)p->walk_phase;                     /* 0 repos, 1 pas, 2 repos, 3 autre pas */
    int frame = beat == 1 ? 1 : beat == 3 ? 2 : 0;
    SpriteId base = facing(body, SPR_AMA_BODY_BACK, SPR_AMA_BODY_FRONT, SPR_AMA_BODY_SIDE);
    float left = cx - (float)(sprite_width(base) / 2);
    float body_top = feet - (float)(sprite_bottom(base) + 1);
    sprite_draw(renderer, (SpriteId)(base + frame), left, body_top, body == DIR_LEFT, false);

    /* Tête : direction du tir, bouche ouverte quand il chante ; elle descend d'un pixel à chaque pas. */
    bool sing = p->sing_timer > 0.0f;
    SpriteId head = facing(p->look, sing ? SPR_AMA_HEAD_BACK_SING : SPR_AMA_HEAD_BACK,
                           sing ? SPR_AMA_HEAD_FRONT_SING : SPR_AMA_HEAD_FRONT,
                           sing ? SPR_AMA_HEAD_SIDE_SING : SPR_AMA_HEAD_SIDE);
    float head_left = cx - (float)(sprite_width(head) / 2);
    float head_top = body_top - (float)sprite_ama_head_above_body() + (frame ? 1.0f : 0.0f);
    sprite_draw(renderer, head, head_left, head_top, p->look == DIR_LEFT, false);

    /* Jauge de charge (flûte) au-dessus de la tête. */
    if (p->charging && p->charge > 0.0f) {
        bool full = p->charge >= 1.0f;
        draw_rect(renderer, cx - 17.0f, head_top - 8.0f, 34.0f, 6.0f, (SDL_Color){ 30, 20, 20, 255 });
        draw_rect(renderer, cx - 16.0f, head_top - 7.0f, 32.0f * p->charge, 4.0f,
                  full ? (SDL_Color){ 255, 230, 120, 255 } : (SDL_Color){ 240, 160, 60, 255 });
    }
}
