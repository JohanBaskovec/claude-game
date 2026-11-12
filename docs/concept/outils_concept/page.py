"""Un étage = une page de manuscrit de 10×10 salles. Chaque salle montre 1/10 de la largeur et 1/10 de la hauteur.

Coordonnées « page » : x de 0 à 10*CELL_W, y de 0 à 10*CELL_H. La salle (cx, cy) affiche la fenêtre
[cx*CELL_W, (cx+1)*CELL_W[ × [cy*CELL_H, (cy+1)*CELL_H[ sur son sol. Le texte passe « sous » les murs
et continue dans la salle voisine.
"""
import random, re
import numpy as np
from PIL import Image, ImageDraw, ImageFont

GRID = 10
CELL_W, CELL_H = 768, 360                  # zone de sol d'une salle 1×1 (en px du jeu, 960×540)
PAGE_W, PAGE_H = GRID * CELL_W, GRID * CELL_H
MARGIN_X = 150                             # marges de la page (visibles seulement dans les salles du bord)
LINE0, PITCH = 46, 60                      # 6 lignes par salle, toujours aux mêmes hauteurs
S = 2                                      # suréchantillonnage du rendu de l'encre
FONT = "../../../assets/fonts/Junicode-Medium.ttf"
FONT_BOLD = "../../../assets/fonts/Junicode-Bold.ttf"

# Textes liturgiques latins du haut Moyen Âge (domaine public)
CHANTS = [
    ("IN NATIVITATE DOMINI · INTROITUS",
     "Puer natus est nobis et filius datus est nobis cuius imperium super humerum eius et vocabitur nomen eius "
     "magni consilii angelus. Cantate domino canticum novum quia mirabilia fecit."),
    ("HYMNUS · IN NATIVITATE S. IOHANNIS BAPTISTAE",
     "Ut queant laxis resonare fibris mira gestorum famuli tuorum solve polluti labii reatum sancte Iohannes. "
     "Nuntius celso veniens Olympo te patri magnum fore nasciturum nomen et vitae seriem gerendae ordine promit."),
    ("HYMNUS · IN FESTO PENTECOSTES",
     "Veni creator spiritus mentes tuorum visita imple superna gratia quae tu creasti pectora. "
     "Qui diceris paraclitus altissimi donum dei fons vivus ignis caritas et spiritalis unctio."),
    ("KYRIE", "Kyrie eleison. Christe eleison. Kyrie eleison."),
    ("HYMNUS · IN FESTIS BEATAE MARIAE",
     "Ave maris stella dei mater alma atque semper virgo felix caeli porta. "
     "Sumens illud ave gabrielis ore funda nos in pace mutans evae nomen."),
    ("GLORIA",
     "Gloria in excelsis deo et in terra pax hominibus bonae voluntatis. Laudamus te. Benedicimus te. "
     "Adoramus te. Glorificamus te. Gratias agimus tibi propter magnam gloriam tuam."),
    ("HYMNUS · TE DEUM",
     "Te deum laudamus te dominum confitemur. Te aeternum patrem omnis terra veneratur. "
     "Tibi omnes angeli tibi caeli et universae potestates."),
    ("AGNUS DEI", "Agnus dei qui tollis peccata mundi miserere nobis. Agnus dei qui tollis peccata mundi dona nobis pacem."),
]

# ------------------------------------------------------------------ syllabation latine simplifiée
V = "aeiouyæœ"
def syllables(word):
    w = word.lower()
    units = []                                   # (texte, est_voyelle)
    i = 0
    while i < len(w):
        c = w[i]
        if c == "q" and w[i + 1:i + 2] == "u":
            units.append((word[i:i + 2], False)); i += 2; continue
        if c in V:
            if w[i:i + 2] in ("ae", "oe", "au"):
                units.append((word[i:i + 2], True)); i += 2; continue
            prev_v = i > 0 and w[i - 1] in V
            next_v = w[i + 1:i + 2] != "" and w[i + 1] in V
            if c == "i" and next_v and (i == 0 or prev_v):   # i consonne : iam, e-ius
                units.append((word[i], False)); i += 1; continue
            units.append((word[i], True)); i += 1; continue
        units.append((word[i], False)); i += 1
    nuclei = [k for k, (_, v) in enumerate(units) if v]
    if len(nuclei) <= 1:
        return [word]
    cuts = []
    for a, b in zip(nuclei, nuclei[1:]):
        cl = [units[k][0].lower() for k in range(a + 1, b)]
        if len(cl) == 0:
            cuts.append(b)
        elif len(cl) == 1:
            cuts.append(a + 1)
        else:
            pair = cl[-2][-1] + cl[-1][0]
            if (cl[-2] in list("bcdgptf") and cl[-1] in ("l", "r")) or pair in ("ch", "ph", "th", "gn"):
                cuts.append(b - 2)
            else:
                cuts.append(b - 1)
    out, start = [], 0
    for c in cuts:
        out.append("".join(u[0] for u in units[start:c])); start = c
    out.append("".join(u[0] for u in units[start:]))
    return [o for o in out if o]

# ------------------------------------------------------------------ neumes
NW = {"punctum": 5, "tractulus": 6, "virga": 7, "pes": 7, "clivis": 8, "torculus": 9,
      "porrectus": 9, "climacus": 11, "scandicus": 10, "quilisma": 8}
KINDS = ["punctum", "tractulus", "virga", "virga", "pes", "pes", "clivis", "clivis",
         "torculus", "porrectus", "climacus", "scandicus", "quilisma"]

