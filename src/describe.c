#include "describe.h"

#include "draw.h"
#include "text.h"
#include "ui_text.h"
#include "weapon.h"

#include <math.h>

/* Nombre court : au plus deux décimales, zéros inutiles retirés, virgule en français. */
static const char *num(char *buf, size_t size, float v)
{
    SDL_snprintf(buf, size, "%.2f", (double)v);
    char *dot = SDL_strchr(buf, '.');
    if (dot) {
        char *end = buf + SDL_strlen(buf) - 1;
        while (end > dot && *end == '0') *end-- = '\0';
        if (end == dot) *end = '\0';
        else if (text_lang() == LANG_FR) *dot = ',';
    }
    return buf;
}

static void begin(Description *d, const char *title, DescIcon icon, int icon_id)
{
    d->title = title;
    d->count = 0;
    d->icon = icon;
    d->icon_id = icon_id;
    d->warning = -1;
}

/* Ajoute une ligne : texte `id` formaté avec un nombre décimal (%s) ou des entiers (%d). */
static void line_f(Description *d, TextId id, float v)
{
    if (d->count >= DESC_MAX_LINES) return;
    char n[24];
    SDL_snprintf(d->lines[d->count++], DESC_LINE_LEN, text_get(id), num(n, sizeof n, v));
}

static void line_i(Description *d, TextId id, int a, int b)
{
    if (d->count >= DESC_MAX_LINES) return;
    SDL_snprintf(d->lines[d->count++], DESC_LINE_LEN, text_get(id), a, b);
}

static void line_if(Description *d, TextId id, int a, float b)
{
    if (d->count >= DESC_MAX_LINES) return;
    char n[24];
    SDL_snprintf(d->lines[d->count++], DESC_LINE_LEN, text_get(id), a, num(n, sizeof n, b));
}

static void line(Description *d, TextId id)
{
    if (d->count >= DESC_MAX_LINES) return;
    SDL_strlcpy(d->lines[d->count++], text_get(id), DESC_LINE_LEN);
}

void describe_item(Description *d, ItemId id)
{
    begin(d, item_name(id), DESC_ICON_ITEM, (int)id);
    switch (id) {
    case ITEM_LULLY:
        line_f(d, TXT_EID_DAMAGE_ADD, tuning.item_lully_damage);
        if (tuning.item_lully_foot_chance > 0)
            line_i(d, TXT_EID_LULLY_FOOT, tuning.item_lully_foot_chance, tuning.item_lully_foot_damage);
        break;
    case ITEM_HAYDN:
        line_i(d, TXT_EID_MAX_HP_ADD, tuning.item_haydn_max_hp, 0);
        line_i(d, TXT_EID_HEALS, tuning.item_haydn_max_hp, 0);
        break;
    case ITEM_SATIE:
        line_f(d, TXT_EID_FIRE_RATE_ADD, tuning.item_satie_fire_rate);
        break;
    case ITEM_GUIDO:
        line_f(d, TXT_EID_SHOT_SPEED_ADD, tuning.item_guido_shot_speed);
        line_f(d, TXT_EID_RANGE_ADD, tuning.item_guido_shot_range);
        break;
    case ITEM_SABOTS:
        line_f(d, TXT_EID_SPEED_ADD, tuning.item_sabots_speed);
        break;
    case ITEM_CELERITER:
        line_f(d, TXT_EID_FIRE_RATE_ADD, tuning.item_celeriter_fire_rate);
        break;
    case ITEM_ALTIUS:
        line(d, TXT_EID_ALTIUS_1);
        line(d, TXT_EID_ALTIUS_2);
        break;
    case ITEM_EXPECTARE:
        line_f(d, TXT_EID_EXPECTARE_1, tuning.expectare_pause);
        line_i(d, TXT_EID_EXPECTARE_2, (int)SDL_roundf(tuning.expectare_at * 100.0f), 0);
        break;
    case ITEM_LIQUESCENCE:
        line(d, TXT_EID_LIQUESCENCE_1);
        line_if(d, TXT_EID_LIQUESCENCE_2, (int)SDL_roundf(tuning.ink_slow * 100.0f), tuning.ink_time);
        break;
    case ITEM_TROPE:
        line(d, TXT_EID_TROPE);
        break;
    default:
        break;
    }
}

