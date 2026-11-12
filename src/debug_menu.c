#include "debug_menu.h"
#include "config.h"
#include "minimap.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

/* La police de debug de SDL est en ASCII seulement : pas d'accents ici. */

typedef enum StatKind { STAT_INT, STAT_FLOAT } StatKind;

/* Bornes qui dépendent d'autres valeurs. */
#define MAX_IS_MAX_HP  -1.0f   /* PV actuels <= PV maximum */
#define MAX_IS_HP_CAP  -2.0f   /* PV maximum <= tuning.hp_cap */

typedef struct StatRow {
    const char *label;
    const char *unit;
    const char *const *names;   /* valeur affichée par son nom (liste), ou NULL */
    StatKind    kind;
    size_t      offset;    /* position du champ dans PlayerStats */
    float       step;      /* pas d'un clic */
    float       min, max;  /* max : valeur fixe ou MAX_IS_* */
    int         decimals;
} StatRow;

static const char *const ENEMY_NAMES[ENEMY_TYPE_COUNT] = { "Bebe", "Gros Bebe", "Moine", "Plume", "Ver", "Sonneur" };

static const char *const WEAPON_NAMES[WEAPON_COUNT] = { "Voix", "Olifant", "Flute", "Lyre", "Tabourin", "Cloche", "Orgue" };

static const StatRow ROWS[] = {
    { "Points de vie",    "PV", NULL,      STAT_INT,   offsetof(PlayerStats, hp),         1.0f,  0.0f,  MAX_IS_MAX_HP, 0 },
    { "PV maximum",       "PV", NULL,      STAT_INT,   offsetof(PlayerStats, max_hp),     1.0f,  1.0f,  MAX_IS_HP_CAP, 0 },
    { "Vitesse",          "px/s", NULL,    STAT_FLOAT, offsetof(PlayerStats, speed),      10.0f, 30.0f, 1000.0f, 0 },
    { "Cadence de tir",   "notes/s", NULL, STAT_FLOAT, offsetof(PlayerStats, fire_rate),  0.25f, 0.25f, 30.0f,   2 },
    { "Degats",           "PV", NULL,      STAT_FLOAT, offsetof(PlayerStats, damage),     0.5f,  0.5f,  50.0f,   1 },
    { "Vitesse des tirs", "px/s", NULL,    STAT_FLOAT, offsetof(PlayerStats, shot_speed), 20.0f, 60.0f, 2000.0f, 0 },
    { "Portee des tirs",  "px", NULL,      STAT_FLOAT, offsetof(PlayerStats, shot_range), 20.0f, 20.0f, 2000.0f, 0 },
    { "Metronomes",       "", NULL,        STAT_INT,   offsetof(PlayerStats, bombs),      1.0f,  0.0f,  99.0f,   0 },
    { "Royalties",        "", NULL,        STAT_INT,   offsetof(PlayerStats, coins),      1.0f,  0.0f,  99.0f,   0 },
    { "Arme",             "", WEAPON_NAMES, STAT_INT,  offsetof(PlayerStats, weapon),     1.0f,  0.0f,  WEAPON_COUNT - 1, 0 },
};
#define ROW_COUNT ((int)(sizeof(ROWS) / sizeof(ROWS[0])))

/* Identifiants des boutons : ligne*2 = moins, ligne*2+1 = plus, puis les boutons d'action. */
#define BTN_RESET (ROW_COUNT * 2)
#define BTN_BABY  (ROW_COUNT * 2 + 1)
#define BTN_FLOOR (ROW_COUNT * 2 + 2)
#define BTN_KILL  (ROW_COUNT * 2 + 3)
#define BTN_ITEM  (ROW_COUNT * 2 + 4)
#define BTN_ENEMY_PREV (ROW_COUNT * 2 + 5)   /* ligne « Ennemi » : - et + */
#define BTN_ENEMY_NEXT (ROW_COUNT * 2 + 6)
#define BTN_COUNT (ROW_COUNT * 2 + 7)

