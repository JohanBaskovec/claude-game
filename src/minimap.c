#include "minimap.h"
#include "config.h"

/* Une cellule de 19x9 px garde à peu près le rapport 768x360 du sol,
 * et reste assez grande pour cliquer dessus. */
#define MM_CELL_W 19.0f
#define MM_CELL_H 9.0f
#define MM_W      (FLOOR_COLS * MM_CELL_W)
#define MM_H      (FLOOR_ROWS * MM_CELL_H)
#define MM_X      (GAME_W - MM_W - 16.0f)
#define MM_Y      16.0f

float minimap_bottom(void)
{
    return MM_Y + MM_H + 4.0f;
}

static bool room_shown(const Floor *floor, int i, bool reveal_all)
{
    return reveal_all || floor_room_discovered(floor, i);
}

static void fill(SDL_Renderer *r, float x, float y, float w, float h, Uint8 cr, Uint8 cg, Uint8 cb, Uint8 a)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, a);
    SDL_FRect rect = { x, y, w, h };
    SDL_RenderFillRect(r, &rect);
}

/* Contour d'une salle : on trace chaque bord de cellule qui ne touche pas la même salle. */
static void outline(SDL_Renderer *r, const Floor *floor, int i, Uint8 c)
{
    const FloorRoom *room = &floor->rooms[i];
    SDL_SetRenderDrawColor(r, c, c, c, 255);
    for (int k = 0; k < room->cell_count; k++) {
        int cx = room->cell_x[k], cy = room->cell_y[k];
        float x = MM_X + cx * MM_CELL_W, y = MM_Y + cy * MM_CELL_H;
        float w = MM_CELL_W - 1.0f, h = MM_CELL_H - 1.0f;
        if (floor_room_at(floor, cx, cy - 1) != i) SDL_RenderFillRect(r, &(SDL_FRect){ x, y, w + 1.0f, 1.0f });
        if (floor_room_at(floor, cx, cy + 1) != i) SDL_RenderFillRect(r, &(SDL_FRect){ x, y + h - 1.0f, w + 1.0f, 1.0f });
        if (floor_room_at(floor, cx - 1, cy) != i) SDL_RenderFillRect(r, &(SDL_FRect){ x, y, 1.0f, h });
        if (floor_room_at(floor, cx + 1, cy) != i) SDL_RenderFillRect(r, &(SDL_FRect){ x + w - 1.0f, y, 1.0f, h });
    }
}

void minimap_render(const Floor *floor, int current, SDL_Renderer *r, bool reveal_all, int hover)
{
    /* La double page : fond sombre et reliure au milieu de la colonne centrale. */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    fill(r, MM_X - 4.0f, MM_Y - 4.0f, MM_W + 8.0f, MM_H + 8.0f, 14, 11, 10, 170);
    fill(r, MM_X + MM_W * 0.5f - 1.0f, MM_Y - 2.0f, 2.0f, MM_H + 4.0f, 0, 0, 0, 200);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    for (int i = 0; i < floor->room_count; i++) {
        if (!room_shown(floor, i, reveal_all)) continue;
        const FloorRoom *room = &floor->rooms[i];

        if (!floor_room_discovered(floor, i)) {
            outline(r, floor, i, 110);   /* inconnue, montrée par le debug */
        } else {
            Uint8 c[3];
            if (i == current)        { c[0] = 255; c[1] = 70;  c[2] = 168; }
            else if (room->visited)  { c[0] = 206; c[1] = 194; c[2] = 172; }
            else                     { c[0] = 84;  c[1] = 76;  c[2] = 70;  }
            for (int k = 0; k < room->cell_count; k++) {
                int cx = room->cell_x[k], cy = room->cell_y[k];
                /* Pas de joint entre deux cellules de la même salle. */
                float w = MM_CELL_W - (floor_room_at(floor, cx + 1, cy) == i ? 0.0f : 1.0f);
                float h = MM_CELL_H - (floor_room_at(floor, cx, cy + 1) == i ? 0.0f : 1.0f);
                fill(r, MM_X + cx * MM_CELL_W, MM_Y + cy * MM_CELL_H, w, h, c[0], c[1], c[2], 255);
            }
        }

        if (i == floor->boss) {
            /* Marque du boss : un carré rouge au centre de sa première cellule. */
            float bx = MM_X + room->cell_x[0] * MM_CELL_W + MM_CELL_W * 0.5f - 2.0f;
            float by = MM_Y + room->cell_y[0] * MM_CELL_H + MM_CELL_H * 0.5f - 2.0f;
            fill(r, bx, by, 5.0f, 4.0f, 190, 30, 30, 255);
        }
    }

    if (hover >= 0 && hover < floor->room_count && room_shown(floor, hover, reveal_all)) {
        outline(r, floor, hover, 255);
    }
}

int minimap_room_at(const Floor *floor, float x, float y, bool reveal_all, int *cell_x, int *cell_y)
{
    if (x < MM_X || y < MM_Y || x >= MM_X + MM_W || y >= MM_Y + MM_H) {
        return -1;
    }
    int cx = (int)((x - MM_X) / MM_CELL_W), cy = (int)((y - MM_Y) / MM_CELL_H);
    int room = floor_room_at(floor, cx, cy);
    if (room < 0 || !room_shown(floor, room, reveal_all)) {
        return -1;
    }
    if (cell_x) *cell_x = cx;
    if (cell_y) *cell_y = cy;
    return room;
}
