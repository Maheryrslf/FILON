#pragma once
// Décor et lumière, d'après drawSurface, drawAir et le halo de la version JS.
#include <cstdint>
#include <vector>

#include "filon/core/drill.hpp"
#include "filon/core/world.hpp"

// Collines, colonne d'accostage, base flottante (moteurs, pont, trappe, garde-corps, bâtiments, pompe) et panneau MINE.
// En pixels du monde, dans BeginMode2D, avant le terrain.
void drawSurface(double camX, double vwT, double time, bool hatchOpen, bool furnaceBusy);

// Bassins de lave animés sur les cases explorées (pixels du monde).
void drawLava(const filon::World& w, const std::vector<std::uint8_t>& explored, int x0, int x1, int y0, int y1, double time);

// Radar : losanges sur les minerais à portée mais hors de la lumière (pixels du monde).
void drawRadar(const filon::World& w, const filon::Drill& d, int radius, double light, double time);

// Obscurité autour de la foreuse, sous la surface : (sx, sy) centre, sg haut du sous-sol, r rayon, en pixels d'écran.
void drawDarkness(float sx, float sy, float sg, float r, bool bossAwake);
