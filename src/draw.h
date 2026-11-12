#ifndef FUGUE_DRAW_H
#define FUGUE_DRAW_H

#include <SDL3/SDL.h>

/*
 * Petites fonctions de dessin partagées (placeholders en formes simples).
 * Toutes en coordonnées écran : soustraire la caméra avant de les appeler.
 */

void draw_rect(SDL_Renderer *r, float x, float y, float w, float h, SDL_Color c);

/* Disque et ellipse pleins (éventail de triangles), avec transparence. */
void draw_ellipse(SDL_Renderer *r, float cx, float cy, float rx, float ry, SDL_Color c);

/* Trait épais de (ax, ay) à (bx, by). */
void draw_line(SDL_Renderer *r, float ax, float ay, float bx, float by, float width, SDL_Color c);

/* Icônes, centrées en (cx, cy). scale = 1 pour la taille du HUD (≈ 16 px). */
void draw_metronome(SDL_Renderer *r, float cx, float cy, float scale, float pendulum_angle);
void draw_crown(SDL_Renderer *r, float cx, float cy, float scale);

/* Nombre en chiffres pixel (3x5) ; renvoie la largeur dessinée. */
float draw_number(SDL_Renderer *r, float x, float y, int value, int min_digits, float scale, SDL_Color c);

/* Texte avec la police de debug de SDL (ASCII) : les accents sont retirés. */
void draw_debug_text(SDL_Renderer *r, float x, float y, const char *utf8, float scale, SDL_Color c);
float draw_debug_text_width(const char *utf8, float scale);

#endif
