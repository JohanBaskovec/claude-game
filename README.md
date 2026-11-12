# Fugue in Amadeus Minor

Roguelike façon *The Binding of Isaac* à travers l'histoire de la musique. C11 + SDL3.
Le design est dans [`docs/concept/GDD.md`](docs/concept/GDD.md).

**État : Phase 1** — une salle, déplacement et tir de notes (placeholders).

## Compiler et lancer

```sh
cmake -S . -B build
cmake --build build -j
./build/fugue
```

SDL3 est utilisé s'il est installé sur le système. Sinon, il est téléchargé
et compilé avec le projet (plusieurs minutes la première fois).

Build web (Emscripten, avec `emsdk` activé) :

```sh
emcmake cmake -S . -B build-web
cmake --build build-web -j
# servir build-web/ puis ouvrir fugue.html
```

## Commandes

| Touche | Action |
|---|---|
| ZQSD (AZERTY) / WASD (QWERTY) | Se déplacer (touches lues par position physique) |
| Flèches | Chanter (tirer) — la dernière flèche enfoncée l'emporte |
| F1 | Infos de debug (FPS, paramètres) |
| F11 | Plein écran |
| Échap | Quitter |

## Régler les sensations

Tous les paramètres de gameplay sont dans [`src/config.c`](src/config.c)
(vitesse, accélération, cadence, vitesse et portée des notes, héritage de vitesse…).