def draw_neume(d, kind, x, y):
    def stroke(pts, w=3):
        d.line([(x + px * S, y + py * S) for px, py in pts], fill=255, width=w, joint="curve")
    def dot(dx, dy, r=1.6):
        d.ellipse([x + (dx - r) * S, y + (dy - r) * S, x + (dx + r) * S, y + (dy + r) * S], fill=255)
    if kind == "punctum": dot(2, 6)
    elif kind == "tractulus": stroke([(0, 6), (5, 5)])
    elif kind == "virga": stroke([(0, 11), (4, 0), (6, -1)])
    elif kind == "pes": stroke([(0, 7), (2, 10), (5, 8), (6, -1)])
    elif kind == "clivis": stroke([(0, 9), (2, 0), (5, -1), (7, 11)])
    elif kind == "torculus": stroke([(0, 6), (2, 9), (4, 1), (6, 0), (8, 9)])
    elif kind == "porrectus": stroke([(0, 0), (4, 9), (7, 9), (8, -1)])
    elif kind == "climacus": stroke([(0, 10), (3, 0)]); dot(6, 4); dot(9, 9)
    elif kind == "scandicus": dot(1, 10); dot(4, 6); stroke([(6, 10), (9, -1)])
    elif kind == "quilisma": stroke([(0, 8), (1, 6), (2, 8), (3, 6), (4, 8), (7, -1)])

# ------------------------------------------------------------------ mise en page de toute la page
class Page:
    def __init__(self, seed=1):
        self.font = ImageFont.truetype(FONT, 22 * S)
        self.rub = ImageFont.truetype(FONT, 13 * S)
        self.versal = ImageFont.truetype(FONT_BOLD, 40 * S)
        self.cap = ImageFont.truetype(FONT_BOLD, 92 * S)
        meas = ImageDraw.Draw(Image.new("L", (1, 1)))
        tl = lambda s, f: meas.textlength(s, font=f) / S
        R = random.Random(seed)
        self.ops = []                               # (x, y, kind, data) en coordonnées page (px du jeu)
        n_lines = PAGE_H // PITCH
        line, x = 1, MARGIN_X                      # la ligne 0 = marge du haut
        reserved = {}                              # ligne -> (x0, x1) réservé par une grande lettrine
        x0, x1 = MARGIN_X, PAGE_W - MARGIN_X
        ci = 0
        first = True

        def newline():
            nonlocal line, x
            line += 1; x = x0
            r = reserved.get(line)
            if r and x >= r[0] and x < r[1]:
                x = r[1]

        def fit(w):
            nonlocal x
            r = reserved.get(line)
            if r and x < r[1] and x + w > r[0]:
                x = r[1]
            if x + w > x1:
                newline()

        while line < n_lines - 1:
            title, text = CHANTS[ci % len(CHANTS)]; ci += 1
            by = lambda: LINE0 + line * PITCH
            # rubrique rouge
            w = tl(title, self.rub) + 14
            fit(w)
            if line >= n_lines - 1: break
            self.ops.append((x, by(), "rubric", title)); x += w
            words = text.split()
            first_letter, words[0] = words[0][0], words[0][1:]
            if first:                               # grande lettrine sur deux lignes au début de la page
                fit(64)
                self.ops.append((x, by() - 62, "cap", first_letter))
                reserved[line + 1] = (x - 4, x + 62)
                x += 58; first = False
            else:                                   # initiale rouge sur une ligne
                w = tl(first_letter, self.versal) + 4
                fit(w)
                self.ops.append((x, by(), "versal", first_letter)); x += w
            for wi, word in enumerate(words):
                syls = syllables(word) if word else []
                for k, syl in enumerate(syls):
                    n = R.choices([1, 2, 3], [70, 22, 8])[0]
                    kinds = [R.choice(KINDS) for _ in range(n)]
                    nw = sum(NW[q] + 3 for q in kinds)
                    tw = tl(syl, self.font)
                    box = max(nw, tw)
                    slot = box + (5 if k < len(syls) - 1 else 13)
                    fit(slot)
                    if line >= n_lines - 1: break
                    self.ops.append((x + (box - tw) / 2, by(), "syl", syl))
                    nx, ny = x + (box - nw) / 2, by() - 36 + R.randint(-3, 3)
                    for q in kinds:
                        self.ops.append((nx, ny + R.randint(-2, 2), "neume", q)); nx += NW[q] + 3
                    x += slot
            x += 10

    def render(self, cx, cy):
        """Renvoie (encre noire, encre rouge) en float 0..1, taille CELL_W×CELL_H, pour la salle (cx, cy)."""
        wx, wy = cx * CELL_W, cy * CELL_H
        ink = Image.new("L", (CELL_W * S, CELL_H * S), 0)
        red = Image.new("L", (CELL_W * S, CELL_H * S), 0)
        di, dr = ImageDraw.Draw(ink), ImageDraw.Draw(red)
        for (x, y, kind, data) in self.ops:
            if not (wx - 200 < x < wx + CELL_W + 50 and wy - 120 < y < wy + CELL_H + 120):
                continue
            X, Y = (x - wx) * S, (y - wy) * S
            if kind == "syl":
                di.text((X, Y - 17 * S), data, font=self.font, fill=255)
            elif kind == "neume":
                draw_neume(di, data, X, Y)
            elif kind == "rubric":
                dr.text((X, Y - 11 * S), data, font=self.rub, fill=255)
            elif kind == "versal":
                dr.text((X, Y - 35 * S), data, font=self.versal, fill=255)
            elif kind == "cap":
                dr.text((X, Y), data, font=self.cap, fill=255)
        f = lambda im: np.asarray(im.resize((CELL_W, CELL_H), Image.LANCZOS), np.float32) / 255.0
        return f(ink), f(red)
