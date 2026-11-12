#include "audio.h"
#include "config.h"

#include <math.h>

/*
 * Ut queant laxis, première strophe (Liber Usualis 1961 p. 1504, mode 2), syllabe par syllabe.
 * Notes en lettres GABC avec la clé de fa sur la 3e ligne : e = ut (do), f = ré, g = mi,
 * h = fa, i = sol, j = la. Chaque demi-vers commence une note plus haut que le précédent :
 * c'est de là que viennent les noms des notes.
 */
typedef struct Syllable {
    const char *text;
    char        vowel;   /* voyelle chantée : a, e, i, o, u */
    const char *neume;   /* notes de la syllabe */
} Syllable;

static const Syllable HYMN[] = {
    { "Ut", 'u', "e" },   { "que", 'e', "f" },  { "ant", 'a', "h" },  { "la", 'a', "fg" }, { "xis", 'i', "f" },
    { "re", 'e', "f" },   { "so", 'o', "f" },   { "na", 'a', "e" },   { "re", 'e', "f" },  { "fi", 'i', "g" },  { "bris", 'i', "g" },
    { "Mi", 'i', "ghi" }, { "ra", 'a', "g" },   { "ge", 'e', "f" },   { "sto", 'o', "ge" }, { "rum", 'u', "f" },
    { "fa", 'a', "h" },   { "mu", 'u', "i" },   { "li", 'i', "j" },   { "tu", 'u', "i" },  { "o", 'o', "hf" },  { "rum", 'u', "f" },
    { "Sol", 'o', "iji" }, { "ve", 'e', "hg" }, { "pol", 'o', "h" },  { "lu", 'u', "i" },  { "ti", 'i', "f" },
    { "la", 'a', "j" },   { "bi", 'i', "i" },   { "i", 'i', "j" },    { "re", 'e', "hi" }, { "a", 'a', "j" },   { "tum", 'u', "j" },
    { "San", 'a', "ih" }, { "cte", 'e', "gf" }, { "Jo", 'o', "e" },   { "an", 'a', "g" },  { "nes", 'e', "f" },
};
#define HYMN_LEN ((int)(sizeof HYMN / sizeof HYMN[0]))

/* Voix d'enfant : une octave au-dessus de la notation, ut = do5. */
#define UT_HZ 523.25f

/* Formants (résonances de la bouche) d'une voix d'enfant : fréquence, largeur, volume. */
typedef struct Vowel { float f[3], bw[3], amp[3]; } Vowel;
static const Vowel VOWELS[5] = {
    /* a */ { { 950.0f, 1500.0f, 3000.0f }, { 120.0f, 140.0f, 200.0f }, { 1.0f, 0.60f, 0.25f } },
    /* e */ { { 500.0f, 2200.0f, 3100.0f }, { 110.0f, 150.0f, 200.0f }, { 1.0f, 0.45f, 0.30f } },
    /* i */ { { 330.0f, 2700.0f, 3400.0f }, { 100.0f, 160.0f, 220.0f }, { 1.0f, 0.35f, 0.30f } },
    /* o */ { { 520.0f, 950.0f, 2900.0f },  { 110.0f, 130.0f, 200.0f }, { 1.0f, 0.70f, 0.15f } },
    /* u */ { { 360.0f, 820.0f, 2700.0f },  { 100.0f, 120.0f, 200.0f }, { 1.0f, 0.50f, 0.10f } },
};

#define MAX_SYL_SAMPLES (AUDIO_RATE * 34 / 100)   /* 0,34 s */
#define MAX_VOICES      8
#define MIX_CHUNK       256
#define LEAD_SECONDS    0.05f   /* le flux garde tant d'avance : assez pour une image lente, peu de latence */

static Sint16 pcm[HYMN_LEN][MAX_SYL_SAMPLES];
static int    pcm_len[HYMN_LEN];
static bool   synthesized;

typedef struct Voice {
    int sound;   /* syllabe jouée, -1 si libre */
    int pos;     /* échantillon suivant */
} Voice;

static SDL_AudioStream *stream;
static Voice            voices[MAX_VOICES];
static int              next_syllable;

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

/* Lettre GABC (clé de fa 3e ligne) -> demi-tons au-dessus de l'ut. */
static int semitones(char c)
{
    static const int SCALE[] = { -3, -1, 0, 2, 4, 5, 7, 9, 10, 12, 14 };   /* c d e f g h i j k l m */
    int k = c - 'c';
    return (k >= 0 && k < (int)(sizeof SCALE / sizeof SCALE[0])) ? SCALE[k] : 0;
}

