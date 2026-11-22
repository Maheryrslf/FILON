#pragma once
// Créatures des couches, gardiens dans leur salle, éclats projetés (fiche technique §8).
// Le hasard passe par un Rng, appelé exactement dans le même ordre que Math.random dans la version JS.
#include <array>
#include <cstdint>
#include <vector>

#include "filon/core/drill.hpp"
#include "filon/core/random.hpp"
#include "filon/core/world.hpp"

namespace filon {

constexpr double FW = .7, FH = .6;  // boîte d'une créature, en cases
constexpr double BOSS_R = 1.25;     // rayon d'un gardien

struct FoeDef {
    const char* name;
    double hp, dmg, spd;
    std::uint32_t color;
    int coins, xp;
};
extern const std::array<FoeDef, NL> FOES;

struct BossDef {
    const char* name;
    double hp, dmg;
    std::uint32_t color;
    int shots, coins, xp;
};
extern const std::array<BossDef, NL> BOSSES;

struct Foe : Body {
    int k = 0, i = 0;  // couche, indice de création
    double hp = 0, flash = 0, wait = 0;
    int dir = 0;
    bool alive = true;
};

enum class BossState { Roam, Wind, Charge, Jump };
struct Boss {
    int L = 0;
    Chamber ch{};
    double x = 0, y = 0, hp = 0, max = 0;  // x, y : centre
    bool dead = false, active = false;
    BossState state = BossState::Roam;
    double t = 0, gx = 0, vx = 0, vy = 0, flash = 0;
};

struct Bullet {
    double x, y, vx, vy, life, dmg;
    int L;  // gardien qui l'a lancé (couleur)
};

struct Creatures {
    std::vector<Foe> foes;
    std::array<Boss, NL> bosses;
    std::vector<Bullet> bullets;
};

// Ce qui s'est passé ; le jeu en tire pièces, XP, sauvegarde, messages et effets
struct CreatureEvents {
    struct Hit {
        double x, y, dmg;
        std::uint32_t color;
    };
    std::vector<Hit> hits;
    std::vector<int> killedFoes;  // indices de création
    int bossWoke = -1, bossKilled = -1;
    bool bossLanded = false, shots = false;
};

// Comme boot() en JS : l'attente de chaque créature est tirée de rng dans l'ordre de création (mkFoe dans gen()) ;
// créatures tuées et gardiens vaincus d'après la sauvegarde.
Creatures makeCreatures(const World& w, const std::vector<int>& dead, const std::array<int, NL>& bossDone, Rng& rng);

// Coup de bélier dans l'axe (ux, uy) : créature proche d'abord, sinon gardien devant. Renvoie vrai s'il y a une cible ;
// frappe si d.ramCd <= 0 (dmg : effects().ram).
bool ram(Creatures& c, Drill& d, double ux, double uy, double dmg, CreatureEvents& ev);

// Une image : créatures, puis gardiens, puis éclats, comme updFoes, updBoss et updBullets en JS.
void updateCreatures(Creatures& c, Drill& d, const Terrain& t, const DrillStats& s, double dt, Rng& rng, CreatureEvents& ev, DrillEvents& dev);

// Après une panne sèche ou une destruction : gardiens réveillés remis en place, éclats effacés.
void resetActiveBosses(Creatures& c);

// Gardien vaincu : les deux rangées de sceau sous sa salle deviennent de la roche ordinaire.
void breakSeal(World& w, int layer);

}  // namespace filon
