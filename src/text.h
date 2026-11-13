#ifndef FUGUE_TEXT_H
#define FUGUE_TEXT_H

/*
 * Textes affichés au joueur, en français et en anglais (le jeu est bilingue).
 * Aucun texte affiché ne doit être écrit en dur ailleurs : ajouter une entrée ici.
 * (Les outils de debug F1 / F2 ne sont pas traduits.)
 */
typedef enum Lang { LANG_FR, LANG_EN, LANG_COUNT } Lang;

typedef enum TextId {
    TXT_ITEM_LULLY_NAME,
    TXT_ITEM_LULLY_DESC,
    TXT_ITEM_HAYDN_NAME,
    TXT_ITEM_HAYDN_DESC,
    TXT_ITEM_SATIE_NAME,
    TXT_ITEM_SATIE_DESC,
    TXT_ITEM_GUIDO_NAME,
    TXT_ITEM_GUIDO_DESC,
    TXT_ITEM_SABOTS_NAME,
    TXT_ITEM_SABOTS_DESC,
    TXT_ITEM_CELERITER_NAME,
    TXT_ITEM_CELERITER_DESC,
    TXT_ITEM_ALTIUS_NAME,
    TXT_ITEM_ALTIUS_DESC,
    TXT_ITEM_EXPECTARE_NAME,
    TXT_ITEM_EXPECTARE_DESC,
    TXT_ITEM_LIQUESCENCE_NAME,
    TXT_ITEM_LIQUESCENCE_DESC,
    TXT_ITEM_TROPE_NAME,
    TXT_ITEM_TROPE_DESC,
    TXT_WEAPON_VOICE_NAME,
    TXT_WEAPON_VOICE_DESC,
    TXT_WEAPON_OLIFANT_NAME,
    TXT_WEAPON_OLIFANT_DESC,
    TXT_WEAPON_FLUTE_NAME,
    TXT_WEAPON_FLUTE_DESC,
    TXT_WEAPON_LYRE_NAME,
    TXT_WEAPON_LYRE_DESC,
    TXT_WEAPON_TABOURIN_NAME,
    TXT_WEAPON_TABOURIN_DESC,
    TXT_WEAPON_BELL_NAME,
    TXT_WEAPON_BELL_DESC,
    TXT_WEAPON_ORGAN_NAME,
    TXT_WEAPON_ORGAN_DESC,
    TXT_SHOP_HEAL_NAME,
    TXT_SHOP_HEAL_DESC,
    TXT_GAME_TITLE,
    TXT_MENU_PLAY,
    TXT_MENU_QUIT,
    TXT_ERA_1,
    TXT_COUNT
} TextId;

/* Choisit la langue d'après les préférences du système (français par défaut). */
void text_init_from_locale(void);

void text_set_lang(Lang lang);
Lang text_lang(void);

const char *text_get(TextId id);

#endif
