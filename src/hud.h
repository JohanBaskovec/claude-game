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
    float       shown_hp;      /* PV affichés par la partie fantôme (rattrape les PV réels) */
    const char *banner_title;  /* bandeau (objet ramassé) : nom ... */
    const char *banner_text;   /* ... et effet */
    float       banner_timer;  /* > 0 : bandeau affiché */
} Hud;

void hud_init(Hud *hud, const PlayerStats *stats);
void hud_update(Hud *hud, const PlayerStats *stats, float dt);

/* Affiche un bandeau au milieu de l'écran pendant tuning.item_banner_time. */
void hud_show_banner(Hud *hud, const char *title, const char *text);

/* Barre de vie, métronomes, royalties et bandeau. */
void hud_render(const Hud *hud, SDL_Renderer *renderer, const PlayerStats *stats);

/* Barre de vie du boss, en bas de l'écran. */
void hud_render_boss(SDL_Renderer *renderer, float hp, float max_hp);

#endif
