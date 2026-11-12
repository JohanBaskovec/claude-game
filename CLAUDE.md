# Fugue in Amadeus Minor

Roguelike façon *The Binding of Isaac* à travers l'histoire de la musique. C11 + SDL3, sans moteur.

## Design

- Le GDD est `docs/concept/GDD.md`. Il est conçu dans une **autre session Claude** et recopié ici par l'utilisateur : le relire au début de chaque nouvelle tâche, il change entre les sessions.
- **N'implémenter que la phase en cours** du GDD (section « PHASE … à implémenter »). Le reste est du contexte : ne rien coder au-delà sans demande explicite.
- `docs/concept/outils_concept/` : prototypes Python (Pillow) des visuels, à porter en C plus tard. Ce sont des références, pas du code du jeu.
- Légende du GDD : ✅ décidé, 💡 idée à valider, ❓ question ouverte.

## Build

```sh
cmake -S . -B build          # télécharge et compile SDL3 si absent du système
cmake --build build -j
./build/fugue
SDL_VIDEO_DRIVER=offscreen timeout 3 ./build/fugue   # test de lancement sans fenêtre
```

Web : `emcmake cmake -S . -B build-web` (Emscripten non installé pour l'instant).

## Contraintes techniques (portabilité Windows / Linux / navigateur)

- Modèle à callbacks SDL3 (`SDL_AppInit/Iterate/Event/Quit`) : jamais de boucle `while` bloquante.
- Rendu uniquement via `SDL_Renderer`. Pas de threads, pas de chemins absolus, pas d'API spécifique à un OS. Fichiers chargés via `SDL_IOStream`.
- Résolution logique 960×540, mise à l'échelle par facteur entier, `SDL_SCALEMODE_NEAREST`.
- Logique à pas fixe de 60 Hz (`TICK_HZ`) ; le rendu interpole entre `prev_*` et la position courante avec `alpha`. Tout nouvel objet mobile doit stocker sa position précédente.
- Pas de `malloc` en jeu : pools statiques de taille fixe (`MAX_PROJECTILES`…).
- Clavier lu par **scancodes** (position physique) : `SDL_SCANCODE_W` = Z en AZERTY.

## Code

- `src/config.h` / `config.c` : **tous** les paramètres de gameplay dans la struct globale `tuning`. Un nouveau réglage va là, pas en dur dans le code.
- Un module par entité (`player`, `projectile`, `room`, `input`), chacun avec `_init`, `_update`, `_render`.
- Coordonnées en pixels du jeu (960×540), temps en secondes, vitesses en px/s.
- Commentaires et textes en français. Le jeu sera bilingue FR/EN : pas de texte affiché en dur dans le code une fois qu'il y aura du texte.
- Compiler sans avertissement (`-Wall -Wextra -Wpedantic -Wshadow`).