/* Mise en page, en pixels du jeu. La police de debug fait 8x8 px. */
#define GLYPH     8.0f
#define PANEL_W   308.0f
#define PANEL_X   (GAME_W - PANEL_W - 8.0f)
#define PANEL_Y   (minimap_bottom() + 10.0f)   /* sous la mini-carte, qui reste cliquable */
#define ROWS_Y    (PANEL_Y + 30.0f)
#define ROW_H     24.0f
#define BTN_W     20.0f
#define BTN_H     18.0f
#define MINUS_X   (PANEL_X + 256.0f)
#define PLUS_X    (PANEL_X + 280.0f)
#define VALUE_END (PANEL_X + 248.0f)
#define RESET_Y   (ROWS_Y + ROW_COUNT * ROW_H + 6.0f)
#define RESET_W   142.0f
#define BABY_W    142.0f
#define ACTION2_Y (RESET_Y + BTN_H + 6.0f)
#define ACTION3_Y (ACTION2_Y + BTN_H + 6.0f)
#define ENEMY_Y   (ACTION3_Y + BTN_H + 12.0f)
#define PANEL_H   (ENEMY_Y + BTN_H + 4.0f - PANEL_Y)

#define REPEAT_DELAY 0.40f  /* s avant la répétition automatique */
#define REPEAT_EVERY 0.06f  /* s entre deux répétitions */

static const char *RESET_LABEL = "Valeurs de depart";
static const char *BABY_LABEL  = "Ajouter";
static const char *FLOOR_LABEL = "Etage suivant";
static const char *KILL_LABEL  = "Tuer les ennemis";
static const char *ITEM_LABEL  = "Donner un objet";

static SDL_FRect button_rect(int id)
{
    if (id == BTN_RESET) {
        return (SDL_FRect){ PANEL_X + 10.0f, RESET_Y, RESET_W, BTN_H };
    }
    if (id == BTN_BABY) {
        return (SDL_FRect){ PANEL_X + PANEL_W - 10.0f - BABY_W, RESET_Y, BABY_W, BTN_H };
    }
    if (id == BTN_FLOOR) {
        return (SDL_FRect){ PANEL_X + 10.0f, ACTION2_Y, RESET_W, BTN_H };
    }
    if (id == BTN_KILL) {
        return (SDL_FRect){ PANEL_X + PANEL_W - 10.0f - BABY_W, ACTION2_Y, BABY_W, BTN_H };
    }
    if (id == BTN_ITEM) {
        return (SDL_FRect){ PANEL_X + 10.0f, ACTION3_Y, RESET_W, BTN_H };
    }
    if (id == BTN_ENEMY_PREV || id == BTN_ENEMY_NEXT) {
        return (SDL_FRect){ id == BTN_ENEMY_PREV ? MINUS_X : PLUS_X, ENEMY_Y - 5.0f, BTN_W, BTN_H };
    }
    int row = id / 2;
    float y = ROWS_Y + (float)row * ROW_H;
    float x = (id % 2 == 0) ? MINUS_X : PLUS_X;
    return (SDL_FRect){ x, y - 5.0f, BTN_W, BTN_H };
}

static int hit_test(float x, float y)
{
    SDL_FPoint pt = { x, y };
    for (int id = 0; id < BTN_COUNT; id++) {
        SDL_FRect r = button_rect(id);
        if (SDL_PointInRectFloat(&pt, &r)) {
            return id;
        }
    }
    return -1;
}

static float row_max(const StatRow *row, const PlayerStats *st)
{
    if (row->max == MAX_IS_MAX_HP) return (float)st->max_hp;
    if (row->max == MAX_IS_HP_CAP) return (float)tuning.hp_cap;
    return row->max;
}

