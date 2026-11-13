#include "audio.h"
#include "chant_data.h"
#include "config.h"

#include <math.h>

/*
 * Tous les sons sont calculés par le jeu, sans fichier audio.
 * - Chant d'Amadeus (GDD §19) : chants grégoriens de chant_data.h (relevés sur GregoBase
 *   depuis le Liber Usualis 1961) ; chaque tir chante la syllabe suivante, calculée au
 *   moment du tir. Après une pause, ou à la fin d'un chant, on passe à un autre.
 * - Bruitages (GDD §20) : calculés une fois à l'ouverture, rejoués à la hauteur voulue.
 */

/* Voix d'enfant : le milieu de l'ambitus de chaque chant est placé vers fa5. */
#define MIDDLE_HZ 698.46f

/* Formants (résonances de la bouche) : fréquence, largeur, volume. */
typedef struct Vowel { float f[3], bw[3], amp[3]; } Vowel;
static const Vowel VOWELS[5] = {   /* voix d'enfant */
    /* a */ { { 950.0f, 1500.0f, 3000.0f }, { 120.0f, 140.0f, 200.0f }, { 1.0f, 0.60f, 0.25f } },
    /* e */ { { 500.0f, 2200.0f, 3100.0f }, { 110.0f, 150.0f, 200.0f }, { 1.0f, 0.45f, 0.30f } },
    /* i */ { { 330.0f, 2700.0f, 3400.0f }, { 100.0f, 160.0f, 220.0f }, { 1.0f, 0.35f, 0.30f } },
    /* o */ { { 520.0f, 950.0f, 2900.0f },  { 110.0f, 130.0f, 200.0f }, { 1.0f, 0.70f, 0.15f } },
    /* u */ { { 360.0f, 820.0f, 2700.0f },  { 100.0f, 120.0f, 200.0f }, { 1.0f, 0.50f, 0.10f } },
};
/* Le moine : « o » d'une voix d'homme, sombre. */
static const Vowel MONK_VOWEL = { { 450.0f, 800.0f, 2600.0f }, { 80.0f, 90.0f, 160.0f }, { 1.0f, 0.55f, 0.12f } };
#define MONK_HZ 146.83f   /* ré3 */

#define MAX_SYL_SECONDS 0.45f
#define MAX_SYL_SAMPLES (AUDIO_RATE * 45 / 100)
#define MAX_VOICES      16
#define MAX_HARMONICS   64
#define MIX_CHUNK       256
#define LEAD_SECONDS    0.05f   /* le flux garde tant d'avance : assez pour une image lente, peu de latence */

/* Une voix lit un son (syllabe ou bruitage) ; rate ≠ 1 le rend plus aigu ou plus grave. */
typedef struct Voice {
    const Sint16 *src;
    int           len;
    float         pos;     /* échantillon suivant ; pos >= len : voix libre */
    float         rate;
    float         gain;
    int           sound;   /* Sound joué, -1 pour une syllabe chantée */
} Voice;

static SDL_AudioStream *stream;
static Voice            voices[MAX_VOICES];
static Sint16           chant_buf[MAX_VOICES][MAX_SYL_SAMPLES];   /* syllabe calculée pour chaque voix */
static int              chant;            /* chant en cours */
static int              next_syllable;    /* syllabe chantée au prochain tir */
static float            last_sing = -1e9f;
static Uint64           rng = 0x9E3779B97F4A7C15ull;

/* Bruitages : tous rangés à la suite dans une seule réserve. */
#define SOUND_BANK_SAMPLES (AUDIO_RATE * 14)
#define MAX_SOUND_SAMPLES  (AUDIO_RATE * 2)
static Sint16 sound_bank[SOUND_BANK_SAMPLES];
static int    sound_bank_used;
static int    sound_start[SOUND_COUNT];
static int    sound_len[SOUND_COUNT];

/* --- Voix chantée ---------------------------------------------------------------- */

static int vowel_index(char v)
{
    switch (v) {
    case 'a': return 0;
    case 'e': return 1;
    case 'i': return 2;
    case 'o': return 3;
    default:  return 4;
    }
}

/* Plus grand écart (en demi-tons) entre la note la plus grave et la plus aiguë du chant. */
static int ambitus(int c)
{
    int hi = 0;
    for (int i = 0; i < CHANTS[c].count; i++) {
        for (const char *n = CHANTS[c].syllables[i].notes; *n; n++) hi = SDL_max(hi, *n - 'a');
    }
    return hi;
}

