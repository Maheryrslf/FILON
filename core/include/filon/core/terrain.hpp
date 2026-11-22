#pragma once
// Terrain en pixels de terre (8 x 8 par case), base flottante et forage en disque (fiche technique §3 et §6).
#include <cstdint>
#include <vector>

#include "filon/core/world.hpp"

namespace filon {

constexpr int MW = W * 8;  // largeur du masque, en pixels de terre
constexpr int MH = H * 8;  // hauteur du masque

// Base flottante, en cases
constexpr double DECK = SURF - 3.5;  // dessus du pont
constexpr double DX1 = 5, DX2 = 21;  // étendue du pont
constexpr double GX1 = 12.4, GX2 = 13.6;  // trappe

struct DigResult {
    double budget;       // budget restant
    int removed;         // pixels retirés
    int left;            // pixels du disque encore en place
    std::uint8_t blocked;  // BED ou SEAL si le disque en touchait, sinon 0
    std::uint8_t t;      // type de la case du pixel le plus proche (0 si aucun)
    int tx, ty;          // case du pixel le plus proche
};

struct FreedOre {  // pépite détachée d'un filon : à créer par le jeu
    std::uint8_t id;
    double x, y;     // centre du pixel retiré, en cases
};

class Terrain {
public:
    // Masque construit d'après les cases, ou d'après les bits d'une sauvegarde (MW * MH / 8 octets, ignorés sinon),
    // puis le pont et les garde-corps ; trappe fermée
    explicit Terrain(World& world, const std::vector<std::uint8_t>& bits = {});
    std::vector<std::uint8_t> packMask() const;  // bits du masque, bit de poids faible d'abord (sauvegarde)

    bool cellSolid(int sx, int sy) const;
    bool cellAt(double x, double y) const { return cellSolid(fl8(x), fl8(y)); }
    bool boxHits(double x, double y, double w, double h) const;

    // Retire les pixels du disque, les plus proches du centre d'abord, tant que le budget le permet.
    DigResult dig(double cx, double cy, double rad, double budget);

    // Trappe : à fermer seulement si la foreuse n'est pas dans l'ouverture (à vérifier par l'appelant).
    void setHatch(bool open);
    bool hatchOpen() const { return hatch_; }

    const std::vector<std::uint8_t>& mask() const { return m_; }
    const std::vector<std::uint8_t>& remaining() const { return r_; }  // pixels pleins restants par case

    // Événements accumulés pendant le forage ; à lire puis vider par le jeu
    double xpGained = 0;
    std::vector<FreedOre> freed;
    std::vector<int> changedCells;  // indices sy * MW + sx retirés (à redessiner)
    std::vector<int> changedTiles;  // cases dont le filon a été extrait (à recolorer)

private:
    static int fl8(double v);
    void removeCell(int sx, int sy, int tx, int ty, std::uint8_t t);

    World& w_;
    std::vector<std::uint8_t> m_;  // 1 = pixel de terre plein
    std::vector<std::uint8_t> r_;
    bool hatch_ = false;
};

}  // namespace filon
