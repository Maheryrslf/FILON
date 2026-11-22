#pragma once
// Dessin de la foreuse (chenilles, caisse inclinée, tourelle et tête de forage), en pixels du monde.
#include "filon/core/drill.hpp"

void drawDrill(const filon::Drill& d, double time);
void drawArm(const filon::Drill& d);  // bras d'accroche télescopique et sa pince
