#ifndef FUGUE_UI_TEXT_H
#define FUGUE_UI_TEXT_H

#include <SDL3/SDL.h>

#include "font.h"

/*
 * Texte de l'interface (menu, titre d'époque) écrit en Junicode, avec les accents.
 * Chaque texte est rastérisé une fois dans une texture blanche, gardée en cache,
 * puis teintée à l'affichage (couleur et transparence).
 */

/* Écrit `utf8` avec le haut de la ligne en y ; align = 0 : x à gauche, 0.5 : x au centre, 1 : x à droite. */
void  ui_text_draw(SDL_Renderer *r, FontId f, const char *utf8, float x, float y, float align, SDL_Color c);
float ui_text_width(FontId f, const char *utf8);

/* Libère les textures du cache (à la fermeture). */
void  ui_text_quit(void);

#endif
