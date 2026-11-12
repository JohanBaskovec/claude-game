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
