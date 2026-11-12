#ifndef FUGUE_AUDIO_H
#define FUGUE_AUDIO_H

#include <SDL3/SDL.h>
#include <stdbool.h>

/*
 * Sons du jeu (GDD §19), synthétisés au démarrage : aucun fichier audio.
 * Le flux SDL est rempli depuis la boucle principale (audio_update), sans thread à nous.
 * Sans périphérique audio, tout fonctionne en silence.
 */

/* Ouvre la sortie audio et calcule les sons. Renvoie false si le jeu sera muet. */
bool audio_init(void);
void audio_quit(void);

/* À appeler à chaque image : mixe les voix en cours et garde le flux un peu en avance. */
void audio_update(void);

/* Amadeus chante la syllabe suivante d'Ut queant laxis (son du tir). */
void audio_sing(void);

/* Pour les tests : nombre de syllabes de l'hymne, syllabe qui sera chantée au prochain tir,
 * et accès au son calculé d'une syllabe (échantillons 16 bits mono à AUDIO_RATE Hz). */
#define AUDIO_RATE 22050
int          audio_syllable_count(void);
int          audio_next_syllable(void);
const Sint16 *audio_syllable_samples(int i, int *count);
float        audio_syllable_pitch(int i, int note);   /* fréquence de la note `note` de la syllabe, Hz (0 si aucune) */

#endif
