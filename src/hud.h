#ifndef FUGUE_HUD_H
#define FUGUE_HUD_H

#include <SDL3/SDL.h>

#include "player.h"

/*
 * Barre de vie en forme de soufflet de crescendo (<), GDD §4.
 * L'intérieur se remplit depuis la pointe selon PV / PV max ;
 * une partie claire « fantôme » montre les PV perdus en se vidant doucement.
 */
typedef struct Hud {
    float shown_hp;   /* PV affichés par la partie fantôme (rattrape les PV réels) */
} Hud;

void hud_init(Hud *hud, const PlayerStats *stats);
void hud_update(Hud *hud, const PlayerStats *stats, float dt);
void hud_render(const Hud *hud, SDL_Renderer *renderer, const PlayerStats *stats);

#endif
