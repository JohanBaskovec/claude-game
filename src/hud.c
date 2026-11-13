#include "hud.h"
#include "sprite.h"
#include "config.h"
#include "draw.h"
#include "weapon.h"

#include <math.h>

/* Position du soufflet : en haut à gauche, sur le mur du haut. */
#define BAR_X       24.0f
#define BAR_CY      44.0f
#define BAR_OPEN_H  28.0f   /* ouverture du soufflet, côté droit */
#define BAR_LINE_W  3.0f    /* épaisseur des deux traits */

/* Compteurs sous la barre de vie. */
#define COUNT_X     36.0f
#define COUNT_Y     76.0f
#define COUNT_STEP  22.0f

void hud_init(Hud *hud, const PlayerStats *stats)
{
    hud->shown_hp = (float)stats->hp;
    hud->banner_title = hud->banner_text = NULL;
    hud->banner_timer = 0.0f;
}

void hud_show_banner(Hud *hud, const char *title, const char *text)
{
    hud->banner_title = title;
    hud->banner_text = text;
    hud->banner_timer = tuning.item_banner_time;
}

void hud_update(Hud *hud, const PlayerStats *stats, float dt)
{
    if (hud->banner_timer > 0.0f) hud->banner_timer -= dt;
    float hp = (float)stats->hp;
    if (hud->shown_hp <= hp) {
        hud->shown_hp = hp;   /* un soin s'affiche tout de suite */
    } else {
        float tc = tuning.hud_bar_drain_time;
        hud->shown_hp = (tc <= 0.0f) ? hp : hud->shown_hp + (hp - hud->shown_hp) * (1.0f - expf(-dt / tc));
        if (hud->shown_hp - hp < 0.01f) {
            hud->shown_hp = hp;
        }
    }
}

