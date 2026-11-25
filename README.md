# Filon (C++)

Portage en C++20 + raylib 6.0 du jeu de forage Filon.
Référence : `docs/reference-js/filon-v17.html` ; règles détaillées : `docs/fiche-technique.md`.

## Organisation
| Dossier | Rôle |
|---|---|
| `core/` | cœur du jeu en C++ pur (aucune dépendance graphique), testable seul |
| `app/` | le jeu : fenêtre, rendu, son, commandes (raylib) |
| `tests/` | tests de parité avec la version JS |
| `external/raylib/` | raylib 6.0 (sous-module git) |
| `web/` | la version web jouable (HTML + canvas), interface en atomic design |

## Version web (atomic design)
L'interface (relevés, menu de la base, écran titre) est découpée selon l'atomic design ; le moteur du jeu reste dans `web/src/game.js`.
| Niveau | Dossier | Contenu |
|---|---|---|
| Jetons | `web/src/tokens/` | couleurs, polices |
| Atomes | `web/src/atoms/` | bouton, jauge, pièce, losange de minerai, textes, champs de réglage |
| Molécules | `web/src/molecules/` | ligne d'article, total, réglage, boîte de confirmation, onglets, relevés |
| Organismes | `web/src/organisms/` | panneaux de la base, relevés, jauge du gardien, message, boutons, écran titre |
| Modèle | `web/src/templates/` | canvas plein écran, interface en surimpression |
| Page | `web/index.html` | assemble le tout ; s'ouvre tel quel dans un navigateur |

Les composants sont des fonctions pures (données en entrée, HTML en sortie) ; `game.js` leur prépare les données.
Pour le téléphone, un seul fichier hors ligne :
```bash
node tools/build_web.js
```
donne `web/dist/filon.html` (non suivi par git). `docs/reference-js/filon-v17.html` reste la version figée qui sert aux tests de parité.

## Compiler et lancer (Linux)
```bash
git submodule update --init
cmake -S . -B build/dev -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/dev
ctest --test-dir build/dev --output-on-failure
./build/dev/app/filon
```
