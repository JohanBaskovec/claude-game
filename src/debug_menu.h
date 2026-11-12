#ifndef FUGUE_DEBUG_MENU_H
#define FUGUE_DEBUG_MENU_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "player.h"

/*
 * Menu de debug (F2) : attributs du joueur avec boutons - et +.
 * Outil de développement, textes non traduits.
 */
/* Actions que le menu demande au jeu. */
typedef enum DebugAction {
    DEBUG_ACTION_NONE,
    DEBUG_ACTION_SPAWN_BABY,
    DEBUG_ACTION_NEW_FLOOR,
    DEBUG_ACTION_KILL_ENEMIES,
    DEBUG_ACTION_GIVE_ITEM,
} DebugAction;

typedef struct DebugMenu {
    bool  open;
    int   hover;         /* bouton sous la souris, -1 si aucun */
    int   pressed;       /* bouton maintenu enfoncé, -1 si aucun */
    float repeat_timer;  /* répétition automatique quand on garde le bouton enfoncé */
} DebugMenu;

void debug_menu_init(DebugMenu *m);
void debug_menu_toggle(DebugMenu *m);

/* L'événement doit déjà être converti en coordonnées du jeu
 * (SDL_ConvertEventToRenderCoordinates). */
DebugAction debug_menu_handle_event(DebugMenu *m, const SDL_Event *event, PlayerStats *stats);

/* Gère la répétition quand un bouton reste enfoncé. */
void debug_menu_update(DebugMenu *m, float dt, PlayerStats *stats);

void debug_menu_render(const DebugMenu *m, SDL_Renderer *renderer, const PlayerStats *stats);

#endif
