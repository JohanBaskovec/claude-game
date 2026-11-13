# Amadeus' Minor Fugue

Roguelike façon *The Binding of Isaac* à travers l'histoire de la musique. C11 + SDL3.
Le design est dans [`docs/concept/GDD.md`](docs/concept/GDD.md).

**État : Phase 17** — Amadeus chante des chansons pop originales (clichés pop anglais) pendant que les moines chantent du grégorien ; des tirs en notes modernes dont la valeur suit la cadence (de la ronde à la quadruple croche, le chant suit le rythme), un cri d'Amadeus quand il est touché, des descriptions détaillées des objets, un jeu qui remplit la fenêtre, des moines qui chantent de vrais chants grégoriens syllabe par syllabe (chaque tir est le neume de la syllabe, écrite dessous), des neumes tirés par les sonneurs, un menu principal (Jouer, Quitter), un titre d'époque à l'arrivée au premier étage (« 1012 après J.-C. »), une vraie partie (étages, prix, métronomes-bombes, salle du trésor, Gros Bébé, trappe), les armes du Haut Moyen Âge (voix, olifant, flûte, lyre, tabourin, cloche, orgue) avec les premiers neumes, et les vrais graphismes du décor (murs peints façon tables des canons, portes en arcs peints, sol en parchemin couvert de neumes sur une double page avec sa reliure, mouches écrasées, fleurs séchées, sceaux de cire et *marginalia*), d'Amadeus et du bébé médiéval, et les ennemis du premier étage : moine chanteur, plume géante qui crache de l'encre toxique, ver de livre qui surgit du sol, sonneur de cloche (sprites provisoires), qui explosent en laissant des taches de sang ou d'encre. Visée libre à la souris. À chaque tir, Amadeus chante la syllabe suivante d'une chanson pop originale ; les moines chantent six chants grégoriens, dont *Ut queant laxis* (voix synthétisées par le jeu). Bruitages : ramassage d'un métronome ou de royalties, mort des ennemis, tic-tac et explosion du métronome, crachat d'encre des plumes, chant grave des moines, cloche des sonneurs, ver qui creuse, pleurs des bébés. Salles plus vivantes : trous dans le parchemin (déchirés, brûlés, recousus, et ceux que les vers percent à chaque sortie), obstacles (piles de livres, encriers, chandelles, aiguilles du relieur, signets, fils de la reliure), salles sombres éclairées par Amadeus, les chandelles et les notes ; chaque porte reste toujours accessible depuis chaque porte. Clefs de sol, de fa et d'ut, magasin du Luthier (fermé par une clef de sol : trois objets et des PV contre des royalties), boîtes à musique grises et dorées qui jouent une mélodie et répandent leur contenu, une récompense à chaque salle vidée, téléportation vers les salles voisines pas encore visitées. Les autres graphismes sont encore provisoires.

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
- **Personnages et obstacles** (`assets/sprites/`, table `SHEETS` de `src/sprite.c`) : des planches
  PNG ; si un PNG manque, le jeu se rabat sur les grilles de caractères de `src/sprite_data.h`.
  Les planches des ennemis de la phase 7 et des obstacles sont provisoires.

## Licences tierces

- [Junicode](https://github.com/psb1558/Junicode-font) de Peter S. Baker (`assets/fonts/`) : SIL Open Font License 1.1, voir `assets/fonts/OFL.txt`.
- [stb_truetype](https://github.com/nothings/stb) de Sean Barrett (`src/third_party/`) : domaine public.
- Mélodies grégoriennes du *Liber Usualis* (1961), domaine public ; notes relevées sur GregoBase : [*Ut queant laxis*](https://gregobase.selapa.net/chant.php?id=2539) (p. 1504), [*Veni Creator Spiritus*](https://gregobase.selapa.net/chant.php?id=2923) (p. 885), [*Ave maris stella*](https://gregobase.selapa.net/chant.php?id=2232) (p. 1259), [*Pange lingua*](https://gregobase.selapa.net/chant.php?id=1310) (p. 957), [*Dies irae*](https://gregobase.selapa.net/chant.php?id=1198) (p. 1810), [*Victimae paschali laudes*](https://gregobase.selapa.net/chant.php?id=1086) (p. 780).