/* Les lignes d'une arme se déduisent de ses réglages : tout ce qui n'est pas « comme la voix ». */
void describe_weapon(Description *d, WeaponId id)
{
    begin(d, weapon_name(id), DESC_ICON_WEAPON, (int)id);
    const WeaponTuning *w = &tuning.weapons[id];
    if (w->kind == SHOT_WAVE) line(d, TXT_EID_W_WAVE);
    if (w->kind == SHOT_RING) line(d, TXT_EID_W_RING);
    if (w->kind == SHOT_BEAM) line(d, TXT_EID_W_BEAM);
    if (SDL_fabsf(w->damage_mul - 1.0f) > 1e-3f) line_f(d, TXT_EID_W_DAMAGE, w->damage_mul);
    if (SDL_fabsf(w->fire_rate_mul - 1.0f) > 1e-3f) line_f(d, TXT_EID_W_FIRE_RATE, w->fire_rate_mul);
    if (SDL_fabsf(w->shot_speed_mul - 1.0f) > 1e-3f && w->kind != SHOT_RING && w->kind != SHOT_BEAM)
        line_f(d, TXT_EID_W_SHOT_SPEED, w->shot_speed_mul);
    if (SDL_fabsf(w->range_mul - 1.0f) > 1e-3f && w->kind != SHOT_BEAM) line_f(d, TXT_EID_W_RANGE, w->range_mul);
    if (SDL_fabsf(w->size - 1.0f) > 1e-3f && w->kind == SHOT_NOTE) line_f(d, TXT_EID_W_SIZE, w->size);
    if (w->count > 1) line_i(d, TXT_EID_W_COUNT, w->count, 0);
    if (w->stun > 0.0f) line_f(d, TXT_EID_W_STUN, w->stun);
    if (w->lock_time > 0.0f) line_f(d, TXT_EID_W_LOCK, w->lock_time);
    if (w->charge) line_f(d, TXT_EID_W_CHARGE, tuning.flute_charge_time);
    if (d->count == 0) line(d, TXT_EID_W_BASE);
}

void describe_heal(Description *d)
{
    begin(d, text_get(TXT_SHOP_HEAL_NAME), DESC_ICON_HEAL, 0);
    line_i(d, TXT_EID_HEAL, tuning.shop_heal, 0);
}

void describe_price(Description *d, int price, int coins, bool full_hp)
{
    line_i(d, TXT_EID_PRICE, price, 0);
    if (coins < price) {
        d->warning = d->count;
        line(d, TXT_EID_NO_MONEY);
    } else if (full_hp) {
        d->warning = d->count;
        line(d, TXT_EID_FULL_HP);
    }
}

/* Icône du soin : la pointe rouge du soufflet de vie du HUD. */
static void heal_icon(SDL_Renderer *r, float cx, float cy)
{
    for (int i = 0; i < 24; i++) {
        float h = (float)i * 0.55f;
        draw_rect(r, cx - 12.0f + (float)i, cy - h - 1.0f, 1.0f, 2.0f * h + 2.0f, (SDL_Color){ 60, 20, 18, 255 });
        if (i > 1 && i < 23) draw_rect(r, cx - 12.0f + (float)i, cy - h + 0.5f, 1.0f, SDL_max(2.0f * h - 1.0f, 1.0f), (SDL_Color){ 210, 40, 44, 255 });
    }
}

void describe_render(const Description *d, SDL_Renderer *r)
{
    if (!d->title) return;
    const float pad = 10.0f, icon_w = d->icon != DESC_ICON_NONE ? 46.0f : 0.0f;
    float lh_title = font_line_height(FONT_LABEL), lh = font_line_height(FONT_SMALL) - 1.0f;
    float w = ui_text_width(FONT_LABEL, d->title);
    for (int i = 0; i < d->count; i++) w = SDL_max(w, ui_text_width(FONT_SMALL, d->lines[i]));
    w = SDL_ceilf(w + icon_w + pad * 2.0f);
    float h = SDL_ceilf(SDL_max(pad * 2.0f + lh_title + 4.0f + lh * (float)d->count, icon_w + pad));
    float x = 14.0f, y = GAME_H - 14.0f - h;

    SDL_Color ink = { 58, 38, 28, 255 }, rubric = { 168, 34, 26, 255 };
    draw_rect(r, x + 3, y + 4, w, h, (SDL_Color){ 0, 0, 0, 110 });             /* ombre */
    draw_rect(r, x, y, w, h, ink);                                               /* cadre */
    draw_rect(r, x + 2, y + 2, w - 4, h - 4, (SDL_Color){ 236, 223, 192, 245 }); /* parchemin */

    float tx = x + pad + icon_w;
    if (d->icon == DESC_ICON_ITEM) item_render((ItemId)d->icon_id, r, x + pad + 18.0f, y + pad + 20.0f);
    if (d->icon == DESC_ICON_WEAPON) weapon_render_icon((WeaponId)d->icon_id, r, x + pad + 18.0f, y + pad + 20.0f, 1.0f);
    if (d->icon == DESC_ICON_HEAL) heal_icon(r, x + pad + 18.0f, y + pad + 20.0f);

    ui_text_draw(r, FONT_LABEL, d->title, tx, y + pad - 2.0f, 0.0f, rubric);
    draw_rect(r, tx, y + pad + lh_title, w - (tx - x) - pad, 1.0f, (SDL_Color){ rubric.r, rubric.g, rubric.b, 120 });
    for (int i = 0; i < d->count; i++) {
        SDL_Color c = i == d->warning ? rubric : ink;
        ui_text_draw(r, FONT_SMALL, d->lines[i], tx, y + pad + lh_title + 4.0f + lh * (float)i, 0.0f, c);
    }
}
