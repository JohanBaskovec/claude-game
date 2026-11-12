#include "weapon.h"
#include "draw.h"
#include "text.h"

#include <math.h>

static const WeaponTuning *def(int id)
{
    return &tuning.weapons[(id >= 0 && id < WEAPON_COUNT) ? id : WEAPON_VOICE];
}

const char *weapon_name(WeaponId id)
{
    static const TextId names[WEAPON_COUNT] = {
        TXT_WEAPON_VOICE_NAME, TXT_WEAPON_OLIFANT_NAME, TXT_WEAPON_FLUTE_NAME, TXT_WEAPON_LYRE_NAME,
        TXT_WEAPON_TABOURIN_NAME, TXT_WEAPON_BELL_NAME, TXT_WEAPON_ORGAN_NAME,
    };
    return (id >= 0 && id < WEAPON_COUNT) ? text_get(names[id]) : "";
}

const char *weapon_desc(WeaponId id)
{
    static const TextId descs[WEAPON_COUNT] = {
        TXT_WEAPON_VOICE_DESC, TXT_WEAPON_OLIFANT_DESC, TXT_WEAPON_FLUTE_DESC, TXT_WEAPON_LYRE_DESC,
        TXT_WEAPON_TABOURIN_DESC, TXT_WEAPON_BELL_DESC, TXT_WEAPON_ORGAN_DESC,
    };
    return (id >= 0 && id < WEAPON_COUNT) ? text_get(descs[id]) : "";
}

float weapon_fire_rate(const PlayerStats *st)
{
    return SDL_max(st->fire_rate * def(st->weapon)->fire_rate_mul, 0.01f);
}

float weapon_charge_time(const PlayerStats *st)
{
    if (!def(st->weapon)->charge) return 0.0f;
    /* Plus la cadence est haute, plus la charge est rapide. */
    return tuning.flute_charge_time * tuning.start_fire_rate / weapon_fire_rate(st);
}

ShotSpec weapon_shot(const PlayerStats *st, const bool owned[ITEM_COUNT], float charge)
{
    const WeaponTuning *w = def(st->weapon);
    ShotSpec s = {
        .kind     = w->kind,
        .damage   = st->damage * w->damage_mul,
        .speed    = st->shot_speed * w->shot_speed_mul,
        .range    = st->shot_range * w->range_mul,
        .size     = w->size,
        .stun     = w->stun,
        .spread   = w->spread,
        .count    = w->count,
        .spectral = owned[ITEM_ALTIUS],
        .pause    = owned[ITEM_EXPECTARE],
        .ink      = owned[ITEM_LIQUESCENCE],
    };
    if (w->charge) {
        charge = SDL_clamp(charge, 0.0f, 1.0f);
        s.damage *= tuning.flute_min_damage + (tuning.flute_max_damage - tuning.flute_min_damage) * charge;
        s.size *= 1.0f + (tuning.flute_max_size - 1.0f) * charge;
    }
    /* Trope : une note de plus, en éventail (sans effet sur la cloche et l'orgue). */
    if (owned[ITEM_TROPE] && (s.kind == SHOT_NOTE || s.kind == SHOT_WAVE)) {
        s.count += 1;
        s.spread += tuning.trope_spread;
    }
    return s;
}

