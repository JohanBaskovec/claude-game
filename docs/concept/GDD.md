# FUGUE IN AMADEUS MINOR — Game Design Document (brouillon)

> Document de travail destiné à Claude Code. Il décrit l'univers et les choix de design.
> **Statut : en cours de conception.** Les sections marquées ✅ sont décidées, 💡 sont des idées à valider, ❓ sont des questions ouvertes.
> **Source de vérité du design : ce fichier, dans le dépôt** (`docs/concept/GDD.md`). La conception se fait désormais uniquement avec Claude Code, qui le met à jour au fil des décisions prises avec Johan.
> **Phase en cours : Phase 5 (section 14).** Les Phases 1 à 4 (sections 10 à 13) sont terminées. Le reste sert de contexte, ne rien coder au-delà sans demande explicite.

---

## 1. Concept en une phrase

Un roguelike en 2D vue de dessus, dans la lignée de *The Binding of Isaac* : on va de salle en salle, on ramasse des objets qui rendent plus puissant et forcent des choix. Le thème est **l'histoire de la musique, du IXe siècle à nos jours**, avec un ton d'humour noir, absurde et érudit. L'objectif est que les gens réagissent par « mais c'est quoi ce jeu de fou ? ».

- **Gameplay** : volontairement ultra simple, façon TBOI. Ce n'est **pas** un jeu de rythme : on n'est pas obligé de tirer en cadence.
- **Différenciation** (vs Soundfall, Kill The Music, Crypt of the NecroDancer…) : l'axe **historique + anachronique + humour noir**. Aucun concurrent identifié ne le fait.
- **Langues** : le jeu doit fonctionner en français **et en anglais**. Les noms d'objets doivent avoir une version dans chaque langue.

## 2. Titre

✅ **Fugue in Amadeus Minor** (FR : *Fugue en Amadeus mineur*).

- **« Fugue »** a un double sens : une forme musicale ET la fuite de l'enfant.
- Le titre parodie les titres d'œuvres classiques (« Fugue en sol mineur »).
- **« mineur / minor »** désigne à la fois la tonalité et un enfant : le titre se lit aussi « la fuite d'Amadeus, mineur ». Le double sens marche en français et en anglais.
- Une recherche web n'a trouvé aucun jeu sous ce nom, seulement de vraies fugues de Mozart.

## 3. Histoire ✅

**Cinématique d'intro**, sans dialogue explicite, tout est suggéré :
1. De nos jours, un père fou de musique écoute un enregistrement de chanteur castrat, en extase.
2. Il regarde son fils de 8 ans, qui chante. L'idée lui vient.
3. Il prend une paire de ciseaux et court après l'enfant.
4. L'enfant s'enfuit et s'enferme dans une bibliothèque.
5. Il ouvre un livre ancien rempli de vieilles partitions, qui l'aspire et le téléporte au **IXe siècle**.

**Suite** : l'enfant décide de combattre la musique et ses créateurs. Il traverse les siècles jusqu'à revenir au présent.

💡 **Boss final** : le père, avec ses ciseaux.

**Ton** : la castration n'est jamais nommée ni montrée. Le malaise vient de la suggestion, comme la mère d'Isaac dans TBOI.

✅ **Nom du protagoniste : Amadeus** (« aimé de Dieu »). C'est un prénom que des parents fanatiques de musique classique donneraient à leur fils.
- L'ironie est forte : le père de Mozart, Leopold, exhibait son enfant prodige à travers l'Europe. C'est la figure du père qui sacrifie son fils à la musique.
- Le parallèle avec Isaac et Abraham (un père prêt à sacrifier son fils) est volontaire.
## 4. Le personnage et le combat

- ✅ Le héros commence avec **sa voix seulement** : il chante, et ses projectiles sont des notes de musique.
- ✅ On trouve ensuite des **instruments d'époque**, qui servent d'armes.
- ✅ L'anachronisme est voulu et fait partie de l'humour : on peut se retrouver avec un saxophone face à des moines du Xe siècle.

### Attributs du joueur ✅
Comme dans TBOI, le joueur a des attributs chiffrés que les objets modifieront.

| Attribut | Valeur de départ | Unité | Notes |
|---|---|---|---|
| Points de vie | **10** | PV | PV actuels. Ne dépassent jamais les PV maximum. |
| PV maximum | **10** | PV | Équivalent des réceptacles de cœur de TBOI. Plafond absolu : 100 (provisoire). Voir « Barre de vie » en section 5. |
| Vitesse de déplacement | 270 | px/s | |
| Cadence de tir | 3,0 | notes/s | |
| Dégâts | 1,0 | PV ennemis | Dégâts de chaque note, fixés au moment où elle part. Peut être fractionnaire (objets multiplicateurs). |
| Vitesse des tirs | 480 | px/s | |
| Portée des tirs | 340 | px | Distance parcourue avant que la note disparaisse. |

- Les valeurs de départ sont provisoires, à régler avec le menu de debug (section 10).
- ✅ Comme dans TBOI, on distingue **PV actuels** et **PV maximum** : un soin remplit la barre sans l'allonger, un objet « réceptacle » augmente le maximum.
- **Dégâts subis** : on perd des PV, puis on a une courte **invincibilité** (≈ 1 s, provisoire) pendant laquelle le personnage **clignote**, comme dans TBOI. À 0 PV, c'est la mort.
- ✅ Attribut **Dégâts**, comme TBOI. Les PV des ennemis sont donc fractionnaires ; ceux du joueur restent entiers.

### Barre de vie : un soufflet de crescendo ✅
- La barre de vie a la forme d'un **soufflet de crescendo** (<) : deux traits qui s'écartent de gauche à droite.
- L'intérieur du soufflet **se remplit** selon les PV actuels, depuis la pointe : à 0 PV il est vide, à PV max il est plein. Une perte de PV se vide en douceur, pour que le joueur la voie.
- La **longueur** du soufflet grandit avec les PV maximum : gagner un réceptacle allonge le crescendo.
- Placée en haut à gauche de l'écran, sur le mur du haut.

