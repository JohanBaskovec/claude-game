#include "room.h"
#include "config.h"

/* Distance à laquelle le joueur arrive devant la porte, depuis le bord du sol. */
#define ENTRY_INSET   40.0f
/* Le centre du joueur doit dépasser le bord du sol d'autant pour passer la porte. */
#define TRIGGER_DEPTH 12.0f

static bool overlaps(const SDL_FRect *a, const SDL_FRect *b)
{
    return a->x < b->x + b->w && b->x < a->x + a->w &&
           a->y < b->y + b->h && b->y < a->y + a->h;
}

static void add_wall(Room *room, float x, float y, float w, float h)
{
    if (room->wall_count < ROOM_MAX_WALLS && w > 0.0f && h > 0.0f) {
        room->walls[room->wall_count++] = (SDL_FRect){ x, y, w, h };
    }
}

static bool cell_in_room(const FloorRoom *fr, int cx, int cy)
{
    for (int i = 0; i < fr->cell_count; i++) {
        if (fr->cell_x[i] == cx && fr->cell_y[i] == cy) return true;
    }
    return false;
}

/* Prépare une porte sur le côté `side` du sol `c`, et renvoie la dalle de mur
 * coupée en deux autour de l'ouverture. */
static void build_side(Room *room, SDL_FRect c, Dir side, const FloorDoor *door)
{
    SDL_FRect slab;
    switch (side) {
    case DIR_UP:    slab = (SDL_FRect){ c.x, c.y - WALL_TOP, c.w, WALL_TOP }; break;
    case DIR_DOWN:  slab = (SDL_FRect){ c.x, c.y + c.h, c.w, WALL_BOTTOM }; break;
    case DIR_LEFT:  slab = (SDL_FRect){ c.x - WALL_SIDE, c.y, WALL_SIDE, c.h }; break;
    default:        slab = (SDL_FRect){ c.x + c.w, c.y, WALL_SIDE, c.h }; break;
    }

    if (!door || room->door_count >= ROOM_MAX_DOORS) {
        add_wall(room, slab.x, slab.y, slab.w, slab.h);
        return;
    }

    RoomDoor *d = &room->doors[room->door_count++];
    d->side = side;
    d->target = door->target;
    d->cell_x = door->cell_x;
    d->cell_y = door->cell_y;
    float mx = c.x + c.w * 0.5f, my = c.y + c.h * 0.5f;
    const float far = 400.0f;   /* zone de passage généreuse au-delà du mur */

    if (side == DIR_UP || side == DIR_DOWN) {
        d->opening = (SDL_FRect){ mx - DOOR_W * 0.5f, slab.y, DOOR_W, slab.h };
        add_wall(room, slab.x, slab.y, d->opening.x - slab.x, slab.h);
        add_wall(room, d->opening.x + DOOR_W, slab.y, slab.x + slab.w - d->opening.x - DOOR_W, slab.h);
        if (side == DIR_UP) {
            d->trigger = (SDL_FRect){ d->opening.x, c.y - TRIGGER_DEPTH - far, DOOR_W, far };
            d->entry_x = mx;
            d->entry_y = c.y + ENTRY_INSET;
        } else {
            d->trigger = (SDL_FRect){ d->opening.x, c.y + c.h + TRIGGER_DEPTH, DOOR_W, far };
            d->entry_x = mx;
            d->entry_y = c.y + c.h - ENTRY_INSET;
        }
    } else {
        d->opening = (SDL_FRect){ slab.x, my - DOOR_H * 0.5f, slab.w, DOOR_H };
        add_wall(room, slab.x, slab.y, slab.w, d->opening.y - slab.y);
        add_wall(room, slab.x, d->opening.y + DOOR_H, slab.w, slab.y + slab.h - d->opening.y - DOOR_H);
        if (side == DIR_LEFT) {
            d->trigger = (SDL_FRect){ c.x - TRIGGER_DEPTH - far, d->opening.y, far, DOOR_H };
            d->entry_x = c.x + ENTRY_INSET;
            d->entry_y = my;
        } else {
            d->trigger = (SDL_FRect){ c.x + c.w + TRIGGER_DEPTH, d->opening.y, far, DOOR_H };
            d->entry_x = c.x + c.w - ENTRY_INSET;
            d->entry_y = my;
        }
    }
}