static SDL_FColor rgba(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    return (SDL_FColor){ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
}

static void triangle(SDL_Renderer *r, float x1, float y1, float x2, float y2, float x3, float y3, SDL_FColor c)
{
    SDL_Vertex v[3] = {
        { { x1, y1 }, c, { 0.0f, 0.0f } },
        { { x2, y2 }, c, { 0.0f, 0.0f } },
        { { x3, y3 }, c, { 0.0f, 0.0f } },
    };
    SDL_RenderGeometry(r, NULL, v, 3, NULL, 0);
}

/* Trait épais de A à B (un quadrilatère = deux triangles). */
static void thick_line(SDL_Renderer *r, float ax, float ay, float bx, float by, float width, SDL_FColor c)
{
    float dx = bx - ax, dy = by - ay;
    float len = sqrtf(dx * dx + dy * dy);
    if (len <= 0.0f) return;
    float nx = -dy / len * width * 0.5f, ny = dx / len * width * 0.5f;
    triangle(r, ax + nx, ay + ny, bx + nx, by + ny, bx - nx, by - ny, c);
    triangle(r, ax + nx, ay + ny, bx - nx, by - ny, ax - nx, ay - ny, c);
}

/* Partie du soufflet entre la pointe et la fraction f (0..1) de sa longueur. */
static void wedge(SDL_Renderer *r, float len, float f, SDL_FColor c)
{
    if (f <= 0.0f) return;
    float x = BAR_X + len * f;
    float h = BAR_OPEN_H * 0.5f * f;
    triangle(r, BAR_X, BAR_CY, x, BAR_CY - h, x, BAR_CY + h, c);
}

void hud_render(const Hud *hud, SDL_Renderer *r, const PlayerStats *stats)
{
    float max_hp = (float)SDL_max(stats->max_hp, 1);
    float len = tuning.hud_bar_base_len + tuning.hud_bar_len_per_hp * max_hp;
    float f_hp = SDL_clamp((float)stats->hp / max_hp, 0.0f, 1.0f);
    float f_ghost = SDL_clamp(hud->shown_hp / max_hp, 0.0f, 1.0f);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    wedge(r, len, 1.0f, rgba(18, 14, 12, 200));          /* intérieur vide */
    wedge(r, len, f_ghost, rgba(240, 222, 196, 255));    /* PV qui viennent d'être perdus */
    wedge(r, len, f_hp, rgba(204, 52, 44, 255));         /* PV actuels, encre rouge */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    /* Les deux traits du soufflet. */
    SDL_FColor line = rgba(236, 226, 204, 255);
    float end = BAR_X + len;
    thick_line(r, BAR_X, BAR_CY, end, BAR_CY - BAR_OPEN_H * 0.5f, BAR_LINE_W, line);
    thick_line(r, BAR_X, BAR_CY, end, BAR_CY + BAR_OPEN_H * 0.5f, BAR_LINE_W, line);

    /* Royalties (couronne) puis métronomes, comme pièces et bombes dans TBOI. */
    SDL_Color text = { 236, 226, 204, 255 };
    draw_crown(r, COUNT_X, COUNT_Y, 1.0f);
    draw_number(r, COUNT_X + 14.0f, COUNT_Y - 5.0f, stats->coins, 2, 2.0f, text);
    draw_metronome(r, COUNT_X, COUNT_Y + COUNT_STEP, 1.0f, 0.35f);
    draw_number(r, COUNT_X + 14.0f, COUNT_Y + COUNT_STEP - 5.0f, stats->bombs, 2, 2.0f, text);
    /* Clefs de sol, de fa, d'ut (GDD §22). */
    for (int k = 0; k < KEY_COUNT; k++) {
        float y = COUNT_Y + COUNT_STEP * (float)(2 + k);
        sprite_draw(r, (SpriteId)(SPR_KEY_SOL_SMALL + k), COUNT_X - 26.0f, y - 24.0f, false, false);
        draw_number(r, COUNT_X + 14.0f, y - 5.0f, stats->keys[k], 2, 2.0f, text);
    }

    /* Arme tenue, dans un cadre à côté des compteurs. */
    draw_rect(r, 86, 64, 40, 40, (SDL_Color){ 236, 226, 204, 255 });
    draw_rect(r, 88, 66, 36, 36, (SDL_Color){ 196, 184, 160, 255 });   /* fond parchemin, comme le sol */
    weapon_render_icon((WeaponId)stats->weapon, r, 106, 84, 0.95f);

    /* Bandeau de l'objet ramassé, qui s'efface à la fin. */
    if (hud->banner_timer > 0.0f && hud->banner_title) {
        float a = SDL_clamp(hud->banner_timer / 0.4f, 0.0f, 1.0f);
        float y = 150.0f;
        draw_rect(r, 0, y - 10, GAME_W, 58, (SDL_Color){ 14, 10, 10, (Uint8)(190 * a) });
        float tw = draw_debug_text_width(hud->banner_title, 3.0f);
        draw_debug_text(r, SDL_floorf((GAME_W - tw) * 0.5f), y, hud->banner_title, 3.0f,
                        (SDL_Color){ 255, 220, 120, (Uint8)(255 * a) });
        if (hud->banner_text) {
            float dw = draw_debug_text_width(hud->banner_text, 1.0f);
            draw_debug_text(r, SDL_floorf((GAME_W - dw) * 0.5f), y + 32, hud->banner_text, 1.0f,
                            (SDL_Color){ 236, 226, 204, (Uint8)(255 * a) });
        }
    }
}

void hud_render_boss(SDL_Renderer *r, float hp, float max_hp)
{
    const float w = 360.0f, h = 12.0f, x = (GAME_W - w) * 0.5f, y = GAME_H - 34.0f;
    float f = SDL_clamp(hp / SDL_max(max_hp, 1.0f), 0.0f, 1.0f);
    draw_rect(r, x - 3, y - 3, w + 6, h + 6, (SDL_Color){ 236, 226, 204, 255 });
    draw_rect(r, x, y, w, h, (SDL_Color){ 18, 14, 12, 255 });
    draw_rect(r, x, y, w * f, h, (SDL_Color){ 204, 52, 44, 255 });
    draw_rect(r, x, y, w * f, 3, (SDL_Color){ 236, 110, 90, 255 });
}
