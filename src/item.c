#include "item.h"
#include "config.h"
#include "draw.h"
#include "text.h"

const char *item_name(ItemId id)
{
    static const TextId names[ITEM_COUNT] = {
        TXT_ITEM_LULLY_NAME, TXT_ITEM_HAYDN_NAME, TXT_ITEM_SATIE_NAME, TXT_ITEM_GUIDO_NAME, TXT_ITEM_SABOTS_NAME,
        TXT_ITEM_CELERITER_NAME, TXT_ITEM_ALTIUS_NAME, TXT_ITEM_EXPECTARE_NAME, TXT_ITEM_LIQUESCENCE_NAME,
        TXT_ITEM_TROPE_NAME,
    };
    return (id >= 0 && id < ITEM_COUNT) ? text_get(names[id]) : "";
}

const char *item_desc(ItemId id)
{
    static const TextId descs[ITEM_COUNT] = {
        TXT_ITEM_LULLY_DESC, TXT_ITEM_HAYDN_DESC, TXT_ITEM_SATIE_DESC, TXT_ITEM_GUIDO_DESC, TXT_ITEM_SABOTS_DESC,
        TXT_ITEM_CELERITER_DESC, TXT_ITEM_ALTIUS_DESC, TXT_ITEM_EXPECTARE_DESC, TXT_ITEM_LIQUESCENCE_DESC,
        TXT_ITEM_TROPE_DESC,
    };
    return (id >= 0 && id < ITEM_COUNT) ? text_get(descs[id]) : "";
}

void item_apply(ItemId id, PlayerStats *st)
{
    switch (id) {
    case ITEM_LULLY:
        st->damage += tuning.item_lully_damage;   /* le coup de pied est géré au tir (game.c) */
        break;
    case ITEM_HAYDN:
        st->max_hp = SDL_min(st->max_hp + tuning.item_haydn_max_hp, tuning.hp_cap);
        st->hp = SDL_min(st->hp + tuning.item_haydn_max_hp, st->max_hp);
        break;
    case ITEM_SATIE:
        st->fire_rate += tuning.item_satie_fire_rate;
        break;
    case ITEM_GUIDO:
        st->shot_speed += tuning.item_guido_shot_speed;
        st->shot_range += tuning.item_guido_shot_range;
        break;
    case ITEM_SABOTS:
        st->speed += tuning.item_sabots_speed;
        break;
    case ITEM_CELERITER:
        st->fire_rate += tuning.item_celeriter_fire_rate;
        break;
    default:   /* les autres changent la forme des tirs : lus au moment du tir (game.c) */
        break;
    }
}

