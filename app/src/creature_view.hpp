#pragma once
// Dessin des créatures, des gardiens, des éclats et des sceaux, en pixels du monde (dans BeginMode2D).
#include "filon/core/creatures.hpp"

// cases SEAL visibles et explorées
void drawSeals(const filon::World& w, const std::vector<std::uint8_t>& explored, int x0, int x1, int y0, int y1, double time);
void drawFoe(const filon::Foe& f, const filon::Drill& d, double time);
void drawBoss(const filon::Boss& b, const filon::Drill& d, double time);
void drawBullets(const filon::Creatures& c);
void drawBossBar(const filon::Creatures& c);  // jauge du gardien réveillé, en bas de l'écran (hors BeginMode2D)
