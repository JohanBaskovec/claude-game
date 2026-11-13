#ifndef FUGUE_MENU_H
#define FUGUE_MENU_H

#include <SDL3/SDL.h>

/*
 * Menu principal (GDD §23) : le titre du jeu, et les boutons Jouer et Quitter
 * (pas de Quitter dans le navigateur, qui ne peut pas fermer l'onglet).
 * Souris (survol, clic) et clavier (haut / bas, Entrée ou Espace, Échap).
 */
typedef enum MenuChoice { MENU_NONE, MENU_PLAY, MENU_QUIT } MenuChoice;

typedef struct Menu {
    int   selected;   /* bouton en surbrillance */
    float time;       /* pour l'animation, s */
} Menu;

void menu_init(Menu *m);
void menu_update(Menu *m, float dt);

/* Les événements de souris doivent déjà être en coordonnées du jeu. Renvoie le bouton activé. */
MenuChoice menu_handle_event(Menu *m, const SDL_Event *event);

/* Par-dessus la salle de fond : voile, titre, boutons. */
void menu_render(const Menu *m, SDL_Renderer *renderer);

#endif