static float get_value(const StatRow *row, const PlayerStats *st)
{
    const char *base = (const char *)st + row->offset;
    if (row->kind == STAT_INT) {
        int v;
        memcpy(&v, base, sizeof v);
        return (float)v;
    }
    float v;
    memcpy(&v, base, sizeof v);
    return v;
}

static void set_value(const StatRow *row, PlayerStats *st, float v)
{
    char *base = (char *)st + row->offset;
    if (row->kind == STAT_INT) {
        int i = (int)lroundf(v);
        memcpy(base, &i, sizeof i);
    } else {
        memcpy(base, &v, sizeof v);
    }
}

/* Vrai si le bouton peut encore agir (la valeur n'est pas déjà à la limite). */
static bool button_enabled(int id, const PlayerStats *st)
{
    if (id >= BTN_RESET) {
        return true;
    }
    const StatRow *row = &ROWS[id / 2];
    float v = get_value(row, st);
    return (id % 2 == 0) ? v > row->min : v < row_max(row, st);
}

static DebugAction press(DebugMenu *m, int id, PlayerStats *st)
{
    if (id == BTN_ENEMY_PREV || id == BTN_ENEMY_NEXT) {
        int d = id == BTN_ENEMY_NEXT ? 1 : ENEMY_TYPE_COUNT - 1;
        m->enemy = (m->enemy + d) % ENEMY_TYPE_COUNT;
        return DEBUG_ACTION_NONE;
    }
    if (id == BTN_RESET) {
        *st = player_start_stats();
        return DEBUG_ACTION_NONE;
    }
    if (id == BTN_BABY)  return DEBUG_ACTION_SPAWN_ENEMY;
    if (id == BTN_FLOOR) return DEBUG_ACTION_NEW_FLOOR;
    if (id == BTN_KILL)  return DEBUG_ACTION_KILL_ENEMIES;
    if (id == BTN_ITEM)  return DEBUG_ACTION_GIVE_ITEM;
    const StatRow *row = &ROWS[id / 2];
    float v = get_value(row, st) + ((id % 2 == 0) ? -row->step : row->step);
    v = roundf(v / row->step) * row->step;   /* évite la dérive des flottants */
    v = SDL_clamp(v, row->min, row_max(row, st));
    set_value(row, st, v);
    if (st->hp > st->max_hp) {
        st->hp = st->max_hp;   /* baisser le maximum peut retirer des PV */
    }
    return DEBUG_ACTION_NONE;
}

void debug_menu_init(DebugMenu *m)
{
    m->open = false;
    m->hover = -1;
    m->pressed = -1;
    m->repeat_timer = 0.0f;
    m->enemy = ENEMY_BABY;
}

void debug_menu_toggle(DebugMenu *m)
{
    m->open = !m->open;
    m->hover = -1;
    m->pressed = -1;
}

DebugAction debug_menu_handle_event(DebugMenu *m, const SDL_Event *event, PlayerStats *stats)
{
    DebugAction action = DEBUG_ACTION_NONE;
    if (!m->open) {
        return action;
    }
    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        m->hover = hit_test(event->motion.x, event->motion.y);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            m->hover = hit_test(event->button.x, event->button.y);
            if (m->hover >= 0) {
                action = press(m, m->hover, stats);
                m->pressed = m->hover;
                m->repeat_timer = REPEAT_DELAY;
            }
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            m->pressed = -1;
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        m->pressed = -1;
        break;
    default:
        break;
    }
    return action;
}

void debug_menu_update(DebugMenu *m, float dt, PlayerStats *stats)
{
    /* Répétition seulement pour - et +, et tant que la souris reste sur le bouton. */
    if (!m->open || m->pressed < 0 || m->pressed >= BTN_RESET || m->pressed != m->hover) {
        return;
    }
    m->repeat_timer -= dt;
    while (m->repeat_timer <= 0.0f) {
        press(m, m->pressed, stats);
        m->repeat_timer += REPEAT_EVERY;
    }
}

