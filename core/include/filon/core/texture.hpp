#pragma once
// Textures de terre en pixels (fiche technique §8) : couleur de chaque pixel, ombrage selon le masque,
// blocs de 12 x 8 cases (96 x 64 pixels de terre). Calcul pur, sans dépendance graphique.
#include <cstdint>
#include <vector>

#include "filon/core/terrain.hpp"
#include "filon/core/world.hpp"

namespace filon {

constexpr int PX = 4;               // un pixel de terre = 4 pixels du monde
constexpr int CW = 12, CH = 8;      // taille d'un bloc de texture, en cases
constexpr int CHUNK_W = CW * 8;     // 96 pixels de terre
constexpr int CHUNK_H = CH * 8;     // 64 pixels de terre
constexpr int CHUNKS_X = (W + CW - 1) / CW;
constexpr int CHUNKS_Y = (H + CH - 1) / CH;

inline int textureSeed(std::uint32_t worldSeed) { return static_cast<int>(worldSeed % 9973 + 1); }

struct Rgb {
    double r, g, b;
};
// Couleur de base d'un pixel (u, v : centre du pixel en pixels du monde ; hc : relief ; sh : pente du relief)
Rgb texel(const World& w, int texSeed, double u, double v, double hc, double sh);

struct Chunk {
    int cx = 0, cy = 0;
    std::vector<std::uint8_t> base;  // couleur de base, RGBA (alpha 0 = ciel)
    std::vector<std::uint8_t> rgba;  // couleur affichée, avec l'ombrage du masque
};

Chunk buildChunk(const World& w, const Terrain& t, int texSeed, int cx, int cy);

// Après un forage : redessine un pixel (coordonnées globales du masque) s'il appartient au bloc.
void shadePixel(Chunk& c, const Terrain& t, int sx, int sy);
// Après l'extraction d'un filon : recalcule les pixels de la case qui appartiennent au bloc.
void recolorTile(Chunk& c, const World& w, const Terrain& t, int texSeed, int tx, int ty);

// Couleur moyenne d'une case (éclats de forage) : tileColor() en JS. Renvoie 0xRRGGBB.
std::uint32_t tileColor(std::uint8_t t, int y);

}  // namespace filon
