# Outils de concept — Amadeus' Minor Fugue

Scripts Python (Pillow + numpy) qui ont servi à produire les concepts. Ils servent de référence pour le portage en C.

- `page.py` : la page de l'étage (10×10 cellules), mise en page du latin, syllabation, neumes, rendu d'une cellule.
- `room_base.py` : murs en perspective, portes, dalles du sol (960×540).
- `compose_page.py` : génère `page_salle_4-4.png`, `page_coin_3x2.png`, `page_entiere.png`.
- `amadeus_hd.py` : le sprite d'Amadeus (56×68), repos et chante.
- `../../../assets/fonts/` : Junicode 2.211 (Peter S. Baker, SIL Open Font License 1.1, https://github.com/psb1558/Junicode-font).

Lancer depuis ce dossier : `python3 compose_page.py` (nécessite `pip install pillow numpy`).
