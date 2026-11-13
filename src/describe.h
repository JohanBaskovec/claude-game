#ifndef FUGUE_DESCRIBE_H
#define FUGUE_DESCRIBE_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "config.h"
#include "item.h"

/*
 * Descriptions détaillées des objets posés (GDD §24), comme le mod External Item Descriptions
 * de TBOI : nom et effet exact, chiffres tirés des réglages (tuning), prix au magasin.
 */
#define DESC_MAX_LINES 7
#define DESC_LINE_LEN  96

typedef enum DescIcon { DESC_ICON_NONE, DESC_ICON_ITEM, DESC_ICON_WEAPON, DESC_ICON_HEAL } DescIcon;

typedef struct Description {
    const char *title;
    char        lines[DESC_MAX_LINES][DESC_LINE_LEN];
    int         count;
    DescIcon    icon;
    int         icon_id;        /* ItemId ou WeaponId */
    int         warning;        /* indice de la ligne écrite en rouge (prix trop cher...), -1 si aucune */
} Description;

void describe_item(Description *d, ItemId id);
void describe_weapon(Description *d, WeaponId id);
void describe_heal(Description *d);

/* Ajoute le prix (et une ligne rouge si on ne peut pas acheter : pas assez de royalties, vie pleine). */
void describe_price(Description *d, int price, int coins, bool full_hp);

/* Cadre en bas à gauche de l'écran. */
void describe_render(const Description *d, SDL_Renderer *renderer);

#endif