void room_build(Room *room, const Floor *floor, int index)
{
    const FloorRoom *fr = &floor->rooms[index];
    *room = (Room){ 0 };
    room->index = index;
    room->cell_count = fr->cell_count;
    room->doors_open = true;

    float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;
    for (int i = 0; i < fr->cell_count; i++) {
        SDL_FRect c = { fr->cell_x[i] * CELL_W, fr->cell_y[i] * CELL_H, CELL_W, CELL_H };
        room->cells[i] = c;
        minx = SDL_min(minx, c.x);
        miny = SDL_min(miny, c.y);
        maxx = SDL_max(maxx, c.x + c.w);
        maxy = SDL_max(maxy, c.y + c.h);
    }
    room->bounds = (SDL_FRect){ minx - WALL_SIDE, miny - WALL_TOP,
                                maxx - minx + 2.0f * WALL_SIDE, maxy - miny + WALL_TOP + WALL_BOTTOM };

    static const int DX[DIR_COUNT] = { [DIR_LEFT] = -1, [DIR_RIGHT] = 1 };
    static const int DY[DIR_COUNT] = { [DIR_UP] = -1, [DIR_DOWN] = 1 };

    /* Un mur sur chaque côté de cellule qui ne donne pas sur une autre cellule de la salle. */
    for (int i = 0; i < fr->cell_count; i++) {
        int cx = fr->cell_x[i], cy = fr->cell_y[i];
        for (int s = 0; s < DIR_COUNT; s++) {
            if (cell_in_room(fr, cx + DX[s], cy + DY[s])) continue;
            const FloorDoor *door = NULL;
            for (int k = 0; k < fr->door_count; k++) {
                if (fr->doors[k].cell_x == cx && fr->doors[k].cell_y == cy && fr->doors[k].side == (Dir)s) {
                    door = &fr->doors[k];
                }
            }
            build_side(room, room->cells[i], (Dir)s, door);
        }
    }
}

bool room_overlaps_wall(const Room *room, const SDL_FRect *r)
{
    for (int i = 0; i < room->wall_count; i++) {
        if (overlaps(r, &room->walls[i])) return true;
    }
    if (!room->doors_open) {
        for (int i = 0; i < room->door_count; i++) {
            if (overlaps(r, &room->doors[i].opening)) return true;
        }
    }
    return false;
}

/* Repousse la boîte hors du rectangle sur l'axe donné, selon le sens du déplacement. */
static int push_out(SDL_FRect *box, const SDL_FRect *w, float dx, float dy)
{
    if (!overlaps(box, w)) return 0;
    if (dx != 0.0f) {
        box->x = (dx > 0.0f) ? w->x - box->w : w->x + w->w;
        return ROOM_BLOCKED_X;
    }
    box->y = (dy > 0.0f) ? w->y - box->h : w->y + w->h;
    return ROOM_BLOCKED_Y;
}

static int collide_axis(const Room *room, SDL_FRect *box, float dx, float dy)
{
    int blocked = 0;
    for (int i = 0; i < room->wall_count; i++) {
        blocked |= push_out(box, &room->walls[i], dx, dy);
    }
    if (!room->doors_open) {
        for (int i = 0; i < room->door_count; i++) {
            blocked |= push_out(box, &room->doors[i].opening, dx, dy);
        }
    }
    return blocked;
}

int room_move_box(const Room *room, SDL_FRect *box, float dx, float dy)
{
    int blocked = 0;
    box->x += dx;
    if (dx != 0.0f) blocked |= collide_axis(room, box, dx, 0.0f);
    box->y += dy;
    if (dy != 0.0f) blocked |= collide_axis(room, box, 0.0f, dy);
    return blocked;
}

void room_set_doors_open(Room *room, bool open, SDL_FRect *keep_out)
{
    if (room->doors_open == open) return;
    room->doors_open = open;
    if (open || !keep_out) return;
    for (int i = 0; i < room->door_count; i++) {
        const RoomDoor *d = &room->doors[i];
        if (!overlaps(keep_out, &d->opening)) continue;
        switch (d->side) {
        case DIR_UP:    keep_out->y = d->opening.y + d->opening.h; break;
        case DIR_DOWN:  keep_out->y = d->opening.y - keep_out->h; break;
        case DIR_LEFT:  keep_out->x = d->opening.x + d->opening.w; break;
        default:        keep_out->x = d->opening.x - keep_out->w; break;
        }
    }
}

const RoomDoor *room_door_at(const Room *room, float x, float y)
{
    SDL_FPoint p = { x, y };
    for (int i = 0; i < room->door_count; i++) {
        if (SDL_PointInRectFloat(&p, &room->doors[i].trigger)) return &room->doors[i];
    }
    return NULL;
}

