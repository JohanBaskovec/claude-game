#include "room.h"
#include "config.h"

static void add_wall(Room *room, float x, float y, float w, float h)
{
    if (room->wall_count < ROOM_MAX_WALLS) {
        room->walls[room->wall_count++] = (SDL_FRect){ x, y, w, h };
    }
}

void room_init(Room *room)
{
    room->floor = (SDL_FRect){ ROOM_FLOOR_X, ROOM_FLOOR_Y, ROOM_FLOOR_W, ROOM_FLOOR_H };
    room->wall_count = 0;

    const float right  = ROOM_FLOOR_X + ROOM_FLOOR_W;
    const float bottom = ROOM_FLOOR_Y + ROOM_FLOOR_H;

    add_wall(room, 0.0f,  0.0f,   GAME_W,         ROOM_FLOOR_Y);           /* haut */
    add_wall(room, 0.0f,  bottom, GAME_W,         GAME_H - bottom);        /* bas */
    add_wall(room, 0.0f,  0.0f,   ROOM_FLOOR_X,   GAME_H);                 /* gauche */
    add_wall(room, right, 0.0f,   GAME_W - right, GAME_H);                 /* droite */
}

static bool overlaps(const SDL_FRect *a, const SDL_FRect *b)
{
    return a->x < b->x + b->w && b->x < a->x + a->w &&
           a->y < b->y + b->h && b->y < a->y + a->h;
}

bool room_overlaps_wall(const Room *room, const SDL_FRect *r)
{
    for (int i = 0; i < room->wall_count; i++) {
        if (overlaps(r, &room->walls[i])) {
            return true;
        }
    }
    return false;
}

int room_move_box(const Room *room, SDL_FRect *box, float dx, float dy)
{
    int blocked = 0;
    box->x += dx;
    for (int i = 0; i < room->wall_count; i++) {
        const SDL_FRect *w = &room->walls[i];
        if (overlaps(box, w)) {
            box->x = (dx > 0.0f) ? w->x - box->w : w->x + w->w;
            blocked |= ROOM_BLOCKED_X;
        }
    }

    box->y += dy;
    for (int i = 0; i < room->wall_count; i++) {
        const SDL_FRect *w = &room->walls[i];
        if (overlaps(box, w)) {
            box->y = (dy > 0.0f) ? w->y - box->h : w->y + w->h;
            blocked |= ROOM_BLOCKED_Y;
        }
    }
    return blocked;
}

void room_render(const Room *room, SDL_Renderer *renderer)
{
    /* Murs : pierre sombre. */
    SDL_SetRenderDrawColor(renderer, 62, 54, 50, 255);
    for (int i = 0; i < room->wall_count; i++) {
        SDL_RenderFillRect(renderer, &room->walls[i]);
    }

    /* Liseré intérieur pour marquer le pied des murs. */
    SDL_FRect edge = { room->floor.x - 6.0f, room->floor.y - 6.0f,
                       room->floor.w + 12.0f, room->floor.h + 12.0f };
    SDL_SetRenderDrawColor(renderer, 92, 82, 76, 255);
    SDL_RenderFillRect(renderer, &edge);

    /* Sol : parchemin terne. */
    SDL_SetRenderDrawColor(renderer, 196, 184, 160, 255);
    SDL_RenderFillRect(renderer, &room->floor);

    /* Lignes très pâles : 6 lignes de texte par cellule (repère d'échelle). */
    SDL_SetRenderDrawColor(renderer, 184, 171, 146, 255);
    for (int k = 0; k < 6; k++) {
        SDL_FRect line = { room->floor.x, room->floor.y + 46.0f + 60.0f * (float)k,
                           room->floor.w, 2.0f };
        SDL_RenderFillRect(renderer, &line);
    }
}
