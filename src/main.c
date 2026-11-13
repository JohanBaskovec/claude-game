/*
 * Amadeus' Minor Fugue — fenêtre, entrées, menu principal, boucle de jeu.
 * Les règles du jeu sont dans game.c.
 *
 * Modèle à callbacks de SDL3 (pas de boucle while bloquante),
 * indispensable pour le build navigateur (Emscripten).
 */
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "audio.h"
#include "config.h"
#include "debug_menu.h"
#include "draw.h"
#include "decor.h"
#include "game.h"
#include "input.h"
#include "menu.h"
#include "minimap.h"
#include "sprite.h"
#include "text.h"
#include "ui_text.h"

#define TICK_NS       (SDL_NS_PER_SECOND / TICK_HZ)
#define MAX_FRAME_NS  (SDL_NS_PER_SECOND / 4)   /* au-delà, on ralentit plutôt que d'enchaîner les pas */

typedef struct App {
    SDL_Window     *window;
    SDL_Renderer   *renderer;

    Game            game;
    DebugMenu       debug;
    Menu            menu;
    bool            in_menu;   /* menu principal affiché (au lancement) */

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

static Uint64 random_seed(void)
{
    return ((Uint64)SDL_rand_bits() << 32) | SDL_rand_bits();
}

static void step(App *app, float dt)
{
    if (app->in_menu) {
        menu_update(&app->menu, dt);
        return;
    }
    debug_menu_update(&app->debug, dt, &app->game.player.stats);

    Input in = input_sample();
    game_step(&app->game, &in, dt);
}

static void render(App *app, float alpha)
{
    SDL_Renderer *r = app->renderer;
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);

    const Game *g = &app->game;
    game_render(g, r, alpha);
    if (app->in_menu) {
        menu_render(&app->menu, r);
        SDL_RenderPresent(r);
        return;
    }

    if (app->show_debug) {
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        const FloorRoom *fr = &g->floor.rooms[g->room.index];
        static const char *kinds[] = { "", "  DEPART", "  BOSS", "  TRESOR" };
        SDL_RenderDebugTextFormat(r, 8, GAME_H - 54, "etage %d  graine %llu  salle %d/%d (%s, %d ennemis, profondeur %d)%s",
                                  g->floor_number, (unsigned long long)g->floor.seed, g->room.index + 1,
                                  g->floor.room_count, floor_shape_name(fr->shape), fr->spawn_count, fr->depth,
                                  kinds[fr->kind]);
        char owned[128] = "";
        for (int i = 0; i < ITEM_COUNT; i++) {
            static const char *short_names[ITEM_COUNT] = { " Lully", " Haydn", " Satie", " Guido", " Sabots",
                                                           " c", " a", " x", " Liquescence", " Trope" };
            if (g->owned[i]) SDL_strlcat(owned, short_names[i], sizeof owned);
        }
        SDL_RenderDebugTextFormat(r, 8, GAME_H - 42, "objets :%s", owned[0] ? owned : " aucun");
        SDL_RenderDebugTextFormat(r, 8, GAME_H - 30, "FPS %.0f  notes %d  ennemis %d  prix %d  metronomes poses %d  briques %d  morts %d",
                                  app->fps, projectile_count(&g->shots), enemy_count(&g->enemies),
                                  pickup_count(&g->pickups), bomb_count(&g->bombs), hazard_count(&g->hazards), g->deaths);
        SDL_RenderDebugTextFormat(r, 8, GAME_H - 18, "joueur v=(%.0f, %.0f)  PV %d/%d  invincible %.2fs",
                                  g->player.vx, g->player.vy, g->player.stats.hp, g->player.stats.max_hp,
                                  SDL_max(g->player.invuln_timer, 0.0f));
    }
    debug_menu_render(&app->debug, r, &g->player.stats);

    /* Viseur : affiché tant que la souris sert (il disparaît si on joue au clavier). */
    float mx, my;
    if (input_mouse_visible(&mx, &my)) draw_crosshair(r, mx, my);

    SDL_RenderPresent(r);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    SDL_SetAppMetadata("Amadeus' Minor Fugue", "0.1", "fr.baskovec.fugue");

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
    if (!SDL_CreateWindowAndRenderer("Amadeus' Minor Fugue", GAME_W * scale, GAME_H * scale,
                                     SDL_WINDOW_RESIZABLE, &app->window, &app->renderer)) {
        SDL_Log("Création de la fenêtre impossible : %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(app->renderer, GAME_W, GAME_H,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_SetRenderVSync(app->renderer, 1);
    /* Le curseur du système sert au menu ; en partie, le jeu dessine son viseur (GDD §17). */

    SDL_srand(0);   /* graine aléatoire tirée de l'horloge */
    text_init_from_locale();
    decor_init();
    audio_init();
    game_init(&app->game, random_seed());
    debug_menu_init(&app->debug);
    menu_init(&app->menu);
    app->in_menu = true;
    app->game.backdrop = true;   /* derrière le menu : la salle de départ */

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

    /* Menu principal : il reçoit tous les événements (sauf F11), le jeu aucun. */
    if (app->in_menu) {
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == SDL_SCANCODE_F11 && !event->key.repeat) {
            bool fs = (SDL_GetWindowFlags(app->window) & SDL_WINDOW_FULLSCREEN) != 0;
            SDL_SetWindowFullscreen(app->window, !fs);
            return SDL_APP_CONTINUE;
        }
        SDL_Event local = *event;
        SDL_ConvertEventToRenderCoordinates(app->renderer, &local);
        switch (menu_handle_event(&app->menu, &local)) {
        case MENU_PLAY:
            app->in_menu = false;
            app->game.backdrop = false;
            game_new_run(&app->game, random_seed());
            app->accumulator_ns = 0;
            SDL_HideCursor();
            break;
        case MENU_QUIT:
            return SDL_APP_SUCCESS;
        default:
            break;
        }
        return SDL_APP_CONTINUE;
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
        case SDL_SCANCODE_F2:
            debug_menu_toggle(&app->debug);
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

    /* Souris : passage des coordonnées fenêtre aux coordonnées du jeu (960x540). */
    SDL_Event local = *event;
    SDL_ConvertEventToRenderCoordinates(app->renderer, &local);
    input_handle_event(&local);

    /* Clic sur la mini-carte : téléportation (et pas de tir). Clic sur le menu de debug : pas de tir. */
    if (local.type == SDL_EVENT_MOUSE_BUTTON_DOWN && local.button.button == SDL_BUTTON_LEFT) {
        int cx, cy;
        int room = minimap_room_at(&app->game.floor, local.button.x, local.button.y, app->game.reveal_map, &cx, &cy);
        if (room >= 0) {
            game_teleport(&app->game, room, cx, cy);
            input_block_mouse();
        }
        if (debug_menu_contains(&app->debug, local.button.x, local.button.y)) input_block_mouse();
    }

    switch (debug_menu_handle_event(&app->debug, &local, &app->game.player.stats)) {
    case DEBUG_ACTION_SPAWN_ENEMY:   game_spawn_enemy(&app->game, (EnemyType)app->debug.enemy); break;
    case DEBUG_ACTION_NEW_FLOOR:     game_next_floor(&app->game, random_seed()); break;
    case DEBUG_ACTION_KILL_ENEMIES:  game_kill_enemies(&app->game); break;
    case DEBUG_ACTION_GIVE_ITEM:     game_give_next_item(&app->game); break;
    case DEBUG_ACTION_SPAWN_BOX:     game_spawn_box(&app->game); break;
    default: break;
    }
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
    audio_update();

    /* Mini-carte : toute la carte quand le menu de debug est ouvert, salle survolée par la souris. */
    float mx, my, lx, ly;
    SDL_GetMouseState(&mx, &my);
    SDL_RenderCoordinatesFromWindow(app->renderer, mx, my, &lx, &ly);
    app->game.reveal_map = app->debug.open;
    app->game.map_hover = minimap_room_at(&app->game.floor, lx, ly, app->game.reveal_map, NULL, NULL);

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
    audio_quit();
    ui_text_quit();
    decor_quit();
    sprite_quit();
    if (app) {
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        SDL_free(app);
    }
}
