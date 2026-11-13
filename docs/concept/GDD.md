# AMADEUS' MINOR FUGUE — Game Design Document (brouillon)

> Document de travail destiné à Claude Code. Il décrit l'univers et les choix de design.
> **Statut : en cours de conception.** Les sections marquées ✅ sont décidées, 💡 sont des idées à valider, ❓ sont des questions ouvertes.
> **Source de vérité du design : ce fichier, dans le dépôt** (`docs/concept/GDD.md`). La conception se fait désormais uniquement avec Claude Code, qui le met à jour au fil des décisions prises avec Johan.
> **Phase en cours : aucune (Phase 17 terminée).** Les Phases 1 à 17 (sections 10 à 26) sont terminées. Le reste sert de contexte, ne rien coder au-delà sans demande explicite.

---

## 1. Concept en une phrase

Un roguelike en 2D vue de dessus, dans la lignée de *The Binding of Isaac* : on va de salle en salle, on ramasse des objets qui rendent plus puissant et forcent des choix. Le thème est **l'histoire de la musique, du IXe siècle à nos jours**, avec un ton d'humour noir, absurde et érudit. L'objectif est que les gens réagissent par « mais c'est quoi ce jeu de fou ? ».

- **Gameplay** : volontairement ultra simple, façon TBOI. Ce n'est **pas** un jeu de rythme : on n'est pas obligé de tirer en cadence.
- **Différenciation** (vs Soundfall, Kill The Music, Crypt of the NecroDancer…) : l'axe **historique + anachronique + humour noir**. Aucun concurrent identifié ne le fait.
- **Langues** : le jeu doit fonctionner en français **et en anglais**. Les noms d'objets doivent avoir une version dans chaque langue.

## 2. Titre

✅ **Amadeus' Minor Fugue** (écrit avec l'apostrophe typographique à l'écran : *Amadeus’ Minor Fugue*). Ancien titre de travail : *Fugue in Amadeus Minor*.

