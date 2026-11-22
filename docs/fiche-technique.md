# Filon — fiche technique pour le portage C++

Référence : `docs/reference-js/filon-v17.html` (version JS publiée, v17).
Les numéros `L123` renvoient aux lignes de ce fichier. En cas de doute, **le code JS fait foi**.

---

## 1. Unités et repères

| Grandeur | Valeur | Note |
|---|---|---|
| Unité de logique | **case** (bloc) | toutes les positions physiques sont en cases (flottants) |
| Taille d'une case à l'écran | `TS = 32` px monde | multiplié ensuite par l'échelle de la caméra |
| Pixel de terre | `PX = 4` px monde, soit **8 × 8 pixels de terre par case** | unité du masque de terrain |
| Axe Y | **vers le bas** | angle 0 = droite, +90° = bas, −90° = haut |
| Pas de temps | `dt = min(0.05, temps écoulé)` | boucle à l'image, pas de pas fixe en JS |
| Flottants | JS utilise des `double` partout | **utiliser `double` en C++** pour la parité |

---

## 2. Monde

### 2.1 Dimensions (L128)
| Constante | Valeur | Sens |
|---|---|---|
| `W` | 36 | largeur en cases |
| `SURF` | 6 | première rangée de terre (rangées 0–5 = ciel) |
| `LH` | 90 | hauteur d'une couche |
| `NL` | 4 | nombre de couches |
| `H` | `SURF + NL·LH + 1` = **367** | hauteur totale (dernière rangée = socle) |
| `SPAWN` | 17 | colonne du panneau « MINE » au sol |

`layerOf(y) = y < SURF ? −1 : min(NL−1, floor((y−SURF)/LH))` (L208)

### 2.2 Types de case (L129)
`AIR=0, SOFT=1 (meuble), ROCK=2, HARD=3, BED=4 (socle), SEAL=5 (sceau), LAVA=6`, minerais `10..18`.
`solid(t) = t ≠ AIR && t ≠ LAVA` (L258).

### 2.3 Couches (L132)
| # | Nom | Multiplicateur de dureté `mult` |
|---|---|---|
| 0 | Terres brunes | 1 |
| 1 | Grès rouge | 2.4 |
| 2 | Basalte | 5.5 |
| 3 | Cœur ardent | 12 |

### 2.4 Minerais (L146)
| id | Nom | Valeur `v` | Dureté `hp` |
|---|---|---|---|
| 10 | Charbon | 3 | 1 |
| 11 | Cuivre | 7 | 1.2 |
| 12 | Fer | 13 | 1.5 |
| 13 | Argent | 24 | 1.6 |
| 14 | Or | 42 | 1.8 |
| 15 | Rubis | 75 | 2 |
| 16 | Émeraude | 130 | 2.2 |
| 17 | Diamant | 230 | 2.5 |
| 18 | Astralite | 400 | 3 |

Poids de tirage par couche `ORE_W` (L159) :
```
couche 0 : [40,30,18, 0, 0, 0, 0, 0, 0]
couche 1 : [10,20,30,25,12, 0, 0, 0, 0]
couche 2 : [ 0, 0,10,25,30,22,10, 0, 0]
couche 3 : [ 0, 0, 0, 5,15,25,30,18, 8]
```

### 2.5 Points de vie d'une case (L259)
```
m = LAYERS[max(0, layerOf(y))].mult
SOFT → m   ROCK → 2m   HARD → 4m   minerai → 2·ORES[id].hp·m   autres → ∞
```

### 2.6 Génération — ordre exact des tirages (L226–255)
**Critique pour la parité : même graine ⇒ même monde.** Un seul générateur `r = rng(seed)`, consommé dans cet ordre :

1. **Remplissage** : pour `y` de `SURF` à `H−1`, puis `x` de 0 à `W−1` :
   `n = r()` (tiré **même pour les bords**), `d = (y − SURF − L·LH)/LH`
   bord (`x=0`, `x=W−1`, `y=H−1`) → `BED` ; sinon `n < .5−.25d` → `SOFT` ; `n < .9−.15d` → `ROCK` ; sinon `HARD`.
