#ifndef FUGUE_ENEMY_H
#define FUGUE_ENEMY_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "config.h"
#include "room.h"

typedef enum EnemyType {
    ENEMY_BABY,   /* bébé médiéval : suit le joueur, blesse au contact (GDD §4) */
    ENEMY_BOSS,   /* Gros Bébé, boss du premier étage (GDD §13) */
} EnemyType;

/* États du Gros Bébé. */
typedef enum BossState {
    BOSS_WANDER,    /* marche vers un point au hasard, s'arrête, recommence */
    BOSS_CROUCH,    /* s'accroupit avant de sauter */
    BOSS_AIR,       /* en l'air : intouchable, son ombre grandit là où il va tomber */
    BOSS_RECOVER,   /* se relève après l'atterrissage */
} BossState;

typedef struct Enemy {
    bool      active;
    EnemyType type;
    SDL_FRect box;            /* boîte de collision (les pieds) */
    float     prev_x, prev_y; /* position au pas précédent, pour l'interpolation */
    float     vx, vy;         /* px/s */
    float     hp;             /* flottant : les dégâts des notes peuvent être fractionnaires */
    float     hurt_timer;     /* > 0 : clignote après une touche */
    float     anim_time;      /* pour le dandinement */
    float     max_hp;
    float     stun_timer;     /* > 0 : étourdi, ne bouge plus */
    float     slow_timer;     /* > 0 : ralenti (encre) */

    /* Boss */
    BossState state;
    float     state_time;     /* s depuis le début de l'état */
    float     state_dur;      /* durée de l'état (pause, accroupi...), s */
    float     jump_timer;     /* s avant le prochain saut */
    float     target_x, target_y;   /* point visé (errance) */
    float     jump_from_x, jump_from_y; /* point de départ du saut (affichage) */
    bool      landed;         /* vrai pendant le pas de l'atterrissage, lu par game.c */
} Enemy;

typedef struct EnemyPool {
    Enemy items[MAX_ENEMIES];
} EnemyPool;

void enemy_pool_clear(EnemyPool *pool);

/* Fait apparaître un bébé centré en (x, y). Renvoie NULL si le pool est plein. */
Enemy *enemy_spawn_baby(EnemyPool *pool, float x, float y);

/* Fait apparaître le Gros Bébé centré en (x, y). */
Enemy *enemy_spawn_boss(EnemyPool *pool, float x, float y);

/* Fait apparaître un bébé à un endroit libre du sol, loin de (avoid_x, avoid_y). */
Enemy *enemy_spawn_baby_away_from(EnemyPool *pool, const Room *room, float avoid_x, float avoid_y);

/* Déplacements : chaque ennemi se dirige vers la cible (le joueur). */
void enemy_update_all(EnemyPool *pool, const Room *room, float target_x, float target_y, float dt);

/* Faux si l'ennemi ne peut ni toucher ni être touché (boss en l'air). */
bool enemy_tangible(const Enemy *e);

/* PV retirés au joueur au contact. */
int enemy_contact_damage(const Enemy *e);

/* Retire des PV et donne une poussée (kx, ky) en px/s. Renvoie true s'il meurt. */
bool enemy_hurt(Enemy *e, float damage, float kx, float ky);

/* États : étourdi (ne bouge plus), ralenti. Gardent la plus longue durée. */
void enemy_stun(Enemy *e, float seconds);
void enemy_slow(Enemy *e, float seconds);

/* Le boss vivant de la salle, NULL si aucun. */
const Enemy *enemy_find_boss(const EnemyPool *pool);

int enemy_count(const EnemyPool *pool);

/* Bas de la boîte de collision, pour trier l'affichage par profondeur. */
float enemy_feet_y(const Enemy *e, float alpha);

/* Ombres au sol (dessinées avant tous les personnages). */
void enemy_render_shadow(const Enemy *e, SDL_Renderer *renderer, float alpha, Camera cam);
void enemy_render(const Enemy *e, SDL_Renderer *renderer, float alpha, Camera cam);

#endif