/* Icônes provisoires, environ 32x32. */
void item_render(ItemId id, SDL_Renderer *r, float cx, float cy)
{
    SDL_Color ink = { 40, 26, 20, 255 };
    switch (id) {
    case ITEM_LULLY: {
        /* Grand bâton de direction, pommeau doré : Lully s'en frappa le pied. */
        draw_line(r, cx - 13, cy + 15, cx + 11, cy - 13, 6, ink);
        draw_line(r, cx - 12, cy + 14, cx + 10, cy - 12, 3.5f, (SDL_Color){ 150, 96, 50, 255 });
        draw_ellipse(r, cx + 12, cy - 14, 6, 6, ink);
        draw_ellipse(r, cx + 12, cy - 14, 4.5f, 4.5f, (SDL_Color){ 236, 190, 52, 255 });
        break;
    }
    case ITEM_HAYDN: {
        /* Crâne (celui de Haydn fut volé après sa mort). */
        SDL_Color bone = { 236, 228, 206, 255 };
        draw_ellipse(r, cx, cy - 3, 14, 13, ink);
        draw_ellipse(r, cx, cy - 3, 12.5f, 11.5f, bone);
        draw_rect(r, cx - 7, cy + 6, 14, 9, ink);
        draw_rect(r, cx - 6, cy + 6, 12, 8, bone);
        draw_ellipse(r, cx - 5, cy - 2, 3.5f, 4, ink);
        draw_ellipse(r, cx + 5, cy - 2, 3.5f, 4, ink);
        draw_rect(r, cx - 1, cy + 4, 2, 3, ink);
        for (int i = -1; i <= 1; i++) draw_rect(r, cx + i * 4 - 0.5f, cy + 10, 1, 4, ink);
        break;
    }
    case ITEM_SATIE: {
        /* Une page de partition, à jouer 840 fois. */
        draw_rect(r, cx - 12, cy - 15, 24, 30, ink);
        draw_rect(r, cx - 11, cy - 14, 22, 28, (SDL_Color){ 244, 236, 214, 255 });
        for (int k = 0; k < 2; k++) {
            for (int l = 0; l < 4; l++) draw_rect(r, cx - 9, cy - 11 + k * 13 + l * 2.5f, 18, 1, ink);
        }
        draw_rect(r, cx - 6, cy - 9, 3, 2, ink);
        draw_rect(r, cx + 2, cy + 2, 3, 2, ink);
        draw_number(r, cx - 5, cy + 9, 840, 3, 1, (SDL_Color){ 190, 40, 40, 255 });
        break;
    }
    case ITEM_GUIDO: {
        /* Main ouverte, une syllabe sur chaque phalange. */
        SDL_Color skin = { 232, 196, 160, 255 };
        draw_rect(r, cx - 9, cy - 3, 18, 17, ink);
        draw_rect(r, cx - 8, cy - 2, 16, 15, skin);
        for (int f = 0; f < 4; f++) {
            float fx = cx - 8 + f * 4.2f;
            draw_rect(r, fx - 0.5f, cy - 15 + (f == 0 || f == 3 ? 3 : 0), 4, 14, ink);
            draw_rect(r, fx, cy - 14 + (f == 0 || f == 3 ? 3 : 0), 3, 13, skin);
            draw_rect(r, fx + 1, cy - 10 + (f == 0 || f == 3 ? 3 : 0), 1, 1, (SDL_Color){ 190, 40, 40, 255 });
            draw_rect(r, fx + 1, cy - 5, 1, 1, (SDL_Color){ 190, 40, 40, 255 });
        }
        draw_line(r, cx + 8, cy + 6, cx + 15, cy - 2, 5, ink);
        draw_line(r, cx + 8, cy + 6, cx + 14, cy - 1, 3, skin);
        break;
    }
    case ITEM_SABOTS: {
        /* Deux sabots de bois. */
        SDL_Color wood = { 186, 132, 70, 255 }, dark = { 120, 80, 40, 255 };
        for (int s = 0; s < 2; s++) {
            float ox = cx - 14 + s * 15, oy = cy - 4 + s * 6;
            draw_rect(r, ox - 1, oy - 1, 15, 11, ink);
            draw_rect(r, ox, oy, 13, 9, wood);
            draw_rect(r, ox + 7, oy - 4, 7, 6, ink);
            draw_rect(r, ox + 8, oy - 3, 5, 5, wood);
            draw_rect(r, ox + 1, oy + 1, 6, 3, dark);
        }
        break;
    }
    case ITEM_CELERITER:
    case ITEM_ALTIUS:
    case ITEM_EXPECTARE: {
        /* Lettre significative : une lettre rouge sur un bout de parchemin. */
        static const char letters[] = { 'c', 'a', 'x' };
        char s[2] = { letters[id - ITEM_CELERITER], 0 };
        draw_rect(r, cx - 13, cy - 14, 26, 28, ink);
        draw_rect(r, cx - 12, cy - 13, 24, 26, (SDL_Color){ 244, 236, 214, 255 });
        draw_debug_text(r, cx - 12, cy - 13, s, 3.0f, (SDL_Color){ 190, 40, 40, 255 });
        break;
    }
    case ITEM_LIQUESCENCE: {
        /* Goutte d'encre qui fond. */
        SDL_Color blue = { 30, 34, 70, 255 };
        draw_ellipse(r, cx, cy + 5, 10, 10, ink);
        draw_ellipse(r, cx, cy + 5, 8.5f, 8.5f, blue);
        for (int k = 0; k < 6; k++) draw_rect(r, cx - (float)k * 1.2f, cy - 12 + k * 2.5f, 2.4f * (float)k + 1, 3, blue);
        draw_ellipse(r, cx + 9, cy + 15, 7, 2.5f, blue);
        draw_rect(r, cx - 4, cy + 1, 3, 3, (SDL_Color){ 120, 130, 190, 255 });
        break;
    }
    case ITEM_TROPE: {
        /* Une croche et un « + ». */
        SDL_Color pink = { 255, 70, 168, 255 };
        draw_rect(r, cx - 10, cy + 4, 12, 9, ink);
        draw_rect(r, cx - 9, cy + 5, 10, 7, pink);
        draw_rect(r, cx, cy - 13, 2, 18, ink);
        draw_rect(r, cx + 2, cy - 13, 4, 3, ink);
        draw_rect(r, cx + 5, cy - 11, 2, 4, ink);
        draw_rect(r, cx + 7, cy - 4, 10, 4, (SDL_Color){ 190, 40, 40, 255 });
        draw_rect(r, cx + 10, cy - 7, 4, 10, (SDL_Color){ 190, 40, 40, 255 });
        break;
    }
    default:
        break;
    }
}
