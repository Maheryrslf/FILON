#pragma once
// Monde : dimensions, types de case, minerais, couches et génération (fiche technique §2).
#include <array>
#include <cstdint>
#include <vector>

namespace filon {

constexpr int TS = 32;  // taille d'une case en pixels du monde (avant mise à l'échelle)

// Dimensions, en cases
constexpr int W = 36;                 // largeur
constexpr int SURF = 6;               // première rangée de terre (0..5 = ciel)
constexpr int LH = 90;                // hauteur d'une couche
constexpr int NL = 4;                 // nombre de couches
constexpr int H = SURF + NL * LH + 1; // hauteur totale (dernière rangée = socle)

// Types de case ; les minerais vont de ORE_FIRST à ORE_FIRST + ORE_COUNT - 1
enum Tile : std::uint8_t { AIR = 0, SOFT = 1, ROCK = 2, HARD = 3, BED = 4, SEAL = 5, LAVA = 6 };
constexpr std::uint8_t ORE_FIRST = 10;
constexpr int ORE_COUNT = 9;

struct LayerDef {
    const char* name;
    double mult;  // multiplicateur de dureté
};
extern const std::array<LayerDef, NL> LAYERS;

struct OreDef {
    const char* name;
    std::uint32_t color, glint;  // 0xRRGGBB
    int value;
    double hp;
};
extern const std::array<OreDef, ORE_COUNT> ORES;

constexpr bool isOre(std::uint8_t t) { return t >= ORE_FIRST && t < ORE_FIRST + ORE_COUNT; }
constexpr bool solid(std::uint8_t t) { return t != AIR && t != LAVA; }
constexpr int layerOf(int y) {
    return y < SURF ? -1 : ((y - SURF) / LH < NL - 1 ? (y - SURF) / LH : NL - 1);
}
double tileHP(std::uint8_t t, int y);  // points de vie d'une case (infini si indestructible)

struct Chamber {
    int x1, x2, y1, y2;  // salle du gardien de la couche, bornes incluses
};
Chamber chamber(int layer);

struct FoeSpawn {
    int kind;   // = couche de la créature
    int index;  // ordre de création (sert à la sauvegarde des créatures tuées)
    int tx, ty; // case de départ, posée au sol
};

struct World {
    std::uint32_t seed = 0;
    std::vector<std::uint8_t> tiles;     // W * H, rangée par rangée ; évolue (filon extrait, sceau brisé)
    std::vector<std::uint8_t> original;  // types d'origine, pour les textures
    std::vector<FoeSpawn> foes;
    std::uint8_t at(int x, int y) const {
        return (x < 0 || x >= W || y < 0 || y >= H) ? std::uint8_t{BED} : tiles[static_cast<std::size_t>(y * W + x)];
    }
};

// Même graine => même monde que gen() dans la version JS, octet par octet.
World generate(std::uint32_t seed);

}  // namespace filon
