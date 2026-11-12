"""Rendus de démonstration : une salle, un coin de page (3×2 salles), et la page entière (10×10)."""
import numpy as np
from PIL import Image, ImageDraw
import room_base as rb
from page import Page, GRID, CELL_W, CELL_H, PAGE_W, PAGE_H

INK, RED = np.array((58, 40, 30)), np.array((150, 52, 38))
page = Page(seed=1)

def room(cx, cy):
    img = rb.room_base(cx, cy)
    ink, red = page.render(cx, cy)
    wear = np.clip(0.55 + 0.6 * rb.fbm(41 + cx * 10 + cy) - 0.2 * rb.noise(2, 43), 0, 1)
    sl = (slice(rb.FY0, rb.FY0 + CELL_H), slice(rb.FX0, rb.FX0 + CELL_W))
    w = wear[sl]
    for a, col in ((ink * 0.50 * w, INK), (red * 0.50 * w, RED)):
        img[sl] = img[sl] * (1 - a[..., None]) + col * a[..., None]
    return Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))

def with_amadeus(im):
    a = Image.open("amadeus_hd_repos.png")
    r = im.convert("RGBA")
    r.alpha_composite(a, ((rb.FX0 + rb.FX1) // 2 - a.width // 2, (rb.FY0 + rb.FY1) // 2 - a.height // 2))
    return r.convert("RGB")

# 1) une salle au milieu de la page, avec Amadeus
r = with_amadeus(room(4, 4))
r.resize((1920, 1080), Image.NEAREST).save("page_salle_4-4.png")

# 2) le coin haut-gauche de la page : 3×2 salles côte à côte
corner = Image.new("RGB", (960 * 3, 540 * 2))
for cy in range(2):
    for cx in range(3):
        corner.paste(room(cx, cy), (cx * 960, cy * 540))
corner.save("page_coin_3x2.png")

# 3) la page entière (vue « carte »), réduite
full = np.zeros((PAGE_H, PAGE_W, 3), np.float32)
full[:] = (150, 138, 120)
for cy in range(GRID):
    for cx in range(GRID):
        ink, red = page.render(cx, cy)
        sl = (slice(cy * CELL_H, (cy + 1) * CELL_H), slice(cx * CELL_W, (cx + 1) * CELL_W))
        for a, col in ((ink * 0.85, INK), (red * 0.85, RED)):
            full[sl] = full[sl] * (1 - a[..., None]) + col * a[..., None]
ov = Image.fromarray(full.astype(np.uint8)).resize((PAGE_W // 6, PAGE_H // 6), Image.LANCZOS).convert("RGBA")
# exemple d'étage à la TBOI : salles occupées (1×1, 2×1, L), le reste estompé
used = {(4, 4), (5, 4), (4, 3), (3, 4), (3, 5), (5, 5), (4, 5), (6, 4), (7, 4), (4, 2), (2, 5), (5, 6), (5, 7)}
start = (4, 4)
cw, ch = CELL_W / 6, CELL_H / 6
veil = Image.new("RGBA", ov.size, (0, 0, 0, 0))
d = ImageDraw.Draw(veil)
for cy in range(GRID):
    for cx in range(GRID):
        box = [cx * cw, cy * ch, (cx + 1) * cw - 1, (cy + 1) * ch - 1]
        if (cx, cy) not in used:
            d.rectangle(box, fill=(30, 24, 22, 150))
        d.rectangle(box, outline=(60, 48, 40, 120))
for (cx, cy) in used:
    d.rectangle([cx * cw, cy * ch, (cx + 1) * cw - 1, (cy + 1) * ch - 1], outline=(255, 255, 255, 230), width=2)
d.rectangle([start[0] * cw + 3, start[1] * ch + 3, (start[0] + 1) * cw - 4, (start[1] + 1) * ch - 4],
            outline=(255, 63, 164, 255), width=3)
ov.alpha_composite(veil)
ov.convert("RGB").save("page_entiere.png")
print("ok")