2. **Pour chaque couche `L` = 0..3** (`y0 = SURF + L·LH`, `ylo = y0+4`, `yhi = y0+LH−17`) :
   1. **Grottes** : `9 + 2L` vers. Chaque ver : `x = 2 + floor(r()·(W−4))`, `y = ylo+3 + floor(r()·(yhi−ylo−3))`, `len = 10 + floor(r()·22)`. À chaque pas : creuse `(x,y)`, `(x+1,y)`, `(x,y+1)` si `0<X<W−1` et `ylo≤Y≤yhi`, puis `r()<.5` ? `x ± 1` (2ᵉ tirage `r()<.5` → −1) : `y` (+1 si `r()<.6`, sinon −1). Bornes : `x ∈ [1, W−3]`, `y ∈ [ylo, yhi−1]`.
   2. **Lave** (L ≥ 2) : pour `y` de `ylo` à `yhi`, `x` de 1 à `W−2` : si case ≠ AIR, ≠ BED, case du dessus = AIR, et `r() < (L=3 ? .35 : .08)` → `LAVA`. **Attention** : `r()` n'est tiré que si les 3 premières conditions sont vraies (évaluation paresseuse de `&&`).
   3. **Filons** : `n = floor(W·LH·0.1/3.5)` = **92** filons. `id = 10 + pickW(ORE_W[L], r)`, `x = 1+floor(r()·(W−2))`, `y = y0+1+floor(r()·(LH−5))`, `len = 2+floor(r()·4)`. À chaque pas : remplace si SOFT/ROCK/HARD, puis `r()<.5` ? `x ± 1` (2ᵉ tirage `r()<.5` → −1) : `y ± 1` (2ᵉ tirage `r()<.5` → **−1**, à la différence des grottes), bornes `x ∈ [1, W−2]`, `y ∈ [y0, y0+LH−3]`.
   4. **Salle du gardien** : `x1=5, x2=W−6, y1=y0+LH−14, y2=y0+LH−3` → AIR. Rangées `y2+1 .. y0+LH−1` → `SEAL` (couches 0–2) ou `HARD` (couche 3).
   5. **Créatures** : jusqu'à `7+3L` placées, 900 essais max. `x = 1+floor(r()·(W−2))`, `y = ylo+floor(r()·(yhi−ylo))`. Si case ≠ AIR → essai suivant. Sinon on descend tant que la case dessous est AIR (posée au sol), refus si une créature occupe déjà `(x, yy)`.
   `pickW(w, r)` : `x = r()·Σw`, renvoie le premier `i` tel que `x − Σ_{≤i} w < 0`.

### 2.7 Fonctions de hasard et de bruit (L211–218)
À reproduire **à l'identique** (entiers 32 bits) :
```
rng(s) (mulberry32) : s = (s + 0x6D2B79F5) | 0
                      t = imul(s ^ (s>>>15), 1|s)
                      t = (t + imul(t ^ (t>>>7), 61|t)) ^ t
                      return ((t ^ (t>>>14)) >>> 0) / 4294967296
hash(x,y)           : h = (imul(x,374761393) + imul(y,668265263)) | 0
                      h = imul(h ^ (h>>>13), 1274126177) ; return (h ^ (h>>>16)) >>> 0
nh(x,y,s)           : h = imul(x,374761393) ^ imul(y,668265263) ^ imul(s,1442695041)
                      h = imul(h ^ (h>>>13), 1274126177) ; return ((h ^ (h>>>16)) >>> 0) / 4294967296
vnoise(x,y,s)       : bruit de valeur bilinéaire lissé (smoothstep) sur nh
fbm(x,y,s,o)        : o octaves, amplitude ×0.5, fréquence ×2.03, graine s + 17·i, normalisé
```
C++ : `imul` = multiplication `int32_t` avec débordement (passer par `uint32_t`), `>>>` = décalage sur `uint32_t`, `| 0` = conversion en `int32_t`.

