#pragma once
// Pépites détachées des filons : chacune a son centre de gravité, tombe, rebondit, roule le long des pentes,
// est aimantée par la foreuse et ramassée au contact (fiche technique §5.3).
#include <cstdint>
#include <vector>

#include "filon/core/terrain.hpp"

namespace filon {

struct Nugget {
    std::uint8_t id;  // type de minerai
    double x, y;      // centre, en cases
    double vx, vy;
};

struct NuggetUpdate {
    std::vector<std::uint8_t> collected;  // minerais ramassés pendant l'image, dans l'ordre
    bool cargoFull = false;               // une pépite touchait la foreuse mais la soute est pleine
};

// pcx, pcy : centre de la foreuse. cargoCount est augmenté à chaque ramassage, jusqu'à cargoMax.
NuggetUpdate updateNuggets(std::vector<Nugget>& list, const Terrain& t, double pcx, double pcy, int& cargoCount, int cargoMax, double dt);

}  // namespace filon