- **« Fugue »** a un double sens : une forme musicale ET la fuite de l'enfant.
- **« Minor Fugue »** se lit « fugue en mineur » (la tonalité) et « fugue de mineur » (la fuite d'un enfant) ; **« Amadeus' »** : c'est la fugue d'Amadeus, comme on dit « la petite fugue de Bach ».
- Le titre parodie les titres d'œuvres classiques (« Little Fugue in G minor »).
- ❓ **Titre français** : pour l'instant le titre anglais est affiché dans les deux langues. (Une traduction comme *La Fugue mineure d'Amadeus* garderait le double sens.)
- ❓ Pas encore vérifié qu'aucun jeu ne porte déjà ce nom (la recherche avait été faite pour l'ancien titre).

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
- ✅ **Tête et corps en sprites séparés**, comme dans TBOI : la tête regarde dans la direction du tir, le corps dans la direction du déplacement (à l'arrêt, le corps suit la tête).
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

Concept visuel ✅ (planche de Johan, `assets/sprites/baby/baby.png`, cases de 25×56 px) :
- Enfant Jésus en « petit adulte » : corps de bébé potelé, mais **visage d'adulte**. Torse nu, **en couche** (c'est plus drôle qu'un lange long).
- Visage de vieillard : calvitie, rides, regard en coin méfiant, bouche pincée.
- Auréole légère derrière la tête. Pas de geste de bénédiction.
- Il regarde dans le sens où il marche ; il marche en trois images (dessinées de profil vers la droite, miroir pour la gauche) et devient tout blanc un instant quand il est touché.
- Carnation ocre rosée aux ombres brunes, qui ressort sur le sol parchemin.
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

| TBOI | Amadeus' Minor Fugue | Statut | Notes |
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
| Boutique | **Luthier** | ✅ | Le magasin (phase 13). |

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

✅ **Chaque ennemi a le style de l'époque où il apparaît pour la première fois** (un moine du premier étage chante des neumes), **mais il peut revenir dans les étages suivants**, avec son style d'origine : un moine grégorien au milieu des automates classiques, c'est drôle.

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
  - sol : ~~grandes dalles de pierre peu contrastées~~ → ✅ **du parchemin** (voir « Sol en parchemin » plus bas) ;
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

### ✅ Sol en parchemin (remplace les dalles de pierre)
- ✅ **Le sol n'est plus de la pierre mais le parchemin de la page** : c'est la page du livre de chœur qu'on foule, ce qui va avec « le niveau est une partition ».
- 💡 Rendu proposé : peau crème un peu jaunie, grain fin et points des follicules, teinte irrégulière (plus sombre vers les bords de la page et dans le creux de la reliure) ; la **réglure** à la pointe sèche (lignes très fines qui guident l'écriture) et les **piqûres** dans la marge, comme sur les vrais manuscrits.
- ✅ **Les murs** : on garde une **architecture**, mais **peinte sur la page** à la manière des *tables des canons* carolingiennes (colonnes et arcs dessinés à l'encre, aplats rouge, bleu et or), plutôt que des pierres réalistes. Les portes deviennent les arcs.

### ✅ Rendre les salles plus vivantes
- ✅ **Des trous dans le parchemin**, qui ressemblent à des trous dans du papier (bord déchiré, fibres, ombre ; on voit le noir ou la page du dessous). Fait réel : les parchemins ont souvent des trous, défauts de la peau, et les copistes écrivaient autour ; certains étaient recousus, voire brodés de soie.
  - ✅ Comme les trous de TBOI : on ne peut pas marcher dessus, les notes passent au-dessus. Le texte du sol contourne les trous.
  - ✅ Variantes : **trou recousu** (on peut passer sur la couture), **brûlure de chandelle** (bord noirci), **galerie du ver de livre** (le ver perce un trou **à chaque fois** qu'il sort).
- ✅ **Obstacles** (équivalents des rochers, pots, feux, TNT et pointes de TBOI), tous tirés du scriptorium :
  - ~~sceaux de cire rouge comme « rochers »~~ : ✅ abandonné, un sceau c'est plat ; les sceaux deviennent des **éléments du décor** posés sur la page (voir « petites choses sur la page ») ;
  - **encriers** : cassables par les tirs, ils répandent une flaque d'encre (qui ralentit) ; parfois des royalties dedans ;
  - **chandelles** : les « feux », elles brûlent au contact ; un tir les éteint ; la cire fondue fait une flaque glissante ;
  - ~~lettrines qui se dressent~~ : ✅ abandonné (pas aimé en jeu) ;
  - **aiguilles du relieur** qui sortent du sol par intermittence : les « pointes » ;
  - **piles de livres**, **pierre ponce** et **grattoir** du copiste, **signet** (ruban qui traverse la salle comme un muret bas) ;
  - les **fils de la reliure** dans la colonne centrale (déjà en idée).
- **Autres idées** :
  - 💡 **plans de salles faits à la main**, comme TBOI (des dizaines de dispositions d'obstacles et d'ennemis), plutôt que des obstacles posés au hasard : **plus tard** ;
  - ✅ **petites choses sur la page**, sans effet : fleur séchée pressée entre les pages, mouche écrasée, cheveu, tache de vin, empreinte de doigt, neumes raturés, **sceaux de cire** écrasés à plat sur la page avec leurs queues de parchemin, *marginalia* dessinées dans les marges (lapin, escargot chevalier) ;
  - 💡 **salles secrètes** derrière un mur qu'on fait sauter au métronome : **plus tard** ;
  - ✅ **salles sombres** éclairées par les chandelles.

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
  - ✅ En SDL : police chargée depuis `assets/fonts/` et rendue avec **stb_truetype** (glyphes rastérisés une fois au chargement), dans la texture de chaque salle.
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
  - ✅ Les notes **n'héritent pas** de la vitesse d'Amadeus : elles partent toujours droit vers où il vise, même en courant (`tuning.shot_inherit` = 0 ; le réglage reste si on veut y revenir).
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

## 14. PHASE 5 — Armes et premiers neumes ✅ (terminée)

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

## 15. PHASE 6 — Vrais graphismes : décor, Amadeus, bébé ✅ (terminée)

**But** : remplacer les placeholders de ce qu'on voit tout le temps par les vrais graphismes en pixel art, d'après la section 9 et les prototypes de `docs/concept/outils_concept/`.

### Périmètre ✅
- **Décor** (portage de `room_base.py` et `page.py`) : murs de briques en perspective, portes en bois sous un arc de claveaux (ouvertes et fermées), dalles du sol, **sol en manuscrit à neumes** (texte latin continu sur la double page, rubriques et lettrines rouges, encre usée), **reliure** dans la colonne centrale. Grandes salles et salles en L comprises.
- **Amadeus** (d'après `amadeus_hd.py`) : les 4 directions, la marche, la bouche ouverte quand il chante, le clignotement d'invincibilité.
- **Le bébé médiéval** : sprite final (section 4), dandinement, éclair blanc quand il est touché.

### Méthode de fabrication ✅
- Claude ne génère pas d'images directement : il les dessine en écrivant du code ou des données pixel par pixel. Pas de script Python.
- **Décor calculé en C** par le jeu au début de chaque étage (procédural, comme les prototypes) : chaque salle est dessinée une fois dans une texture, puis affichée. Aucun fichier image.
- **Police du sol** : Junicode (fichier TTF chargé via `SDL_IOStream`), rendue avec `stb_truetype` (un seul fichier C, domaine public).
- **Sprites écrits comme grilles de caractères** dans le code (une lettre = une couleur de la palette) : lisibles et retouchables dans un éditeur de texte. Le chargement de PNG (Aseprite) pourra venir plus tard.
- ✅ **Planches de sprites faites par Johan** : générées par une IA puis nettoyées dans LibreSprite. Elles remplacent les grilles de caractères, qui restent en secours (le jeu les utilise si le PNG manque). Amadeus et le bébé sont chargés depuis leurs planches (tête et corps centrés chacun sur les pieds) ; bébé : marche à 0,15 s par image, en aller-retour 1-2-3-2 (`tuning.baby_walk_frame`, `tuning.baby_walk_pingpong`).
- ✅ **Format des planches** (dans `assets/sprites/`) :
  - PNG en RGBA 32 bits, fond vraiment transparent, alpha tout ou rien sur le contour du personnage (pas de bords semi-transparents) ; seul un effet voulu, comme une auréole, peut rester translucide ;
  - **1 pixel de l'image = 1 pixel du jeu**, à la taille exacte (pas d'image agrandie en « faux pixels ») ;
  - grille de cases de taille fixe, sans marge ni espace, lues de gauche à droite et de haut en bas ;
  - contour sombre de 1 px ; pas de limite stricte de couleurs : on garde le modelé de l'image (une réduction à ~32 couleurs ne se voit presque pas et nettoie les pixels parasites, mais elle est facultative) ;
  - dans chaque case, le personnage est centré horizontalement et ses pieds touchent le bas de la case, à la même place d'une image à l'autre ;
  - les profils regardent à droite (le jeu fait le miroir pour la gauche).
- ✅ **Planches** (chemins relatifs à `assets/sprites/`, déclarés dans la table `SHEETS` de `sprite.c`) :

  | Fichier | Case | Lignes | Colonnes |
  |---|---|---|---|
  | `amadeus/amadeus_head.png` | 56×48 | face, dos, profil droit | repos, chante (de dos : la tête se soulève) |
  | `amadeus/amadeus_body.png` | 40×32 | face, dos, profil droit | repos, pas 1, pas 2 |
  | `baby/baby.png` | 25×56 | une seule | 3 images de marche (ordre de lecture à juger en jeu : 1-2-3-2 proposé) |

  - La tête est posée sur le corps : le bas de la case de la tête recouvre les 8 premières lignes de la case du corps (le menton passe devant le col). Amadeus fait donc au plus 72 px de haut.
  - L'éclair blanc des dégâts et le clignotement sont calculés par le jeu : pas d'image à faire.

### Précisions de la mise en œuvre ✅
- **Double page** : chaque page commence par une grande lettrine rouge. Le chant en cours à la fin de la page de gauche s'arrête là ; la page de droite commence par le chant suivant.
- **Reliure** : ombre du creux sur les dalles et un point de couture (fil clair, son ombre, deux trous) tous les 120 px.
- **Murs des grandes salles** : un mur qui continue dans la cellule voisine se raccorde brique à brique ; les coins rentrants des salles en L ont leur arête en onglet ; le creux d'une salle en L est noir.
- **Portes** : la porte dessinée fait à peu près la largeur du passage réel (≈ 50 px au pied du mur en haut et en bas, ≈ 40 px sur les côtés). Ouverte, le passage est noir ; fermée, le bois (planches, ferrures, heurtoir) est posé par-dessus.
- **Dalles** alignées sur la page entière (pas sur chaque salle), pour que les grandes salles n'aient pas de raccord.
- **Temps de calcul** : une salle se calcule en 25 à 80 ms. Les salles voisines sont préparées à l'avance, 3 ms par image, pour ne jamais faire sauter d'image.
- **Amadeus** : deux temps de marche (un chausson levé, les bras qui se balancent, la tête qui descend d'un pixel), tête de face, de dos et de profil, bouche ouverte de face et de profil. Le profil gauche est le miroir du droit (la croche de l'écouteur est donc inversée).
- **Fichiers** : la police Junicode est rangée dans `assets/fonts/` (avec sa licence), copiée à côté de l'exécutable.

### Hors périmètre (ne pas faire)
Le Gros Bébé, les objets, les armes, les prix, les meubles (lutrins, coffre, trappe), les tirs et l'interface (phase ultérieure) ; les animations au-delà de la marche et du chant ; les autres époques.

## 16. PHASE 7 — Ennemis du premier étage ✅ (terminée)

**But** : peupler le Haut Moyen Âge avec quatre nouveaux ennemis, dont trois qui tirent. Il faut donc des **tirs ennemis** et de l'**encre toxique** au sol.

### Tirs ennemis ✅
- Un pool à part de celui d'Amadeus (`bullet.c`). Un tir ennemi blesse Amadeus (1 PV, puis l'invincibilité habituelle) et disparaît au contact ou contre un mur. Les notes d'Amadeus ne les détruisent pas.
- Ils se distinguent des notes d'Amadeus (roses) : **notes carrées rouges** à hampe (neumes carrés). Celles du sonneur sont plus grosses et volent plus haut (elles partent de la cloche).

### Moine chanteur ✅
- Chante des phrases : une rafale de 4 notes visées sur Amadeus, puis une pause. Cadence et vitesse rapides, mais moins qu'Amadeus : 2,5 notes/s à 340 px/s (Amadeus : 3 notes/s à 480 px/s), pause de 1,4 à 2,2 s.
- Garde ses distances : s'approche au-delà de 260 px, recule en deçà de 170 px. Lent (70 px/s). 6 PV.
- Bouche ouverte pendant la rafale.

### Plume géante ✅
- Une plume d'oie plantée sur sa pointe, avec des yeux. Se déplace par petits bonds vers des points au hasard près d'Amadeus. 5 PV.
- Toutes les 2,5 à 3,5 s : s'arrête, se penche en arrière (0,4 s), puis **crache une goutte d'encre en cloche** vers l'endroit où se trouve Amadeus. L'ombre de la goutte au sol annonce l'impact (vol de 0,9 s).
- À l'impact : **flaque d'encre toxique** (verdâtre, avec des bulles), qui dure 4 s et retire 1 PV à Amadeus s'il marche dedans. Elle ne gêne pas les ennemis.

### Ver de livre ✅
- Sous le sol, il est invisible et intouchable (les portes restent fermées tant qu'il vit).
- Cycle : caché (1 à 2 s) → **le sol se soulève et se fissure** là où se trouve Amadeus, à 24 px près (0,9 s, pour avoir le temps de s'écarter) → il **jaillit** (blesse au contact) → reste dehors 1,4 s, où on peut le toucher → replonge (0,25 s) → caché.
- ✅ (Phase 12) **Il perce toujours un trou dans le parchemin en sortant** : plus on le laisse vivre, plus la salle se troue. Il faut l'éliminer vite. Voir §21.
- Larve pâle de ver de livre, avec de petites lunettes rondes. 4 PV. Les tirs ne le repoussent pas ; étourdi, il reste dehors.

### Sonneur de cloche ✅
- Marche lentement (55 px/s), garde au moins 150 px avec Amadeus. Sa **cloche lévite au-dessus de sa tête**. 10 PV.
- Toutes les 2,6 à 3,2 s : la cloche se balance (0,5 s, pour prévenir), puis sonne et lance un **éventail de 3 grosses notes très lentes** (110 px/s).

### Pour tous ✅
- Les tireurs regardent Amadeus. Étourdis, ils ne bougent plus et ne tirent plus ; ralentis par l'encre (Liquescence), ils marchent moins vite.
- **Composition des salles** : même nombre d'ennemis qu'avant, chaque ennemi tiré au hasard (graine de l'étage) avec des poids : bébé 4, moine 2, plume 2, ver 2, sonneur 1.
- Toutes les valeurs sont des réglages (`tuning`), à ajuster en jouant.

### Sprites temporaires ✅
- Claude dessine des planches PNG provisoires dans `assets/sprites/<perso>/<perso>.png` (même format que celles de Johan, profil vers la droite), que Johan remplacera. Elles sont produites par un outil jetable hors du dépôt.

  | Fichier | Case | Images |
  |---|---|---|
  | `monk/monk.png` | 36×60 | marche 1, marche 2, chante |
  | `quill/quill.png` | 32×64 | flotte 1, flotte 2, se penche (crache) |
  | `worm/worm.png` | 32×48 | sort (tête), à moitié, dehors 1, dehors 2 |
  | `worm/worm_hole.png` | 48×24 | sol soulevé, sol fissuré, trou |
  | `bellringer/bellringer.png` | 36×60 | marche 1, marche 2, bras levés (sonne) |
  | `bellringer/bell.png` | 28×28 | immobile, penchée à gauche, penchée à droite |

### Debug
- **F2** : le bouton « Ajouter » fait apparaître l'ennemi choisi sur la ligne « Ennemi ».

### Hors périmètre (ne pas faire)
Les autres ennemis du §8 (blemmyes, cynocéphales, sciapodes, chapiteaux…), les nouveaux boss, les sons, les tirs ennemis détruits par les notes.

## 17. PHASE 8 — Visée libre à la souris ✅ (terminée)

**But** : pouvoir viser dans n'importe quelle direction, pas seulement les 4 des flèches.

### Commandes ✅
- **Clic gauche maintenu** : Amadeus chante vers le curseur, dans n'importe quelle direction (cadence, charge de la flûte et toutes les armes comme avec les flèches).
- Les **flèches restent** (4 directions). Si une flèche est enfoncée, elle l'emporte sur la souris.
- La note part de la bouche d'Amadeus et passe sous le curseur (on vise depuis la hauteur de vol des notes, pas depuis ses pieds).
- La **tête** regarde du côté où il tire (face, dos ou profil : la direction la plus proche).
- **Flûte** : on charge en gardant le clic, le tir part au relâchement, vers le curseur à ce moment-là.
- Un clic sur la mini-carte (téléportation) ou sur le menu de debug ne tire pas.

### Curseur ✅
- Le curseur du système est remplacé par un **viseur** dessiné par le jeu. Il disparaît si la souris ne bouge pas pendant quelques secondes (on joue alors au clavier).

### Hors périmètre (ne pas faire)
- **Manette** (joystick droit pour viser) : plus tard, elle utilisera la même visée libre.
- Viser au clavier en diagonale.

## 18. PHASE 9 — Les ennemis explosent ✅ (terminée)

**But** : rendre les morts plus satisfaisantes, dans le ton d'humour noir du jeu.

### Mort d'un ennemi ✅
- L'ennemi **explose** : un éclat bref, puis des gouttes projetées tout autour, qui retombent sur le sol.
- Il laisse une **grande tache** là où il est mort, et chaque goutte fait une petite tache là où elle tombe.
- Couleur : **sang rouge** pour tous (bébés, moine, sonneur, Gros Bébé), **sang vert** pour le ver de livre, **encre noire** pour la plume.
- Le Gros Bébé explose plus fort (plus de gouttes, plus loin, plus grande tache).
- Les taches restent dans la salle tant qu'on y est ; elles disparaissent quand on la quitte (comme les flaques et les tirs).
- « Tuer les ennemis » (F2) les fait exploser aussi.
- Les taches ne font rien (ni dégâts, ni ralentissement) : c'est du décor.

### Hors périmètre (ne pas faire)
Les taches qui restent quand on revient dans une salle, les morceaux d'ennemis (os, auréoles, plumes), les sons.

## 19. PHASE 10 — Sons : le chant d'Amadeus ✅ (terminée)

**But** : les premiers sons, en commençant par le tir. Tout est synthétisé par le jeu (aucun fichier audio), comme le décor.

### Son du tir : Amadeus chante *Ut queant laxis* ✅
- À chaque tir, Amadeus **chante la syllabe suivante d'un chant grégorien**. Au début de la partie, c'est l'hymne ***Ut queant laxis*** (Paul Diacre, VIIIe s.), celui dont Guido d'Arezzo a tiré les noms des notes ut, ré, mi, fa, sol, la. Son texte est déjà écrit sur le sol. *(Remplacé en Phase 17 : Amadeus chante de la pop moderne, section 26 ; les chants grégoriens sont aux moines.)*
- Chaque syllabe a ses vraies notes : une seule note, ou un petit groupe (neume) chanté d'un trait. Il chante la **vraie voyelle** de la syllabe (« Ut » sur u, « la » sur a…).
- La mélodie continue d'un tir à l'autre.
- Voix d'enfant, un peu synthétique : ça va avec le pixel art. Chaque chant est transposé pour que le milieu de sa tessiture tombe vers fa5.
- Toutes les armes chantent pour l'instant ; les sons propres aux instruments viendront plus tard.

### Plusieurs chants ✅
- Pour que ce ne soit pas répétitif, Amadeus connaît **six chants** (Liber Usualis 1961, domaine public, relevés sur GregoBase) :
  - *Ut queant laxis* (hymne de saint Jean-Baptiste, p. 1504) ;
  - *Veni Creator Spiritus* (p. 885) ;
  - *Ave maris stella* (p. 1259) ;
  - *Pange lingua* (p. 957) ;
  - *Dies irae* (séquence des défunts, 2 strophes, p. 1810) ;
  - *Victimae paschali laudes* (séquence de Pâques, p. 780).
- **Il change de chant quand on arrête de tirer plus de 2 secondes** (`tuning.chant_pause`), et aussi quand le chant est fini. *(Remplacé en Phase 15 : il change à chaque arrêt du tir, section 24.)* Le nouveau chant est tiré au hasard parmi les autres, et commence à sa première syllabe.
- Les mélodies sont dans `src/chant_data.h`, généré depuis les fichiers GABC de GregoBase.

### Technique ✅
- Module `audio.c` : un flux SDL (`SDL_OpenAudioDeviceStream`), rempli à chaque image depuis la boucle principale (pas de thread à nous). Douze voix mixées (chant et bruitages) ; chaque syllabe est calculée au moment du tir dans le tampon de sa voix (quelques millisecondes).
- Synthèse vocale par formants : harmoniques de la note, dont le volume suit les résonances de la voyelle ; vibrato léger ; petit souffle sur les consonnes (s, t, x…).
- Sans carte son (ou si l'audio ne s'ouvre pas), le jeu tourne en silence.
- Volume réglable dans `tuning`.

### Hors périmètre (ne pas faire)
Les autres sons (dégâts, morts, métronome, ennemis, interface), les sons des instruments, la musique, le réglage du volume dans un menu.

## 20. PHASE 11 — Bruitages : ramassage, morts, métronome, ennemis ✅ (terminée)

**But** : les premiers bruitages, en plus du chant. Toujours synthétisés par le jeu, sans fichier audio.

### Sons ✅
- **Métronome ramassé** : un **tic-tac** de métronome mécanique (deux chocs de bois, le second plus grave).
- **Royalties ramassées** : **deux pièces qui tintent**, la seconde une quarte plus haut (« ka-ching »). Son de métal (modes de vibration d'un disque), pas une note de synthé.
- **Un ennemi meurt** : un « splotch » mouillé qui va avec l'explosion (§18) : choc grave, éclaboussure qui s'assourdit, petites bulles. Le même pour tous les ennemis ; celui du **boss** est plus grave et plus long.
- **Métronome posé** : un **tic-tac** à chaque battement du balancier (tic, tac, tic…), qui accélère avec lui (de 2 à 10 battements par seconde), puis l'**explosion** : craquement, grondement grave, et les morceaux de bois du métronome qui retombent.
- **Plume géante** : un **crachat d'encre** quand elle tire (« pfut », jet mouillé qui descend, petit « bloup »).
- **Moine chanteur** : à chaque note tirée, il **chante une note grave de chant grégorien**, toujours la même (ré3, voix d'homme sur « o ») avec une légère variation. *(Remplacé en Phase 16 : il chante de vrais chants, section 25.)*
- **Sonneur de cloche** : un **coup de cloche** d'église (bourdon, tierce mineure, quinte…) qui résonne plus d'une seconde, à chaque salve.
- **Ver de livre** : un bruit de **ver qui creuse** pendant que le sol se soulève avant sa sortie : grondement sourd et grattements de plus en plus serrés.
- **Bébés** : ils **pleurent de temps en temps, pas en continu**. Deux pleurs : un long « ouiiin » ou un sanglot « oua… ouaaah » avec une inspiration entre les deux. Voix aiguë, tendue et un peu éraillée, qui monte puis retombe. Chaque bébé pleure toutes les 4 à 10 s, au hasard (`baby_cry_min`, `baby_cry_max`). Jamais deux pleurs à moins de 3 s d'écart dans la salle, quel que soit le nombre de bébés (`baby_cry_gap`). Le **Gros Bébé** pleure aussi, plus grave et plus lentement (`boss_cry_pitch`).
- Chaque son varie un peu de hauteur à chaque fois (± 6 %), pour ne pas sonner mécanique.
- Le même son lancé plusieurs fois au même instant (trois ennemis tués par un métronome, deux moines qui tirent ensemble) n'est joué qu'une fois : pas de son trois fois plus fort.
- Un prix encore en vol quand on change de salle est donné sans bruit.

### Technique ✅
- Dans `audio.c` : chaque bruitage est calculé une fois à l'ouverture (résonances amorties, bruit filtré), puis rejoué dans une voix libre avec `audio_play(son, hauteur)`. Seize voix ; une voix lit le son à la hauteur voulue sans le recopier. Les battements du métronome (`Bomb.tick`) et le début du creusement du ver (`Enemy.dig`) sont signalés à `game.c` comme les tirs (`fire`).
- Réglages dans `tuning` : volume général (`sound_volume`), du chant (`chant_volume`), de tous les bruitages (`sfx_volume`) et **de chaque bruitage** (`sound_gain[son]`), plus `splat_boss_pitch`. Les sons sont calculés à plein volume ; le volume est appliqué à la lecture.

### Hors périmètre (ne pas faire)
Les autres sons (dégâts reçus, mort d'Amadeus, atterrissage du boss, briques, encre qui tombe au sol, ver qui replonge, portes, objets, interface), les sons des instruments, la musique, le réglage du volume dans un menu.

## 21. PHASE 12 — Parchemin, murs peints, trous et obstacles ✅ (terminée)

**But** : changer l'aspect du premier étage (§9 « Sol en parchemin ») et rendre les salles plus vivantes (§9 « Rendre les salles plus vivantes »).

### Décor ✅
- **Sol en parchemin** à la place des dalles : peau crème, grain, follicules, teinte irrégulière, réglure à la pointe sèche sous chaque ligne de texte et piqûres dans la marge extérieure.
- **Murs peints** façon tables des canons : colonnes marbrées avec chapiteau et base, arcs entre les colonnes, fonds bleu lapis, écoinçons rouges, bord doré cerné d'encre. Les portes sont des arcs à claveaux peints, rouges et blancs ; le bois des portes fermées reste.
- **Petites choses sur la page**, sans effet de jeu : fleur séchée, mouche écrasée, cheveu, tache de vin, empreinte de doigt, neumes raturés ; *marginalia* (lapin, escargot chevalier) dans les marges.
- **Sceaux de cire** à plat sur la page (rouges, verts ou bruns), avec leurs deux queues de parchemin, un bourrelet en relief et l'empreinte de la matrice (grènetis et croix) : du décor, pas un obstacle.

### Grille des salles ✅
- Chaque cellule est une grille de **16 × 9 cases de 48 × 40 px**. Trous et obstacles occupent des cases, tirés au hasard avec la graine de l'étage (même graine = mêmes salles), souvent en **symétrie** pour que la salle ait l'air composée.
- Seulement dans les **salles normales** : rien dans la salle de départ, la salle du trésor et la salle du boss.
- **Règle d'accès** : on peut aller de **chaque porte à chaque porte** à pied. Plus fort encore, toutes les cases où l'on peut marcher sont reliées entre elles : aucun ennemi ne peut apparaître enfermé. Pour cette vérification, tout obstacle compte comme infranchissable, même s'il se casse. Une disposition qui casserait la règle est refusée.
- Les cases devant chaque porte et le centre de chaque cellule (prix, téléportation) restent libres.
- Les ennemis contournent les obstacles et les trous : ils suivent le chemin le plus court vers le joueur quand la ligne droite est bouchée.

### Trous ✅
- Trou déchiré (bord fibreux, ombre, noir dedans), **brûlure** (bord noirci), **trou recousu** (on peut marcher dessus ; depuis la phase 13, on ne voit plus le trou, seulement la couture).
- On ne marche pas sur un trou ; les notes et les tirs ennemis passent au-dessus.
- Le texte du sol contourne les trous.
- **Ver de livre** : il perce **toujours** un trou rond en sortant, **sans limite** : le laisser vivre troue la salle, il faut l'éliminer vite. Il choisit donc sa sortie sur la case la plus proche d'Amadeus où un trou **ne bouche aucun chemin** (même règle d'accès, en comptant aussi les trous que vont percer les autres vers déjà annoncés). Pas deux vers sur la même case. S'il ne reste aucune case possible, il ressort par un trou de ver déjà percé. Ceux qui se tenaient sur la case sont poussés au bord. Ces trous restent quand on revient.

### Obstacles ✅
| Obstacle | Marcher | Tirs | Se casse |
|---|---|---|---|
| Pile de livres | bloque | bloque | métronome |
| Encrier | bloque | bloque | tirs ou métronome : flaque d'encre (ralentit les ennemis), parfois des royalties |
| Chandelle | bloque, brûle au contact | un tir l'éteint | métronome ; éteinte, elle laisse une flaque de cire glissante |
| Aiguilles du relieur | on peut marcher dessus ; blessent quand elles sortent (par intermittence, annoncées) | passent | non |
| Signet (ruban) | bloque (muret bas) | passent au-dessus | non |
| Fils de la reliure | bloquent (muret bas, dans la colonne de la reliure) | passent au-dessus | non |

- Les objets qui font passer les notes au-dessus des obstacles (*Altius*) les font aussi passer au-dessus de ceux-ci.
- Sprites provisoires des obstacles faits par Claude (`assets/sprites/obstacles/`), à remplacer.

### Salles sombres ✅
- Quelques salles normales sont **plongées dans le noir** : on y voit autour d'Amadeus, des chandelles allumées (la lumière vacille) et des notes en vol. Éteindre une chandelle assombrit la salle.

### Hors périmètre (ne pas faire)
Plans de salles faits à la main, salles secrètes, sons des obstacles, autres époques.

---

## 22. PHASE 13 — Clefs, magasin, boîtes à musique ✅ terminée

**But** : donner un usage aux royalties et des choses à ramasser : clefs, magasin (le **Luthier**), boîtes à musique, et une récompense à chaque salle vidée.

### Trou recousu ✅
- Quand on recoud un trou, **on ne voit plus le trou** : seulement une **couture** à l'endroit où il était (la fente refermée, les points de fil en travers, les trous de l'aiguille). Le texte de la page passe par-dessus.

### Salle vidée ✅
- **Toujours une récompense** quand on vide une salle (avant : 70 %). Tirage provisoire : royalties 30, métronome 22, clef de sol 10, clef de fa 8, clef d'ut 8, boîte à musique grise 15, boîte dorée 7.
- Une salle normale sans ennemis donne aussi sa récompense à la première entrée.
- Tous les obstacles qu'on peut casser en tirant (encriers, chandelles) **se cassent tout seuls** quand la salle est vidée, avec leurs effets habituels (flaque, royalties de l'encrier, cire de la chandelle).

### Clefs ✅
- Trois clefs : **clef de sol**, **clef de fa**, **clef d'ut**. Une clef dorée dont l'anneau a la forme de la clef musicale.
- Comptées dans le HUD sous les royalties et les métronomes. Lignes dans le menu F2.
- **Clef de sol** : ouvre la porte du magasin. **Clef de fa** : ouvre les boîtes à musique dorées. **Clef d'ut** : ✅ on la ramasse, mais **elle ne sert à rien pour l'instant** (plus tard : salles secrètes, Tartini, Hildegard…).
- On en trouve en récompense de salle et dans les boîtes à musique.

### Portes à serrure ✅
- Une porte fermée à clef reste fermée même quand la salle est vidée ; le bois porte le symbole de la clef qu'il faut.
- On l'ouvre en la touchant avec la bonne clef (une clef utilisée). Ouverte pour de bon.
- On ne peut pas se téléporter dans une salle fermée à clef.

### Magasin (le Luthier) ✅
- Un **cul-de-sac 1 × 1** (ni le départ, ni le boss, ni le trésor), dont la porte est **fermée par une clef de sol**. Marque sur la mini-carte (une couronne).
- Sur des lutrins : **3 objets** (tirés comme ceux du trésor, jamais les mêmes) et **10 PV**. Chacun coûte **10 royalties**, prix affiché sous le lutrin.
- On achète en touchant le lutrin si on a assez de royalties ; l'objet acheté disparaît, les autres restent. Les 10 PV ne s'achètent pas si la vie est pleine.

### Boîtes à musique (coffres) ✅
- **Grise** : s'ouvre en la touchant. **Dorée** : il faut une **clef de fa**.
- En s'ouvrant, elle joue une **courte mélodie** de boîte à musique (lames de métal pincées) : la grise le début d'*Ut queant laxis*, la dorée la gamme *ut ré mi fa sol la* qu'en a tirée Guido d'Arezzo.
- Son contenu **se répand autour** puis est aspiré comme les prix : royalties, métronomes, clefs. Grise : 2 à 4 choses ; dorée : 3 à 6, et **très rarement** (8 %) un **objet** à la place, posé dans la boîte ouverte : on le prend en **revenant** toucher la boîte (pas dans l'élan qui l'a ouverte, pour qu'on puisse le laisser). Les choses répandues atterrissent sur du sol libre (jamais dans un mur, un trou ou un obstacle).
- On les trouve en récompense de salle. Une boîte pas encore ouverte (une dorée sans clef) reste là quand on revient.
- On marche à travers (ce ne sont pas des obstacles).
- F2 : bouton pour faire apparaître une boîte.

### Téléportation ✅
- On peut aussi se téléporter dans une salle **pas encore visitée** mais **reliée à une salle visitée** (les salles sombres de la mini-carte). On y entre **comme si on y avait marché par le plus court chemin** : par la porte qui vient de la salle visitée, pas au milieu de la salle. Toujours interdit en combat.

### Sprites et sons provisoires
- Clefs et boîtes dessinées par Claude (`assets/sprites/pickups/`), à remplacer.
- Sons synthétisés : mélodies des boîtes, clef ramassée, serrure qui s'ouvre.

### Hors périmètre (ne pas faire)
Usage de la clef d'ut, vendeur dessiné dans le magasin, autres choses en vente (clefs, métronomes), salles secrètes, Tartini, Hildegard.

## 23. PHASE 14 — Menu principal et titre d'époque ✅ terminée

**But** : le jeu s'ouvre sur un vrai menu, et l'arrivée au premier étage annonce l'époque.

### Nouveau titre ✅
- Le jeu s'appelle **Amadeus' Minor Fugue** (section 2) : titre de la fenêtre, menu principal, documents.

### Menu principal ✅
- Au lancement, le jeu s'ouvre sur le **menu principal** : le titre *Amadeus’ Minor Fugue* en grand, et deux boutons, **Jouer** et **Quitter**.
- Derrière : la salle de départ, assombrie (sans HUD ni mini-carte).
- Souris (survol, clic) et clavier (haut / bas ou Z / S, Entrée ou Espace ; Échap quitte).
- **Jouer** lance une nouvelle partie. **Quitter** ferme le jeu ; dans le navigateur on ne peut pas fermer l'onglet, donc pas de bouton Quitter.
- Les textes passent par la police Junicode du décor (enfin avec les accents).

### Titre d'époque ✅
- En arrivant au **premier étage** (à chaque nouvelle partie, y compris après une mort), un titre s'affiche : **« 1012 après J.-C. »** (EN : *AD 1012*).
- Il apparaît en fondu, reste environ 3 s, puis disparaît, sans arrêter le jeu (on peut déjà bouger, comme le nom de l'étage dans TBOI).
- Les étages suivants n'ont pas encore de titre (leurs époques ne sont pas décidées en détail, section 8).

### Hors périmètre (ne pas faire)
Menu pause, options (volume, langue, touches), écran de mort, cinématique d'intro, titres des autres étages, Échap pendant la partie (il quitte toujours le jeu sur ordinateur).

## 24. PHASE 15 — Notes modernes, cri, descriptions, fenêtre, neumes ✅ terminée

**But** : des tirs qui se lisent comme une partition, un retour sonore aux dégâts, des objets compréhensibles avant de les prendre, et un jeu qui remplit la fenêtre.

### Cri d'Amadeus ✅
- Quand Amadeus prend un dégât (ennemi, tir, piège, coup de pied de Lully…), il pousse un **cri** bref d'enfant (« aïe ! »), synthétisé comme sa voix chantée.

### Descriptions des objets ✅
- Comme le mod *External Item Descriptions* de TBOI : quand Amadeus s'approche d'un objet posé (lutrin du trésor, du boss, du magasin, socle d'arme, objet dans une boîte dorée ouverte), un **cadre** affiche son nom et ce qu'il fait **exactement**, avec les chiffres (« +1 dégât », « 1 tir sur 20 part dans ton pied : −1 PV »…).
- Armes : la description se calcule depuis leurs réglages (dégâts ×3, cadence ×0,4, étourdit 1 s…), pour rester juste quand on les règle.
- Au magasin : le prix, en rouge si on n'a pas assez de royalties.
- Le bandeau qui s'affiche quand on prend l'objet reste (nom et petite phrase drôle).

### Tirs d'Amadeus : notes modernes selon la cadence ✅
- Les notes d'Amadeus deviennent des **notes de musique modernes**, dont la **valeur dépend de la cadence de tir** : plus on tire vite, plus la note est courte.
- De la plus lente à la plus rapide : **ronde, blanche, noire, croche, double croche, triple croche, quadruple croche**. Au départ (3 tirs/s), Amadeus tire des noires.
- Réglage : durée de la noire (`tuning.note_quarter_time`) ; la note choisie est celle dont la durée est la plus proche du temps entre deux tirs.
- **Le son suit** : chaque syllabe chantée dure le temps entre deux tirs, moins un **petit silence** (au moins 30 ms), pour que les sons ne se chevauchent jamais. Cadence extrêmement rapide : quadruple croche, son très court ; cadence extrêmement lente : ronde, son très long.
- Les autres formes de tir (onde du tabourin, cloche, orgue) ne changent pas.

### Chant : changement de mélodie ✅
- Amadeus change de chant **à chaque fois qu'on arrête de tirer** (aucun tir pendant un peu plus que le temps entre deux tirs), et quand le chant est fini. Remplace la pause de 2 s de la Phase 10.

### Fenêtre ✅
- Le jeu s'agrandit pour **remplir la fenêtre**, à n'importe quelle taille, plus seulement en plein écran par paliers entiers. Il garde ses proportions (16:9) : il ne reste des bandes noires que si la fenêtre n'a pas ces proportions.
- Agrandissement net : le jeu est grossi d'un facteur entier sans lissage, puis ramené à la taille de la fenêtre avec un léger lissage (pas de pixels de tailles inégales).

### Tirs des moines et des sonneurs : neumes ✅
- Les notes tirées par les moines et les sonneurs deviennent des **neumes** de Saint-Gall (virga, pes, clivis, torculus, porrectus…), comme ceux écrits sur le sol, à l'encre rouge cernée de noir pour rester bien visibles.
- C'est la règle de la section 8 : chaque ennemi a le style de son époque.

### Hors périmètre (ne pas faire)
Sons des autres dégâts (ennemis touchés), descriptions des prix et des clefs, autres étages et leurs ennemis, choix de la taille de fenêtre dans un menu.

## 25. PHASE 16 — Les moines chantent de vrais chants ✅ terminée

**But** : les moines ne tirent plus des neumes au hasard : ils **chantent pour de vrai** un chant grégorien, syllabe par syllabe, et chaque tir est le neume de la syllabe chantée.

### Le chant des moines ✅
- Chaque moine chante un **vrai chant grégorien**, tiré au hasard parmi ceux du jeu (*Ut queant laxis*, *Veni Creator*, *Ave maris stella*, *Pange lingua*, *Dies irae*, *Victimae paschali laudes*). Chaque note de sa rafale est la **syllabe suivante**. Le chant continue d'une rafale à l'autre ; fini, il en commence un autre.
- Il chante la syllabe **pour de vrai** : sa voyelle, sa mélodie (toutes ses notes), d'une voix d'homme deux octaves sous Amadeus. Plusieurs moines dans une salle : une polyphonie involontaire.
- **Le tir est le neume de la syllabe** : un neume de Saint-Gall qui suit la mélodie. Une note : *virga* si elle monte, *punctum* si elle descend, *tractulus* si elle répète. Deux notes : *pes* (monte), *clivis* (descend), *distropha* (répète). Trois : *scandicus*, *climacus*, *torculus* (monte puis descend), *porrectus* (descend puis remonte). Plus de trois notes : plusieurs neumes à la suite (jusqu'à trois).
- **La syllabe latine est écrite sous le neume**, à l'encre, comme dans un manuscrit (« Di », « es », « í », « rae »…).
- Volume de la voix des moines : `tuning.monk_voice_volume` (remplace le son de la note grave fixe).

### Hors périmètre (ne pas faire)
Sonneurs (leurs tirs restent des neumes au hasard avec la cloche), nouveaux chants, chœur de moines synchronisés.

## 26. PHASE 17 — Amadeus chante de la pop ✅ terminée

**But** : le contraste. Les moines chantent le Moyen Âge ; Amadeus, enfant d'aujourd'hui, chante **dans un style moderne**.

### Les chansons d'Amadeus ✅
- Amadeus ne chante plus de chants grégoriens : il chante des **chansons pop originales**, composées pour le jeu (pas de vraies chansons récentes : mélodies et paroles protégées par le droit d'auteur).
- Style tube d'aujourd'hui : gamme majeure, refrains qui se répètent, petites vocalises façon R&B sur les « oh », « yeah », « ooh ».
- **Paroles : clichés pop anglais**, drôles dans la bouche d'un enfant de 8 ans (« oh baby yeah », « tonight », « na na na », « my heart goes boom », « put your hands up »). On n'entend que les voyelles et un peu les consonnes.
- Six chansons : *Yeah Yeah Baby*, *Tonight*, *Na Na Summer*, *Heartbeat*, *Forever*, *Hands Up*. Les règles ne changent pas : une syllabe par tir, durée selon la cadence (section 24), nouvelle chanson à chaque arrêt du tir et en fin de chanson.
- Les mélodies sont écrites en degrés de la gamme dans `docs/concept/outils_concept/make_songs.py`, qui génère `src/song_data.h`.
- 💡 Plus tard : une chanson par époque traversée qui « contamine » Amadeus ? Accompagnement (boîte à rythmes) ?

### Hors périmètre (ne pas faire)
Accompagnement instrumental, autres langues de paroles, texte des paroles affiché.
