# Fugue in Amadeus Minor

Roguelike façon *The Binding of Isaac* à travers l'histoire de la musique. C11 + SDL3.
Le design est dans [`docs/concept/GDD.md`](docs/concept/GDD.md).

**État : Phase 7** — une vraie partie (étages, prix, métronomes-bombes, salle du trésor, Gros Bébé, trappe), les armes du Haut Moyen Âge (voix, olifant, flûte, lyre, tabourin, cloche, orgue) avec les premiers neumes, et les vrais graphismes du décor (château, portes, sol en manuscrit à neumes sur une double page avec sa reliure), d'Amadeus et du bébé médiéval, et les ennemis du premier étage : moine chanteur, plume géante qui crache de l'encre toxique, ver de livre qui surgit du sol, sonneur de cloche (sprites provisoires), qui explosent en laissant des taches de sang ou d'encre. Visée libre à la souris. Les autres graphismes sont encore provisoires.

## Compiler et lancer

```sh
cmake -S . -B build
cmake --build build -j
./build/fugue
```

SDL3 est utilisé s'il est installé sur le système. Sinon, il est téléchargé
et compilé avec le projet (plusieurs minutes la première fois).

Le dossier `assets/` (police du sol) est copié à côté de l'exécutable à chaque build :
le jeu le cherche là. Sans lui, le sol s'affiche sans texte.

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
| Clic gauche | Chanter vers le viseur, dans n'importe quelle direction (une flèche enfoncée l'emporte) |
| E | Poser un métronome (bombe) |
| F1 | Infos de debug (FPS, étage et graine, salle actuelle, objets pris) |
| Clic sur la mini-carte | Se téléporter dans une salle déjà visitée (hors combat) |
| F2 | Menu de debug : attributs du joueur modifiables à la souris (− / +, maintenir pour répéter), métronomes, royalties et arme, ajouter un ennemi au choix, étage suivant, tuer les ennemis, donner un objet ; carte entière visible et téléportation partout |
| F11 | Plein écran |
| Échap | Quitter |

## Régler les sensations

En jeu, **F2** permet de modifier les attributs du joueur en direct.
Les valeurs de départ et tous les autres paramètres sont dans [`src/config.c`](src/config.c).

## Graphismes

- **Décor** (`src/decor.c`) : calculé par le jeu, sans fichier image, d'après les prototypes de
  `docs/concept/outils_concept/`. Chaque salle est dessinée une fois dans une texture ; les salles
  voisines sont préparées à l'avance, quelques millisecondes par image.
- **Personnages** (`src/sprite_data.h`) : des grilles de caractères, une lettre par couleur de la
  palette. On les retouche dans un éditeur de texte.

## Licences tierces

- [Junicode](https://github.com/psb1558/Junicode-font) de Peter S. Baker (`assets/fonts/`) : SIL Open Font License 1.1, voir `assets/fonts/OFL.txt`.
- [stb_truetype](https://github.com/nothings/stb) de Sean Barrett (`src/third_party/`) : domaine public.
