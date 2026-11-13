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
    [TXT_EID_DAMAGE_ADD]      = { "+%s dégâts", "+%s damage" },
    [TXT_EID_LULLY_FOOT]      = { "1 tir sur %d part dans ton pied : −%d PV", "1 shot in %d hits your foot: −%d HP" },
    [TXT_EID_MAX_HP_ADD]      = { "+%d PV maximum", "+%d max HP" },
    [TXT_EID_HEALS]           = { "Soigne aussi %d PV", "Also heals %d HP" },
    [TXT_EID_FIRE_RATE_ADD]   = { "+%s tirs par seconde", "+%s shots per second" },
    [TXT_EID_SHOT_SPEED_ADD]  = { "Vitesse des tirs +%s px/s", "Shot speed +%s px/s" },
    [TXT_EID_RANGE_ADD]       = { "Portée des tirs +%s px", "Shot range +%s px" },
    [TXT_EID_SPEED_ADD]       = { "Vitesse de déplacement +%s px/s", "Move speed +%s px/s" },
    [TXT_EID_ALTIUS_1]        = { "Les notes volent plus haut", "Notes fly higher" },
    [TXT_EID_ALTIUS_2]        = { "et passent au-dessus des obstacles", "and pass over obstacles" },
    [TXT_EID_EXPECTARE_1]     = { "Les notes s'arrêtent %s s en l'air", "Notes stop in mid-air for %s s" },
    [TXT_EID_EXPECTARE_2]     = { "après %d %% de leur course", "after %d%% of their flight" },
    [TXT_EID_LIQUESCENCE_1]   = { "Les notes finissent en flaque d'encre", "Notes end as an ink puddle" },
    [TXT_EID_LIQUESCENCE_2]   = { "Ennemis ralentis à %d %% pendant %s s", "Enemies slowed to %d%% for %s s" },
    [TXT_EID_TROPE]           = { "+1 note par tir, en éventail", "+1 note per shot, fanned out" },
    [TXT_EID_W_DAMAGE]        = { "Dégâts ×%s", "Damage ×%s" },
    [TXT_EID_W_FIRE_RATE]     = { "Cadence ×%s", "Fire rate ×%s" },
    [TXT_EID_W_SHOT_SPEED]    = { "Vitesse des tirs ×%s", "Shot speed ×%s" },
    [TXT_EID_W_RANGE]         = { "Portée ×%s", "Range ×%s" },
    [TXT_EID_W_SIZE]          = { "Notes ×%s plus grosses", "Notes ×%s bigger" },
    [TXT_EID_W_COUNT]         = { "%d notes en éventail", "%d notes in a fan" },
    [TXT_EID_W_STUN]          = { "Étourdit %s s", "Stuns for %s s" },
    [TXT_EID_W_LOCK]          = { "Te cloue sur place %s s à chaque tir", "Roots you for %s s per shot" },
    [TXT_EID_W_CHARGE]        = { "Maintenir pour charger (%s s), relâcher", "Hold to charge (%s s), release" },
    [TXT_EID_W_WAVE]          = { "Onde de choc qui traverse les ennemis", "Shockwave that goes through enemies" },
    [TXT_EID_W_RING]          = { "Onde circulaire tout autour de toi", "Circular wave all around you" },
    [TXT_EID_W_BEAM]          = { "Rayon jusqu'au mur, traverse tout", "Beam to the wall, pierces everything" },
    [TXT_EID_W_BASE]          = { "Le tir de départ", "The starting shot" },
    [TXT_EID_HEAL]            = { "+%d PV", "+%d HP" },
    [TXT_EID_PRICE]           = { "Prix : %d royalties", "Price: %d royalties" },
    [TXT_EID_NO_MONEY]        = { "Pas assez de royalties", "Not enough royalties" },
    [TXT_EID_FULL_HP]         = { "Ta vie est déjà pleine", "Your health is already full" },
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
