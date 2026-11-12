#include "obstacle.h"
#include "config.h"
#include "draw.h"
#include "sprite.h"

#include <math.h>

/* Le sol (bas de l'objet) est à 8 px du bas de la case de la planche. */
#define SPRITE_FEET 8.0f

int needle_stage(const Obstacle *o, float time)
{
    float cycle = SDL_max(tuning.needle_cycle, 0.1f);
    float t = SDL_fmodf(time + (float)o->variant * cycle / 3.0f, cycle);
    if (t >= cycle - tuning.needle_out) return 2;
    if (t >= cycle - tuning.needle_out - tuning.needle_warn) return 1;
    return 0;
}

bool obstacle_flat(int type)
{
    return type == OBST_NEEDLES;
}

void obstacle_center(const RoomLayout *L, const Obstacle *o, float *x, float *y)
{
    SDL_FRect t = layout_tile_rect(L, o->tx, o->ty);
    *x = t.x + t.w * 0.5f;
    *y = t.y + t.h * 0.5f;
}

float obstacle_feet_y(const RoomLayout *L, const Obstacle *o)
{
    SDL_FRect b = obstacle_rect(L, o);
    return b.y + b.h;
}

static bool ribbon_at(const RoomLayout *L, int tx, int ty, int type)
{
    for (int i = 0; i < L->obstacle_count; i++) {
        const Obstacle *o = &L->obstacles[i];
        if (o->alive && o->type == type && o->tx == tx && o->ty == ty) return true;
    }
    return false;
}

/* Signet de soie verte posé sur la page, avec une queue fourchue aux deux bouts. */
static void render_ribbon(const RoomLayout *L, const Obstacle *o, SDL_Renderer *r, Camera cam)
{
    SDL_FRect t = layout_tile_rect(L, o->tx, o->ty);
    float x = t.x - cam.x, y = t.y - cam.y;
    SDL_Color edge = { 18, 54, 34, 255 }, silk = { 44, 118, 74, 255 }, shine = { 96, 170, 120, 255 };
    bool up = ribbon_at(L, o->tx, o->ty - 1, OBST_RIBBON), down = ribbon_at(L, o->tx, o->ty + 1, OBST_RIBBON);
    if (up || down) {
        float cx = x + t.w * 0.5f;
        float y0 = up ? y : y + 6.0f, y1 = down ? y + t.h : y + t.h - 6.0f;
        draw_rect(r, cx - 4.0f, y0 + 4.0f, 12.0f, y1 - y0, (SDL_Color){ 60, 40, 30, 70 });   /* ombre */
        draw_rect(r, cx - 6.0f, y0, 12.0f, y1 - y0, edge);
        draw_rect(r, cx - 5.0f, y0, 10.0f, y1 - y0, silk);
        draw_rect(r, cx - 3.0f, y0, 2.0f, y1 - y0, shine);
        return;
    }
    bool left = ribbon_at(L, o->tx - 1, o->ty, OBST_RIBBON), right = ribbon_at(L, o->tx + 1, o->ty, OBST_RIBBON);
    float cy = y + t.h * 0.5f;
    float x0 = left ? x : x + 4.0f, x1 = right ? x + t.w : x + t.w - 4.0f;
    draw_rect(r, x0 + 2.0f, cy + 3.0f, x1 - x0, 6.0f, (SDL_Color){ 60, 40, 30, 70 });   /* ombre */
    draw_rect(r, x0, cy - 6.0f, x1 - x0, 12.0f, edge);
    draw_rect(r, x0, cy - 5.0f, x1 - x0, 10.0f, silk);
    draw_rect(r, x0, cy - 3.0f, x1 - x0, 2.0f, shine);
    /* Bouts coupés en queue d'aronde. */
    if (!left) draw_rect(r, x0, cy - 1.0f, 4.0f, 2.0f, edge);
    if (!right) draw_rect(r, x1 - 4.0f, cy - 1.0f, 4.0f, 2.0f, edge);
}

/* Gros fils de couture tendus le long du pli de la reliure, noués aux deux bouts. */
static void render_thread(const RoomLayout *L, const Obstacle *o, SDL_Renderer *r, Camera cam)
{
    SDL_FRect t = layout_tile_rect(L, o->tx, o->ty);
    bool up = ribbon_at(L, o->tx, o->ty - 1, OBST_THREAD), down = ribbon_at(L, o->tx, o->ty + 1, OBST_THREAD);
    float x = t.x - cam.x, y0 = t.y - cam.y + (up ? 0.0f : 4.0f), y1 = t.y - cam.y + t.h - (down ? 0.0f : 4.0f);
    SDL_Color light = { 232, 216, 176, 255 }, mid = { 190, 168, 124, 255 }, dark = { 96, 76, 52, 255 };
    draw_rect(r, x + 1.0f, y0 + 3.0f, 6.0f, y1 - y0, (SDL_Color){ 40, 28, 20, 80 });   /* ombre */
    for (float yy = y0; yy < y1; yy += 1.0f) {
        /* Deux fils torsadés : la torsade descend en diagonale. */
        int ph = (int)(yy + t.y) % 6;
        draw_rect(r, x - 4.0f, yy, 8.0f, 1.0f, dark);
        draw_rect(r, x - 3.0f + (float)(ph < 3 ? ph : 5 - ph), yy, 4.0f, 1.0f, ph < 3 ? light : mid);
    }
    if (!up) draw_ellipse(r, x, y0, 5.0f, 3.0f, mid);
    if (!down) draw_ellipse(r, x, y1, 5.0f, 3.0f, mid);
}

void obstacle_render(const RoomLayout *L, const Obstacle *o, SDL_Renderer *r, Camera cam, float time)
{
    if (!o->alive) return;
    if (o->type == OBST_RIBBON) {
        render_ribbon(L, o, r, cam);
        return;
    }
    if (o->type == OBST_THREAD) {
        render_thread(L, o, r, cam);
        return;
    }
    float cx, cy;
    obstacle_center(L, o, &cx, &cy);
    /* Le bas de l'objet sur la planche est posé sur le bas de sa boîte de collision (le sol). */
    SDL_FRect b = obstacle_rect(L, o);
    float ground = obstacle_flat(o->type) ? cy : b.y + b.h - 2.0f;
    float sx = SDL_floorf(cx - cam.x - 24.0f), sy = SDL_floorf(ground - cam.y + SPRITE_FEET - 64.0f);
    SpriteId id;
    switch (o->type) {
    case OBST_BOOKS:    id = (o->variant & 1) ? SPR_BOOKS_1 : SPR_BOOKS_0; break;
    case OBST_INKWELL:  id = SPR_INKWELL; break;
    case OBST_CANDLE:   id = ((int)(time * 7.0f + (float)(o->tx * 3 + o->ty)) & 1) ? SPR_CANDLE_1 : SPR_CANDLE_0; break;
    default: {
        int st = needle_stage(o, time);
        id = st == 2 ? SPR_NEEDLES_OUT : st == 1 ? SPR_NEEDLES_TIPS : SPR_NEEDLES_IN;
        break;
    }
    }
    if (!obstacle_flat(o->type)) {
        /* Ombre collée sous l'objet : la moitié du haut est cachée derrière lui. */
        draw_ellipse(r, cx - cam.x + 2.0f, SDL_floorf(ground - cam.y) + 1.0f, b.w * 0.62f, 5.0f, (SDL_Color){ 60, 40, 30, 120 });
    }
    sprite_draw(r, id, sx, sy, (o->variant & 2) && o->type == OBST_BOOKS, false);
}