### 2.8 Tableaux d'état du monde
| Tableau | Taille | Contenu |
|---|---|---|
| `T0` | W·H octets | types d'origine (sert aux textures) |
| `T` | W·H octets | types actuels (minerai extrait → ROCK, sceau brisé → ROCK) |
| `E` | W·H octets | cases explorées (brouillard) ; rangée `SURF` toujours explorée |
| `M` | `MW·MH` = 288 × 2936 octets | **masque de terrain** : 1 = pixel de terre plein |
| `R` | W·H octets | nombre de pixels pleins restant par case (0–64) |

---

## 3. Masque de terrain et forage

### 3.1 Masque (L384–412)
- `MW = W·8 = 288`, `MH = H·8 = 2936`.
- `cellSolid(sx,sy)` : hors largeur ou `sy ≥ MH` → plein ; `sy < 0` → vide ; sinon `M`.
- Init : rangées `sy ≥ SURF·8` = `solid(T[case])` (ou sauvegarde). Puis la **base** (§6).

### 3.2 Forage en disque (L413–429)
`digAt(cx, cy, rad, budget)` :
1. Liste des pixels pleins du disque (centre de pixel `(sx+.5)/8`), en ignorant `sy < SURF·8`. Pixels de case `BED`/`SEAL` exclus (on note `blocked`).
2. Tri par distance croissante (**tri stable**, `std::stable_sort`).
3. Coût d'un pixel = `tileHP(case)/64`. On retire tant que `budget ≥ coût`.
4. Renvoie `{budget restant, removed, left, blocked, t, tx, ty}` (t/tx/ty = pixel le plus proche).

`removeCell` : `M=0`, `R[case]−−`, XP `+(HARD ? 2 : 1)/64`.
**Extraction d'un filon** : si la case est un minerai et `64 − R ≥ 20` → 1 pépite (2 si talent Filon double réussi : `random < .06·luck`), XP `+2 + 2·(id−10)`, `T[case] = ROCK`, recoloration de la texture.

### 3.3 Paramètres du forage (L505–513)
- Pointe : `centre + 0.45·(ux,uy)` ; si `|uy| < .38`, la pointe remonte de 0.05 (évite de creuser le sol en roulant).
- Rayon `0.47`. Budget : `digAcc += dps·dt` (max 50), conservé seulement s'il reste des pixels.
- **En biais** (`|ux|>.38` et `|uy|>.38`) : 2ᵉ disque au côté (`centre + (sign(ux)·.45, −.05)`), avec le budget restant → galerie en pente.
- Alerte « roche très dure » si `tileHP/dps > 7`.

---

## 4. La foreuse

### 4.1 Constantes (L130, L472)
| | Valeur |
|---|---|
| Boîte de collision | `PW = 0.78` × `PH = 0.8` cases, `(x,y)` = coin haut-gauche |
| Gravité `G` | 22 cases/s² |
| Poussée `THR` | 34 cases/s² |
| Vitesse verticale | bornée à [−6, 14] |
| Tête : débattement | ±45° (`D45`) par rapport à la caisse |
| Inclinaison max de la caisse | 50.4° (`TIP = 0.28π`) |
| Seuil de glissade | **50°** (`SLIDE`) |