float audio_syllable_pitch(int c, int i, int note)
{
    if (c < 0 || c >= CHANT_COUNT || i < 0 || i >= CHANTS[c].count) return 0.0f;
    const char *notes = CHANTS[c].syllables[i].notes;
    if (note < 0 || note >= (int)SDL_strlen(notes)) return 0.0f;
    float semis = (float)(notes[note] - 'a') - (float)ambitus(c) * 0.5f;
    return MIDDLE_HZ * powf(2.0f, semis / 12.0f);
}

/* Gain de la voyelle à la fréquence f : somme de résonances. */
static float vowel_gain(const Vowel *v, float f)
{
    float g = 0.0f;
    for (int k = 0; k < 3; k++) {
        float x = (f - v->f[k]) / (v->bw[k] * 0.5f);
        g += v->amp[k] / (1.0f + x * x);
    }
    return g;
}

/* Petit générateur de bruit (souffle des consonnes). */
static Uint32 noise_state = 0x12345678u;
static float noise(void)
{
    noise_state ^= noise_state << 13;
    noise_state ^= noise_state >> 17;
    noise_state ^= noise_state << 5;
    return (float)(noise_state & 0xFFFF) / 32768.0f - 1.0f;
}

/* Chante `notes` notes à la suite (Hz), liées, sur la voyelle v, pendant n échantillons.
 * Écrit dans out et renvoie le maximum atteint (pour normaliser). */
static float synth_voice(const float *pitch, int notes, const Vowel *v, float vibrato, bool fricative, float *out, int n)
{
    float dur = (float)n / AUDIO_RATE, note_len = dur / (float)notes;
    float glide = 1.0f - expf(-1.0f / (AUDIO_RATE * 0.012f));
    float f = pitch[0], phase = 0.0f, peak = 1e-6f, prev_noise = 0.0f;
    float amps[MAX_HARMONICS];
    int harmonics = 0;
    for (int k = 0; k < n; k++) {
        float t = (float)k / AUDIO_RATE;
        int note = SDL_min((int)(t / note_len), notes - 1);
        f += (pitch[note] - f) * glide;
        float vib = 1.0f + vibrato * sinf(2.0f * SDL_PI_F * 5.5f * t) * SDL_clamp((t - 0.06f) / 0.08f, 0.0f, 1.0f);
        float f0 = f * vib;
        /* Volume des harmoniques : pente naturelle de la voix × résonances de la voyelle. */
        if (k % 32 == 0) {
            harmonics = 0;
            for (int h = 1; h <= MAX_HARMONICS && (float)h * f0 < AUDIO_RATE * 0.5f - 600.0f; h++) {
                amps[harmonics++] = vowel_gain(v, (float)h * f0) / (float)h;
            }
        }
        phase += f0 / AUDIO_RATE;
        phase -= floorf(phase);
        /* Somme des harmoniques sin(h·θ) par récurrence : un seul sinus et un cosinus par échantillon. */
        float th = 2.0f * SDL_PI_F * phase, c1 = 2.0f * cosf(th);
        float s_prev = 0.0f, s_cur = sinf(th), x = 0.0f;
        for (int h = 0; h < harmonics; h++) {
            x += amps[h] * s_cur;
            float s_next = c1 * s_cur - s_prev;
            s_prev = s_cur;
            s_cur = s_next;
        }
        /* Consonne : petit souffle aigu au début. */
        if (fricative && t < 0.03f) {
            float w = noise(), hp = w - prev_noise;
            prev_noise = w;
            x += hp * 0.35f * (1.0f - t / 0.03f);
        }
        float env = SDL_min(t / 0.012f, 1.0f) * SDL_min((dur - t) / 0.06f, 1.0f) * (1.0f - 0.25f * t / dur);
        out[k] = x * SDL_max(env, 0.0f);
        peak = SDL_max(peak, fabsf(out[k]));
    }
    return peak;
}

int audio_render_syllable(int c, int i, Sint16 *out, int max)
{
    static float buf[MAX_SYL_SAMPLES];
    if (c < 0 || c >= CHANT_COUNT || i < 0 || i >= CHANTS[c].count) return 0;
    const ChantSyllable *s = &CHANTS[c].syllables[i];
    int notes = SDL_min((int)SDL_strlen(s->notes), 16);
    float pitch[16];
    for (int k = 0; k < notes; k++) pitch[k] = audio_syllable_pitch(c, i, k);
    float dur = SDL_min(0.19f + 0.06f * (float)(notes - 1), MAX_SYL_SECONDS);
    int n = SDL_min(SDL_min((int)(dur * AUDIO_RATE), MAX_SYL_SAMPLES), max);
    char first = (char)(s->text[0] | 0x20);
    bool fricative = first == 's' || first == 'x' || first == 't' || first == 'c' || first == 'f';
    float peak = synth_voice(pitch, notes, &VOWELS[vowel_index(s->vowel)], 0.008f, fricative, buf, n);
    for (int k = 0; k < n; k++) out[k] = (Sint16)(buf[k] / peak * 0.85f * 32767.0f);
    return n;
}

