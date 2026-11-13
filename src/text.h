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
    /* descriptions détaillées des objets (GDD §24), formatées avec les réglages */
    TXT_EID_DAMAGE_ADD,
    TXT_EID_LULLY_FOOT,
    TXT_EID_MAX_HP_ADD,
    TXT_EID_HEALS,
    TXT_EID_FIRE_RATE_ADD,
    TXT_EID_SHOT_SPEED_ADD,
    TXT_EID_RANGE_ADD,
    TXT_EID_SPEED_ADD,
    TXT_EID_ALTIUS_1,
    TXT_EID_ALTIUS_2,
    TXT_EID_EXPECTARE_1,
    TXT_EID_EXPECTARE_2,
    TXT_EID_LIQUESCENCE_1,
    TXT_EID_LIQUESCENCE_2,
    TXT_EID_TROPE,
    TXT_EID_W_DAMAGE,
    TXT_EID_W_FIRE_RATE,
    TXT_EID_W_SHOT_SPEED,
    TXT_EID_W_RANGE,
    TXT_EID_W_SIZE,
    TXT_EID_W_COUNT,
    TXT_EID_W_STUN,
    TXT_EID_W_LOCK,
    TXT_EID_W_CHARGE,
    TXT_EID_W_WAVE,
    TXT_EID_W_RING,
    TXT_EID_W_BEAM,
    TXT_EID_W_BASE,
    TXT_EID_HEAL,
    TXT_EID_PRICE,
    TXT_EID_NO_MONEY,
    TXT_EID_FULL_HP,
    TXT_COUNT
} TextId;

/* Choisit la langue d'après les préférences du système (français par défaut). */
void text_init_from_locale(void);

void text_set_lang(Lang lang);
Lang text_lang(void);

const char *text_get(TextId id);

#endif
