/* Généré depuis les fichiers GABC de GregoBase (outil jetable hors du dépôt). Ne pas modifier à la main. */
#ifndef FUGUE_CHANT_DATA_H
#define FUGUE_CHANT_DATA_H

/* Une syllabe : texte, voyelle chantée, et ses notes, une lettre par note :
 * 'a' = la note la plus grave du chant, 'b' un demi-ton au-dessus, etc. */
typedef struct ChantSyllable {
    const char *text;
    char        vowel;
    const char *notes;
} ChantSyllable;

/* Ut queant laxis (Liber Usualis 1961, p. 1504) */
static const ChantSyllable CHANT_UT_QUEANT[] = {
    { "ut", 'u', "a" }, { "que", 'e', "c" }, { "ant", 'a', "f" }, { "la", 'a', "ce" }, { "xis", 'i', "c" },
    { "re", 'e', "c" }, { "so", 'o', "c" }, { "ná", 'a', "a" }, { "re", 'e', "c" }, { "fi", 'i', "e" },
    { "bris", 'i', "e" }, { "mi", 'i', "efh" }, { "ra", 'a', "e" }, { "ge", 'e', "c" }, { "stó", 'o', "ea" },
    { "rum", 'u', "c" }, { "fá", 'a', "f" }, { "mu", 'u', "h" }, { "li", 'i', "j" }, { "tu", 'u', "h" },
    { "ó", 'o', "fc" }, { "rum", 'u', "c" }, { "sol", 'o', "hjh" }, { "ve", 'e', "fe" }, { "pol", 'o', "f" },
    { "lú", 'u', "h" }, { "ti", 'i', "c" }, { "lá", 'a', "j" }, { "bi", 'i', "h" }, { "i", 'i', "j" },
    { "re", 'e', "fh" }, { "á", 'a', "j" }, { "tum", 'u', "j" }, { "san", 'a', "hf" }, { "cte", 'e', "ec" },
    { "jo", 'o', "a" }, { "án", 'a', "e" }, { "nes", 'e', "c" },
};

/* Veni Creator Spiritus (Liber Usualis 1961, p. 885) */
static const ChantSyllable CHANT_VENI_CREATOR[] = {
    { "ve", 'e', "c" }, { "ni", 'i', "e" }, { "cre", 'e', "ca" }, { "á", 'a', "c" }, { "tor", 'o', "ec" },
    { "spí", 'i', "h" }, { "ri", 'i', "j" }, { "tus", 'u', "h" }, { "men", 'e', "h" }, { "tes", 'e', "c" },
    { "tu", 'u', "e" }, { "ó", 'o', "h" }, { "rum", 'u', "jh" }, { "ví", 'i', "j" }, { "si", 'i', "l" },
    { "ta", 'a', "j" }, { "im", 'i', "h" }, { "ple", 'e', "jl" }, { "su", 'u', "hg" }, { "pér", 'e', "ec" },
    { "na", 'a', "hj" }, { "grá", 'a', "c" }, { "ti", 'i', "e" }, { "a", 'a', "h" }, { "quae", 'e', "gh" },
    { "tu", 'u', "e" }, { "cre", 'e', "ca" }, { "á", 'a', "e" }, { "sti", 'i', "ege" }, { "pé", 'e', "c" },
    { "cto", 'o', "a" }, { "ra", 'a', "c" },
};

/* Ave maris stella (Liber Usualis 1961, p. 1259) */
static const ChantSyllable CHANT_AVE_MARIS[] = {
    { "a", 'a', "c" }, { "ve", 'e', "jl" }, { "ma", 'a', "h" }, { "ris", 'i', "jl" }, { "stel", 'e', "o" },
    { "la", 'a', "mljhj" }, { "de", 'e', "j" }, { "i", 'i', "j" }, { "ma", 'a', "ce" }, { "ter", 'e', "h" },
    { "al", 'a', "fe" }, { "ma", 'a', "c" }, { "at", 'a', "f" }, { "que", 'e', "e" }, { "sem", 'e', "h" },
    { "per", 'e', "j" }, { "vir", 'i', "j" }, { "go", 'o', "cefeca" }, { "fe", 'e', "e" }, { "lix", 'i', "h" },
    { "cae", 'e', "e" }, { "li", 'i', "f" }, { "por", 'o', "e" }, { "ta", 'a', "c" },
};

/* Pange lingua (Liber Usualis 1961, p. 957) */
static const ChantSyllable CHANT_PANGE_LINGUA[] = {
    { "pan", 'a', "c" }, { "ge", 'e', "c" }, { "lin", 'i', "d" }, { "gua", 'a', "ca" }, { "glo", 'o', "f" },
    { "ri", 'i', "f" }, { "ó", 'o', "hk" }, { "si", 'i', "k" }, { "cór", 'o', "km" }, { "po", 'o', "k" },
    { "ris", 'i', "k" }, { "my", 'i', "j" }, { "sté", 'e', "h" }, { "ri", 'i', "k" }, { "um", 'u', "jhf" },
    { "san", 'a', "f" }, { "gui", 'i', "h" }, { "nís", 'i', "k" }, { "que", 'e', "j" }, { "pre", 'e', "h" },
    { "ti", 'i', "f" }, { "ó", 'o', "h" }, { "si", 'i', "f" }, { "quem", 'e', "h" }, { "in", 'i', "j" },
    { "mun", 'u', "f" }, { "di", 'i', "f" }, { "pré", 'e', "c" }, { "ti", 'i', "h" }, { "um", 'u', "ha" },
    { "fru", 'u', "c" }, { "ctus", 'u', "f" }, { "ven", 'e', "f" }, { "tris", 'i', "c" }, { "ge", 'e', "f" },
    { "ne", 'e', "h" }, { "ró", 'o', "h" }, { "si", 'i', "f" }, { "rex", 'e', "h" }, { "ef", 'e', "j" },
    { "fú", 'u', "f" }, { "dit", 'i', "hf" }, { "gén", 'e', "dc" }, { "ti", 'i', "a" }, { "um", 'u', "c" },
};

