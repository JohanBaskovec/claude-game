#include "menu.h"

#include "config.h"
#include "draw.h"
#include "text.h"
#include "ui_text.h"

#include <math.h>

#define BUTTON_W   260.0f
#define BUTTON_H   54.0f
#define BUTTON_Y   300.0f   /* haut du premier bouton */
#define BUTTON_GAP 72.0f    /* d'un bouton au suivant */
#define TITLE_CY   160.0f

/* Le navigateur ne peut pas fermer l'onglet : pas de bouton Quitter. */
#ifdef SDL_PLATFORM_EMSCRIPTEN
static const MenuChoice BUTTONS[] = { MENU_PLAY };
static const TextId     LABELS[]  = { TXT_MENU_PLAY };
#else
static const MenuChoice BUTTONS[] = { MENU_PLAY, MENU_QUIT };
static const TextId     LABELS[]  = { TXT_MENU_PLAY, TXT_MENU_QUIT };
#endif
#define BUTTON_COUNT ((int)SDL_arraysize(BUTTONS))

static SDL_FRect button_rect(int i)
{
    return (SDL_FRect){ (GAME_W - BUTTON_W) * 0.5f, BUTTON_Y + BUTTON_GAP * (float)i, BUTTON_W, BUTTON_H };
}

static int button_at(float x, float y)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        SDL_FRect b = button_rect(i);
        if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) return i;
    }
    return -1;
}

void menu_init(Menu *m)
{
    *m = (Menu){ 0 };
}

void menu_update(Menu *m, float dt)
{
    m->time += dt;
}

MenuChoice menu_handle_event(Menu *m, const SDL_Event *e)
{
    switch (e->type) {
    case SDL_EVENT_MOUSE_MOTION: {
        int i = button_at(e->motion.x, e->motion.y);
        if (i >= 0) m->selected = i;
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (e->button.button == SDL_BUTTON_LEFT) {
            int i = button_at(e->button.x, e->button.y);
            if (i >= 0) return BUTTONS[i];
        }
        break;
    case SDL_EVENT_KEY_DOWN:
        switch (e->key.scancode) {
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W:   /* Z en AZERTY */
            m->selected = (m->selected + BUTTON_COUNT - 1) % BUTTON_COUNT;
            break;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S:
            m->selected = (m->selected + 1) % BUTTON_COUNT;
            break;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
        case SDL_SCANCODE_SPACE:
            if (!e->key.repeat) return BUTTONS[m->selected];
            break;
#ifndef SDL_PLATFORM_EMSCRIPTEN
        case SDL_SCANCODE_ESCAPE:
            if (!e->key.repeat) return MENU_QUIT;
            break;
#endif
        default:
            break;
        }
        break;
    default:
        break;
    }
    return MENU_NONE;
}

/* Petit carré de neume (punctum) penché, en marque de sélection. */
static void punctum(SDL_Renderer *r, float cx, float cy, SDL_Color c)
{
    for (int i = -5; i <= 5; i++) {
        float h = 5.0f - SDL_fabsf((float)i);
        draw_rect(r, cx + (float)i, cy - h - (float)i * 0.3f, 1.0f, h * 2.0f + 1.0f, c);
    }
}

static void render_button(SDL_Renderer *r, int i, bool selected, float time)
{
    SDL_FRect b = button_rect(i);
    SDL_Color ink    = { 58, 38, 28, 255 };
    SDL_Color rubric = { 168, 34, 26, 255 };
    SDL_Color edge   = selected ? rubric : ink;

    draw_rect(r, b.x + 4, b.y + 5, b.w, b.h, (SDL_Color){ 0, 0, 0, 110 });                 /* ombre */
    draw_rect(r, b.x, b.y, b.w, b.h, edge);                                                  /* cadre */
    draw_rect(r, b.x + 3, b.y + 3, b.w - 6, b.h - 6, (SDL_Color){ 234, 220, 188, 255 });   /* parchemin */
    draw_rect(r, b.x + 6, b.y + 6, b.w - 12, 1, (SDL_Color){ edge.r, edge.g, edge.b, 90 });          /* réglure */
    draw_rect(r, b.x + 6, b.y + b.h - 7, b.w - 12, 1, (SDL_Color){ edge.r, edge.g, edge.b, 90 });

    const char *label = text_get(LABELS[i]);
    float top = b.y + (b.h - font_line_height(FONT_UI)) * 0.5f + 1.0f;
    ui_text_draw(r, FONT_UI, label, b.x + b.w * 0.5f, top, 0.5f, selected ? rubric : ink);

    if (selected) {
        float w = ui_text_width(FONT_UI, label), bob = 2.0f * sinf(time * 4.0f);
        float cy = b.y + b.h * 0.5f;
        punctum(r, b.x + b.w * 0.5f - w * 0.5f - 22.0f - bob, cy, rubric);
        punctum(r, b.x + b.w * 0.5f + w * 0.5f + 22.0f + bob, cy, rubric);
    }
}

void menu_render(const Menu *m, SDL_Renderer *r)
{
    /* voile sur la salle de fond */
    draw_rect(r, 0, 0, GAME_W, GAME_H, (SDL_Color){ 10, 6, 6, 150 });

    /* titre, lettres d'or ombrées, filets dessous */
    const char *title = text_get(TXT_GAME_TITLE);
    float lh = font_line_height(FONT_TITLE), top = TITLE_CY - lh * 0.5f;
    ui_text_draw(r, FONT_TITLE, title, GAME_W * 0.5f + 3.0f, top + 3.0f, 0.5f, (SDL_Color){ 0, 0, 0, 170 });
    ui_text_draw(r, FONT_TITLE, title, GAME_W * 0.5f, top, 0.5f, (SDL_Color){ 240, 200, 100, 255 });
    float w = ui_text_width(FONT_TITLE, title), ly = TITLE_CY + lh * 0.5f + 6.0f;
    SDL_Color gold = { 210, 168, 80, 230 };
    draw_rect(r, (GAME_W - w) * 0.5f, ly, w, 2.0f, gold);
    draw_rect(r, (GAME_W - w * 0.6f) * 0.5f, ly + 6.0f, w * 0.6f, 1.0f, gold);
    draw_ellipse(r, GAME_W * 0.5f, ly + 1.0f, 4.0f, 4.0f, gold);

    for (int i = 0; i < BUTTON_COUNT; i++) render_button(r, i, i == m->selected, m->time);
}
