# Fugue in Amadeus Minor

Roguelike façon *The Binding of Isaac* à travers l'histoire de la musique. C11 + SDL3.
Le design est dans [`docs/concept/GDD.md`](docs/concept/GDD.md).

**État : Phase 5** — une vraie partie (étages, prix, métronomes-bombes, salle du trésor, Gros Bébé, trappe) et les armes du Haut Moyen Âge (voix, olifant, flûte, lyre, tabourin, cloche, orgue) avec les premiers neumes (graphismes provisoires).

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
| Flèches | Chanter (tirer) — la dernière flèche enfoncée l'emporte. Flûte : maintenir pour charger, relâcher pour tirer |
| E | Poser un métronome (bombe) |
| F1 | Infos de debug (FPS, étage et graine, salle actuelle, objets pris) |
| Clic sur la mini-carte | Se téléporter dans une salle déjà visitée (hors combat) |
| F2 | Menu de debug : attributs du joueur modifiables à la souris (− / +, maintenir pour répéter), métronomes, royalties et arme, ajouter un bébé, étage suivant, tuer les ennemis, donner un objet ; carte entière visible et téléportation partout |
| F11 | Plein écran |
| Échap | Quitter |

## Régler les sensations

En jeu, **F2** permet de modifier les attributs du joueur en direct.
Les valeurs de départ et tous les autres paramètres sont dans [`src/config.c`](src/config.c).
