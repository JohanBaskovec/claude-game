# Fugue in Amadeus Minor

Roguelike façon *The Binding of Isaac* à travers l'histoire de la musique. C11 + SDL3, sans moteur.

## Design

- **`docs/concept/GDD.md` est la source de vérité du design.** Quand l'utilisateur donne une décision de design (même en passant, au milieu d'une demande de code), **mettre à jour le GDD** dans la même tâche, au bon endroit, avec le bon statut (✅ / 💡 / ❓).
- La conception se fait uniquement dans les sessions Claude Code (plus de session de conception séparée). Relire le GDD au début de chaque nouvelle tâche.
- Une demande qui dépasse la phase en cours = une nouvelle phase : l'ajouter au GDD (section « PHASE N ») avec son périmètre et son hors-périmètre.
- **N'implémenter que la phase en cours** du GDD (section « PHASE … à implémenter »). Le reste est du contexte : ne rien coder au-delà sans demande explicite.
- `docs/concept/outils_concept/` : prototypes Python (Pillow) des visuels, portés en C en Phase 6 (`decor.c`, `sprite_data.h`). Ce sont des références, pas du code du jeu.
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
- Visée (GDD §17) : un tir a une direction libre (vecteur unitaire), pas une `Dir`. Flèches = 4 directions, clic gauche = vers le curseur (`aim_direction` dans `game.c`, depuis la hauteur de vol des notes). `main.c` convertit les événements en coordonnées du jeu avant `input_handle_event` ; un clic qui sert à l'interface appelle `input_block_mouse()`.

## Code

- `src/config.h` / `config.c` : **tous** les paramètres de gameplay dans la struct globale `tuning`. Un nouveau réglage va là, pas en dur dans le code.
- `game.c` : état d'une partie (`Game`) et règles (collisions entre entités, dégâts, mort, changement de salle). `main.c` ne contient que la plomberie SDL (fenêtre, événements, boucle, debug).
- `floor.c` : plan de l'étage (topologie seule : salles, formes, portes, graine). `room.c` : géométrie de la salle courante construite depuis le plan (murs, portes, caméra, rendu).
- **Coordonnées du monde = coordonnées « page » du GDD** : le sol de la cellule (cx, cy) est en (cx*768, cy*360). Tout rendu dans le monde prend une `Camera` (écran = monde − caméra) ; le HUD, la mini-carte et le debug sont en coordonnées écran.
- Graines : `floor_generate` mélange la graine (splitmix64) car le générateur de SDL donne des premiers tirages identiques pour les petites graines. Tout aléatoire de génération passe par `SDL_rand_r` sur cet état, pour que même graine = même étage.
- Attributs du joueur (PV, PV max, vitesse, cadence, dégâts, vitesse et portée des tirs) : `tuning.start_*` = valeurs de départ, `player.stats` (`PlayerStats`) = valeurs courantes. Le gameplay lit toujours `player.stats`. Un nouvel attribut s'ajoute aussi au tableau `ROWS` de `debug_menu.c`.
- Outils de debug : F1 (infos), F2 (menu des attributs et actions comme « Ajouter un bébé », `debug_menu.c` ; une action renvoie un `DebugAction` que `main.c` exécute). La police de debug SDL est en ASCII : pas d'accents dans ces textes.
- Un module par entité (`player`, `enemy` (bébé, boss, moine, plume, ver, sonneur ; un ennemi qui tire met `fire` à vrai et `game.c` crée ses tirs), `bullet` (tirs ennemis : notes, goutte d'encre), `projectile` (notes, ondes, rayon), `pickup` (prix), `bomb` (métronome), `hazard` (briques, flaques d'encre, encre toxique), `splatter` (explosion des ennemis morts : éclat, gouttes, taches au sol ; `enemy_died` dans `game.c`), `item` (objets passifs), `weapon` (armes), `room`, `floor`, `hud`, `minimap`, `input`, `debug_menu`), chacun avec `_init`, `_update`, `_render`.
- **Graphismes** (Phase 6) :
  - `decor.c` : décor calculé pixel par pixel (portage de `room_base.py` et `page.py`) : murs en trapèzes, portes, dalles, manuscrit de la double page, reliure. Une texture par salle, mise en cache pour l'étage (graine) ; le bois des portes fermées est une petite texture par-dessus. La construction est découpée en tranches (`job_run`) : `decor_prefetch` prépare les salles voisines en 3 ms par image.
  - `font.c` : Junicode via `stb_truetype` (`src/third_party/`), glyphes rastérisés une fois au chargement, texte écrit dans des tampons de couverture.
  - `sprite.c` + `sprite_data.h` : personnages en planches PNG dessinées par Johan (`assets/sprites/`, table `SHEETS` : fichier et taille de case ; chaque `SpriteId` pointe vers une case) ou, si le PNG manque, en grilles de caractères (une lettre = une couleur de la palette, `.` = transparent). Chaque image a une version blanche pour l'éclair de dégât. Un nouveau sprite = une entrée de `SpriteId` + sa case de planche (grille de secours facultative : `DEF_PNG` n'en a pas). Les planches provisoires des ennemis de la phase 7 sont de Claude, à remplacer par celles de Johan.
  - `audio.c` (Phase 10) : sons synthétisés au démarrage (voix chantée par formants : chaque tir chante la syllabe suivante d'*Ut queant laxis*), flux SDL rempli à chaque image par `audio_update()` depuis `SDL_AppIterate`. Aucun fichier audio ; muet sans périphérique (`SDL_AUDIO_DRIVER=dummy` pour les tests).
  - Les fichiers du jeu sont dans `assets/` (copié à côté de l'exécutable par CMake, `--preload-file` pour le web) et chargés via `SDL_GetBasePath()` + `SDL_IOStream`.
- **Tirs** : armes et objets ne codent pas de tir à part, ils règlent une `ShotSpec` (`weapon_shot` dans `weapon.c` : (départ + objets) × arme, plus les objets de forme comme Trope ou Altius) que `fire()` dans `game.c` transforme en projectiles. Un nouvel effet de tir = un champ de `ShotSpec` / `Projectile`, pour qu'il se combine avec tout le reste. Les armes sont décrites dans `tuning.weapons[]`.
- `draw.c` : petites primitives de dessin partagées (rectangle, ellipse, trait, icônes métronome/couronne, chiffres pixel, texte avec la police de debug en retirant les accents).
- Rendu d'une salle en couches (`game.c`) : sol et trappe → taches de sang et d'encre → ombres au sol (et trou du ver, ombres des tirs ennemis) → tout ce qui est trié par la position des pieds (joueur, ennemis, lutrins, métronomes, prix) → ce qui est en l'air (briques qui tombent, notes, tirs ennemis, gouttes et éclats des morts, explosions). Un nouvel objet posé au sol s'ajoute à la liste triée.
- Piège : `DIR_NONE` vaut -1, donc un `Input` initialisé à zéro signifie « tirer vers le haut ». Toujours écrire `{ 0, 0, DIR_NONE }`.
- Comparaisons de minuteries flottantes (cadence, invincibilité) : utiliser une petite tolérance (`1e-4f`), sinon un arrondi décale d'une image.
- Coordonnées en pixels du jeu (960×540), temps en secondes, vitesses en px/s.
- Commentaires en français. Le jeu est bilingue FR/EN : **tout texte affiché au joueur passe par `text.c`** (`TextId` + `text_get`), avec ses deux versions. Seuls les outils de debug (F1, F2) ne sont pas traduits. La langue suit les préférences du système (`text_init_from_locale`).
- Compiler sans avertissement (`-Wall -Wextra -Wpedantic -Wshadow`).
