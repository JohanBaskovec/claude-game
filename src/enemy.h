#ifndef FUGUE_ENEMY_H
#define FUGUE_ENEMY_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "config.h"
#include "room.h"

/* États du Gros Bébé. */
typedef enum BossState {
    BOSS_WANDER,    /* marche vers un point au hasard, s'arrête, recommence */
    BOSS_CROUCH,    /* s'accroupit avant de sauter */
    BOSS_AIR,       /* en l'air : intouchable, son ombre grandit là où il va tomber */
    BOSS_RECOVER,   /* se relève après l'atterrissage */
} BossState;

/* États du ver de livre (GDD §16). */
typedef enum WormState {
    WORM_HIDDEN,    /* sous le sol : invisible et intouchable */
    WORM_WARN,      /* le sol se soulève et se fissure là où il va sortir */
    WORM_EMERGE,    /* il jaillit */
    WORM_UP,        /* dehors : on peut le toucher */
    WORM_DIVE,      /* il replonge */
} WormState;

/* États de la plume et du sonneur : ils se déplacent, puis s'arrêtent pour tirer. */
typedef enum ShooterState {
    SHOOTER_MOVE,
    SHOOTER_WINDUP, /* prévient avant de tirer (plume penchée, cloche qui se balance) */
} ShooterState;

typedef struct Enemy {
    bool      active;
    EnemyType type;
    SDL_FRect box;            /* boîte de collision (les pieds) */
    float     prev_x, prev_y; /* position au pas précédent, pour l'interpolation */
    float     vx, vy;         /* px/s */
    float     hp;             /* flottant : les dégâts des notes peuvent être fractionnaires */
    float     hurt_timer;     /* > 0 : clignote après une touche */
    float     anim_time;      /* pour le dandinement */
    bool      face_left;      /* regarde où il veut aller (pas où une poussée l'envoie) */
    float     max_hp;
    float     stun_timer;     /* > 0 : étourdi, ne bouge plus */
    float     slow_timer;     /* > 0 : ralenti (encre) */

    /* Comportement */
    int       state;          /* BossState, WormState ou ShooterState selon le type */
    float     action_timer;   /* s avant la prochaine rafale, le prochain crachat ou coup de cloche */
    int       burst_left;     /* moine : notes restantes de la rafale */
    float     sing_timer;     /* > 0 : bouche ouverte (moine), cloche qui sonne (sonneur) */
    bool      fire;           /* vrai pendant le pas où il tire, lu par game.c */
    bool      dig;            /* ver : vrai pendant le pas où il commence à creuser vers la surface */
    float     cry_timer;      /* bébés : s avant de pleurer */
    bool      cry;            /* bébés : vrai pendant le pas où il se met à pleurer, lu par game.c */

    /* Boss */
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

/* Fait apparaître un ennemi centré en (x, y). Renvoie NULL si le pool est plein. */
Enemy *enemy_spawn(EnemyPool *pool, EnemyType type, float x, float y);

/* Fait apparaître un ennemi à un endroit libre du sol, loin de (avoid_x, avoid_y). */
Enemy *enemy_spawn_away_from(EnemyPool *pool, const Room *room, EnemyType type, float avoid_x, float avoid_y);

/* Déplacements et décisions ; la cible est le joueur. Un ennemi qui tire met `fire` à vrai. */
void enemy_update_all(EnemyPool *pool, const Room *room, float target_x, float target_y, float dt);

/* Point d'où partent ses tirs, au sol. */
void enemy_center(const Enemy *e, float *x, float *y);

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
