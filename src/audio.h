#ifndef FUGUE_AUDIO_H
#define FUGUE_AUDIO_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#include "config.h"

/*
 * Sons du jeu (GDD §19), synthétisés en jeu : aucun fichier audio.
 * Le flux SDL est rempli depuis la boucle principale (audio_update), sans thread à nous.
 * Sans périphérique audio, tout fonctionne en silence.
 */

/* Ouvre la sortie audio. Renvoie false si le jeu sera muet. */
bool audio_init(void);
void audio_quit(void);

/* À appeler à chaque image : mixe les voix en cours et garde le flux un peu en avance. */
void audio_update(void);

/* Amadeus chante la syllabe suivante de la chanson pop en cours (son du tir, GDD §26). `now` = temps de la partie, s ;
 * `interval` = temps entre deux tirs à la cadence actuelle, s : la syllabe dure ce temps moins un
 * petit silence (audio_syllable_duration). Si on avait arrêté de tirer (plus de
 * tuning.chant_stop_factor intervalles sans tir), ou à la fin de la chanson, il passe à une autre. */
void  audio_sing(float now, float interval);
float audio_syllable_duration(float interval);

/* Un moine chante la syllabe `syllable` du chant `chant` (GDD §25), pendant `duration` s. */
void audio_monk_sing(int chant, int syllable, float duration);

/* Bruitages (GDD §20, liste dans config.h). `pitch` = 1 pour le son normal ; plus bas, il est
 * plus grave et plus long. Volume : tuning.sound_volume × sfx_volume × sound_gain[sound]. */
void audio_play(Sound sound, float pitch);

/* Pour les tests et les moines. Chansons d'Amadeus (song_data.h) : nombre, nom, longueur, celle en
 * cours, la syllabe qui sera chantée au prochain tir. Chants grégoriens des moines (chant_data.h) :
 * nombre, nom, longueur, texte et notes des syllabes. Sons : 16 bits mono à AUDIO_RATE Hz. */
#define AUDIO_RATE 22050
int         audio_song_count(void);
const char *audio_song_name(int song);
int         audio_song_length(int song);
int         audio_current_song(void);
int         audio_next_syllable(void);
float       audio_song_pitch(int song, int syllable, int note);   /* Hz, 0 si aucune */
int         audio_render_song_syllable(int song, int syllable, float duration, Sint16 *out, int max);   /* renvoie le nombre d'échantillons */
int         audio_chant_count(void);
const char *audio_chant_name(int chant);
int         audio_chant_length(int chant);   /* nombre de syllabes */
float       audio_syllable_pitch(int chant, int syllable, int note);   /* Hz, 0 si aucune */
int         audio_render_monk_syllable(int chant, int syllable, float duration, Sint16 *out, int max);
const char *audio_syllable_text(int chant, int syllable);    /* « Di », « es »... (UTF-8) */
const char *audio_syllable_notes(int chant, int syllable);   /* une lettre par note : 'a' = la plus grave du chant */
const Sint16 *audio_sound_samples(Sound sound, int *count);   /* bruitage calculé par audio_init */

#endif
