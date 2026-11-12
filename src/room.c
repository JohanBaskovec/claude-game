#include "room.h"
#include "config.h"
#include "decor.h"
#include "draw.h"

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

    if (fr->kind == ROOM_TREASURE) {
        room_place_offers(room, false);
    }
    /* Salle du boss déjà vaincu : la trappe et les récompenses sont toujours là. */
    if (fr->kind == ROOM_BOSS && fr->cleared) {
        room_open_trapdoor(room);
        room_place_offers(room, true);
    }
}

void room_place_offers(Room *room, bool boss)
{
    float mx, my;
    room_middle(room, &mx, &my);
    /* Trésor : lutrins au-dessus du centre, socle d'arme en dessous.
     * Boss : la trappe au centre, lutrins plus haut, socle plus bas. */
    float lutrin_y = my + (boss ? -85.0f : -40.0f);
    float chest_y = my + (boss ? 100.0f : 90.0f);
    room->prop_count = 0;
    for (int i = 0; i < TREASURE_OFFERS; i++) {
        float x = mx + ((float)i - (TREASURE_OFFERS - 1) * 0.5f) * LUTRIN_GAP;
        room->prop_kind[room->prop_count] = PROP_LUTRIN;
        room->props[room->prop_count++] = (SDL_FRect){ x - LUTRIN_W * 0.5f, lutrin_y - LUTRIN_H * 0.5f, LUTRIN_W, LUTRIN_H };
    }
    room->prop_kind[room->prop_count] = PROP_WEAPON;
    room->props[room->prop_count++] = (SDL_FRect){ mx - CHEST_W * 0.5f, chest_y - CHEST_H * 0.5f, CHEST_W, CHEST_H };
}

void room_middle(const Room *room, float *x, float *y)
{
    float sx = 0.0f, sy = 0.0f;
    for (int i = 0; i < room->cell_count; i++) {
        sx += room->cells[i].x + room->cells[i].w * 0.5f;
        sy += room->cells[i].y + room->cells[i].h * 0.5f;
    }
    sx /= (float)room->cell_count;
    sy /= (float)room->cell_count;
    float best = 1e18f;
    for (int i = 0; i < room->cell_count; i++) {
        float cx = room->cells[i].x + room->cells[i].w * 0.5f, cy = room->cells[i].y + room->cells[i].h * 0.5f;
        float d = (cx - sx) * (cx - sx) + (cy - sy) * (cy - sy);
        if (d < best) {
            best = d;
            *x = cx;
            *y = cy;
        }
    }
}

void room_open_trapdoor(Room *room)
{
    room->trapdoor = true;
    room_middle(room, &room->trapdoor_x, &room->trapdoor_y);
}

bool room_overlaps_wall(const Room *room, const SDL_FRect *r)
{
    return room_overlaps_solid(room, r, true);
}

bool room_overlaps_solid(const Room *room, const SDL_FRect *r, bool props)
{
    for (int i = 0; i < room->wall_count; i++) {
        if (overlaps(r, &room->walls[i])) return true;
    }
    for (int i = 0; props && i < room->prop_count; i++) {
        if (overlaps(r, &room->props[i])) return true;
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
    for (int i = 0; i < room->prop_count; i++) {
        blocked |= push_out(box, &room->props[i], dx, dy);
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

void room_render(const Room *room, SDL_Renderer *r, Camera cam)
{
    decor_render_room(room, r, cam);

    if (room->trapdoor) {
        /* Trappe : cadre de bois et trou noir. */
        float x = room->trapdoor_x - cam.x, y = room->trapdoor_y - cam.y;
        draw_rect(r, x - 34, y - 22, 68, 44, (SDL_Color){ 92, 58, 32, 255 });
        draw_rect(r, x - 28, y - 17, 56, 34, (SDL_Color){ 10, 8, 8, 255 });
        draw_rect(r, x - 28, y - 17, 56, 5, (SDL_Color){ 40, 30, 26, 255 });
        for (int k = -1; k <= 1; k += 2) {
            draw_rect(r, x - 36, y + k * 12 - 2, 8, 4, (SDL_Color){ 60, 60, 66, 255 });   /* charnières */
        }
    }
}

/* Coffre de bois cerclé de fer (placeholder du socle d'arme). */
static void render_chest(SDL_Renderer *r, float cx, float feet)
{
    SDL_Color ink = { 40, 24, 16, 255 }, wood = { 128, 78, 40, 255 }, light = { 166, 108, 58, 255 };
    SDL_Color iron = { 70, 70, 80, 255 };
    draw_ellipse(r, cx, feet - 3, 38, 8, (SDL_Color){ 60, 40, 30, 90 });
    draw_rect(r, cx - 33, feet - 34, 66, 33, ink);
    draw_rect(r, cx - 32, feet - 33, 64, 31, wood);
    draw_rect(r, cx - 32, feet - 33, 64, 9, light);   /* couvercle */
    draw_rect(r, cx - 32, feet - 24, 64, 2, ink);
    draw_rect(r, cx - 24, feet - 33, 4, 31, iron);
    draw_rect(r, cx + 20, feet - 33, 4, 31, iron);
    draw_rect(r, cx - 4, feet - 24, 8, 8, iron);       /* serrure */
    draw_rect(r, cx - 1, feet - 21, 2, 3, ink);
}

void room_render_prop(const Room *room, int i, SDL_Renderer *r, Camera cam)
{
    const SDL_FRect *p = &room->props[i];
    float cx = p->x + p->w * 0.5f - cam.x, feet = p->y + p->h - cam.y;
    if (room->prop_kind[i] == PROP_WEAPON) {
        render_chest(r, cx, feet);
        return;
    }
    /* Lutrin médiéval (placeholder du socle des objets) : pied, colonne et pupitre incliné. */
    SDL_Color ink = { 40, 24, 16, 255 }, wood = { 128, 78, 40, 255 }, light = { 166, 108, 58, 255 };

    draw_ellipse(r, cx, feet - 4, 30, 8, (SDL_Color){ 60, 40, 30, 90 });
    /* Pied en croix */
    draw_rect(r, cx - 25, feet - 10, 50, 9, ink);
    draw_rect(r, cx - 24, feet - 9, 48, 7, wood);
    /* Colonne tournée */
    draw_rect(r, cx - 6, feet - 52, 12, 44, ink);
    draw_rect(r, cx - 5, feet - 51, 10, 42, wood);
    draw_rect(r, cx - 8, feet - 36, 16, 5, light);
    draw_rect(r, cx - 8, feet - 22, 16, 5, light);
    /* Pupitre incliné, avec un livre de chœur ouvert */
    draw_rect(r, cx - 26, feet - 66, 52, 16, ink);
    draw_rect(r, cx - 25, feet - 65, 50, 14, light);
    draw_rect(r, cx - 22, feet - 72, 21, 10, (SDL_Color){ 236, 226, 200, 255 });
    draw_rect(r, cx + 1, feet - 72, 21, 10, (SDL_Color){ 226, 214, 186, 255 });
    draw_rect(r, cx - 1, feet - 72, 2, 10, ink);
    for (int k = 0; k < 3; k++) {
        draw_rect(r, cx - 20, feet - 70 + k * 3, 17, 1, (SDL_Color){ 190, 40, 40, 255 });
        draw_rect(r, cx + 3, feet - 70 + k * 3, 17, 1, (SDL_Color){ 190, 40, 40, 255 });
    }
}
