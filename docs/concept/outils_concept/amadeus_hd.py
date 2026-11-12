"""Amadeus en 56×68 px pour la résolution 960×540 : pyjama, casque sur la tête, mèche coupée."""
import sys
from PIL import Image

W, H = 56, 68
OUT = (38, 22, 40)                 # contour extérieur
SK, SKS, SKD = (246, 204, 170), (226, 170, 138), (196, 132, 108)
BLU = (255, 150, 160)
HR, HH, HD = (102, 64, 38), (140, 94, 56), (66, 40, 26)
WH = (255, 255, 255)
IR, PU = (110, 70, 40), (26, 16, 20)
MO, TG, TE = (112, 22, 42), (232, 96, 112), (250, 244, 236)
PJ, PJL, PJD = (45, 226, 230), (140, 246, 246), (18, 150, 170)
LI, LID = (182, 255, 59), (118, 196, 30)
PK, PKL, PKD = (255, 70, 168), (255, 160, 210), (176, 26, 108)

def ell(cx, cy, rx, ry):
    return {(x, y) for y in range(H) for x in range(W)
            if ((x + .5 - cx) / rx) ** 2 + ((y + .5 - cy) / ry) ** 2 <= 1}

def draw(singing):
    px = {}
    put = lambda pts, c: [px.__setitem__(p, c) for p in pts]

    # ---------------- corps
    shirt = {(x, y) for y in range(39, 57) for x in range(18 - (y - 39) // 9, 38 + (y - 39) // 9)}
    put(shirt, PJ)
    put({(x, y) for (x, y) in shirt if x >= 33}, PJD)               # ombre côté droit
    put({(x, y) for (x, y) in shirt if x <= 20 and y < 52}, PJL)    # lumière côté gauche
    for side in (-1, 1):                                            # manches
        sx = 15 if side < 0 else 39
        sleeve = {(x, y) for y in range(41, 54) for x in range(sx, sx + 4)}
        put(sleeve, PJ if side < 0 else PJD)
        put({(sx + (0 if side < 0 else 3), y) for y in range(42, 53)}, PJL if side < 0 else PJD)
        put({(x, 53) for x in range(sx, sx + 4)}, LI)               # poignet
        put(ell(sx + 2, 56, 2.6, 2.6), SK)                          # main
        put({(sx + 3 if side < 0 else sx + 1, 57)}, SKS)
    put({(x, 55) for x in range(19, 38)} | {(x, 56) for x in range(19, 38)}, PJD)   # ceinture du haut
    # col rond vert citron + boutons
    put({(x, 39) for x in range(21, 35)} | {(x, 40) for x in range(23, 33)}, LI)
    put({(27, y) for y in (43, 47, 51)} | {(28, y) for y in (43, 47, 51)}, WH)
    # petits dinos imprimés
    dino = [(0, 1), (1, 1), (2, 1), (3, 1), (3, 0), (4, 0), (0, 2), (1, 2), (2, 2), (3, 2), (1, 3), (3, 3), (-1, 1)]
    for ox, oy in ((20, 44), (31, 49), (21, 51)):
        put({(ox + dx, oy + dy) for dx, dy in dino}, LI)
    put({(34, 43), (24, 48), (32, 54)}, LI)
    # plis
    put({(25, 53), (26, 54), (30, 45), (31, 46)}, PJD)

    # ---------------- jambes et chaussons-dinos
    for lx in (20, 30):
        put({(x, y) for y in range(57, 62) for x in range(lx, lx + 7)}, PJ if lx == 20 else PJD)
        put({(lx + 3, y) for y in range(58, 62)}, PJD)
    for cx in (22, 34):
        sl = ell(cx, 63.5, 6.5, 3.6)
        put(sl, LI)
        put({(x, y) for (x, y) in sl if y >= 65}, LID)
        put({(cx - 3, 62), (cx + 1, 62)}, PU)                         # yeux du dino
        put({(cx - 4, 64), (cx - 3, 65), (cx - 2, 64)}, WH)           # dents
        put({(cx + 4, 61), (cx + 5, 61)}, LID)                        # crête

    # ---------------- tête
    cx, cy, rx, ry = 28, 22, 20.5, 18.5
    head = ell(cx, cy, rx, ry)
    put(head, SK)
    put({(x, y) for (x, y) in head if ((x + .5 - cx - 6) / rx) ** 2 + ((y + .5 - cy - 5) / ry) ** 2 > 0.78}, SKS)
    put({(x, y) for (x, y) in head if y >= 37}, SKD)
    # cheveux
    hair = {(x, y) for (x, y) in head if y < 13 or (y < 17 and (x < 14 or x > 41)) or (y < 22 and (x < 11 or x > 44))}
    put(hair, HR)
    spikes = [(13, 3), (14, 2), (15, 2), (16, 3), (21, 1), (22, 0), (23, 1), (30, 1), (31, 0), (32, 1), (33, 2),
              (38, 2), (39, 1), (40, 2), (9, 6), (8, 7), (45, 5), (46, 6), (47, 7)]
    put(spikes, HR)
    # mèches de frange qui retombent
    for fx, ln in ((14, 4), (17, 6), (20, 4), (23, 7), (26, 5), (29, 6)):
        put({(fx, 13 + i) for i in range(ln)} | {(fx + 1, 13 + i) for i in range(ln - 1)} | {(fx + 2, 13 + i) for i in range(max(ln - 3, 0))}, HR)
        put({(fx, 13 + i) for i in range(ln - 2)}, HH)
    # reflets et ombres des cheveux
    put({(x, 7) for x in range(14, 22)} | {(x, 8) for x in range(12, 18)} | {(x, 9) for x in range(11, 15)}, HH)
    put({(x, 4) for x in range(19, 24)} | {(x, 5) for x in range(17, 21)}, HH)
    put({(x, y) for (x, y) in hair if x >= 42 and y > 10}, HD)
    # MÈCHE COUPÉE : trou net dans la frange, cuir chevelu visible + repousse
    gap = {(x, y) for y in range(9, 19) for x in range(33, 41) if y - 9 > (x - 33) * 0.35}
    put(gap, SKS)
    put({(34, 12), (36, 11), (38, 14), (35, 15), (39, 12)}, HD)          # repousse
    put({(32, y) for y in range(9, 19)}, HD)                              # coupe nette
    put({(x, 19) for x in range(33, 41)}, SKS)

    # ---------------- visage
    for ex, side in ((19, -1), (37, 1)):
        put(ell(ex, 26, 4.6, 5.2), WH)
        put(ell(ex + side * 0.6, 26.8, 3.1, 3.8), IR)
        put(ell(ex + side * 0.6, 27.2, 1.8, 2.3), PU)
        put({(ex - 1, 24), (ex - 1 + side, 25)}, WH)
        put({(ex + 1, 29)}, WH)
        # paupière supérieure + sourcil froncé (détermination inquiète)
        put({(x, 21) for x in range(ex - 4, ex + 4)}, PU)
        brow = [(ex - 4, 17 + (1 if side < 0 else -1) * 0), (ex - 3, 17), (ex - 2, 17), (ex - 1, 18), (ex, 18), (ex + 1, 19), (ex + 2, 19)]
        if side > 0:
            brow = [(2 * ex - x, y) for x, y in brow]
        put(brow, HD)
    put({(27, 31), (28, 31), (29, 32)}, SKS)                                # nez
    put(ell(13, 32, 3, 1.6), BLU); put(ell(43, 32, 3, 1.6), BLU)           # joues
    if singing:
        mouth = ell(28, 36.5, 4.6, 4.8)
        put(mouth, MO)
        put({(x, y) for (x, y) in mouth if y >= 39}, TG)
        put({(x, 32) for x in range(26, 31)}, TE)
        put({(x, y) for x in range(23, 34) for y in range(31, 43)
             if (x, y) not in mouth and any((x + dx, y + dy) in mouth for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))}, PU)
    else:
        put({(26, 34), (27, 34), (28, 34), (29, 34), (30, 35), (25, 35)}, PU)
        put({(27, 35), (28, 35)}, SKS)

    # ---------------- casque audio sur la tête
    band = {(x, y) for y in range(0, 24) for x in range(W)
            if 0.93 <= (((x + .5 - cx) / (rx + 1.5)) ** 2 + ((y + .5 - cy) / (ry + 1.5)) ** 2) ** .5 <= 1.08}
    put(band, PK)
    put({(x, y) for (x, y) in band if (((x + .5 - cx) / (rx + 1.5)) ** 2 + ((y + .5 - cy) / (ry + 1.5)) ** 2) ** .5 > 1.03 and x < 30}, PKL)
    put({(x, y) for (x, y) in band if (((x + .5 - cx) / (rx + 1.5)) ** 2 + ((y + .5 - cy) / (ry + 1.5)) ** 2) ** .5 < 0.97}, PKD)
    for ox, side in ((6.5, -1), (49.5, 1)):
        cup = ell(ox, 25, 5.2, 8.5)
        put(cup, PK)
        put({(x, y) for (x, y) in cup if (x - ox) * side > 2.2}, PKD)          # coussinet côté tête... non : côté extérieur sombre
        put({(x, y) for (x, y) in cup if (x - ox) * side < -2.5}, PKD)         # coussinet contre la tête
        put({(int(ox) - 1 + (1 if side > 0 else 0), y) for y in (19, 20, 21)}, PKL)   # reflet
        # petite croche blanche sur l'écouteur
        nx = int(ox) - 1
        put({(nx + 1, 24), (nx + 1, 25), (nx + 1, 26), (nx + 1, 27), (nx, 28), (nx + 1, 28), (nx, 29), (nx + 2, 24), (nx + 3, 25)}, WH)

    # ---------------- contour extérieur
    out = dict(px)
    for (x, y) in px:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            q = (x + dx, y + dy)
            if q not in px and 0 <= q[0] < W and 0 <= q[1] < H:
                out[q] = OUT
    im = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    for (x, y), c in out.items():
        if 0 <= x < W and 0 <= y < H:
            im.putpixel((x, y), c + (255,))
    return im

if __name__ == "__main__":
    for name, sing in (("amadeus_hd_repos", False), ("amadeus_hd_chante", True)):
        im = draw(sing)
        im.save(f"{name}.png")
        im.resize((W * 8, H * 8), Image.NEAREST).save(f"{name}_x8.png")