### Design d'Amadeus ✅
- ✅ **Objectif** : un gamin moderne qui fait tache dans les époques anciennes.
- ✅ **Proportions façon TBOI** : grosse tête (environ la moitié du sprite), petit corps. Indispensable pour rester lisible à petite taille.
- **Tête et corps en sprites séparés**, comme dans TBOI : la tête regarde dans la direction du tir, le corps dans la direction du déplacement.
- ✅ **Tir** : il chante, donc sa bouche s'ouvre en grand « O » à chaque note (équivalent des larmes d'Isaac).
- **Couleurs** : des teintes fluo absentes des palettes historiques (rose, vert citron, cyan), pour qu'il ressorte à chaque époque. Au dernier étage (aujourd'hui), il se fond enfin dans le décor.
- ✅ **Tenue : pyjama + casque audio.** Il s'est enfui la nuit :
  - pyjama cyan fluo à petits dinosaures vert citron ;
  - chaussons vert citron ;
  - casque audio rose fluo **sur la tête**, écouteurs sur les oreilles : il se protège des bruits produits par ses ennemis (justification interne de l'univers) ;
  - cheveux en bataille, sortie de lit.
- ✅ **Mèche coupée** : il manque une mèche de cheveux, coupée par les ciseaux du père qui l'a raté de peu. Rappel discret de l'intro, jamais expliqué.
- ✅ **Sprite concept v2 : 56×68 px** (tête ≈ 42×38 px), pour la résolution 960×540. Détails :
  - casque rose avec une croche blanche sur chaque écouteur ;
  - yeux marron avec reflets ;
  - mèche coupée avec repousse visible ;
  - pyjama cyan avec col et poignets vert citron, boutons et petits dinos ;
  - **chaussons-dinosaures** vert citron, avec yeux et dents.
  - Deux états : repos (petite bouche inquiète) et chante (bouche grande ouverte).
  - L'ancien concept 28×33 px (pour 480×270) est abandonné.

### Bébé médiéval : premier ennemi ✅
- **Premier ennemi implémenté** (Phase 2) : il **suit le joueur** et le blesse **au contact**, comme les Gapers de TBOI.
- **5 PV.** Contact : 1 PV de dégâts au joueur (provisoire). Plus lent que le joueur, avec de l'inertie, pour qu'on puisse l'esquiver.
- Les notes le repoussent légèrement et le font clignoter à chaque touche.

Concept visuel 💡 :
- Enfant Jésus en « petit adulte » : **proportions d'adulte** (petite tête, corps raide et musclé), l'inverse d'Amadeus.
- Visage de vieillard : calvitie, rides, regard en coin méfiant, bouche pincée.
- Nimbe doré crucifère derrière la tête, geste de bénédiction à deux doigts, lange.
- Carnation ocre avec ombres verdâtres (verdaccio des peintres médiévaux), pour ressortir sur le sol parchemin.
- 💡 Comme dans TBOI, **les objets ramassés modifient son apparence** : perruque baroque, tonsure de moine, etc. Il finit en patchwork d'époques.

### Armes (instruments) ✅
- Amadeus porte **une arme** qui a ses propres attributs de tir.
- **Calcul** : attribut final = (valeur de départ + bonus des objets) × multiplicateur de l'arme. L'arme définit ses multiplicateurs et sa façon de tirer ; les objets donnent des bonus.
- Il commence avec **sa voix**. Ramasser une arme **remplace** l'arme actuelle, qui **reste posée sur le socle** : on peut revenir la reprendre.
- **Où les trouver** : **1 arme** dans chaque salle du trésor et **1 arme** après chaque boss, **en plus** des objets passifs. L'arme ne fait pas partie du choix « 1 objet sur 3 » : on peut prendre l'arme et un objet.
- **Un jeu d'armes par époque** (luth à la Renaissance, clavecin au Baroque, guitare électrique au XXe…). Ci-dessous, les armes du Haut Moyen Âge.
- Qu'un objet passif ressemble à une arme n'est pas un problème : ça laisse le choix au joueur.

| Arme | Effet |
|---|---|
| Voix | Arme de départ : la note. |
| Olifant | Gros projectile lent, gros dégâts, cadence et vitesse de tir faibles. **Étourdit.** |
| Flûte | Tir à charger : plus on relâche tard, plus les dégâts sont gros. Le temps de charge dépend de la cadence de tir. |
| Lyre | 3 notes en même temps, en arc de cercle ; cadence plus faible que la voix. |
| Tabourin | Pas de note : une **onde de choc en arc de cercle** qui traverse les ennemis. Sinon mêmes attributs que la voix. Les objets passifs s'y appliquent quand ça a un sens (flaque d'encre au bout de l'onde, pause en l'air…). |
| Cloche d'église | Dégâts ×10, cadence ÷8. Le son **résonne tout autour d'Amadeus** (onde circulaire), pour qu'un tir ne soit jamais raté. |
| Orgue | Dégâts ×20, cadence ÷10. Un **gros rayon qui traverse tout**. Amadeus est **cloué sur place ≈ 0,5 s** à chaque tir, le temps de jouer l'accord. |

- **États des ennemis** (système commun) : **étourdi** (ne bouge plus) et **ralenti**.

## 5. Équivalences avec TBOI

