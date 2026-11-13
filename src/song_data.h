/* Chansons pop originales d'Amadeus (GDD §26), composées pour le jeu (paroles : clichés pop anglais).
 * Générées par docs/concept/outils_concept/make_songs.py (degrés de la gamme majeure) : modifier le script, pas ce fichier. */
#ifndef FUGUE_SONG_DATA_H
#define FUGUE_SONG_DATA_H

#include "chant_data.h"   /* ChantSyllable, Chant */

static const ChantSyllable SONG_YEAH_YEAH_BABY[] = {
    { "oh", 'o', "igf" }, { "ba", 'a', "b" }, { "by", 'i', "f" }, { "yeah", 'e', "i" }, { "yeah", 'e', "i" }, { "yeah", 'e', "ki" },
    { "ba", 'a', "f" }, { "by", 'i', "f" }, { "you", 'u', "i" }, { "know", 'o', "gf" }, { "I", 'a', "b" }, { "got", 'o', "f" },
    { "the", 'e', "f" }, { "love", 'a', "i" }, { "oh", 'o', "k" }, { "oh", 'o', "i" }, { "ba", 'a', "f" }, { "by", 'i', "d" },
    { "yeah", 'e', "bab" },
};

static const ChantSyllable SONG_TONIGHT[] = {
    { "to", 'u', "h" }, { "night", 'a', "m" }, { "to", 'u', "h" }, { "night", 'a', "m" }, { "we're", 'i', "j" }, { "gon", 'o', "h" },
    { "na", 'a', "e" }, { "shine", 'a', "he" }, { "to", 'u', "h" }, { "night", 'a', "m" }, { "oh", 'o', "qm" }, { "oh", 'o', "jh" },
    { "we", 'i', "j" }, { "gon", 'o', "h" }, { "na", 'a', "e" }, { "fly", 'a', "ae" }, { "to", 'u', "e" }, { "night", 'a', "a" },
};

static const ChantSyllable SONG_NA_NA_SUMMER[] = {
    { "na", 'a', "a" }, { "na", 'a', "a" }, { "na", 'a', "e" }, { "na", 'a', "h" }, { "sum", 'a', "j" }, { "mer", 'e', "h" },
    { "is", 'i', "e" }, { "here", 'i', "h" }, { "na", 'a', "a" }, { "na", 'a', "a" }, { "na", 'a', "e" }, { "na", 'a', "h" },
    { "dan", 'a', "j" }, { "cing", 'i', "h" }, { "all", 'o', "e" }, { "year", 'i', "ca" }, { "hey", 'e', "m" },
};

static const ChantSyllable SONG_HEARTBEAT[] = {
    { "my", 'a', "e" }, { "heart", 'a', "h" }, { "goes", 'o', "h" }, { "boom", 'u', "e" }, { "boom", 'u', "a" }, { "boom", 'u', "e" },
    { "you", 'u', "h" }, { "make", 'e', "j" }, { "me", 'i', "h" }, { "move", 'u', "eh" }, { "oh", 'o', "jh" }, { "yeah", 'e', "eca" },
    { "my", 'a', "e" }, { "heart", 'a', "h" }, { "goes", 'o', "h" }, { "boom", 'u', "a" },
};

static const ChantSyllable SONG_FOREVER[] = {
    { "ooh", 'u', "nmi" }, { "for", 'o', "f" }, { "ev", 'e', "i" }, { "er", 'e', "k" }, { "you", 'u', "i" }, { "and", 'a', "f" },
    { "me", 'i', "i" }, { "ooh", 'u', "nrn" }, { "for", 'o', "f" }, { "ev", 'e', "i" }, { "er", 'e', "k" }, { "ba", 'a', "i" },
    { "by", 'i', "f" }, { "can't", 'a', "d" }, { "you", 'u', "f" }, { "see", 'i', "bab" },
};

static const ChantSyllable SONG_HANDS_UP[] = {
    { "put", 'u', "d" }, { "your", 'o', "d" }, { "hands", 'a', "d" }, { "up", 'a', "a" }, { "hands", 'a', "d" }, { "up", 'a', "a" },
    { "eh", 'e', "f" }, { "oh", 'o', "da" }, { "put", 'u', "d" }, { "your", 'o', "d" }, { "hands", 'a', "d" }, { "up", 'a', "a" },
    { "ev", 'e', "a" }, { "ery", 'i', "d" }, { "bo", 'o', "f" }, { "dy", 'i', "d" }, { "eh", 'e', "fi" }, { "oh", 'o', "d" },
};

static const Chant SONGS[] = {
    { "Yeah Yeah Baby", SONG_YEAH_YEAH_BABY, (int)(sizeof SONG_YEAH_YEAH_BABY / sizeof SONG_YEAH_YEAH_BABY[0]) },
    { "Tonight", SONG_TONIGHT, (int)(sizeof SONG_TONIGHT / sizeof SONG_TONIGHT[0]) },
    { "Na Na Summer", SONG_NA_NA_SUMMER, (int)(sizeof SONG_NA_NA_SUMMER / sizeof SONG_NA_NA_SUMMER[0]) },
    { "Heartbeat", SONG_HEARTBEAT, (int)(sizeof SONG_HEARTBEAT / sizeof SONG_HEARTBEAT[0]) },
    { "Forever", SONG_FOREVER, (int)(sizeof SONG_FOREVER / sizeof SONG_FOREVER[0]) },
    { "Hands Up", SONG_HANDS_UP, (int)(sizeof SONG_HANDS_UP / sizeof SONG_HANDS_UP[0]) },
};
#define SONG_COUNT ((int)(sizeof SONGS / sizeof SONGS[0]))

#endif
