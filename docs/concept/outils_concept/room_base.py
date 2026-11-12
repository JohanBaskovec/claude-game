"""Salle 1×1 en 960×540 : château médiéval, sol couvert d'un chant grégorien en neumes (IXe–Xe s.)."""
import math, random, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont

W, H = 960, 540
FX0, FY0, FX1, FY1 = 96, 108, 864, 470            # sol
FONT = "fonts/Junicode-Medium.ttf"
rng = np.random.default_rng(7)

# ------------------------------------------------------------------ outils
def noise(scale, seed):
    r = np.random.default_rng(seed).random((H // scale + 2, W // scale + 2)).astype(np.float32)
    im = Image.fromarray((r * 255).astype(np.uint8)).resize((W + scale * 2, H + scale * 2), Image.BICUBIC)
    return np.asarray(im, np.float32)[:H, :W] / 255.0

def fbm(seed):
    return 0.5 * noise(48, seed) + 0.3 * noise(12, seed + 1) + 0.2 * noise(3, seed + 2)

def hash01(*arrs):
    h = np.zeros_like(arrs[0], dtype=np.int64)
    for i, a in enumerate(arrs):
        h ^= (a.astype(np.int64) + 1000) * [73856093, 19349663, 83492791, 2654435761][i]
    return ((h % 10007) / 10007.0).astype(np.float32)

Y, X = np.mgrid[0:H, 0:W].astype(np.float32)
X += 0.5; Y += 0.5
img = np.zeros((H, W, 3), np.float32)
grain = fbm(1)

# ------------------------------------------------------------------ géométrie des murs (u le long du mur, t de l'extérieur (0) vers le sol (1))
floor = (X >= FX0) & (X < FX1) & (Y >= FY0) & (Y < FY1)
U = np.zeros((H, W), np.float32); T = np.zeros_like(U); L = np.ones_like(U); D = np.ones_like(U)
WID = np.full((H, W), -1, np.int32)
taken = floor.copy()
def wall(wid, t, along, lo, hi, depth):
    m = (~taken) & (t >= 0) & (t <= 1) & (along >= lo) & (along < hi)
    U[m] = ((along - lo) / (hi - lo))[m]; T[m] = t[m]; L[m] = (hi - lo)[m]; D[m] = depth; WID[m] = wid
    taken[m] = True
tN = Y / FY0;                 wall(0, tN, X, FX0 * tN, W - (W - FX1) * tN, FY0)
tS = (H - Y) / (H - FY1);     wall(1, tS, X, FX0 * tS, W - (W - FX1) * tS, H - FY1)
tO = X / FX0;                 wall(2, tO, Y, FY0 * tO, H - (H - FY1) * tO, FX0)
tE = (W - X) / (W - FX1);     wall(3, tE, Y, FY0 * tE, H - (H - FY1) * tE, W - FX1)
walls = WID >= 0

# ------------------------------------------------------------------ briques des murs
courses = 5
NB = np.where(WID < 2, 20, 10).astype(np.float32)
ct = T * courses; ci = np.floor(ct); fv = ct - ci
wv = U * NB + 0.5 * (ci % 2); bi = np.floor(wv); fu = wv - bi
mu = 2.0 * NB / L; mv = 2.0 * courses / D
bh = hash01(bi, ci, WID)
base = np.stack([112 + bh * 26 - 13, 98 + bh * 22 - 11, 86 + bh * 18 - 9], -1)
base *= (0.86 + 0.28 * grain)[..., None]
light = (fv < mv * 2.4) | (fu < mu * 1.6)
dark = (fv > 1 - mv * 1.6) | (fu > 1 - mu * 1.2)
base[light] *= 1.13; base[dark] *= 0.8
mortar = (fu < mu) | (fv < mv)
base[mortar] = np.array([54, 44, 39]) * (0.9 + 0.2 * grain[mortar])[..., None]
# éclats et briques abîmées
chip = (hash01(bi, ci, WID * 7 + 3) < 0.08) & (fbm(9) > 0.62)
base[chip & ~mortar] *= 0.78
base *= (0.5 + 0.5 * T)[..., None]                         # profondeur : plus sombre vers le haut du mur
cap = T < 0.075
base[cap] = np.array([46, 37, 34]) * (0.85 + 0.3 * grain[cap])[..., None]
edge = (T >= 0.075) & (T < 0.095)
base[edge] = np.array([92, 80, 70])
img[walls] = base[walls]

# ------------------------------------------------------------------ portes
DU = {0: (0.435, 0.565), 1: (0.435, 0.565), 2: (0.37, 0.63), 3: (0.37, 0.63)}
a = np.choose(np.clip(WID, 0, 3), [DU[i][0] for i in range(4)])
b = np.choose(np.clip(WID, 0, 3), [DU[i][1] for i in range(4)])
du = (U - (a + b) / 2) / ((b - a) / 2)
T0, ARCH = 0.14, 0.40
dt = (T - T0) / (1 - T0)
def inside(g):
    ay = (ARCH - dt) / (ARCH + g)
    return walls & (dt >= -g) & (np.abs(du) <= 1 + g) & ((dt >= ARCH) | (du ** 2 + ay ** 2 <= (1 + g) ** 2))
wood = inside(0.0)
frame = inside(0.30) & ~wood
door = wood | frame

# encadrement : claveaux rayonnants dans l'arc, blocs droits en dessous
theta = np.arctan2((ARCH - dt) / ARCH, du)
seg = theta / (math.pi / 9)
fr = np.stack([150 + 0 * X, 140 + 0 * X, 128 + 0 * X], -1) * (0.85 + 0.3 * grain)[..., None]
fh = hash01(np.floor(seg), np.floor(dt * 6), WID)
fr *= (0.92 + 0.14 * fh)[..., None]
joint_arch = (dt < ARCH) & (np.abs(seg - np.round(seg)) < 0.07)
joint_jamb = (dt >= ARCH) & (np.abs(dt * 6 - np.round(dt * 6)) < 0.05)
fr[joint_arch | joint_jamb] = [70, 60, 54]
fr *= (0.62 + 0.38 * T)[..., None]
img[frame] = fr[frame]

# bois : planches, veinure, ombre sous l'arc
plank = (du + 1) * 2.5
pf = plank - np.floor(plank)
streak = np.sin(du * 55 + fbm(21) * 9) * 0.5 + 0.5
wd = np.stack([118 + 0 * X, 72 + 0 * X, 38 + 0 * X], -1)
wd *= (0.82 + 0.22 * streak + 0.1 * hash01(np.floor(plank), WID))[..., None]
wd[(pf < 0.07) | (pf > 0.95)] = [62, 36, 20]
strap = ((dt > 0.36) & (dt < 0.43)) | ((dt > 0.80) & (dt < 0.87))
wd[strap] = [62, 60, 68]
wd[strap & ((dt > 0.42) & (dt < 0.43) | (dt > 0.86))] = [40, 38, 44]
rivet = strap & (np.abs(pf - 0.5) < 0.1) & (np.abs(((dt > 0.6) * 0.835 + (dt <= 0.6) * 0.395) - dt) < 0.014)
wd[rivet] = [150, 146, 150]
rr = np.sqrt(((du - 0.42) / 0.16) ** 2 + ((dt - 0.62) / 0.085) ** 2)     # heurtoir
wd[(rr > 0.72) & (rr < 1.0)] = [70, 66, 72]
wd[(rr > 0.72) & (rr < 0.82)] = [130, 126, 132]
wd *= (0.55 + 0.45 * np.clip(dt / 0.35, 0, 1))[..., None]          # ombre sous l'arc
wd *= (0.62 + 0.38 * T)[..., None]
img[wood] = wd[wood]

# contours
def outline(mask, col, other=None):
    m = mask
    e = np.zeros_like(m)
    for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        e |= m & ~np.roll(np.roll(m, dy, 0), dx, 1)
    if other is not None:
        e &= other
    img[e] = col
outline(door, (24, 18, 18), walls)
outline(wood, (34, 22, 16))

def room_base(cx=0, cy=0):
    # ------------------------------------------------------------------ sol : grandes dalles
    SW, SH = 72, 46
    ly, lx = Y - FY0, X - FX0
    row = np.floor(ly / SH); off = (row % 2) * SW / 2
    col = np.floor((lx + off) / SW)
    sv = hash01(col + 37 * cx, row + 91 * cy)
    fl = np.stack([158 + sv * 14 - 7, 145 + sv * 12 - 6, 126 + sv * 10 - 5], -1)
    fl *= (0.94 + 0.12 * grain)[..., None]
    fl *= (0.985 + 0.03 * fbm(31))[..., None]
    jy = (ly % SH) < 2; jx = ((lx + off) % SW) < 2
    fl[jy | jx] = [132, 120, 105]
    fl[((ly % SH) >= 2) & ((ly % SH) < 3)] *= 1.05                     # arête éclairée
    # ombres des murs + vignette
    f = np.ones((H, W), np.float32)
    f -= np.clip(26 - ly, 0, None) * 0.012
    f -= np.clip(14 - lx, 0, None) * 0.012 + np.clip(14 - (FX1 - X), 0, None) * 0.012
    nx, ny = (X - W / 2) / (W / 2), (Y - (FY0 + FY1) / 2) / ((FY1 - FY0) / 2)
    f -= 0.07 * (nx ** 2 + ny ** 2)
    fl *= f[..., None]
    out = img.copy()
    out[floor] = fl[floor]
    return out