/* --- Bruitages ------------------------------------------------------------------- */

static float sfx[MAX_SOUND_SAMPLES];

/* Petit générateur à graine fixe : les bruitages sont les mêmes à chaque lancement. */
static Uint32 sfx_seed;
static float sfx_rand(void)
{
    sfx_seed = sfx_seed * 1664525u + 1013904223u;
    return (float)(sfx_seed >> 8) / 16777216.0f;
}

static void sfx_begin(Uint32 seed)
{
    SDL_memset(sfx, 0, sizeof sfx);
    sfx_seed = seed;
}

/* Ajoute une résonance amortie (un mode de vibration du bois ou du métal) à partir de `start` s. */
static void add_partial(float start, float freq, float amp, float decay)
{
    int k0 = (int)(start * AUDIO_RATE);
    for (int k = k0; k < MAX_SOUND_SAMPLES; k++) {
        float t = (float)(k - k0) / AUDIO_RATE;
        float e = expf(-t / decay);
        if (e < 1e-3f) break;
        sfx[k] += amp * e * sinf(2.0f * SDL_PI_F * freq * t) * SDL_min(t / 0.0008f, 1.0f);
    }
}

/* Ajoute un souffle bref (l'attaque d'un choc) : bruit filtré, `bright` entre 0 (sourd) et 1. */
static void add_noise(float start, float amp, float decay, float bright)
{
    int k0 = (int)(start * AUDIO_RATE);
    float low = 0.0f;
    for (int k = k0; k < MAX_SOUND_SAMPLES; k++) {
        float t = (float)(k - k0) / AUDIO_RATE;
        float e = expf(-t / decay);
        if (e < 1e-3f) break;
        low += (sfx_rand() * 2.0f - 1.0f - low) * bright;
        sfx[k] += amp * e * low;
    }
}

/* Ajoute un choc grave dont la hauteur tombe de f_hi à f_lo (coup sourd). */
static void add_thump(float start, float f_hi, float f_lo, float fall, float amp, float decay)
{
    int k0 = (int)(start * AUDIO_RATE);
    float phase = 0.0f;
    for (int k = k0; k < MAX_SOUND_SAMPLES; k++) {
        float t = (float)(k - k0) / AUDIO_RATE;
        float e = expf(-t / decay);
        if (e < 1e-3f) break;
        phase += (f_lo + (f_hi - f_lo) * expf(-t / fall)) / AUDIO_RATE;
        sfx[k] += amp * e * sinf(2.0f * SDL_PI_F * phase) * SDL_min(t / 0.002f, 1.0f);
    }
}

/* Copie le tampon de travail dans la réserve, à plein volume (le volume de chaque son est
 * dans tuning.sound_gain, appliqué à la lecture). */
static void store_sound(Sound s, float seconds)
{
    int n = SDL_min(SDL_min((int)(seconds * AUDIO_RATE), MAX_SOUND_SAMPLES), SOUND_BANK_SAMPLES - sound_bank_used);
    float m = 1e-6f;
    for (int k = 0; k < n; k++) m = SDL_max(m, fabsf(sfx[k]));
    Sint16 *out = &sound_bank[sound_bank_used];
    for (int k = 0; k < n; k++) {
        float fade = SDL_min((float)(n - k) / (AUDIO_RATE * 0.01f), 1.0f);   /* pas de clic à la fin */
        out[k] = (Sint16)(sfx[k] / m * fade * 32767.0f);
    }
    sound_start[s] = sound_bank_used;
    sound_len[s] = n;
    sound_bank_used += n;
}

/* Chocs de bois du métronome : « tic » (aigu) ou « tac » (plus grave). */
static void add_click(float start, bool tock)
{
    static const float TICK[3][3] = { { 2400.0f, 1.0f, 0.007f }, { 3900.0f, 0.5f, 0.004f }, { 1100.0f, 0.45f, 0.012f } };
    static const float TOCK[3][3] = { { 1650.0f, 1.0f, 0.009f }, { 2750.0f, 0.4f, 0.005f }, { 800.0f, 0.5f, 0.015f } };
    const float (*p)[3] = tock ? TOCK : TICK;
    for (int i = 0; i < 3; i++) add_partial(start, p[i][0], p[i][1], p[i][2]);
    add_noise(start, 0.5f, 0.002f, tock ? 0.5f : 0.7f);
}

