/*
 * Fugue in Amadeus Minor — Phase 1 : prototype de déplacement et de tir.
 *
 * Modèle à callbacks de SDL3 (pas de boucle while bloquante),
 * indispensable pour le build navigateur (Emscripten).
 */
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "config.h"
#include "input.h"
#include "player.h"
#include "projectile.h"
#include "room.h"

#define TICK_NS       (SDL_NS_PER_SECOND / TICK_HZ)
#define MAX_FRAME_NS  (SDL_NS_PER_SECOND / 4)   /* au-delà, on ralentit plutôt que d'enchaîner les pas */

typedef struct App {
    SDL_Window     *window;
    SDL_Renderer   *renderer;

    Room            room;
    Player          player;
    ProjectilePool  shots;

    Uint64          last_ns;
    Uint64          accumulator_ns;

    bool            show_debug;
    float           fps;
    Uint64          fps_window_start;
    int             fps_frames;
} App;

/* Plus grand facteur entier qui tient dans l'espace disponible de l'écran. */
static int initial_scale(void)
{
    SDL_Rect usable;
    int scale = 1;
    if (SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &usable)) {
        while (GAME_W * (scale + 1) <= usable.w && GAME_H * (scale + 1) <= usable.h - 40) {
            scale++;
        }
    }
    return scale;
}

static void fire(App *app, Dir dir)
{
    float dx, dy, cx, cy;
    dir_vector(dir, &dx, &dy);
    player_center(&app->player, &cx, &cy);

    float vx = dx * tuning.shot_speed + app->player.vx * tuning.shot_inherit;
    float vy = dy * tuning.shot_speed + app->player.vy * tuning.shot_inherit;
    projectile_spawn(&app->shots, cx + dx * 8.0f, cy + dy * 8.0f, vx, vy, tuning.shot_range);
}

static void step(App *app, float dt)
{
    Input in = input_sample();
    Dir shot = player_update(&app->player, &in, &app->room, dt);
    projectile_update_all(&app->shots, &app->room, dt);
    if (shot != DIR_NONE) {
        fire(app, shot);
    }
}

static void render(App *app, float alpha)
{
    SDL_Renderer *r = app->renderer;
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);

    room_render(&app->room, r);
    player_render(&app->player, r, alpha);
    projectile_render_all(&app->shots, r, alpha);

    if (app->show_debug) {
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderDebugTextFormat(r, 8, 8, "FPS %.0f  notes %d", app->fps, projectile_count(&app->shots));
        SDL_RenderDebugTextFormat(r, 8, 20, "vitesse %.0f  accel %.2fs  frein %.2fs",
                                  tuning.player_speed, tuning.player_accel_time, tuning.player_decel_time);
        SDL_RenderDebugTextFormat(r, 8, 32, "cadence %.1f/s  tir %.0f px/s  portee %.0f  heritage %.2f",
                                  tuning.fire_rate, tuning.shot_speed, tuning.shot_range, tuning.shot_inherit);
        SDL_RenderDebugTextFormat(r, 8, 44, "joueur v=(%.0f, %.0f)", app->player.vx, app->player.vy);
    }

    SDL_RenderPresent(r);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    SDL_SetAppMetadata("Fugue in Amadeus Minor", "0.1", "fr.baskovec.fugue");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init a échoué : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    App *app = SDL_calloc(1, sizeof(App));
    if (!app) {
        return SDL_APP_FAILURE;
    }
    *appstate = app;

    int scale = initial_scale();
    if (!SDL_CreateWindowAndRenderer("Fugue in Amadeus Minor", GAME_W * scale, GAME_H * scale,
                                     SDL_WINDOW_RESIZABLE, &app->window, &app->renderer)) {
        SDL_Log("Création de la fenêtre impossible : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(app->renderer, GAME_W, GAME_H,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_SetRenderVSync(app->renderer, 1);

    room_init(&app->room);
    player_init(&app->player,
                ROOM_FLOOR_X + ROOM_FLOOR_W * 0.5f,
                ROOM_FLOOR_Y + ROOM_FLOOR_H * 0.5f);
    projectile_pool_clear(&app->shots);

    app->last_ns = SDL_GetTicksNS();
    app->fps_window_start = app->last_ns;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    App *app = appstate;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
        switch (event->key.scancode) {
#ifndef SDL_PLATFORM_EMSCRIPTEN
        case SDL_SCANCODE_ESCAPE:
            return SDL_APP_SUCCESS;
#endif
        case SDL_SCANCODE_F1:
            app->show_debug = !app->show_debug;
            break;
        case SDL_SCANCODE_F11: {
            bool fs = (SDL_GetWindowFlags(app->window) & SDL_WINDOW_FULLSCREEN) != 0;
            SDL_SetWindowFullscreen(app->window, !fs);
            break;
        }
        default:
            break;
        }
    }

    input_handle_event(event);
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    App *app = appstate;

    Uint64 now = SDL_GetTicksNS();
    Uint64 frame = now - app->last_ns;
    app->last_ns = now;
    if (frame > MAX_FRAME_NS) {
        frame = MAX_FRAME_NS;
    }

    /* Logique à pas fixe, rendu interpolé entre les deux derniers pas. */
    app->accumulator_ns += frame;
    while (app->accumulator_ns >= TICK_NS) {
        step(app, 1.0f / (float)TICK_HZ);
        app->accumulator_ns -= TICK_NS;
    }
    float alpha = (float)app->accumulator_ns / (float)TICK_NS;

    render(app, alpha);

    app->fps_frames++;
    if (now - app->fps_window_start >= SDL_NS_PER_SECOND / 2) {
        app->fps = (float)app->fps_frames * (float)SDL_NS_PER_SECOND / (float)(now - app->fps_window_start);
        app->fps_frames = 0;
        app->fps_window_start = now;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;
    App *app = appstate;
    if (app) {
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        SDL_free(app);
    }
}