/* Dies irae (Liber Usualis 1961, p. 1810) */
static const ChantSyllable CHANT_DIES_IRAE[] = {
    { "di", 'i', "i" }, { "es", 'e', "h" }, { "i", 'i', "i" }, { "rae", 'e', "f" }, { "di", 'i', "h" },
    { "es", 'e', "d" }, { "il", 'i', "f" }, { "la", 'a', "f" }, { "sol", 'o', "i" }, { "vet", 'e', "ik" },
    { "sae", 'e', "ih" }, { "clum", 'u', "fd" }, { "in", 'i', "h" }, { "fa", 'a', "i" }, { "víl", 'i', "h" },
    { "la", 'a', "f" }, { "te", 'e', "a" }, { "ste", 'e', "df" }, { "da", 'a', "f" }, { "vid", 'i', "fd" },
    { "cum", 'u', "h" }, { "si", 'i', "i" }, { "býl", 'i', "h" }, { "la", 'a', "f" }, { "quan", 'a', "i" },
    { "tus", 'u', "h" }, { "tre", 'e', "i" }, { "mor", 'o', "f" }, { "est", 'e', "h" }, { "fu", 'u', "d" },
    { "tú", 'u', "f" }, { "rus", 'u', "f" }, { "quan", 'a', "i" }, { "do", 'o', "ik" }, { "ju", 'u', "ih" },
    { "dex", 'e', "fd" }, { "est", 'e', "h" }, { "ven", 'e', "i" }, { "tú", 'u', "h" }, { "rus", 'u', "f" },
    { "cun", 'u', "a" }, { "cta", 'a', "df" }, { "stri", 'i', "f" }, { "cte", 'e', "fd" }, { "dis", 'i', "h" },
    { "cus", 'u', "i" }, { "sú", 'u', "h" }, { "rus", 'u', "f" },
};

/* Victimae paschali laudes (Liber Usualis 1961, p. 780) */
static const ChantSyllable CHANT_VICTIMAE[] = {
    { "vi", 'i', "c" }, { "cti", 'i', "a" }, { "mae", 'e', "c" }, { "pa", 'a', "f" }, { "schá", 'a', "h" },
    { "li", 'i', "f" }, { "lau", 'a', "e" }, { "des", 'e', "c" }, { "ím", 'i', "j" }, { "mo", 'o', "h" },
    { "lent", 'e', "e" }, { "chri", 'i', "h" }, { "sti", 'i', "f" }, { "á", 'a', "e" }, { "ni", 'i', "c" },
    { "a", 'a', "j" }, { "gnus", 'u', "m" }, { "red", 'e', "o" }, { "é", 'e', "j" }, { "mit", 'i', "h" },
    { "o", 'o', "j" }, { "ves", 'e', "j" }, { "chri", 'i', "j" }, { "stus", 'u', "h" }, { "ín", 'i', "j" },
    { "no", 'o', "h" }, { "cens", 'e', "f" }, { "pa", 'a', "e" }, { "tri", 'i', "c" }, { "re", 'e', "f" },
    { "con", 'o', "h" }, { "ci", 'i', "c" }, { "li", 'i', "e" }, { "á", 'a', "c" }, { "vit", 'i', "a" },
    { "pec", 'e', "e" }, { "ca", 'a', "f" }, { "tó", 'o', "e" }, { "res", 'e', "c" },
};

typedef struct Chant {
    const char          *name;
    const ChantSyllable *syllables;
    int                  count;
} Chant;

static const Chant CHANTS[] = {
    { "Ut queant laxis", CHANT_UT_QUEANT, (int)(sizeof CHANT_UT_QUEANT / sizeof CHANT_UT_QUEANT[0]) },
    { "Veni Creator Spiritus", CHANT_VENI_CREATOR, (int)(sizeof CHANT_VENI_CREATOR / sizeof CHANT_VENI_CREATOR[0]) },
    { "Ave maris stella", CHANT_AVE_MARIS, (int)(sizeof CHANT_AVE_MARIS / sizeof CHANT_AVE_MARIS[0]) },
    { "Pange lingua", CHANT_PANGE_LINGUA, (int)(sizeof CHANT_PANGE_LINGUA / sizeof CHANT_PANGE_LINGUA[0]) },
    { "Dies irae", CHANT_DIES_IRAE, (int)(sizeof CHANT_DIES_IRAE / sizeof CHANT_DIES_IRAE[0]) },
    { "Victimae paschali laudes", CHANT_VICTIMAE, (int)(sizeof CHANT_VICTIMAE / sizeof CHANT_VICTIMAE[0]) },
};
#define CHANT_COUNT ((int)(sizeof CHANTS / sizeof CHANTS[0]))

#endif
