#include "input.h"

#include <math.h>

/*
 * Les touches sont lues par position physique (scancodes) :
 * SDL_SCANCODE_W/A/S/D correspond à Z/Q/S/D sur un clavier AZERTY.
 */

/* Numéro d'ordre du dernier appui sur chaque flèche (0 = relâchée). */
static Uint64 arrow_order[DIR_COUNT];
static Uint64 press_counter;

static Dir arrow_from_scancode(SDL_Scancode sc)
{
    switch (sc) {
    case SDL_SCANCODE_UP:    return DIR_UP;
    case SDL_SCANCODE_DOWN:  return DIR_DOWN;
    case SDL_SCANCODE_LEFT:  return DIR_LEFT;
    case SDL_SCANCODE_RIGHT: return DIR_RIGHT;
    default:                 return DIR_NONE;
    }
}

void input_handle_event(const SDL_Event *event)
{
    switch (event->type) {
    case SDL_EVENT_KEY_DOWN: {
        Dir d = arrow_from_scancode(event->key.scancode);
        if (d != DIR_NONE && !event->key.repeat) {
            arrow_order[d] = ++press_counter;
        }
        break;
    }
    case SDL_EVENT_KEY_UP: {
        Dir d = arrow_from_scancode(event->key.scancode);
        if (d != DIR_NONE) {
            arrow_order[d] = 0;
        }
        break;
    }
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        for (int i = 0; i < DIR_COUNT; i++) {
            arrow_order[i] = 0;
        }
        break;
    default:
        break;
    }
}

Input input_sample(void)
{
    const bool *keys = SDL_GetKeyboardState(NULL);
    Input in = { 0.0f, 0.0f, DIR_NONE };

    if (keys[SDL_SCANCODE_W]) in.move_y -= 1.0f;
    if (keys[SDL_SCANCODE_S]) in.move_y += 1.0f;
    if (keys[SDL_SCANCODE_A]) in.move_x -= 1.0f;
    if (keys[SDL_SCANCODE_D]) in.move_x += 1.0f;

    float len = sqrtf(in.move_x * in.move_x + in.move_y * in.move_y);
    if (len > 0.0f) {
        in.move_x /= len;
        in.move_y /= len;
    }

    /* Comme dans TBOI : la dernière flèche enfoncée l'emporte. */
    Uint64 best = 0;
    for (int i = 0; i < DIR_COUNT; i++) {
        if (arrow_order[i] > best) {
            best = arrow_order[i];
            in.shoot = (Dir)i;
        }
    }
    return in;
}

void dir_vector(Dir d, float *x, float *y)
{
    *x = 0.0f;
    *y = 0.0f;
    switch (d) {
    case DIR_UP:    *y = -1.0f; break;
    case DIR_DOWN:  *y =  1.0f; break;
    case DIR_LEFT:  *x = -1.0f; break;
    case DIR_RIGHT: *x =  1.0f; break;
    default: break;
    }
}