### 4.2 Commandes → direction (L497–500, entrées)
- 8 directions (`OCT` : E, SE, S, SO, O, NO, N, NE) : clavier (2 flèches = diagonale), joystick ou souris (secteur de 45° de l'angle vers le curseur ; zone morte 0.6 case autour du centre).
- Changer de sens (`ix ≠ side`) : `side = ix`, `yaw = −yaw`, `yawT = −yawT`, `tilt = −tilt`.
- **Deux modes** : déplacement seul ; forage (clic gauche, Espace, bouton Forer). Les attaques ne se font qu'en mode forage.

### 4.3 Tête de forage et inclinaison (L473–494)
- **Pente du sol** : 3 sondes (`x+.08`, milieu, `x+PW−.08`), chacune cherche le premier pixel plein de `y+PH−.2` vers le bas sur 12 pas de 1/8. Si les 3 existent et que le milieu s'écarte de plus de 0.16 de la moyenne → **marche** (pas de pente). Si une sonde extrême manque, on mesure avec le milieu et l'autre. Angle signé selon `side` (positif = nez en haut).
- Inclinaison visée = pente (ou 0), bornée à ±TIP ; vitesse 3 rad/s.
- Cible de la tête : angle vers le curseur (ou la direction saisie). `yawT` = avant/arrière selon le signe de `cos`. Élévation voulue `pz ∈ [−90°, 90°]`, tête `thT = clamp(pz − tilt·yawT, ±45°)`.
- Vitesses : tête 4 rad/s, demi-tour (yaw) 4 /s.
- Angle final : `e = th + tilt·sign(yaw)`, `ang = atan2(−sin e, side·sign(yaw)·cos e)`.
- **Glissade** si `|pente| > 50°`.

### 4.4 Déplacement (L520–531)
- Vitesse visée `tv = ix · spd · (au sol ? 1 : 0.8)`.
- Accélération : au sol 32 (8 si glissade) ; en l'air 16 si commande, **0 sinon** (pas de freinage en l'air).
- Glissade : `vx += dir · G · sin|pente| · 0.8 · dt`, bornée à ±7, `dir` = vers le bas de la pente.
- **Propulseur** autorisé si `iy < 0`, carburant > 0, bras non accroché, et (dans la mine : `y+PH > SURF+0.25`, ou dans la colonne d'accostage). Dans la colonne, recentrage sous la trappe à 3 cases/s.
- Plafond de vol : `y ≥ DECK − 2.5`.
- `moveBox` (L390) : sous-pas de 0.06 case max ; X puis Y à chaque sous-pas ; si X bloqué et au sol → `stepUp` (monte de 1, 2 ou 3 pixels).
- **Lissage visuel** des marches : `vis += k/8` (max 0.5), décroît en `exp(−14·dt)` ; affichage seulement.

### 4.5 Après le déplacement (L565–581)
- Au sol = boîte décalée de 0.03 vers le bas touche un pixel plein.
- **Chute** : impact = `vy` au contact. Poussière si > 6. **Dégâts** si `impact > 12.5` **et** temps en l'air > 0.3 s : `(impact − 12.5)·8` (×0.4 si bras accroché).
- **Carburant** : à la base → plein + coque réparée. Sinon : propulseur 2.2 L/s ; dans la mine, rouler ou forer 1 L/s ; à l'arrêt 0 ; ×(1 − 0.07·Économe). Panne → `strand('fuel')`.
- Lave : boîte dans une case LAVA → 20 dégâts (délai 0.6 s).
- Exploration : disque de rayon `light` autour du centre.

### 4.6 Dégâts et retour forcé (L432–439)
- `hurt(d)` : `hull −= d·(1 − 0.07·Blindage)`, invulnérabilité 0.6 s.
- Panne sèche : moitié de la soute perdue. Destruction : soute vidée. Dans les deux cas : retour sur le pont (`x = 19.4`), plein, gardiens actifs réinitialisés.

### 4.7 Bras d'accroche (L533–554)
| Paramètre | Valeur |
|---|---|
| Épaule | `(x + PW/2, y + 0.15)` |
| Visée | vers le curseur (si > 0.3 case), sinon la direction saisie, sinon vers le haut |
| Déploiement | 18 cases/s jusqu'à `arm = 3 + 0.6·niveau` |
| Rétraction | 20 cases/s |
| Prise | premier pixel plein touché ; longueur = distance centre–prise |
| Treuil | haut : −3/s (min 0.9) ; bas (hors forage) : +3/s (max `arm + 0.5`) |
| Contrainte | si distance > longueur : on ramène le centre sur le cercle (si libre), on retire la vitesse radiale sortante, `vx ×0.995` |
| Lâcher | bouton relâché, ou pixel de la prise creusé |

---

## 5. Créatures, gardiens, projectiles, pépites

### 5.1 Créatures (L160, L586–601)
| k | Nom | PV | Dégâts | Vitesse | Pièces | XP |
|---|---|---|---|---|---|---|
| 0 | Larve | 24 | 6 | 1.6 | 6 | 4 |
| 1 | Scorpide | 90 | 12 | 2.2 | 25 | 12 |
| 2 | Golem | 320 | 24 | 1.2 | 90 | 35 |
| 3 | Salamandre | 900 | 38 | 2.8 | 260 | 90 |
- Boîte `0.7 × 0.6`, gravité G, `vy` max 14, même `moveBox`.
- Inactives à plus de 16 cases. Toutes les 0.3–0.9 s : si foreuse à moins de 8 cases → direction = point cardinal vers elle (saut `vy = −7.5` si N/NE/NO et au sol) ; sinon errance.
- Bloquée au sol → 3 % de chance par image de sauter.
- Contact (< 0.75 case) → dégâts.
- Coup de foret : `ram = 12·1.42^lames`, délai 0.32 s. Morte → pièces, XP, index ajouté à `dead`.

### 5.2 Gardiens (L166, L602–630)
| Couche | Nom | PV | Dégâts | Éclats | Pièces | XP |
|---|---|---|---|---|---|---|
| 0 | Mâchoire-Racine | 500 | 14 | 0 | 400 | 60 |
| 1 | Reine Scorpide | 2200 | 24 | 6 | 2500 | 200 |
| 2 | Colosse de Basalte | 8000 | 38 | 8 | 9000 | 600 |
| 3 | Cœur de Magma | 26000 | 55 | 12 | 30000 | 1500 |
- Rayon 1.25 case, posé au sol de la salle. S'active quand la foreuse entre dans la salle. Se réinitialise si elle en sort et s'éloigne à plus de 10 cases.
- États : `roam` (marche vers un but, 2 + 0.3L cases/s) → `wind` 0.6 s → `jump` (si foreuse plus haute de 1.5 ou 30 %) `vy = −(10+L)`, `vx = ±(3+0.5L)` ; sinon `charge` `vx = ±(8+L)` 0.9 s.
- Fin de charge (mur ou délai) ou atterrissage (`vy > 4` au contact du sol) → `roam` (durée `max(0.8, 1.6 + 1.2·rand − 0.15L)`) + éclats (`shots`, +4 si PV < 50 %) en éventail vers le haut (angles de −0.12π à −0.88π), vitesse 6–8.5, gravité 9, dégâts 0.6×, durée 4 s, détruits par un pixel plein, touchent la foreuse à moins de 0.5 case.
- Contact gardien–foreuse : distance < 1.65 case. Coup de foret possible à moins de 2.05 case, dans l'axe de la tête (cosinus > 0.35), délai 0.3 s.
- Mort → sceau de la couche changé en ROCK, récompense, sauvegarde.

### 5.3 Pépites (L440–454)
- Apparition au pixel retiré, `vx ∈ ±1.25`, `vy ∈ [−4, −2]` ; 200 max.
- Gravité G (max 12), `vx ×0.985`. Collisions par points sur le masque, rebond `×−0.3`, arrêt si |vy| < 0.8, roule le long des pentes (±5/s²).
- Aimant à moins de 1.6 case (14 cases/s²) si soute non pleine ; ramassée à moins de 0.6.

---

## 6. Base flottante (L131, L364–367, L401–411)
| Élément | Valeur |
|---|---|
| Dessus du pont | `DECK = SURF − 3.5 = 2.5` ; épaisseur 0.5 case (4 rangées de pixels) |
| Étendue | `DX1 = 5` à `DX2 = 21` |
| Trappe | `GX1 = 12.4` à `GX2 = 13.6` |
| Garde-corps | 2 colonnes de pixels à chaque bout, hauteur 1 case |
| Indestructible | le forage ignore `sy < SURF·8` |
- **À la base** = posée sur le pont (`|y+PH − DECK| < 0.06`), ou bras accroché sous le pont (prise `DECK−0.1 < hy < DECK+0.6`).
- **Colonne d'accostage** : centre dans `[GX1−.05, GX2+.05]`, centre plus bas que `DECK−1.6`, `y < SURF`.
- **Trappe** : ouverte si (foreuse sous le pont et dans la colonne) ou (sur le pont, au-dessus de la trappe, et `iy > 0`) ; sinon fermée, **jamais sur la foreuse**.

---

## 7. Économie

### 7.1 Améliorations (L172–182) — coût = `round(base · g^niveau)`
| Clé | Nom | base | g | max | Effet (niveau l) |
|---|---|---|---|---|---|
| power | Foret | 30 | 1.7 | 12 | `dps = 3·1.38^l·(1+0.06·Turbo)` |
| blade | Lames | 40 | 1.7 | 12 | `ram = 12·1.42^l` |
| speed | Moteur | 25 | 1.6 | 10 | `spd = 4 + 0.5l` cases/s |
| tank | Réservoir | 25 | 1.65 | 12 | `fuel = 60 + 28l` L |
| hull | Coque | 35 | 1.7 | 12 | `hull = 50 + 35l` PV |
| cargo | Soute | 30 | 1.65 | 12 | `cargo = 10 + 7l` places |
| light | Phare | 20 | 1.8 | 8 | `light = 3.2 + 0.7l` cases |
| radar | Radar | 60 | 1.9 | 8 | `radar = l ? 3+2l : 0` |
| smelt | Fonderie | 50 | 1.8 | 8 | `smelt = 3/(1+0.35l)` s/minerai |
| arm | Bras | 40 | 1.7 | 6 | `arm = 3 + 0.6l` cases |

### 7.2 Talents (L184) — 5 niveaux chacun, 1 point par niveau de joueur
Prospecteur (+8 % de valeur), Filon double (+6 % de chance), Turbo (+6 % de forage), Économe (−7 % de carburant), Blindage (−7 % de dégâts).

### 7.3 Règles
- XP pour passer le niveau `l` : `round(25·1.3^(l−1))` ; points libres = `niveau − 1 − Σ talents`.
- Valeur d'un minerai : `v·(1 + 0.08·Prospecteur)` ; lingot = ×**2.5** (`BAR`).
- Vente, fonte et achats : **seulement à la base**.
- **Fonderie** : file `[[id, n], ...]`, progression `p`. Avance en **temps réel** (`Date.now`), y compris jeu fermé, plafonnée à 8 h par rattrapage.
- Soute pleine : les pépites restent au sol.

---

## 8. Rendu (résumé des règles)
- **Textures de terre** en blocs de 12 × 8 cases (96 × 64 pixels de terre), générées à la demande (budget ~14 ms par image, 30 blocs en cache), affichées sans lissage (pixels nets).
- Couleur d'un pixel (`texel`) : frontières de matériau ondulées (fbm, ±22 px), palette par couche et matériau (`PAL` L139), strates pour le meuble, fissures pour la roche, grain pour la roche dure, braises en couche 3, pépites incrustées pour les minerais de `T`, relief, variation par pixel ±12 % quantifiée par paliers de 0.1, herbe sur la rangée `SURF`.
- Ombrage selon le masque : pixel plein exposé par le haut ×1.18 ; pixel creusé ×0.27 (×0.17 sous un surplomb).
- Brouillard : case non explorée en noir. Lumière : dégradé radial de rayon `light`.
- Caméra amortie : `1 − e^(−10·dt)` en X, `1 − e^(−6·dt)` en Y, recalage si écart > 6 cases.
- Échelle : `clamp(min(largeur, hauteur)/(10.5·32), 0.9, 2.2)`.

## 9. Son (L301–349)
- Effets : oscillateurs (carré, triangle, dent de scie, sinus) avec enveloppe exponentielle et glissement de fréquence, plus bruit blanc filtré passe-bas. Table des 15 effets L318–331.
- Musique : grille Am–F–C–G (`PROG` L336), 16 pas par accord, 0.22 s par pas (0.15 s pendant un gardien) ; basse sinus tous les 4 pas, arpèges triangle.
- C++ : flux audio raylib (`AudioStream`) rempli par un petit synthétiseur maison.

## 10. Sauvegarde (L283–299)
JSON, clé `filon-save-v1`, version `v:1`.
| Champ | Contenu |
|---|---|
| `seed` | graine du monde |
| `coins`, `cargo{id:n}`, `bars{id:n}`, `furnace{q,p,last}` | économie |
| `up{...}`, `tal{...}`, `xp`, `lvl` | progression |
| `boss[4]`, `dead[]` | gardiens vaincus, créatures tuées (index de génération) |
| `maxDepth`, `play`, `seenL`, `fallTip`, `set{music,sfx,vib,stick}` | divers |
| `pos[x,y]`, `fuel`, `hull` | foreuse |
| `tiles` (base64 de `T`), `exp` (base64 de `E`), `mask` (base64 du masque en bits, 1 bit par pixel) | monde |
| `nug[[id,x,y],...]` | pépites au sol |
**Nouveau monde** : nouvelle graine, `dead`, `boss`, `seenL`, position, monde et pépites remis à zéro ; le reste est conservé.
C++ (`core/save`) : même JSON, champs dans le même ordre, écrit octet pour octet comme `JSON.stringify` (nombres au format JS, pépites arrondies comme `toFixed(2)`, moitiés exactes en s'éloignant de zéro). Fichier `$XDG_DATA_HOME/filon/save.json` (sinon `~/.local/share/filon/save.json`), écrit à côté puis renommé ; un fichier illisible est renommé `save.json.illisible`, jamais écrasé. Valeurs hors bornes ramenées dans les limites (niveaux d'amélioration et de talent). Test : `parite-sauvegarde` relit une vraie sauvegarde JS et la réécrit à l'identique.

---

## 11. Pièges pour la parité
1. **Nombres** : `double` partout, sauf les hachages en entiers 32 bits (§2.7).
2. **Ordre des tirages** de `rng` à la génération (§2.6), y compris les tirages conditionnels de la lave.
3. **Tri stable** dans `digAt` (le tri de JS est stable depuis ES2019).
4. `Math.floor` sur des négatifs = `std::floor` (pas une conversion en entier).
5. `Math.random()` (non déterministe) sert aux effets, à l'IA des créatures, au double minerai et à la graine initiale : la parité ne s'applique pas à ces tirages.
6. `dt` limité à 0.05 s : à garder, ou passer à un pas fixe en ayant vérifié que les tests donnent les mêmes résultats.

## 12. Tests de parité proposés
| Test | Attendu (version JS) |
|---|---|
| `gen(seed)` pour 3 graines | `T` identique octet par octet ; même liste de créatures |
| Chute de toute la hauteur d'une salle de gardien, sans bras | −32 PV (avec seuil 10 ; **à remesurer avec le seuil actuel 12.5**) |
| Même chute, bras accroché à la paroi | 0 PV, foreuse suspendue |
| Pente de 35° construite dans le masque | inclinaison ≈ 31°, pas de glissade |
| Pente de 60° | glissade jusqu'en bas, 0 PV |
| Sol plat, visée vers le bas | tête −45°, inclinaison 0° |
| Trappe : viser vers le bas dessus / remonter par la colonne | s'ouvre / se referme sous la foreuse |
| 20 s à l'arrêt dans la mine | 0 L consommé |
| Fonte de 3 charbons + 2 cuivres, puis vente avec 4 fers | 110 pièces |
