#ifndef FUGUE_ENEMY_H
#define FUGUE_ENEMY_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "config.h"
#include "room.h"

typedef enum EnemyType {
    ENEMY_BABY,   /* bébé médiéval : suit le joueur, blesse au contact (GDD §4) */
} EnemyType;

typedef struct Enemy {
    bool      active;
    EnemyType type;
    SDL_FRect box;            /* boîte de collision (les pieds) */
    float     prev_x, prev_y; /* position au pas précédent, pour l'interpolation */
    float     vx, vy;         /* px/s */
    float     hp;             /* flottant : les dégâts des notes peuvent être fractionnaires */
    float     hurt_timer;     /* > 0 : clignote après une touche */
    float     anim_time;      /* pour le dandinement */
} Enemy;

typedef struct EnemyPool {
    Enemy items[MAX_ENEMIES];
} EnemyPool;

void enemy_pool_clear(EnemyPool *pool);

/* Fait apparaître un bébé centré en (x, y). Renvoie NULL si le pool est plein. */
Enemy *enemy_spawn_baby(EnemyPool *pool, float x, float y);

/* Fait apparaître un bébé à un endroit libre du sol, loin de (avoid_x, avoid_y). */
Enemy *enemy_spawn_baby_away_from(EnemyPool *pool, const Room *room, float avoid_x, float avoid_y);

/* Déplacements : chaque ennemi se dirige vers la cible (le joueur). */
void enemy_update_all(EnemyPool *pool, const Room *room, float target_x, float target_y, float dt);

/* Retire des PV et pousse l'ennemi dans la direction (kx, ky). Renvoie true s'il meurt. */
bool enemy_hurt(Enemy *e, float damage, float kx, float ky);

int enemy_count(const EnemyPool *pool);

/* Bas de la boîte de collision, pour trier l'affichage par profondeur. */
float enemy_feet_y(const Enemy *e, float alpha);

void enemy_render(const Enemy *e, SDL_Renderer *renderer, float alpha, Camera cam);

#endif