/* Métronome ramassé : tic-tac. Métronome posé : un tic ou un tac à chaque battement. */
static void make_metronome(void)
{
    sfx_begin(11);
    add_click(0.0f, false);
    add_click(0.13f, true);
    store_sound(SOUND_PICKUP_BOMB, 0.22f);
    sfx_begin(12);
    add_click(0.0f, false);
    store_sound(SOUND_TICK, 0.07f);
    sfx_begin(13);
    add_click(0.0f, true);
    store_sound(SOUND_TOCK, 0.07f);
}

/* Royalties : deux pièces qui tintent, la seconde une quarte plus haut (« ka-ching »). */
static void make_coin(void)
{
    sfx_begin(23);
    /* Modes de vibration d'un disque de métal : rapports inharmoniques. */
    const float ratio[5] = { 1.0f, 1.59f, 2.14f, 2.30f, 2.65f };
    const float amp[5]   = { 1.0f, 0.55f, 0.40f, 0.30f, 0.22f };
    for (int hit = 0; hit < 2; hit++) {
        float start = (float)hit * 0.07f, base = hit ? 2350.0f * 1.335f : 2350.0f, length = hit ? 0.16f : 0.05f;
        for (int i = 0; i < 5; i++) add_partial(start, base * ratio[i], amp[i] * (hit ? 1.0f : 0.7f), length / (1.0f + 0.6f * (float)i));
        add_noise(start, 0.25f, 0.0015f, 0.9f);
    }
    store_sound(SOUND_PICKUP_COIN, 0.36f);
}

/* Mort d'un ennemi : un « splotch » mouillé (choc sourd, éclaboussure, petites bulles). */
static void make_splat(void)
{
    sfx_begin(37);
    const float dur = 0.32f;
    int n = (int)(dur * AUDIO_RATE);
    float low = 0.0f, low2 = 0.0f;
    for (int k = 0; k < n; k++) {
        float t = (float)k / AUDIO_RATE;
        /* Éclaboussure : bruit dont le filtre se ferme (de clair à sourd). */
        float bright = 0.55f * expf(-t / 0.05f) + 0.04f;
        low += (sfx_rand() * 2.0f - 1.0f - low) * bright;
        low2 += (low - low2) * bright;
        sfx[k] = low2 * SDL_min(t / 0.003f, 1.0f) * expf(-t / 0.075f) * 2.2f;
    }
    add_thump(0.0f, 190.0f, 55.0f, 0.035f, 0.9f, 0.06f);
    /* Bulles : petits glissandos montants. */
    for (int b = 0; b < 4; b++) {
        float start = 0.03f + sfx_rand() * 0.14f, f0 = 350.0f + sfx_rand() * 300.0f, ph = 0.0f;
        int k0 = (int)(start * AUDIO_RATE), len = (int)(0.025f * AUDIO_RATE);
        for (int k = 0; k < len && k0 + k < n; k++) {
            float u = (float)k / (float)len;
            ph += f0 * (1.0f + 1.4f * u) / AUDIO_RATE;
            sfx[k0 + k] += 0.18f * sinf(2.0f * SDL_PI_F * ph) * sinf(SDL_PI_F * u);
        }
    }
    store_sound(SOUND_SPLAT, dur);
}

/* Explosion du métronome : craquement, grondement, et les morceaux de bois qui retombent. */
static void make_explosion(void)
{
    sfx_begin(41);
    const float dur = 1.0f;
    int n = (int)(dur * AUDIO_RATE);
    float l1 = 0.0f, l2 = 0.0f;
    for (int k = 0; k < n; k++) {
        float t = (float)k / AUDIO_RATE;
        /* Grondement : bruit très filtré, qui s'assourdit encore en s'éteignant. */
        float c = 0.10f * expf(-t / 0.08f) + 0.012f;
        l1 += (sfx_rand() * 2.0f - 1.0f - l1) * c;
        l2 += (l1 - l2) * c;
        sfx[k] = l2 * 9.0f * SDL_min(t / 0.004f, 1.0f) * expf(-t / 0.28f);
    }
    add_noise(0.0f, 0.9f, 0.012f, 0.8f);                 /* craquement */
    add_thump(0.0f, 110.0f, 32.0f, 0.06f, 1.0f, 0.18f);  /* coup sourd */
    /* Débris : petits chocs de bois, de plus en plus faibles. */
    for (int i = 0; i < 7; i++) {
        float start = 0.10f + sfx_rand() * 0.45f, f = 1200.0f + sfx_rand() * 1800.0f, a = 0.22f * (1.0f - start);
        add_partial(start, f, a, 0.008f);
        add_partial(start, f * 1.7f, a * 0.5f, 0.005f);
    }
    store_sound(SOUND_EXPLOSION, dur);
}

