#include "hud.h"
#include "config.h"

#include <math.h>

/* Position du soufflet : en haut à gauche, sur le mur du haut. */
#define BAR_X       24.0f
#define BAR_CY      44.0f
#define BAR_OPEN_H  28.0f   /* ouverture du soufflet, côté droit */
#define BAR_LINE_W  3.0f    /* épaisseur des deux traits */

void hud_init(Hud *hud, const PlayerStats *stats)
{
    hud->shown_hp = (float)stats->hp;
}

void hud_update(Hud *hud, const PlayerStats *stats, float dt)
{
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
}