float audio_syllable_pitch(int i, int note)
{
    if (i < 0 || i >= HYMN_LEN || note < 0 || note >= (int)SDL_strlen(HYMN[i].neume)) return 0.0f;
    return UT_HZ * powf(2.0f, (float)semitones(HYMN[i].neume[note]) / 12.0f);
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

/* Petit générateur de bruit (souffle des consonnes), toujours le même. */
static Uint32 noise_state = 0x12345678u;
static float noise(void)
{
    noise_state ^= noise_state << 13;
    noise_state ^= noise_state >> 17;
    noise_state ^= noise_state << 5;
    return (float)(noise_state & 0xFFFF) / 32768.0f - 1.0f;
}

/* Calcule une syllabe : les notes du neume s'enchaînent d'un trait, avec un léger glissé. */
static void synthesize(int i)
{
    static float buf[MAX_SYL_SAMPLES];
    const Syllable *s = &HYMN[i];
    const Vowel *v = &VOWELS[vowel_index(s->vowel)];
    int notes = (int)SDL_strlen(s->neume);
    float dur = SDL_min(0.19f + 0.07f * (float)(notes - 1), 0.33f);
    int n = SDL_min((int)(dur * AUDIO_RATE), MAX_SYL_SAMPLES);
    float note_len = dur / (float)notes;
    float glide = 1.0f - expf(-1.0f / (AUDIO_RATE * 0.012f));
    char first = (char)(s->text[0] | 0x20);
    bool fricative = first == 's' || first == 'x' || first == 't' || first == 'c' || first == 'f';

    float f = audio_syllable_pitch(i, 0), phase = 0.0f, peak = 1e-6f;
    float amps[48];
    int harmonics = 0;
    float prev_noise = 0.0f;
    for (int k = 0; k < n; k++) {
        float t = (float)k / AUDIO_RATE;
        int note = SDL_min((int)(t / note_len), notes - 1);
        f += (audio_syllable_pitch(i, note) - f) * glide;
        float vib = 1.0f + 0.008f * sinf(2.0f * SDL_PI_F * 5.5f * t) * SDL_clamp((t - 0.06f) / 0.08f, 0.0f, 1.0f);
        float f0 = f * vib;
        /* Volume des harmoniques : pente naturelle de la voix × résonances de la voyelle. */
        if (k % 32 == 0) {
            harmonics = 0;
            for (int h = 1; h <= 48 && (float)h * f0 < AUDIO_RATE * 0.5f - 600.0f; h++) {
                amps[harmonics++] = vowel_gain(v, (float)h * f0) / (float)h;
            }
        }
        phase += f0 / AUDIO_RATE;
        phase -= floorf(phase);
        float x = 0.0f;
        for (int h = 0; h < harmonics; h++) x += amps[h] * sinf(2.0f * SDL_PI_F * (float)(h + 1) * phase);

        /* Consonne : petit souffle aigu au début. */
        if (fricative && t < 0.03f) {
            float w = noise(), hp = w - prev_noise;
            prev_noise = w;
            x += hp * 0.35f * (1.0f - t / 0.03f);
        }
        float env = SDL_min(t / 0.012f, 1.0f) * SDL_min((dur - t) / 0.06f, 1.0f) * (1.0f - 0.25f * t / dur);
        buf[k] = x * SDL_max(env, 0.0f);
        peak = SDL_max(peak, fabsf(buf[k]));
    }
    for (int k = 0; k < n; k++) pcm[i][k] = (Sint16)(buf[k] / peak * 0.85f * 32767.0f);
    pcm_len[i] = n;
}

bool audio_init(void)
{
    if (!synthesized) {
        for (int i = 0; i < HYMN_LEN; i++) synthesize(i);
        synthesized = true;
    }
    for (int i = 0; i < MAX_VOICES; i++) voices[i].sound = -1;
    next_syllable = 0;
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
            if (vc->sound < 0) continue;
            int left = pcm_len[vc->sound] - vc->pos, n = SDL_min(left, MIX_CHUNK);
            for (int k = 0; k < n; k++) mix[k] += (float)pcm[vc->sound][vc->pos + k];
            vc->pos += n;
            if (vc->pos >= pcm_len[vc->sound]) vc->sound = -1;
        }
        Sint16 out[MIX_CHUNK];
        for (int k = 0; k < MIX_CHUNK; k++) {
            out[k] = (Sint16)SDL_clamp(mix[k] * tuning.sound_volume, -32767.0f, 32767.0f);
        }
        if (!SDL_PutAudioStreamData(stream, out, (int)sizeof out)) break;
    }
}

static void play(int sound)
{
    /* Une voix libre, sinon celle qui joue depuis le plus longtemps. */
    int best = 0;
    for (int v = 0; v < MAX_VOICES; v++) {
        if (voices[v].sound < 0) {
            best = v;
            break;
        }
        if (voices[v].pos > voices[best].pos) best = v;
    }
    voices[best] = (Voice){ sound, 0 };
}

void audio_sing(void)
{
    int s = next_syllable;
    next_syllable = (next_syllable + 1) % HYMN_LEN;
    if (stream) play(s);
}

int audio_syllable_count(void)
{
    return HYMN_LEN;
}

int audio_next_syllable(void)
{
    return next_syllable;
}

const Sint16 *audio_syllable_samples(int i, int *count)
{
    if (i < 0 || i >= HYMN_LEN || !synthesized) {
        *count = 0;
        return NULL;
    }
    *count = pcm_len[i];
    return pcm[i];
}