/* Crachat d'encre de la plume : « pfut » puis un jet mouillé qui descend. */
static void make_ink_spit(void)
{
    sfx_begin(53);
    const float dur = 0.26f;
    int n = (int)(dur * AUDIO_RATE);
    float low = 0.0f, band = 0.0f;
    for (int k = 0; k < n; k++) {
        float t = (float)k / AUDIO_RATE;
        /* Filtre résonant dont la fréquence descend de 2500 à 600 Hz. */
        float fc = 600.0f + 1900.0f * expf(-t / 0.05f);
        float fq = 2.0f * sinf(SDL_PI_F * fc / AUDIO_RATE);
        float x = sfx_rand() * 2.0f - 1.0f;
        float high = x - low - 0.35f * band;
        band += fq * high;
        low += fq * band;
        sfx[k] = band * SDL_min(t / 0.006f, 1.0f) * expf(-t / 0.07f);
    }
    add_noise(0.0f, 0.6f, 0.004f, 0.9f);                   /* « p » */
    add_thump(0.02f, 320.0f, 120.0f, 0.03f, 0.35f, 0.04f);  /* « bloup » */
    store_sound(SOUND_INK_SPIT, dur);
}

/* Le moine chante une note grave, toujours la même (la variation vient de audio_play). */
static void make_monk_note(void)
{
    sfx_begin(61);
    const float dur = 0.44f, pitch = MONK_HZ;
    synth_voice(&pitch, 1, &MONK_VOWEL, 0.004f, false, sfx, (int)(dur * AUDIO_RATE));
    store_sound(SOUND_MONK_NOTE, dur);
}

/* Cloche du sonneur : partiels d'une cloche d'église (bourdon, fondamentale, tierce mineure...). */
static void make_bell(void)
{
    sfx_begin(71);
    const float f = 330.0f;
    static const float P[][3] = {   /* rapport, volume, durée d'extinction (s) */
        { 0.50f, 0.55f, 1.30f }, { 0.503f, 0.25f, 1.30f },   /* bourdon, avec un léger battement */
        { 1.00f, 0.80f, 0.90f }, { 1.20f, 0.55f, 0.65f }, { 1.50f, 0.30f, 0.45f },
        { 2.00f, 0.50f, 0.40f }, { 2.52f, 0.25f, 0.28f }, { 3.01f, 0.20f, 0.22f }, { 4.17f, 0.12f, 0.12f },
    };
    for (int i = 0; i < (int)SDL_arraysize(P); i++) add_partial(0.0f, f * P[i][0], P[i][1], P[i][2]);
    add_noise(0.0f, 0.4f, 0.003f, 0.9f);   /* le battant */
    store_sound(SOUND_BELL, 1.6f);
}

/* Le ver creuse : grattements de terre et de papier de plus en plus serrés, grondement sourd. */
static void make_dig(void)
{
    sfx_begin(83);
    const float dur = 0.85f;
    int n = (int)(dur * AUDIO_RATE);
    float l1 = 0.0f, l2 = 0.0f;
    for (int k = 0; k < n; k++) {
        float t = (float)k / AUDIO_RATE, u = t / dur;
        l1 += (sfx_rand() * 2.0f - 1.0f - l1) * 0.02f;
        l2 += (l1 - l2) * 0.02f;
        sfx[k] = l2 * 6.0f * (0.3f + 0.7f * u) * SDL_min((dur - t) / 0.05f, 1.0f);
    }
    /* Grattements : courts bruits secs, de plus en plus fréquents et forts jusqu'à la sortie. */
    for (float t = 0.0f; t < dur - 0.04f;) {
        float u = t / dur;
        add_noise(t, 0.25f + 0.45f * u, 0.004f + 0.006f * sfx_rand(), 0.25f + 0.4f * sfx_rand());
        t += (0.09f - 0.065f * u) * (0.6f + 0.8f * sfx_rand());
    }
    store_sound(SOUND_DIG, dur);
}