const RoomDoor *room_find_door(const Room *room, int cx, int cy, Dir side)
{
    for (int i = 0; i < room->door_count; i++) {
        const RoomDoor *d = &room->doors[i];
        if (d->cell_x == cx && d->cell_y == cy && d->side == side) return d;
    }
    return NULL;
}

void room_random_floor_point(const Room *room, float margin, float *x, float *y)
{
    const SDL_FRect *c = &room->cells[SDL_rand(room->cell_count)];
    *x = c->x + margin + SDL_randf() * (c->w - 2.0f * margin);
    *y = c->y + margin + SDL_randf() * (c->h - 2.0f * margin);
}

void room_center(const Room *room, float *x, float *y)
{
    *x = room->cells[0].x + room->cells[0].w * 0.5f;
    *y = room->cells[0].y + room->cells[0].h * 0.5f;
}

Camera room_camera(const Room *room, float x, float y)
{
    /* Le point suivi est placé là où serait le centre du sol d'une salle 1x1 :
     * à chaque porte, la caméra cadre exactement la cellule, et les glissements
     * d'une salle à l'autre tombent juste. */
    Camera cam = { x - (WALL_SIDE + CELL_W * 0.5f), y - (WALL_TOP + CELL_H * 0.5f) };
    cam.x = SDL_clamp(cam.x, room->bounds.x, room->bounds.x + room->bounds.w - GAME_W);
    cam.y = SDL_clamp(cam.y, room->bounds.y, room->bounds.y + room->bounds.h - GAME_H);
    return cam;
}

static void fill(SDL_Renderer *r, Camera cam, SDL_FRect rect, Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    rect.x = SDL_floorf(rect.x - cam.x);
    rect.y = SDL_floorf(rect.y - cam.y);
    SDL_RenderFillRect(r, &rect);
}

static void render_door(SDL_Renderer *r, Camera cam, const RoomDoor *d, bool open)
{
    /* Encadrement de pierre claire, un peu plus large que l'ouverture. */
    SDL_FRect frame = d->opening;
    if (d->side == DIR_UP || d->side == DIR_DOWN) {
        frame.x -= 10.0f;
        frame.w += 20.0f;
    } else {
        frame.y -= 10.0f;
        frame.h += 20.0f;
    }
    fill(r, cam, frame, 120, 110, 100);

    if (open) {
        fill(r, cam, d->opening, 22, 18, 16);
        return;
    }
    /* Porte fermée : planches de bois. */
    fill(r, cam, d->opening, 112, 72, 42);
    SDL_FRect o = d->opening;
    bool vertical_planks = (d->side == DIR_UP || d->side == DIR_DOWN);
    int planks = 4;
    for (int i = 1; i < planks; i++) {
        SDL_FRect line = vertical_planks
            ? (SDL_FRect){ o.x + o.w * (float)i / (float)planks, o.y, 2.0f, o.h }
            : (SDL_FRect){ o.x, o.y + o.h * (float)i / (float)planks, o.w, 2.0f };
        fill(r, cam, line, 78, 48, 28);
    }
}

void room_render(const Room *room, SDL_Renderer *r, Camera cam)
{
    /* Murs : une bande autour de chaque cellule. */
    for (int i = 0; i < room->cell_count; i++) {
        SDL_FRect c = room->cells[i];
        SDL_FRect band = { c.x - WALL_SIDE, c.y - WALL_TOP, c.w + 2.0f * WALL_SIDE, c.h + WALL_TOP + WALL_BOTTOM };
        fill(r, cam, band, 62, 54, 50);
    }
    /* Liseré au pied des murs. */
    for (int i = 0; i < room->cell_count; i++) {
        SDL_FRect c = room->cells[i];
        fill(r, cam, (SDL_FRect){ c.x - 6.0f, c.y - 6.0f, c.w + 12.0f, c.h + 12.0f }, 92, 82, 76);
    }
    for (int i = 0; i < room->door_count; i++) {
        render_door(r, cam, &room->doors[i], room->doors_open);
    }
    /* Sol : parchemin terne, avec 6 lignes très pâles par cellule (repère d'échelle). */
    for (int i = 0; i < room->cell_count; i++) {
        SDL_FRect c = room->cells[i];
        fill(r, cam, c, 196, 184, 160);
        for (int k = 0; k < 6; k++) {
            fill(r, cam, (SDL_FRect){ c.x, c.y + 46.0f + 60.0f * (float)k, c.w, 2.0f }, 184, 171, 146);
        }
    }
}
