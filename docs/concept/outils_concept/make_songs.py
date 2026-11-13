# Chansons pop originales d'Amadeus (GDD §26), écrites en degrés de la gamme majeure
# (0 = tonique, 7 = octave, -1 = sensible en dessous), converties au format de chant_data.h.
import sys
MAJOR = [0, 2, 4, 5, 7, 9, 11]
def semis(d):
    o, k = divmod(d, 7)
    return 12 * o + MAJOR[k]
# (syllabe, voyelle, degrés)
SONGS = [
 ("Yeah Yeah Baby", [
  ("oh",'o',[4,3,2]),("ba",'a',[0]),("by",'i',[2]),("yeah",'e',[4]),("yeah",'e',[4]),("yeah",'e',[5,4]),
  ("ba",'a',[2]),("by",'i',[2]),("you",'u',[4]),("know",'o',[3,2]),("I",'a',[0]),("got",'o',[2]),("the",'e',[2]),
  ("love",'a',[4]),("oh",'o',[5]),("oh",'o',[4]),("ba",'a',[2]),("by",'i',[1]),("yeah",'e',[0,-1,0])]),
 ("Tonight", [
  ("to",'u',[4]),("night",'a',[7]),("to",'u',[4]),("night",'a',[7]),("we're",'i',[5]),("gon",'o',[4]),("na",'a',[2]),
  ("shine",'a',[4,2]),("to",'u',[4]),("night",'a',[7]),("oh",'o',[9,7]),("oh",'o',[5,4]),("we",'i',[5]),("gon",'o',[4]),
  ("na",'a',[2]),("fly",'a',[0,2]),("to",'u',[2]),("night",'a',[0])]),
 ("Na Na Summer", [
  ("na",'a',[0]),("na",'a',[0]),("na",'a',[2]),("na",'a',[4]),("sum",'a',[5]),("mer",'e',[4]),("is",'i',[2]),("here",'i',[4]),
  ("na",'a',[0]),("na",'a',[0]),("na",'a',[2]),("na",'a',[4]),("dan",'a',[5]),("cing",'i',[4]),("all",'o',[2]),
  ("year",'i',[1,0]),("hey",'e',[7])]),
 ("Heartbeat", [
  ("my",'a',[2]),("heart",'a',[4]),("goes",'o',[4]),("boom",'u',[2]),("boom",'u',[0]),("boom",'u',[2]),("you",'u',[4]),
  ("make",'e',[5]),("me",'i',[4]),("move",'u',[2,4]),("oh",'o',[5,4]),("yeah",'e',[2,1,0]),("my",'a',[2]),("heart",'a',[4]),
  ("goes",'o',[4]),("boom",'u',[0])]),
 ("Forever", [
  ("ooh",'u',[7,6,4]),("for",'o',[2]),("ev",'e',[4]),("er",'e',[5]),("you",'u',[4]),("and",'a',[2]),("me",'i',[4]),
  ("ooh",'u',[7,9,7]),("for",'o',[2]),("ev",'e',[4]),("er",'e',[5]),("ba",'a',[4]),("by",'i',[2]),("can't",'a',[1]),
  ("you",'u',[2]),("see",'i',[0,-1,0])]),
 ("Hands Up", [
  ("put",'u',[4]),("your",'o',[4]),("hands",'a',[4]),("up",'a',[2]),("hands",'a',[4]),("up",'a',[2]),("eh",'e',[5]),
  ("oh",'o',[4,2]),("put",'u',[4]),("your",'o',[4]),("hands",'a',[4]),("up",'a',[2]),("ev",'e',[2]),("ery",'i',[4]),
  ("bo",'o',[5]),("dy",'i',[4]),("eh",'e',[5,7]),("oh",'o',[4])]),
]
def ident(name): return "SONG_" + "".join(c if c.isalnum() else "_" for c in name.upper())
out = ["/* Chansons pop originales d'Amadeus (GDD §26), composées pour le jeu (paroles : clichés pop anglais).",
       " * Générées par docs/concept/outils_concept/make_songs.py (degrés de la gamme majeure) : modifier le script, pas ce fichier. */",
       "#ifndef FUGUE_SONG_DATA_H", "#define FUGUE_SONG_DATA_H", "", '#include "chant_data.h"   /* ChantSyllable, Chant */', ""]
for name, syls in SONGS:
    lo = min(semis(d) for _, _, ds in syls for d in ds)
    out.append(f"static const ChantSyllable {ident(name)}[] = {{")
    row = []
    for t, v, ds in syls:
        notes = "".join(chr(ord('a') + semis(d) - lo) for d in ds)
        row.append(f'{{ "{t}", \'{v}\', "{notes}" }}')
    for k in range(0, len(row), 6): out.append("    " + ", ".join(row[k:k+6]) + ",")
    out.append("};"); out.append("")
out.append("static const Chant SONGS[] = {")
for name, _ in SONGS:
    i = ident(name); out.append(f'    {{ "{name}", {i}, (int)(sizeof {i} / sizeof {i}[0]) }},')
out += ["};", "#define SONG_COUNT ((int)(sizeof SONGS / sizeof SONGS[0]))", "", "#endif", ""]
open(sys.argv[1], "w").write("\n".join(out))
