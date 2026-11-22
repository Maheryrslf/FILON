#pragma once
// Déplacement d'une boîte sur le terrain en pixels : petits pas, montée des marches (fiche technique §4.4).
#include "filon/core/terrain.hpp"

namespace filon {

struct Body {
    double x = 0, y = 0;    // coin haut-gauche, en cases
    double vx = 0, vy = 0;  // cases par seconde
    bool grounded = false;
};

struct MoveResult {
    bool hx = false, hy = false;  // bloqué horizontalement / verticalement
    double stepped = 0;           // hauteur totale des marches montées (en cases), pour le lissage visuel
};

// Déplace la boîte (w x h cases) de (dx, dy) par pas de 0,06 case au plus ; au sol, monte les marches de 1 à 3 pixels.
MoveResult moveBox(const Terrain& t, Body& b, double w, double h, double dx, double dy);

}  // namespace filon