void weapon_render_icon(WeaponId id, SDL_Renderer *r, float cx, float cy, float s)
{
    SDL_Color ink = { 40, 24, 16, 255 }, wood = { 150, 96, 50, 255 }, gold = { 226, 182, 52, 255 };
    switch (id) {
    case WEAPON_VOICE: {
        /* Une croche rose. */
        draw_ellipse(r, cx - 4 * s, cy + 8 * s, 7 * s, 5.5f * s, ink);
        draw_ellipse(r, cx - 4 * s, cy + 8 * s, 5.5f * s, 4 * s, (SDL_Color){ 255, 70, 168, 255 });
        draw_rect(r, cx + 1.5f * s, cy - 13 * s, 2.5f * s, 21 * s, ink);
        draw_line(r, cx + 3 * s, cy - 13 * s, cx + 10 * s, cy - 4 * s, 2.5f * s, ink);
        break;
    }
    case WEAPON_OLIFANT: {
        /* Cor d'ivoire recourbé, bagues dorées. */
        SDL_Color ivory = { 240, 228, 196, 255 };
        for (int k = 0; k <= 8; k++) {
            float t = (float)k / 8.0f;
            float x = cx - 12 * s + 24 * s * t, y = cy + 6 * s - sinf(t * SDL_PI_F) * 10 * s;
            float rad = (2 + 7 * t * t) * s;
            draw_ellipse(r, x, y, rad + 1.2f * s, rad + 1.2f * s, ink);
            draw_ellipse(r, x, y, rad, rad, (k == 3 || k == 6) ? gold : ivory);
        }
        draw_ellipse(r, cx + 12 * s, cy + 6 * s, 4 * s, 7 * s, ink);
        break;
    }
    case WEAPON_FLUTE: {
        draw_line(r, cx - 14 * s, cy + 12 * s, cx + 14 * s, cy - 12 * s, 6.5f * s, ink);
        draw_line(r, cx - 13 * s, cy + 11 * s, cx + 13 * s, cy - 11 * s, 4.5f * s, (SDL_Color){ 200, 160, 100, 255 });
        for (int k = 0; k < 4; k++) {
            float t = 0.25f + 0.15f * (float)k;
            draw_rect(r, cx - 13 * s + 26 * s * t - 1, cy + 11 * s - 22 * s * t - 1, 2.5f * s, 2.5f * s, ink);
        }
        break;
    }
    case WEAPON_LYRE: {
        /* Deux bras en U, une traverse, des cordes. */
        draw_line(r, cx - 10 * s, cy + 12 * s, cx - 12 * s, cy - 12 * s, 4.5f * s, ink);
        draw_line(r, cx + 10 * s, cy + 12 * s, cx + 12 * s, cy - 12 * s, 4.5f * s, ink);
        draw_line(r, cx - 10 * s, cy + 12 * s, cx - 12 * s, cy - 12 * s, 2.5f * s, gold);
        draw_line(r, cx + 10 * s, cy + 12 * s, cx + 12 * s, cy - 12 * s, 2.5f * s, gold);
        draw_rect(r, cx - 13 * s, cy - 11 * s, 26 * s, 3 * s, ink);
        draw_rect(r, cx - 11 * s, cy + 10 * s, 22 * s, 5 * s, ink);
        draw_rect(r, cx - 10 * s, cy + 11 * s, 20 * s, 3 * s, wood);
        for (int k = -2; k <= 2; k++) draw_rect(r, cx + (float)k * 3.5f * s, cy - 8 * s, 1, 18 * s, (SDL_Color){ 230, 220, 200, 255 });
        break;
    }
    case WEAPON_TABOURIN: {
        /* Tambour et sa baguette. */
        draw_rect(r, cx - 12 * s, cy - 4 * s, 24 * s, 14 * s, ink);
        draw_rect(r, cx - 11 * s, cy - 3 * s, 22 * s, 12 * s, (SDL_Color){ 176, 60, 50, 255 });
        for (int k = 0; k < 4; k++) draw_line(r, cx - 11 * s + (float)k * 7 * s, cy - 3 * s, cx - 7.5f * s + (float)k * 7 * s, cy + 9 * s, 1.5f * s, gold);
        draw_ellipse(r, cx, cy - 4 * s, 12.5f * s, 4.5f * s, ink);
        draw_ellipse(r, cx, cy - 4 * s, 11 * s, 3.5f * s, (SDL_Color){ 236, 222, 190, 255 });
        draw_line(r, cx + 2 * s, cy - 6 * s, cx + 14 * s, cy - 15 * s, 2.5f * s, ink);
        draw_ellipse(r, cx + 14 * s, cy - 15 * s, 2.5f * s, 2.5f * s, ink);
        break;
    }
    case WEAPON_BELL: {
        SDL_Color bronze = { 196, 150, 60, 255 };
        draw_rect(r, cx - 2 * s, cy - 15 * s, 4 * s, 4 * s, ink);
        draw_ellipse(r, cx, cy - 6 * s, 8 * s, 6 * s, ink);
        draw_ellipse(r, cx, cy - 6 * s, 6.8f * s, 5 * s, bronze);
        draw_rect(r, cx - 8 * s, cy - 6 * s, 16 * s, 10 * s, ink);
        draw_rect(r, cx - 6.8f * s, cy - 6 * s, 13.6f * s, 10 * s, bronze);
        draw_rect(r, cx - 12 * s, cy + 3 * s, 24 * s, 6 * s, ink);
        draw_rect(r, cx - 11 * s, cy + 4 * s, 22 * s, 4 * s, bronze);
        draw_rect(r, cx - 4 * s, cy - 4 * s, 2 * s, 6 * s, (SDL_Color){ 240, 210, 120, 255 });
        draw_ellipse(r, cx, cy + 11 * s, 3 * s, 3 * s, ink);
        break;
    }
    case WEAPON_ORGAN: {
        /* Tuyaux d'orgue sur un buffet de bois. */
        static const float heights[5] = { 14, 20, 26, 20, 14 };
        for (int k = 0; k < 5; k++) {
            float x = cx - 12 * s + (float)k * 5 * s, h = heights[k] * s;
            draw_rect(r, x - 1, cy + 4 * s - h - 1, 5 * s + 1, h + 2, ink);
            draw_rect(r, x, cy + 4 * s - h, 4 * s, h, (SDL_Color){ 190, 194, 204, 255 });
            draw_rect(r, x + s, cy + 4 * s - h * 0.35f, 2 * s, 2 * s, ink);
        }
        draw_rect(r, cx - 14 * s, cy + 4 * s, 28 * s, 10 * s, ink);
        draw_rect(r, cx - 13 * s, cy + 5 * s, 26 * s, 8 * s, wood);
        break;
    }
    default:
        break;
    }
}
