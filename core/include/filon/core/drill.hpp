#pragma once
// La foreuse : physique, tête de forage, pentes, forage, propulseur, trappe, carburant (fiche technique §4 et §6).
// Le bras d'accroche et les coups portés aux créatures viendront ensuite.
#include <cstdint>
#include <vector>

#include "filon/core/physics.hpp"

namespace filon {

constexpr double PW = .78, PH = .8;  // boîte de collision, en cases
constexpr double GRAVITY = 22, THRUST = 34;

struct DrillStats {  // valeurs tirées des améliorations et des talents
    double dps = 3;      // force du foret (PV retirés par seconde)
    double spd = 4;      // vitesse au sol, cases/s
    double fuelMax = 60;
    double hullMax = 50;
    double light = 3.2;  // rayon d'exploration, en cases
    double eco = 0;      // niveau du talent Économe
    double armor = 0;    // niveau du talent Blindage
};

struct DrillInput {
    int ix = 0, iy = 0;         // direction saisie, -1 / 0 / 1 sur chaque axe (y vers le bas)
    bool cursor = false;        // un curseur vise la tête de forage
    double cdx = 0, cdy = 0;    // vecteur centre de la foreuse -> curseur, en cases
    bool drill = false;         // mode forage
};

struct Drill : Body {
    int side = 1;                                   // sens de marche
    double ang = 0;                                 // axe réel de la tête, en radians (0 = droite, y vers le bas)
    double th = 0, thT = 0;                         // tête : angle par rapport à la caisse, et sa cible
    double yaw = 1, yawT = 1;                       // tête vers l'avant (1) ou l'arrière (-1)
    double tilt = 0, slope = 0;                     // inclinaison de la caisse, pente mesurée
    bool sliding = false, thrust = false, drilling = false, under = false;
    double digAcc = 0, digSnd = 0, hurtCd = 0, ramCd = 0;
    double fuel = 60, hull = 50;
    double air = 0, vis = 0, odo = 0, spin = 0, away = 0;
    int warned = -1, tx = 0, ty = 0;
};

// Ce qui s'est passé pendant une image ; le jeu en tire les sons, messages et effets.
struct DrillEvents {
    bool digSound = false;          // un coup de foret audible
    double digX = 0, digY = 0;      // position de la pointe
    std::uint8_t digTile = 0;       // type de la case touchée (pour la couleur des éclats)
    bool hardRock = false;          // roche trop dure pour le foret actuel
    bool bump = false, sealBump = false;
    bool hatchToggled = false;
    double landImpact = 0;          // vitesse d'atterrissage (> 6 : poussière)
    double damage = 0;              // dégâts subis
    bool fellHard = false;          // dégâts de chute
    bool strandedFuel = false, strandedHull = false;
    bool arrivedAtBase = false;
    int depth = 0;                  // profondeur, en cases sous la surface
};

bool onDeck(const Drill& d);
bool inDock(const Drill& d);
bool atBase(const Drill& d);  // le bras d'accroche ajoutera « accroché sous le pont »

void spawnOnDeck(Drill& d, const DrillStats& s);  // retour sur le pont de la base, plein fait

// Une image de simulation. explored : cases découvertes (W * H), mises à jour autour de la foreuse.
void updateDrill(Drill& d, Terrain& t, const World& w, std::vector<std::uint8_t>& explored, const DrillStats& s, const DrillInput& in,
                 double dt, DrillEvents& ev);

}  // namespace filon
