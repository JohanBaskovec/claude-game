#ifndef FUGUE_CAMERA_H
#define FUGUE_CAMERA_H

/*
 * Caméra : coin haut-gauche de l'écran (960x540) en coordonnées du monde.
 * Le monde utilise les coordonnées « page » du GDD : la cellule (cx, cy)
 * de la double page a son sol en (cx*768, cy*360).
 * Position à l'écran = position dans le monde - caméra.
 */
typedef struct Camera {
    float x, y;
} Camera;

#endif