static void draw_text(SDL_Renderer *r, float x, float y, const char *s, Uint8 c)
{
    SDL_SetRenderDrawColor(r, c, c, c, 255);
    SDL_RenderDebugText(r, x, y, s);
}

static void draw_button(SDL_Renderer *r, const DebugMenu *m, int id, const char *label, bool enabled)
{
    SDL_FRect rect = button_rect(id);
    if (!enabled) {
        SDL_SetRenderDrawColor(r, 45, 42, 48, 255);
    } else if (m->pressed == id && m->hover == id) {
        SDL_SetRenderDrawColor(r, 255, 70, 168, 255);
    } else if (m->hover == id) {
        SDL_SetRenderDrawColor(r, 105, 100, 120, 255);
    } else {
        SDL_SetRenderDrawColor(r, 70, 66, 80, 255);
    }
    SDL_RenderFillRect(r, &rect);

    float tw = (float)strlen(label) * GLYPH;
    draw_text(r, rect.x + SDL_floorf((rect.w - tw) * 0.5f), rect.y + 5.0f, label, enabled ? 240 : 110);
}

bool debug_menu_contains(const DebugMenu *m, float x, float y)
{
    SDL_FRect panel = { PANEL_X, PANEL_Y, PANEL_W, PANEL_H };
    SDL_FPoint pt = { x, y };
    return m->open && SDL_PointInRectFloat(&pt, &panel);
}

void debug_menu_render(const DebugMenu *m, SDL_Renderer *r, const PlayerStats *stats)
{
    if (!m->open) {
        return;
    }

    SDL_FRect panel = { PANEL_X, PANEL_Y, PANEL_W, PANEL_H };
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 12, 10, 16, 215);
    SDL_RenderFillRect(r, &panel);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(r, 255, 70, 168, 255);
    SDL_RenderRect(r, &panel);

    SDL_SetRenderDrawColor(r, 255, 70, 168, 255);
    SDL_RenderDebugText(r, PANEL_X + 10.0f, PANEL_Y + 10.0f, "ATTRIBUTS DU JOUEUR  (F2)");

    for (int i = 0; i < ROW_COUNT; i++) {
        const StatRow *row = &ROWS[i];
        float y = ROWS_Y + (float)i * ROW_H;

        draw_text(r, PANEL_X + 10.0f, y, row->label, 200);

        char value[32];
        if (row->names) {
            SDL_snprintf(value, sizeof value, "%s", row->names[(int)get_value(row, stats)]);
        } else {
            SDL_snprintf(value, sizeof value, "%.*f%s%s", row->decimals, (double)get_value(row, stats),
                         row->unit[0] ? " " : "", row->unit);
        }
        draw_text(r, VALUE_END - (float)strlen(value) * GLYPH, y, value, 255);

        draw_button(r, m, i * 2,     "-", button_enabled(i * 2, stats));
        draw_button(r, m, i * 2 + 1, "+", button_enabled(i * 2 + 1, stats));
    }

    draw_button(r, m, BTN_RESET, RESET_LABEL, true);
    draw_button(r, m, BTN_BABY, BABY_LABEL, true);
    draw_button(r, m, BTN_FLOOR, FLOOR_LABEL, true);
    draw_button(r, m, BTN_KILL, KILL_LABEL, true);
    draw_button(r, m, BTN_ITEM, ITEM_LABEL, true);

    /* Ennemi que fait apparaître « Ajouter ». */
    draw_text(r, PANEL_X + 10.0f, ENEMY_Y, "Ennemi", 200);
    const char *name = ENEMY_NAMES[m->enemy];
    draw_text(r, VALUE_END - (float)strlen(name) * GLYPH, ENEMY_Y, name, 255);
    draw_button(r, m, BTN_ENEMY_PREV, "-", true);
    draw_button(r, m, BTN_ENEMY_NEXT, "+", true);
}