/* Un « ouiiin » de bébé : voix aiguë et tendue qui s'ouvre de « ou » vers « è », hauteur qui
 * monte puis retombe, un peu irrégulière et éraillée (le bébé force). */
static void add_wail(float start, float dur, float f_peak, float amp)
{
    static const Vowel OU = { { 450.0f, 1000.0f, 3300.0f }, { 150.0f, 200.0f, 300.0f }, { 1.0f, 0.50f, 0.15f } };
    static const Vowel AE = { { 1150.0f, 2100.0f, 3600.0f }, { 200.0f, 250.0f, 300.0f }, { 1.0f, 0.80f, 0.40f } };
    int k0 = (int)(start * AUDIO_RATE), n = (int)(dur * AUDIO_RATE);
    float phase = 0.0f, half = 0.0f, jitter = 0.0f, prev_noise = 0.0f, amps[MAX_HARMONICS];
    int harmonics = 0;
    for (int k = 0; k < n && k0 + k < MAX_SOUND_SAMPLES; k++) {
        float t = (float)k / AUDIO_RATE, u = t / dur;
        /* Contour : monte vite, se tient en baissant un peu, puis retombe. */
        float c = u < 0.15f ? 0.80f + 0.30f * (u / 0.15f)
                : u < 0.70f ? 1.10f - 0.12f * ((u - 0.15f) / 0.55f)
                            : 0.98f - 0.30f * ((u - 0.70f) / 0.30f);
        jitter += (sfx_rand() * 2.0f - 1.0f - jitter) * 0.002f;
        float f0 = f_peak * c * (1.0f + 3.0f * jitter) * (1.0f + 0.015f * sinf(2.0f * SDL_PI_F * 7.0f * t));
        if (k % 32 == 0) {
            float w = SDL_min(t / 0.10f, 1.0f);
            Vowel v;
            for (int i = 0; i < 3; i++) {
                v.f[i] = OU.f[i] + (AE.f[i] - OU.f[i]) * w;
                v.bw[i] = OU.bw[i] + (AE.bw[i] - OU.bw[i]) * w;
                v.amp[i] = OU.amp[i] + (AE.amp[i] - OU.amp[i]) * w;
            }
            harmonics = 0;
            for (int h = 1; h <= MAX_HARMONICS && (float)h * f0 < AUDIO_RATE * 0.5f - 600.0f; h++) {
                amps[harmonics++] = vowel_gain(&v, (float)h * f0) / powf((float)h, 0.7f);   /* voix tendue : brillante */
            }
        }
        phase += f0 / AUDIO_RATE;
        phase -= floorf(phase);
        half += f0 * 0.5f / AUDIO_RATE;
        half -= floorf(half);
        float th = 2.0f * SDL_PI_F * phase, c1 = 2.0f * cosf(th);
        float s_prev = 0.0f, s_cur = sinf(th), x = 0.0f;
        for (int h = 0; h < harmonics; h++) {
            x += amps[h] * s_cur;
            float s_next = c1 * s_cur - s_prev;
            s_prev = s_cur;
            s_cur = s_next;
        }
        x *= 0.8f + 0.2f * sinf(2.0f * SDL_PI_F * half);   /* voix éraillée : sous-harmonique */
        float w = sfx_rand() * 2.0f - 1.0f;
        x += (w - prev_noise) * 0.06f;                       /* souffle */
        prev_noise = w;
        float env = SDL_min(t / 0.04f, 1.0f) * SDL_min((dur - t) / (0.25f * dur), 1.0f);
        sfx[k0 + k] += amp * env * x;
    }
}

/* Inspiration brève entre deux sanglots. */
static void add_gasp(float start, float dur, float amp)
{
    int k0 = (int)(start * AUDIO_RATE), n = (int)(dur * AUDIO_RATE);
    float prev = 0.0f, low = 0.0f;
    for (int k = 0; k < n && k0 + k < MAX_SOUND_SAMPLES; k++) {
        float w = sfx_rand() * 2.0f - 1.0f, hp = w - prev;
        prev = w;
        low += (hp - low) * 0.3f;
        sfx[k0 + k] += amp * low * sinf(SDL_PI_F * (float)k / (float)n);
    }
}

/* Pleurs de bébé : un long « ouiiin », ou un sanglot « oua… ouaaah ». */
/* Une lame de boîte à musique pincée : fondamentale qui dure, partiel aigu de lame vibrante
 * (rapport 6,27) qui s'éteint vite, une copie à peine désaccordée qui chatoie, et le petit clic du picot. */
