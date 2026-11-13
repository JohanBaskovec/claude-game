#ifndef FUGUE_FONT_H
#define FUGUE_FONT_H

#include <SDL3/SDL.h>
#include <stdbool.h>

/*
 * Police du sol et de l'interface : Junicode (GDD §9, « Police du sol »), rendue avec stb_truetype.
 * Les glyphes sont rastérisés une fois au chargement ; le texte est ensuite
 * écrit dans des tampons de couverture 8 bits (le décor les teinte en encre).
 */
typedef enum FontId {
    FONT_TEXT,     /* texte latin : Medium 22 px */
    FONT_RUBRIC,   /* rubriques : Medium 13 px */
    FONT_VERSAL,   /* initiales de chant : Bold 40 px */
    FONT_CAP,      /* grande lettrine : Bold 92 px */
    FONT_TITLE,    /* titres de l'interface (menu, époque) : Bold 60 px */
    FONT_UI,       /* boutons de l'interface : Medium 30 px */
    FONT_LABEL,    /* titres des petits cadres (descriptions) : Bold 21 px */
    FONT_SMALL,    /* texte des petits cadres : Medium 17 px */
    FONT_COUNT
} FontId;

/* Charge assets/fonts/Junicode-*.ttf à côté de l'exécutable. Renvoie false si absents. */
bool font_init(void);
void font_quit(void);
bool font_ready(void);

/* Hauteur au-dessus de la ligne de base, et hauteur d'une ligne (ascendante + descendante), en px. */
float font_ascent(FontId f);
float font_line_height(FontId f);

/* Largeur du texte UTF-8 (avances et crénage), en px. */
float font_text_width(FontId f, const char *utf8);

/*
 * Écrit le texte dans le tampon `buf` (w x h, un octet par pixel, on garde le maximum).
 * (x, top) : coin haut-gauche de la ligne, comme Pillow (la ligne de base est à top + ascendante).
 */
void font_draw(FontId f, const char *utf8, float x, float top, Uint8 *buf, int w, int h);

#endif