| TBOI | Fugue in Amadeus Minor | Statut | Notes |
|---|---|---|---|
| Clés | **Clés** (de sol, de fa, d'ut) | ✅ | 3 types de clés, donc **3 types de portes**. En anglais, *clef* et *key* marchent aussi (*key* = tonalité). |
| Bombes | **Métronome** | ✅ | Fait tic-tac en accélérant, puis explose. |
| Cartes / runes | **Partitions** | ✅ | |
| Trinkets | **Ornements** (trille, mordant, appoggiature…) | ✅ | « Ornement » = breloque et fioriture musicale. |
| Pilules | **Nuances** (*pp, p, mp, mf, f, ff, sfz, crescendo, decrescendo*…) | 💡 | Une douzaine de visuels distincts. Comme dans TBOI, l'effet associé à chaque nuance est **tiré au hasard à chaque partie** et inconnu tant qu'on ne l'a pas essayé. |
| Malédictions d'étage | **Armure** (altérations à la clé : dièses, bémols) | 💡 | En musique, l'armure placée en début de portée s'applique à tout le morceau. Ici, elle s'applique à tout l'étage. Ex. : 1 bémol = salles plus sombres, 2 dièses = ennemis plus rapides. |
| Pièces | **Royalties** | ✅ | Même mot en FR et en EN. Anachronisme volontaire : toucher des royalties au IXe siècle. |
| Cœurs (vie) | **Barre de vie** | ✅ | On commence à **10**, maximum **100** (provisoire). **Nom à trouver** : il doit convenir à tous les instruments (voix, vents, cordes, percussions), donc pas « souffle ». Pistes : *Résonance*, *Sustain* (utilisé tel quel en FR et EN par les musiciens). Idée visuelle : la barre a la forme d'un **soufflet de crescendo** (<). |
| Salle du Diable | **Tartini** | 💡 | Il prétendait que le Diable lui avait joué la *Sonate du trille du Diable* en rêve. |
| Salle de l'Ange | **Hildegard von Bingen** | 💡 | Nonne visionnaire du XIIe siècle. |
| Boutique | **Luthier** | 💡 | |

## 6. Objets inspirés d'anecdotes réelles 💡

- **Bâton de Lully** : gros dégâts, mais on se frappe parfois le pied. Lully est mort de la gangrène après s'être blessé au pied avec son bâton de direction (1687).
- **Crâne de Haydn** : volé par des phrénologues après sa mort, il n'a été réuni au corps qu'en 1954.
- **Partition de Bosch** : dans *Le Jardin des délices*, une partition est peinte sur les fesses d'un damné.
- **4'33" (Cage)** : le héros devient silencieux, donc invisible pour les ennemis.
- **Vexations (Satie)** : le tir se répète en boucle. La pièce doit être jouée 840 fois.
- **Bibliothèque d'Alkan** : selon la légende, il est mort écrasé par sa bibliothèque.
- **Main guidonienne** : moyen mnémotechnique médiéval où chaque phalange porte une note. Peut aussi devenir un personnage jouable, une main tranchée et vivante.

### Neumes et polyphonie : objets passifs du Haut Moyen Âge 💡
Ils modifient les notes (comme les objets de tir de TBOI) et se combinent entre eux.

**Lettres significatives** (petites lettres ajoutées aux neumes dans les manuscrits de Saint-Gall) :

| Objet | Sens | Effet |
|---|---|---|
| c (*celeriter*) | « vite » | Cadence de tir en hausse. |
| t (*tenete*) | « tenez » | Les notes durent plus longtemps (portée en hausse). |
| a (*altius*) | « plus haut » | Les notes passent au-dessus des obstacles. |
| x (*expectare*) | « attendez » | Les notes marquent une pause en l'air, puis repartent. |
| s (*sursum*) | « vers le haut » | Les notes s'élèvent et retombent en zone (tir en cloche). |

**Formes de neumes** :

| Objet | Effet |
|---|---|
| Épisème (le petit trait qui allonge une note) | Notes plus longues et plus lourdes : plus de dégâts, moins vite. |
| Quilisma (le neume tremblé) | Trajectoire qui vibre, plus facile de toucher. |
| Liquescence (le neume qui « fond ») | Les notes laissent une flaque d'encre qui ralentit les ennemis. |

**Polyphonie et chant** :

| Objet | Effet |
|---|---|
| Organum | Chaque note est doublée par une note parallèle, à la quinte. Première polyphonie, décrite par la *Musica enchiriadis* (fin du IXe siècle). |
| Mélisme | Chaque tir devient une rafale de petites notes (beaucoup de notes sur une seule syllabe). |
| Jubilus | La dernière note de chaque rafale est énorme (le long mélisme final de l'Alléluia). |
| Antiphonie | Les tirs alternent entre la gauche et la droite d'Amadeus, comme deux chœurs qui se répondent. |
| Répons | Chaque tir déclenche un « écho » du chœur une seconde plus tard. |
| Teneur (note de récitation des psaumes) | Tirer sans bouger crée un rayon fixe. |
| Trope | Un projectile de plus à chaque tir (un trope est un ajout dans un chant existant). |

Précisions ✅ :
- **Quilisma** : la trajectoire qui vibre s'accompagne d'une note plus grosse (sinon la vibration rendrait la visée plus difficile).
- **Jubilus** sans Mélisme : une note énorme toutes les 5 notes.
- Les doublons avec les armes sont voulus (Teneur / Orgue, Épisème / Olifant, Trope / Lyre / Organum) : le joueur choisit. Trope tire en éventail, Organum en parallèle, Lyre en arc.
- Les objets d'anecdotes (Lully, Haydn…) deviennent des objets **rares**, possibles à toutes les époques.

## 7. Personnages déblocables 💡

| Personnage | Gimmick |
|---|---|
| Josquin, Monteverdi… | À définir |
| **Bach** | 20 enfants : démarre avec plein de familiers. |
| **Mozart** | Humour scatologique (canon *Leck mich im Arsch*). Équivalent des objets « caca » de TBOI. |
| **Beethoven** | Sourd : le son du jeu est coupé, on joue aux indices visuels. |
| **Paganini** | Soupçonné de pacte avec le Diable. Ses cordes cassent en combat jusqu'à ce qu'il n'en reste qu'une. |
| **Liszt** | Lisztomanie : charme les ennemis. |
| **Gesualdo** | Prince compositeur meurtrier. Chromatique et instable. |
| **Coltrane** | *Sheets of sound* : remplit l'écran de notes. |

## 8. Étages et ennemis 💡

On découpe par **époques** plutôt que par siècles, car entre le IXe et le XIIe rien ne change visuellement. Chaque époque amène des instruments, des partitions et des objets de son temps.

| # | Époque | Idées d'ennemis |
|---|---|---|
| 1 | Haut Moyen Âge (IXe–XIIe) | ✅ **Bébés horribles** de l'iconographie médiévale (enfant Jésus en « petit adulte »). Moines grégoriens qui tirent tous la même note en même temps. Monstres de bestiaires : blemmyes (visage sur le torse), cynocéphales (tête de chien), sciapodes (un pied géant qui sert de parasol). Bêtes des Beatus (Apocalypses enluminées des Xe–XIe s.), chapiteaux romans vivants. |
| 2 | Ars Nova / peste (XIVe) | Squelettes de la danse macabre, médecins de peste |
| 3 | Renaissance (XVe–XVIe) | Danseurs de l'épidémie de danse de Strasbourg (1518), tarentules |
| 4 | Baroque | Perruques volantes |
| 5 | Classique | Automates |
| 6 | Romantique | À définir |
| 7 | XXe siècle | Robots dodécaphoniques |
| 8 | Aujourd'hui | Retour au présent, **boss final : le père** |

💡 **Famille « bébés » qui évolue avec les époques** : petit adulte médiéval raide et ridé, puis putti joufflus de la Renaissance, puis chérubins baroques partout, puis poupons de pub des années 50. Le même ennemi change de style à chaque époque.

💡 **Instruments du premier étage** : organistrum (ancêtre de la vielle à roue, qui se joue à deux), lyre, harpe, psaltérion, olifant, cloches, monocorde.

💡 Autres idées de boss : la musique d'ascenseur, un algorithme de streaming qui ne joue que des extraits de 30 secondes, le Triton (*diabolus in musica*).

## 9. Direction artistique 💡

- ✅ **Pixel art.** Outil de dessin : **Aseprite**, ou LibreSprite (fork gratuit).
- 💡 **Une palette de couleurs limitée par époque** : par exemple or, rouge et bleu lapis pour le manuscrit médiéval. C'est le moyen le plus simple de rendre chaque époque reconnaissable en pixel art.
- 💡 **Référence visuelle** : *Blasphemous*, pour l'iconographie religieuse en pixel art.

- ✅ **Principe central : chaque niveau est une partition.** Le sol des salles est une page de musique de l'époque, et les salles sont délimitées par la mise en page (portées, mesures, colonnes du manuscrit).
- ✅ **Lisibilité** : lignes et symboles du sol **très ternes**, faible contraste, désaturés. Ils ne doivent jamais être confondus avec les ennemis, les projectiles ou les objets.
- ✅ **Forme des salles, comme TBOI** : 1×1, 1×2 / 2×1, 2×2 et en L.
- ✅ **Perspective façon TBOI** : fausse vue de dessus. Le sol est vu de dessus, les murs en trapèzes vus de côté. Le mur du haut est le plus profond (on voit sa face), celui du bas le moins profond. Une porte au milieu de chaque mur ; sur les murs gauche, droit et bas, la porte est « couchée » ou retournée, comme dans TBOI.
- **Concept de salle 1×1 (v2, 960×540, plein écran)** :
  - sol : rectangle de 96 à 864 en x et de 108 à 468 en y, soit **768×360 px** (une « cellule » de la page, voir plus bas) ;
  - profondeur des murs : haut 108 px, côtés 96 px, bas 70 px ;
  - murs : briques de château en perspective, avec biseaux, éclats et assombrissement vers le haut du mur ;
  - sol : grandes dalles de pierre peu contrastées ;
  - portes en bois (planches, veinure, ferrures rivetées, heurtoir), sous un arc de claveaux de pierre.
- ✅ **Sol du premier étage : un manuscrit de chant à neumes**, pas de lignes :
  - comme dans les manuscrits de Saint-Gall (IXe–Xe s.), du **texte latin** avec des **neumes** posés au-dessus des syllabes, sans portée ;
  - syllabes espacées pour laisser la place aux neumes ;
  - **rubriques à l'encre rouge** et **lettrine rouge** ;
  - l'encre est usée et à faible opacité (≈ 50 % pour le noir, 50 % pour le rouge), lisible sans gêner le jeu.
  - Textes utilisés :
    - l'introït de Noël ***Puer natus est nobis*** (« un garçon nous est né »), clin d'œil au héros ;
    - l'hymne ***Ut queant laxis***, dont les premières syllabes ont donné le nom des notes (ut, ré, mi, fa, sol, la).
  - Police : **Junicode** (voir la section « Police du sol » ci-dessous).
  - Pour la version finale, s'inspirer de vrais neumes de Saint-Gall (manuscrits numérisés et libres), plutôt que de neumes tirés au hasard.

### ✅ Un étage = une double page de livre de chœur
- **Tout l'étage est un livre de chœur ouvert : une double page**, deux pages portrait côte à côte, avec la **reliure au milieu**. Le niveau *est* une partition.
- **Grille de la partition : 7 cellules de large × 10 de haut** (au maximum). Chaque salle 1×1 occupe une cellule.
  - Pourquoi 7×10 : deux pages A4 portrait côte à côte forment un rectangle paysage de ratio 1,414:1, comme un A3. Une cellule de sol fait 768×360 px (plus large que haute), donc 7×10 cellules = 5376×3600 px, soit ≈ 1,49:1. C'est très proche, et le ratio reste juste en pixels, pas seulement sur la carte.
  - Chaque page fait 3,5 cellules × 10, soit 2688×3600 px (≈ 0,75, proche du A4 portrait à 0,71).
- ✅ **La reliure** passe verticalement au **milieu de la colonne centrale** (colonne 3, en partant de 0), à x = 2688 en coordonnées page :
  - les salles de cette colonne sont coupées en deux par le pli : ombre sombre du creux de la reliure, fils de couture visibles ;
  - le texte s'arrête de chaque côté, chaque page a sa propre marge intérieure ;
  - 💡 idées de gameplay : la reliure peut devenir un obstacle (crevasse, fils à traverser), ou une colonne de salles spéciales.
- ✅ **Salles hors partition** : des salles bonus ou secrètes peuvent exister **en dehors de la grille 7×10**. Elles se trouvent « dans les marges » du livre :
  - pas de texte au sol ;
  - le décor et les ennemis viennent des **marginalia** médiévales (lapins tueurs, escargots chevaliers, hybrides grotesques).
  - Le joueur comprend instinctivement qu'il est sorti de la partition.
- **Coordonnées « page »** :
  - une cellule = la zone de sol d'une salle 1×1 = **768×360 px** ;
  - la double page entière = **5376×3600 px** (7×10 cellules) ;
  - page gauche : x de 0 à 2688 ; page droite : x de 2688 à 5376 ;
  - la salle (cx, cy) affiche la fenêtre `[cx·768, (cx+1)·768[ × [cy·360, (cy+1)·360[`.
- **Le texte passe sous les murs** et continue dans la salle voisine : une ligne coupée au mur de droite reprend dans la salle de droite. Une salle 2×1, 2×2 ou en L affiche simplement plusieurs cellules contiguës.
- **Lignes** : interligne de **60 px**, donc **exactement 6 lignes par cellule**, à des hauteurs fixes (ligne de base à `46 + 60·k` en coordonnées page). Aucune ligne n'est coupée en hauteur par un mur.
- **Marges** :
  - marge extérieure de 150 px (bord gauche de la page gauche, bord droit de la page droite) ;
  - marge intérieure d'environ 120 px de chaque côté de la reliure ;
  - la première et la dernière ligne de chaque page restent vides ;
  - les marges ne sont visibles que dans les salles du bord de la grille et dans la colonne de la reliure, plus tard le lieu idéal pour des *marginalia*.
- **Sens de lecture** : le texte remplit toute la page de gauche, ligne après ligne, puis continue en haut de la page de droite.
- **Contenu** : des chants latins du haut Moyen Âge (domaine public) qui s'enchaînent en continu :
  - *Puer natus est*, *Ut queant laxis*, *Veni creator spiritus* (IXe s.), *Kyrie*, *Ave maris stella* (IXe s.), *Gloria*, *Te Deum*, *Agnus Dei* ;
  - chaque chant commence par une **rubrique rouge** puis une **initiale rouge** ;
  - chaque page commence par une **grande lettrine rouge sur deux lignes**.
- **Syllabation latine automatique** (règles simplifiées) :
  - une consonne seule va avec la voyelle suivante ;
  - deux consonnes se séparent, sauf muta cum liquida (*bl, br, cl, cr, pl, tr*…) et *ch, ph, th, gn* ;
  - diphtongues *ae, oe, au* ; *qu* compte comme une consonne ; *i* consonne au début d'un mot ou entre deux voyelles (*e-ius*).
  - Résultats vérifiés : *Pu-er*, *ma-gni*, *e-le-i-son*, *fi-bris*, *Io-han-nes*.
- **Neumes** : 1 à 3 par syllabe (pour l'instant tirés au hasard, avec une graine fixe par étage).
- **Carte / minimap** : la vue d'ensemble de l'étage montre la double page entière, reliure comprise, avec les salles découvertes éclairées et le reste dans l'ombre.
- **Prototype de référence** : scripts Python fournis à part, que Claude Code peut porter en C :
  - `page.py` : double page 7×10, mise en page des deux pages, syllabation, neumes, rendu d'une cellule, reliure (ombre du pli + points de couture tous les 120 px) ;
  - `room_base.py` : murs, portes, dalles ;
  - `compose_page.py` : rendus de démonstration (salle de la page gauche, salle de la reliure, haut de la double page en 3×2, vue d'ensemble 7×10) ;
  - `amadeus_hd.py` : le sprite d'Amadeus.
  - En jeu, le plus simple est de **pré-rendre le sol de chaque cellule** dans une texture 768×360 (ou toute la page une fois par étage), plutôt que de dessiner le texte à chaque frame.

### ✅ Police du sol
- **Junicode**, version 2.211, de Peter S. Baker. Police libre sous licence **SIL Open Font License 1.1** : utilisable et redistribuable dans un jeu commercial, à condition de joindre la licence.
- Source : https://github.com/psb1558/Junicode-font (fichiers `Junicode/TTF/` de l'archive de la release).
- Police érudite pour médiévistes. Son dessin humaniste dérive de la minuscule caroline, l'écriture des manuscrits du IXe siècle.
- **Graisses et tailles utilisées** (en px du jeu, 960×540) :

  | Usage | Fichier | Taille | Couleur |
  |---|---|---|---|
  | Texte latin | `Junicode-Medium.ttf` | 22 px | encre brune (58, 40, 30) |
  | Rubriques | `Junicode-Medium.ttf` | 13 px, capitales | rouge (150, 52, 38) |
  | Initiales de chant | `Junicode-Bold.ttf` | 40 px | rouge |
  | Grande lettrine | `Junicode-Bold.ttf` | 92 px, sur 2 lignes | rouge |

- **Rendu** :
  - l'encre est dessinée en ×2 puis réduite (anticrénelage doux), et appliquée à environ **50 % d'opacité** ;
  - elle est modulée par un bruit d'« usure », comme une encre piétinée.
  - En SDL : pré-rendre avec **SDL3_ttf** dans une texture, ou exporter un atlas de glyphes.
- 💡 **La notation évolue avec les époques**, ce qui renforce le voyage dans le temps. Les barres de mesure n'apparaissent qu'à l'époque baroque : avant, les salles sont délimitées par la mise en page du manuscrit.

  | Époque | Notation au sol |
  |---|---|
  | IXe–Xe | Neumes sans portée |
  | XIe–XIIe | Portée à 4 lignes (Guy d'Arezzo), notation carrée |
  | XIVe–XVIe | Notation mensurale, notes blanches losangées |
  | Baroque → Romantique | Portée à 5 lignes, barres de mesure |
  | XXe | Partitions graphiques (Cage, Cardew…) |
  | Aujourd'hui | *Piano roll* de logiciel de MAO |
- **Le style graphique évolue avec les époques** :
  1. manuscrit enluminé à l'or, avec des marges délirantes (lapins tueurs, escargots géants) ;
  2. gravure sur bois ;
  3. peinture baroque ;
  4. affiche Art nouveau ;
  5. bande dessinée années 50 ;
  6. glitch numérique.
- **Les personnages gardent un style simple et constant**, façon TBOI. Seuls le décor et la palette changent, ce qui rend le projet faisable en solo.

---

## 10. PHASE 1 — Prototype minimal ✅ (terminée)

**But** : valider les sensations de déplacement et de tir. Rien d'autre.

### Périmètre
- **Une seule salle vide**, vue de dessus, de la taille d'un écran, entourée de murs.
- **Un personnage** (placeholder : cercle ou carré coloré) qui se déplace dans la salle et ne traverse pas les murs.
- **Tir de notes de musique** (placeholder : un symbole ♪ ou un petit cercle) :
  - contrôles façon TBOI : **déplacement ZQSD/WASD**, **tir avec les flèches** dans 4 directions ;
  - cadence de tir limitée (variable `fireRate`) ;
  - les projectiles ont une vitesse et une portée, puis disparaissent ;
  - ils disparaissent aussi en touchant un mur ;
  - 💡 bonus si simple : le projectile hérite d'une partie de la vitesse du joueur, comme dans TBOI.
- **Paramètres regroupés dans un seul endroit** pour être faciles à ajuster : vitesse du joueur, cadence, vitesse des projectiles, portée.
- ✅ **Attributs du joueur** (section 4) : points de vie, vitesse de déplacement, cadence de tir, vitesse des tirs, portée des tirs. Les valeurs de départ viennent des paramètres ; le joueur garde ses valeurs courantes, que les objets modifieront plus tard.
- ✅ **Outils de debug** (non traduits, réservés au développement) :
  - **F1** : FPS et infos techniques ;
  - **F2** : menu des attributs du joueur, avec des boutons **−** et **+** cliquables à la souris pour les modifier en direct pendant la partie, et un bouton pour revenir aux valeurs de départ.

### Hors périmètre (ne pas faire)
Ennemis, objets, salles multiples, dégâts et mort (les PV existent mais rien ne les fait baisser), sons, graphismes finaux, menus du jeu.

### Technique ✅
- **Langage : C.** **Bibliothèque : SDL3** (fenêtre, entrées, rendu 2D via `SDL_Renderer`). Pas de moteur.
- **Plateformes cibles : Windows, Linux, et navigateur** (WebAssembly via **Emscripten**). Le code doit rester portable dès le départ :
  - utiliser le **modèle à callbacks de SDL3** (`#define SDL_MAIN_USE_CALLBACKS` avec `SDL_AppInit`, `SDL_AppIterate`, `SDL_AppEvent`, `SDL_AppQuit`) **au lieu d'une boucle `while` bloquante**, car le navigateur ne tolère pas de boucle infinie ;
  - le rendu passe uniquement par `SDL_Renderer`, qui utilise WebGL dans le navigateur ;
  - les fichiers (images, sons) sont chargés via les fonctions SDL (`SDL_IOStream`) et embarqués avec `--preload-file` en build web ;
  - pas de threads, pas de chemins absolus, pas d'API spécifique à un OS ;
  - côté son plus tard : dans le navigateur, l'audio ne démarre qu'après une première interaction du joueur.
- **Images** : PNG chargés avec `stb_image.h` (un seul fichier header) ou SDL3_image. Plus tard, regrouper les sprites dans des *sprite sheets*.
- **Pixel art** (si retenu) : petite résolution logique avec `SDL_SetRenderLogicalPresentation`, et filtrage `SDL_SCALEMODE_NEAREST` sur les textures pour garder des pixels nets.
- **Build : CMake.** SDL3 est récupéré via `FetchContent` ou trouvé via `find_package`.
- **Boucle de jeu à pas de temps fixe** (logique à 60 Hz, rendu découplé) pour que les déplacements et la cadence de tir ne dépendent pas des FPS.
- **Rendu placeholder** : uniquement des rectangles et formes simples (`SDL_RenderFillRect`). Aucune image, aucune police pour l'instant.
- **Paramètres de gameplay** regroupés dans un `config.h` (ou une struct globale `Tuning`).
- **Projectiles** dans un tableau statique de taille fixe (pool), sans `malloc` par tir.
- **Structure de départ suggérée** :
  - `main.c` : init SDL, boucle de jeu ;
  - `player.c/.h` ;
  - `projectile.c/.h` ;
  - `room.c/.h` : murs et collisions AABB ;
  - `input.c/.h` ;
  - `config.h`.
- **Collisions** : AABB (rectangles alignés), suffisant pour une salle à la TBOI.

### Résolution 💡
- ✅ **Résolution logique : 960×540** (16:9), mise à l'échelle par un facteur entier : ×2 = 1920×1080, ×4 = 4K. 480×270 a été jugé trop « gros pixels » : on ne lisait ni les neumes ni le texte latin au sol.
- Conséquence : les sprites doivent être dessinés pour 960×540. Amadeus fait désormais 56×68 px.
- Utiliser `SDL_SetRenderLogicalPresentation` en mode *integer scale*, et `SDL_SCALEMODE_NEAREST` pour garder des pixels nets.
- ❓ Taille des tuiles et dimensions exactes d'une salle 1×1, à ajuster une fois le prototype en main. Le concept v2 donne un point de départ.

---

## 11. PHASE 2 — Premier ennemi, vie et dégâts ✅ (terminée)

**But** : valider la boucle de combat de base : esquiver, tirer, prendre des coups.

### Périmètre
- ✅ **PV actuels et PV maximum** (section 4), modifiables dans le menu de debug F2.
- ✅ **Barre de vie en soufflet de crescendo** (section 4), en haut à gauche.
- ✅ **Bébé médiéval** (section 4) : suit le joueur, 5 PV, dégâts au contact. Placeholder en formes simples.
- ✅ **Les notes blessent les ennemis** (attribut Dégâts, 1 au départ) et disparaissent en les touchant.
- ✅ **Invincibilité après un coup** (≈ 1 s) avec clignotement du joueur.
- **Mort (provisoire)** : à 0 PV, la salle recommence (joueur au centre avec tous ses PV, un nouveau bébé). Il n'y a pas encore d'écran de fin de partie.
- **Debug** : le menu F2 permet aussi de faire apparaître un bébé.

### Hors périmètre (ne pas faire)
Autres ennemis, objets, salles multiples, écran de mort, sons, graphismes finaux, menus du jeu.

---

## 12. PHASE 3 — Un étage ✅ (terminée)

**But** : parcourir un étage généré au hasard, de salle en salle, y compris dans de grandes salles plus larges que l'écran.

### Génération de l'étage ✅
- Sur la grille de la double page : **7 colonnes × 10 lignes** de cellules (section 9).
- **10 à 12 salles** au premier étage, comme TBOI.
- **Formes de salles dès cette phase** : 1×1, 2×1, 1×2, 2×2 et en L (4 orientations). Au moins une grande salle par étage pour l'instant, pour tester.
- **Salle de départ au centre de la grille** : colonne 3, ligne 4, donc sur la reliure.
- Algorithme façon TBOI : on part de la salle de départ et on ajoute des salles voisines au hasard. Une nouvelle salle ne doit toucher qu'**une seule** salle existante, ce qui évite les boucles et donne des branches et des culs-de-sac. Si le résultat ne convient pas, on recommence.
- **Graine** : un étage est entièrement déterminé par sa graine (même graine = même étage).
- **Portes** : une porte entre deux salles voisines, au milieu de chaque côté de cellule partagé. Une grande salle peut donc avoir plusieurs portes sur un même mur.

### Salles spéciales ✅
- **Départ** : vide.
- **Salle du boss** : le cul-de-sac le plus éloigné du départ. Pas encore de vrai boss : elle contient plus de bébés (6, provisoire).
- **Salles normales** : 0 à 4 bébés au hasard, plus 2 par cellule supplémentaire pour les grandes salles (provisoire).
- Trésor, boutique, Tartini, Hildegard… : plus tard.

### Portes sans serrure ✅
- Les portes se **ferment** quand on entre dans une salle qui contient des ennemis, et s'**ouvrent** quand elle est vidée.
- Ouvertures : 84 px pour les portes du haut et du bas, 64 px pour celles des côtés (validé en jeu).
- Une salle vidée le reste : on la retraverse librement.
- Les portes à serrure (clés de sol, de fa, d'ut) viendront plus tard.

### Grandes salles et caméra ✅
- Comme dans TBOI, une grande salle **ne tient pas entière à l'écran** : la caméra suit le joueur et s'arrête aux murs de la salle.
- Une salle 1×1 remplit exactement l'écran : la caméra y est fixe.

### Passage d'une salle à l'autre ✅
- En franchissant une porte ouverte, l'écran **glisse** vers la salle voisine (≈ 0,3 s), comme dans TBOI. Le jeu est en pause pendant le glissement.
- Le joueur arrive juste devant la porte correspondante de la nouvelle salle.

### Mini-carte ✅
- En haut à droite : la double page en miniature, reliure comprise.
- Salles **visitées** éclairées, salle **actuelle** en rose, salles **découvertes** (voisines d'une salle visitée) en sombre, les autres invisibles. La salle du boss est marquée dès qu'elle est découverte.
- ✅ **Téléportation** : un clic sur une salle **déjà visitée** de la mini-carte y téléporte le joueur, au centre de la cellule cliquée. **Interdit pendant un combat** (tant qu'il reste des ennemis dans la salle), sinon on pourrait fuir n'importe quel combat. La salle survolée par la souris est entourée de blanc si on peut s'y téléporter.

### Mort
- Remplacé en Phase 4 : à 0 PV, la partie recommence de zéro (section 13).

### Debug
- **F1** affiche la graine et la salle actuelle.
- **F2** : boutons « Nouvel étage » et « Tuer les ennemis ». Quand le menu est ouvert, la mini-carte montre **tout l'étage** (salles inconnues en contour) et permet de se téléporter **partout**, même en combat. Le menu s'affiche sous la mini-carte pour la laisser cliquable.

### Hors périmètre (ne pas faire)
Sol en double page (texte latin, reliure), portes à serrure, salles spéciales autres que le boss, vrai boss, passage à l'étage suivant, objets, sons, graphismes finaux.

---

## 13. PHASE 4 — Une vraie partie ✅ (terminée)

**But** : une partie complète façon roguelike : explorer, ramasser des prix, choisir un objet, battre le boss, descendre à l'étage suivant, mourir, recommencer.

### Prix de fin de salle ✅
- Comme dans TBOI : quand on **vide une salle** de ses ennemis, un prix peut apparaître **au centre de la salle** (au centre de la cellule la plus centrale pour une grande salle).
- Le prix apparaît, puis est **aspiré** : après un court instant (≈ 0,4 s), il vole vers le joueur en accélérant et se ramasse tout seul au contact.
- Prix possibles pour l'instant : **métronome** et **royalties**. Chance d'un prix : 70 %, puis 50/50 entre les deux (provisoire).
- Les salles sans ennemis (départ, trésor) ne donnent pas de prix.

### Métronome = bombe ✅
- On en a **1 au départ**. Touche **E** (même position en AZERTY et QWERTY) pour en poser un à ses pieds.
- Il fait **tic-tac en accélérant** (le balancier oscille de plus en plus vite) pendant ≈ 1,5 s, puis **explose**.
- Explosion : rayon ≈ 100 px, **10 dégâts** aux ennemis (le bébé meurt, le boss encaisse), **2 PV** au joueur s'il est dedans (provisoire). Les ennemis sont repoussés.

### Royalties ✅
- L'argent du jeu, symbole **couronne**. Pas encore d'usage : la boutique (Luthier) viendra plus tard.

### Interface ✅
- Sous la barre de vie : icône de métronome et icône de couronne, avec leur nombre.

### Salle du trésor ✅
- Un **cul-de-sac 1×1** (jamais le départ ni le boss), sans ennemis. Marque dorée sur la mini-carte.
- **Différence avec TBOI : 3 objets au choix**, chacun posé sur un **lutrin** (meuble des livres de chœur, placeholder du socle). On en ramasse un en marchant dessus ; **les deux autres disparaissent**.
- Les lutrins sont des obstacles : on ne les traverse pas, les notes s'y arrêtent.
- En ramassant un objet, son nom et son effet s'affichent quelques secondes.
- Un objet déjà pris dans la partie ne réapparaît pas.

### Objets (provisoires, à discuter) 💡
Cinq objets pour tester la salle du trésor. Les noms viennent des anecdotes de la section 6.

| Objet | Effet provisoire |
|---|---|
| Bâton de Lully | Dégâts +1, mais une note sur 20 frappe le pied du joueur (1 PV). |
| Crâne de Haydn | PV maximum +2, et soigne 2 PV. |
| Vexations (Satie) | Cadence de tir +1,5. |
| Main guidonienne | Vitesse des tirs +120, portée +80. |
| Sabots de Strasbourg | Vitesse de déplacement +40 (l'épidémie de danse de 1518). |

### Boss du premier étage : le Gros Bébé ✅
- Un **gros bébé médiéval tout moche**, version géante du bébé. **40 PV** (provisoire). Contact : 1 PV.
- Il **se déplace au hasard** : il choisit un point, y marche lentement, s'arrête, recommence.
- **Saut** toutes les 4 à 7 s : il s'accroupit (≈ 0,4 s), saute hors de l'écran, puis **retombe à un endroit au hasard** (≈ 1,2 s en l'air). Pendant le saut, il est intouchable, et **son ombre grandit à l'endroit où il va tomber**.
- À l'atterrissage : dégâts s'il tombe sur le joueur, l'écran tremble, et **8 briques tombent du plafond** à des endroits au hasard. **Chaque brique est annoncée par une ombre** qui grandit pendant ≈ 1 s. Une brique fait 1 PV.
- **Barre de vie du boss** en bas de l'écran.

### Trappe vers l'étage suivant ✅
- À la mort du boss, une **trappe** apparaît au centre de la salle. Marcher dessus mène à l'**étage suivant** (nouvelle graine).
- On garde tout : PV, attributs, objets, métronomes, royalties.
- Pour l'instant, tous les étages sont du Haut Moyen Âge (même boss, même bébé).

### Mort ✅
- À 0 PV, **la partie recommence de zéro** : étage 1, attributs de départ, aucun objet, 1 métronome, 0 royalties.

### Debug
- **F2** : lignes « Métronomes » et « Royalties » ; le bouton « Nouvel étage » devient « Étage suivant » (comme la trappe). **F1** : numéro d'étage et objets pris.

### Précisions de la mise en œuvre ✅
- Un prix encore en vol quand on quitte la salle est donné au joueur (pas perdu).
- Le Bâton de Lully : la note qui frappe le pied ne part pas.
- La salle du boss ne contient que le boss (plus de bébés autour).
- La trappe ne marche qu'après en être sorti une fois (on peut apparaître dessus en se téléportant).
- Téléporté au centre de la salle du trésor : on arrive sous les lutrins, sans prendre d'objet.
- Les noms et effets des objets existent en français et en anglais ; la langue suit celle du système.

### Hors périmètre (ne pas faire)
Écran de mort, boutique, clés et portes à serrure, autres prix (cœurs, clés, partitions…), apparence d'Amadeus modifiée par les objets, boss différents selon l'étage, sons, graphismes finaux.

## 14. PHASE 5 — Armes et premiers neumes (en cours)

**But** : le système de tir paramétrable, les armes du Haut Moyen Âge et quelques neumes pour vérifier que tout se combine.

### Tir paramétrable ✅
- Un tir = nombre de notes, dispersion, taille, forme (note, onde en arc, onde circulaire, rayon), trajectoire (pause en l'air), effets au contact (étourdir, flaque d'encre), passe ou non au-dessus des obstacles.
- Armes et objets ne font que régler ces paramètres, pour que tout se combine.

### Armes ✅
- Les 6 armes de la section 4 + la voix, avec le calcul (départ + objets) × arme.
- **Socle d'arme** (placeholder : un coffre en bois) : toucher le socle échange son arme avec celle d'Amadeus ; l'ancienne reste dessus. Il faut s'éloigner du socle avant de pouvoir échanger à nouveau.
- Icône de l'arme actuelle dans l'interface, à côté des compteurs.

### Où trouver armes et objets ✅
- **Salle du trésor** : 3 lutrins (1 objet passif au choix) **+ 1 socle d'arme**.
- **Après le boss** : la trappe, **3 lutrins** (1 objet au choix) **+ 1 socle d'arme**.
- Les objets proposés au trésor et après le boss sont tous différents ; les armes proposées aussi, et jamais celle qu'on tient.

### États des ennemis ✅
- **Étourdi** : l'ennemi ne bouge plus (des étoiles tournent au-dessus de sa tête). **Ralenti** : vitesse réduite.

### Premiers objets passifs ✅
- **c** (*celeriter*) : cadence de tir +1.
- **a** (*altius*) : les notes passent au-dessus des obstacles (lutrins, socles ; pas les murs).
- **x** (*expectare*) : les notes s'arrêtent un instant en l'air, puis repartent.
- **Liquescence** : les notes laissent une flaque d'encre qui ralentit les ennemis.
- **Trope** : une note de plus à chaque tir, en éventail.
- Ils rejoignent les 5 objets provisoires de la Phase 4.

### Debug
- **F2** : ligne « Arme » (choix de l'arme) et bouton « Donner un objet ».

### Hors périmètre (ne pas faire)
Les autres neumes et objets de polyphonie, les armes des autres époques, l'apparence d'Amadeus selon l'arme, les sons, les graphismes finaux.