static void add_tine(float start, float freq, float amp, float length)
{
    add_partial(start, freq, amp, length);
    add_partial(start, freq * 1.0035f, amp * 0.35f, length * 0.8f);
    add_partial(start, freq * 6.27f, amp * 0.22f, 0.04f);
    add_noise(start, amp * 0.12f, 0.0012f, 0.9f);
}

/* Boîtes à musique (GDD §22) : grise, le début d'Ut queant laxis ; dorée, ut ré mi fa sol la, puis un accord. */
static void make_music_boxes(void)
{
    const float DO6 = 1046.5f;
    sfx_begin(71);
    static const int UT_QUEANT[6] = { 0, 2, 5, 2, 4, 2 };   /* ut que-ant la-xis, en demi-tons */
    for (int i = 0; i < 6; i++) {
        add_tine(0.16f * (float)i, DO6 * powf(2.0f, (float)UT_QUEANT[i] / 12.0f), 1.0f, i == 5 ? 0.9f : 0.5f);
    }
    store_sound(SOUND_BOX_GRAY, 1.9f);

    sfx_begin(72);
    static const int HEXACHORD[6] = { 0, 2, 4, 5, 7, 9 };   /* ut ré mi fa sol la */
    for (int i = 0; i < 6; i++) add_tine(0.12f * (float)i, DO6 * powf(2.0f, (float)HEXACHORD[i] / 12.0f), 1.0f, 0.45f);
    static const int CHORD[3] = { 12, 16, 19 };             /* ut mi sol, une octave plus haut */
    for (int i = 0; i < 3; i++) add_tine(0.80f + 0.03f * (float)i, DO6 * powf(2.0f, (float)CHORD[i] / 12.0f), 0.8f, 1.0f);
    store_sound(SOUND_BOX_GOLD, 2.0f);
}

/* Clef ramassée : deux petites clefs de métal qui s'entrechoquent. Serrure : pêne qui claque. */
static void make_keys(void)
{
    sfx_begin(81);
    const float ratio[4] = { 1.0f, 1.47f, 2.09f, 2.71f };
    for (int hit = 0; hit < 2; hit++) {
        float start = 0.085f * (float)hit, base = hit ? 3400.0f : 2900.0f;
        for (int i = 0; i < 4; i++) add_partial(start, base * ratio[i], 0.8f / (1.0f + (float)i), 0.09f / (1.0f + 0.5f * (float)i));
        add_noise(start, 0.2f, 0.001f, 0.9f);
    }
    store_sound(SOUND_KEY, 0.3f);

    sfx_begin(82);
    add_thump(0.0f, 900.0f, 260.0f, 0.01f, 0.9f, 0.03f);
    add_noise(0.0f, 0.6f, 0.004f, 0.6f);
    add_partial(0.0f, 1850.0f, 0.4f, 0.03f);
    add_thump(0.11f, 700.0f, 180.0f, 0.012f, 1.0f, 0.05f);   /* le pêne retombe */
    add_noise(0.11f, 0.7f, 0.006f, 0.45f);
    add_partial(0.11f, 1420.0f, 0.35f, 0.05f);
    store_sound(SOUND_UNLOCK, 0.3f);
}

static void make_baby(void)
{
    sfx_begin(97);
    add_wail(0.0f, 0.95f, 520.0f, 1.0f);
    store_sound(SOUND_BABY_CRY, 1.0f);
    sfx_begin(98);
    add_wail(0.0f, 0.28f, 470.0f, 0.8f);
    add_gasp(0.30f, 0.14f, 1.5f);
    add_wail(0.46f, 0.90f, 545.0f, 1.0f);
    store_sound(SOUND_BABY_SOB, 1.42f);
}

const Sint16 *audio_sound_samples(Sound s, int *count)
{
    if (s < 0 || s >= SOUND_COUNT) {
        *count = 0;
        return NULL;
    }
    *count = sound_len[s];
    return &sound_bank[sound_start[s]];
}

/* --- Sortie ---------------------------------------------------------------------- */

