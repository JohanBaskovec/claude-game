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

/* Amadeus chante la syllabe suivante du chant en cours (son du tir). `now` = temps de
 * la partie, s : après plus de tuning.chant_pause sans chanter, ou à la fin du chant,
 * il passe à un autre chant, au hasard. */
void audio_sing(float now);

/* Bruitages (GDD §20, liste dans config.h). `pitch` = 1 pour le son normal ; plus bas, il est
 * plus grave et plus long. Volume : tuning.sound_volume × sfx_volume × sound_gain[sound]. */
void audio_play(Sound sound, float pitch);

/* Pour les tests : les chants (chant_data.h), le chant en cours, la syllabe qui sera
 * chantée au prochain tir, et le son d'une syllabe (16 bits mono à AUDIO_RATE Hz). */
#define AUDIO_RATE 22050
int         audio_chant_count(void);
const char *audio_chant_name(int chant);
int         audio_chant_length(int chant);   /* nombre de syllabes */
int         audio_current_chant(void);
int         audio_next_syllable(void);
float       audio_syllable_pitch(int chant, int syllable, int note);   /* Hz, 0 si aucune */
int         audio_render_syllable(int chant, int syllable, Sint16 *out, int max);   /* renvoie le nombre d'échantillons */
const Sint16 *audio_sound_samples(Sound sound, int *count);   /* bruitage calculé par audio_init */

#endif
