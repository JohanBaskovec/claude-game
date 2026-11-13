#include "text.h"

#include <SDL3/SDL.h>

static Lang current = LANG_FR;

static const char *const TEXTS[TXT_COUNT][LANG_COUNT] = {
    [TXT_ITEM_LULLY_NAME]  = { "Bâton de Lully", "Lully's Staff" },
    [TXT_ITEM_LULLY_DESC]  = { "Dégâts +1... attention aux pieds", "Damage up... mind your feet" },
    [TXT_ITEM_HAYDN_NAME]  = { "Crâne de Haydn", "Haydn's Skull" },
    [TXT_ITEM_HAYDN_DESC]  = { "PV maximum +2", "Max HP up" },
    [TXT_ITEM_SATIE_NAME]  = { "Vexations", "Vexations" },
    [TXT_ITEM_SATIE_DESC]  = { "Cadence de tir +, 840 fois", "Fire rate up, 840 times" },
    [TXT_ITEM_GUIDO_NAME]  = { "Main guidonienne", "Guidonian Hand" },
    [TXT_ITEM_GUIDO_DESC]  = { "Vitesse et portée des tirs +", "Shot speed and range up" },
    [TXT_ITEM_SABOTS_NAME] = { "Sabots de Strasbourg", "Strasbourg Clogs" },
    [TXT_ITEM_SABOTS_DESC] = { "Vitesse + (on ne peut plus s'arrêter de danser)", "Speed up (you can't stop dancing)" },
    [TXT_ITEM_CELERITER_NAME]   = { "Celeriter", "Celeriter" },
    [TXT_ITEM_CELERITER_DESC]   = { "« Vite » : cadence de tir +", "\"Quickly\": fire rate up" },
    [TXT_ITEM_ALTIUS_NAME]      = { "Altius", "Altius" },
    [TXT_ITEM_ALTIUS_DESC]      = { "« Plus haut » : les notes passent au-dessus des obstacles", "\"Higher\": notes fly over obstacles" },
    [TXT_ITEM_EXPECTARE_NAME]   = { "Expectare", "Expectare" },
    [TXT_ITEM_EXPECTARE_DESC]   = { "« Attendez » : les notes marquent une pause", "\"Wait\": notes pause mid-air" },
    [TXT_ITEM_LIQUESCENCE_NAME] = { "Liquescence", "Liquescence" },
    [TXT_ITEM_LIQUESCENCE_DESC] = { "Les notes fondent en flaques d'encre qui ralentissent", "Notes melt into slowing ink" },
    [TXT_ITEM_TROPE_NAME]       = { "Trope", "Trope" },
    [TXT_ITEM_TROPE_DESC]       = { "Une note de plus à chaque tir", "One more note per shot" },
    [TXT_WEAPON_VOICE_NAME]     = { "Voix", "Voice" },
    [TXT_WEAPON_VOICE_DESC]     = { "Ta bonne vieille voix", "Your good old voice" },
    [TXT_WEAPON_OLIFANT_NAME]   = { "Olifant", "Oliphant" },
    [TXT_WEAPON_OLIFANT_DESC]   = { "Lent, lourd, assourdissant", "Slow, heavy, deafening" },
    [TXT_WEAPON_FLUTE_NAME]     = { "Flûte", "Flute" },
    [TXT_WEAPON_FLUTE_DESC]     = { "Maintenir pour charger, relâcher pour souffler", "Hold to charge, release to blow" },
    [TXT_WEAPON_LYRE_NAME]      = { "Lyre", "Lyre" },
    [TXT_WEAPON_LYRE_DESC]      = { "Trois notes en arc", "Three notes in an arc" },
    [TXT_WEAPON_TABOURIN_NAME]  = { "Tabourin", "Tabor" },
    [TXT_WEAPON_TABOURIN_DESC]  = { "Une onde de choc qui traverse tout", "A shockwave that goes through" },
    [TXT_WEAPON_BELL_NAME]      = { "Cloche d'église", "Church Bell" },
    [TXT_WEAPON_BELL_DESC]      = { "Rare, mais ça résonne", "Rare, but it resonates" },
    [TXT_WEAPON_ORGAN_NAME]     = { "Orgue", "Organ" },
    [TXT_WEAPON_ORGAN_DESC]     = { "Un accord qui traverse tout... et cloue sur place", "A chord that pierces all... and roots you" },
    [TXT_SHOP_HEAL_NAME]        = { "Soin", "Healing" },
    [TXT_SHOP_HEAL_DESC]        = { "+%d PV", "+%d HP" },   /* tuning.shop_heal */
    [TXT_GAME_TITLE]            = { "Amadeus’ Minor Fugue", "Amadeus’ Minor Fugue" },   /* titre français : GDD §2 ❓ */
    [TXT_MENU_PLAY]             = { "Jouer", "Play" },
    [TXT_MENU_QUIT]             = { "Quitter", "Quit" },
    [TXT_ERA_1]                 = { "1012 après J.-C.", "AD 1012" },   /* titre d'époque du premier étage (GDD §23) */
};

void text_init_from_locale(void)
{
    int count = 0;
    SDL_Locale **locales = SDL_GetPreferredLocales(&count);
    if (locales && count > 0 && locales[0]->language) {
        current = SDL_strncasecmp(locales[0]->language, "fr", 2) == 0 ? LANG_FR : LANG_EN;
    }
    SDL_free(locales);
}

void text_set_lang(Lang lang)
{
    if (lang >= 0 && lang < LANG_COUNT) current = lang;
}

Lang text_lang(void)
{
    return current;
}

const char *text_get(TextId id)
{
    if (id < 0 || id >= TXT_COUNT || !TEXTS[id][current]) return "?";
    return TEXTS[id][current];
}