bool audio_init(void)
{
    for (int i = 0; i < MAX_VOICES; i++) voices[i] = (Voice){ .sound = -1 };
    sound_bank_used = 0;
    make_metronome();
    make_coin();
    make_splat();
    make_explosion();
    make_ink_spit();
    make_monk_note();
    make_bell();
    make_dig();
    make_baby();
    make_music_boxes();
    make_keys();
    chant = 0;   /* on commence par Ut queant laxis, écrit sur le sol */
    next_syllable = 0;
    last_sing = -1e9f;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_Log("Audio indisponible : %s", SDL_GetError());
        return false;
    }
    SDL_AudioSpec spec = { SDL_AUDIO_S16, 1, AUDIO_RATE };
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream) {
        SDL_Log("Sortie audio impossible : %s", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    SDL_ResumeAudioStreamDevice(stream);
    return true;
}

void audio_quit(void)
{
    if (stream) {
        SDL_DestroyAudioStream(stream);
        stream = NULL;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

void audio_update(void)
{
    if (!stream) return;
    const int lead = (int)(LEAD_SECONDS * AUDIO_RATE) * (int)sizeof(Sint16);
    while (SDL_GetAudioStreamQueued(stream) < lead) {
        float mix[MIX_CHUNK] = { 0 };
        for (int v = 0; v < MAX_VOICES; v++) {
            Voice *vc = &voices[v];
            for (int k = 0; k < MIX_CHUNK && vc->pos < (float)vc->len; k++) {
                int i = (int)vc->pos;
                float u = vc->pos - (float)i, a = vc->src[i], b = i + 1 < vc->len ? vc->src[i + 1] : 0.0f;
                mix[k] += (a + (b - a) * u) * vc->gain;
                vc->pos += vc->rate;
            }
        }
        Sint16 out[MIX_CHUNK];
        for (int k = 0; k < MIX_CHUNK; k++) {
            out[k] = (Sint16)SDL_clamp(mix[k] * tuning.sound_volume, -32767.0f, 32767.0f);
        }
        if (!SDL_PutAudioStreamData(stream, out, (int)sizeof out)) break;
    }
}

/* Une voix libre, sinon celle qui joue depuis le plus longtemps (en proportion). */
static int free_voice(void)
{
    int best = 0;
    float best_u = -1.0f;
    for (int v = 0; v < MAX_VOICES; v++) {
        if (voices[v].pos >= (float)voices[v].len) return v;
        float u = voices[v].pos / (float)voices[v].len;
        if (u > best_u) {
            best_u = u;
            best = v;
        }
    }
    return best;
}

/* Un autre chant que celui en cours, au hasard. */
static int other_chant(int current)
{
    if (CHANT_COUNT < 2) return 0;
    int c = SDL_rand_r(&rng, CHANT_COUNT - 1);
    return c >= current ? c + 1 : c;
}

void audio_sing(float now)
{
    /* Une pause assez longue, ou la fin du chant : on en commence un autre.
     * (now < last_sing : une nouvelle partie a remis le temps à zéro.) */
    if (now - last_sing > tuning.chant_pause || now < last_sing || next_syllable >= CHANTS[chant].count) {
        if (last_sing > -1e8f) chant = other_chant(chant);
        next_syllable = 0;
    }
    last_sing = now;
    int s = next_syllable++;
    if (!stream) return;
    int v = free_voice();
    int n = audio_render_syllable(chant, s, chant_buf[v], MAX_SYL_SAMPLES);
    voices[v] = (Voice){ chant_buf[v], n, 0.0f, 1.0f, tuning.chant_volume, -1 };
}

void audio_play(Sound s, float pitch)
{
    if (!stream || s < 0 || s >= SOUND_COUNT || sound_len[s] == 0) return;
    /* Plusieurs fois le même son au même instant (une bombe qui tue trois bébés) : un seul. */
    for (int i = 0; i < MAX_VOICES; i++) {
        if (voices[i].sound == (int)s && voices[i].pos == 0.0f && voices[i].len > 0) return;
    }
    /* Un peu de variation, pour que le même son répété ne soit pas mécanique. */
    float rate = pitch * (0.94f + 0.12f * (float)SDL_rand_r(&rng, 1000) / 1000.0f);
    voices[free_voice()] = (Voice){ &sound_bank[sound_start[s]], sound_len[s], 0.0f, rate, tuning.sfx_volume * tuning.sound_gain[s], (int)s };
}

int audio_chant_count(void)
{
    return CHANT_COUNT;
}

const char *audio_chant_name(int c)
{
    return (c >= 0 && c < CHANT_COUNT) ? CHANTS[c].name : "";
}

int audio_chant_length(int c)
{
    return (c >= 0 && c < CHANT_COUNT) ? CHANTS[c].count : 0;
}

int audio_current_chant(void)
{
    return chant;
}

int audio_next_syllable(void)
{
    return next_syllable;
}
