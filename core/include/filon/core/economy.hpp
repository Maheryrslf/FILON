#pragma once
// Économie (fiche technique §7) : améliorations, talents, XP et niveaux, soute, lingots, fonderie en temps réel, vente.
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "filon/core/drill.hpp"
#include "filon/core/world.hpp"

namespace filon {

enum Upgrade : int { POWER, BLADE, SPEED, TANK, HULL, CARGO, LIGHT, RADAR, SMELT, ARM, UPGRADE_COUNT };
enum Talent : int { PROS, LUCK, TURBO, ECO, ARMOR, TALENT_COUNT };

struct UpgradeDef {
    const char* key;   // nom court (sauvegarde)
    const char* name;  // nom affiché
    const char* desc;
    double base, growth;
    int max;
};
extern const std::array<UpgradeDef, UPGRADE_COUNT> UPGRADES;

struct TalentDef {
    const char* key;
    const char* name;
    const char* desc;
};
extern const std::array<TalentDef, TALENT_COUNT> TALENTS;
constexpr int TALENT_MAX = 5;
constexpr double BAR_FACTOR = 2.5;            // un lingot vaut 2,5 fois le minerai brut
constexpr double FURNACE_CATCHUP_MAX = 8 * 3600;  // rattrapage maximal de la fonderie, en secondes

struct Furnace {
    std::vector<std::pair<std::uint8_t, int>> queue;  // minerais en attente : (id, nombre)
    double progress = 0;                              // secondes passées sur le minerai en cours
    std::int64_t last = 0;                            // dernière mise à jour, en millisecondes (horloge réelle)
};

struct Economy {
    double coins = 0;
    int up[UPGRADE_COUNT] = {};
    int tal[TALENT_COUNT] = {};
    double xp = 0;
    int lvl = 1;
    int cargo[ORE_COUNT] = {};
    int bars[ORE_COUNT] = {};
    Furnace furnace;
};

// Effets des améliorations et des talents
struct Effects {
    double dps, ram, spd, fuel, hull;
    int cargo;
    double light, radar, smelt, arm;
};
Effects effects(const Economy& e);
DrillStats drillStats(const Economy& e);
double oreValue(const Economy& e, int oreIndex);  // valeur d'un minerai brut
std::string effectText(Upgrade u, int level);     // « force ×1.9 », « 88 L », ...

int cost(const Economy& e, Upgrade u);
int xpNeed(int level);
int freePoints(const Economy& e);
int cargoCount(const Economy& e);
double cargoValue(const Economy& e);
double barsValue(const Economy& e);

int addXp(Economy& e, double xp);   // renvoie le nombre de niveaux gagnés
bool buy(Economy& e, Upgrade u);    // à la base seulement (vérifié par l'appelant)
bool addTalent(Economy& e, Talent t);
int sellAll(Economy& e);            // pièces gagnées (0 si rien à vendre)
void smeltAll(Economy& e);          // verse la soute dans la fonderie
void updateFurnace(Economy& e, std::int64_t nowMs);
double furnaceRemaining(const Economy& e);  // secondes avant la fin de la file

}  // namespace filon
