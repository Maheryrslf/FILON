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

## Compiler et lancer (Linux)
```bash
git submodule update --init
cmake -S . -B build/dev -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/dev
ctest --test-dir build/dev --output-on-failure
./build/dev/app/filon
```
