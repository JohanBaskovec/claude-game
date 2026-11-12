#include "player.h"
#include "config.h"

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
    p->charge_dir = DIR_NONE;
    p->lock_timer = 0.0f;
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

PlayerFire player_update(Player *p, const Input *in, const Room *room, float fire_rate, float charge_time, float dt)
{
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

    /* Direction de la tête : le tir prime, sinon le déplacement. */
    if (in->shoot != DIR_NONE) {
        p->look = in->shoot;
    } else if (moving) {
        if (fabsf(mx) > fabsf(my)) {
            p->look = mx > 0.0f ? DIR_RIGHT : DIR_LEFT;
        } else {
            p->look = my > 0.0f ? DIR_DOWN : DIR_UP;
        }
    }

    if (p->sing_timer > 0.0f) {
        p->sing_timer -= dt;
    }
    if (p->invuln_timer > 0.0f) {
        p->invuln_timer -= dt;
    }

    PlayerFire fired = { DIR_NONE, 1.0f };

    /* Arme à charger : on charge en tenant une flèche, le tir part au relâchement. */
    if (charge_time > 0.0f) {
        if (in->shoot != DIR_NONE) {
            if (p->fire_cooldown <= 1e-4f) {
                p->charging = true;
                p->charge = SDL_min(p->charge + dt / charge_time, 1.0f);
                p->charge_dir = in->shoot;
            }
        } else if (p->charging) {
            fired.dir = p->charge_dir;
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
    if (in->shoot != DIR_NONE && p->fire_cooldown <= 1e-4f) {
        /* += plutôt que = : la cadence reste exacte quel que soit le pas. */
        p->fire_cooldown += 1.0f / fire_rate;
        p->sing_timer = tuning.sing_duration;
        fired.dir = in->shoot;
    }
    p->fire_cooldown -= dt;
    if (p->fire_cooldown < 0.0f && (in->shoot == DIR_NONE || p->fire_cooldown < -dt)) {
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

static void fill(SDL_Renderer *r, float x, float y, float w, float h,
                 Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    SDL_FRect rect = { SDL_floorf(x), SDL_floorf(y), w, h };
    SDL_RenderFillRect(r, &rect);
}

/*
 * Placeholder aux proportions d'Amadeus (56x68, grosse tête façon TBOI) :
 * corps cyan, tête couleur peau, casque rose, bouche qui s'ouvre au tir.
 * Les pieds sont posés sur le bas de la boîte de collision.
 */
void player_render(const Player *p, SDL_Renderer *renderer, float alpha, Camera cam)
{
    float x = p->prev_x + (p->box.x - p->prev_x) * alpha - cam.x;
    float y = p->prev_y + (p->box.y - p->prev_y) * alpha - cam.y;
    float cx = x + p->box.w * 0.5f;
    float feet = y + p->box.h;

    /* Clignotement pendant l'invincibilité : une demi-période sur deux, invisible. */
    if (p->invuln_timer > 0.0f) {
        int phase = (int)(p->invuln_timer * tuning.player_blink_hz * 2.0f);
        if (phase % 2 == 1) {
            return;
        }
    }

    /* Ombre */
    fill(renderer, cx - 18.0f, feet - 5.0f, 36.0f, 6.0f, 150, 138, 116);

    /* Corps (pyjama cyan) et chaussons vert citron */
    fill(renderer, cx - 13.0f, feet - 30.0f, 26.0f, 26.0f, 40, 210, 230);
    fill(renderer, cx - 14.0f, feet - 6.0f, 12.0f, 6.0f, 170, 240, 40);
    fill(renderer, cx + 2.0f,  feet - 6.0f, 12.0f, 6.0f, 170, 240, 40);

    /* Tête */
    float hx = cx - 21.0f, hy = feet - 68.0f;
    fill(renderer, hx, hy, 42.0f, 38.0f, 240, 200, 170);
    /* Cheveux */
    fill(renderer, hx, hy, 42.0f, 9.0f, 110, 70, 40);
    /* Casque rose : arceau et écouteurs */
    fill(renderer, hx + 2.0f, hy - 4.0f, 38.0f, 5.0f, 255, 70, 168);
    fill(renderer, hx - 5.0f, hy + 12.0f, 8.0f, 16.0f, 255, 70, 168);
    fill(renderer, hx + 39.0f, hy + 12.0f, 8.0f, 16.0f, 255, 70, 168);

    /* Jauge de charge (flûte) au-dessus de la tête. */
    if (p->charging && p->charge > 0.0f) {
        fill(renderer, cx - 17.0f, hy - 14.0f, 34.0f, 6.0f, 30, 20, 20);
        bool full = p->charge >= 1.0f;
        fill(renderer, cx - 16.0f, hy - 13.0f, 32.0f * p->charge, 4.0f, full ? 255 : 240, full ? 230 : 160, full ? 120 : 60);
    }

    /* Visage selon la direction du regard */
    if (p->look == DIR_UP) {
        fill(renderer, hx, hy + 9.0f, 42.0f, 14.0f, 110, 70, 40); /* vu de dos */
        return;
    }
    float ox = 0.0f;
    if (p->look == DIR_LEFT)  ox = -7.0f;
    if (p->look == DIR_RIGHT) ox =  7.0f;
    float fx = cx + ox;

    if (p->look != DIR_RIGHT) fill(renderer, fx - 10.0f, hy + 15.0f, 5.0f, 6.0f, 70, 40, 20);
    if (p->look != DIR_LEFT)  fill(renderer, fx + 5.0f,  hy + 15.0f, 5.0f, 6.0f, 70, 40, 20);

    if (p->sing_timer > 0.0f) {
        fill(renderer, fx - 5.0f, hy + 25.0f, 10.0f, 9.0f, 90, 20, 30);  /* « O » */
    } else {
        fill(renderer, fx - 3.0f, hy + 28.0f, 6.0f, 2.0f, 120, 60, 50);
    }
}
