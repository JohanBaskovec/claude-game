#ifndef FUGUE_WEAPON_H
#define FUGUE_WEAPON_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "config.h"
#include "item.h"
#include "player.h"

/*
 * Armes (GDD §4) : attribut final = (départ + objets) × multiplicateur de l'arme.
 * Les objets qui changent la forme des tirs (Trope, Altius…) s'ajoutent ici aussi.
 */

/* Tout ce qu'il faut pour lancer un tir. */
typedef struct ShotSpec {
    ShotKind kind;
    float    damage;
    float    speed;     /* px/s */
    float    range;     /* px (rayon de l'onde pour la cloche) */
    float    size;      /* taille de la note (1 = voix) */
    float    stun;      /* s */
    float    spread;    /* angle total de l'éventail, rad */
    int      count;     /* notes par tir */
    bool     spectral;  /* passe au-dessus des obstacles */
    bool     pause;     /* s'arrête un instant en l'air */
    bool     ink;       /* laisse une flaque d'encre */
} ShotSpec;

const char *weapon_name(WeaponId id);
const char *weapon_desc(WeaponId id);

/* Notes par seconde avec l'arme. */
float weapon_fire_rate(const PlayerStats *st);
/* Temps entre deux tirs, s, et la valeur de note qui lui correspond (la plus proche en durée). */
float     weapon_shot_interval(const PlayerStats *st);
NoteValue weapon_note_value(const PlayerStats *st);

/* Temps d'une charge complète (flûte), 0 si l'arme ne se charge pas. */
float weapon_charge_time(const PlayerStats *st);

/* charge : 0..1 pour la flûte, ignoré sinon. */
ShotSpec weapon_shot(const PlayerStats *st, const bool owned[ITEM_COUNT], float charge);

/* Icône de l'arme centrée en (cx, cy), coordonnées écran ; scale 1 ≈ 32 px. */
void weapon_render_icon(WeaponId id, SDL_Renderer *r, float cx, float cy, float scale);

#endif
