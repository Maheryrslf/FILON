#include "filon/core/economy.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace filon {

const std::array<UpgradeDef, UPGRADE_COUNT> UPGRADES = {{
    {"power", "Foret", "Vitesse de forage", 30, 1.7, 12},
    {"blade", "Lames", "Dégâts aux créatures", 40, 1.7, 12},
    {"speed", "Moteur", "Vitesse de déplacement", 25, 1.6, 10},
    {"tank", "Réservoir", "Autonomie sous terre", 25, 1.65, 12},
    {"hull", "Coque", "Résistance aux coups et aux chutes", 35, 1.7, 12},
    {"cargo", "Soute", "Minerai transporté", 30, 1.65, 12},
    {"light", "Phare", "Portée de la lumière", 20, 1.8, 8},
    {"radar", "Radar", "Repère les minerais dans le noir", 60, 1.9, 8},
    {"smelt", "Fonderie", "Vitesse de fonte", 50, 1.8, 8},
    {"arm", "Bras", "Portée du bras d'accroche", 40, 1.7, 6},
}};

const std::array<TalentDef, TALENT_COUNT> TALENTS = {{
    {"pros", "Prospecteur", "+8 % de valeur des minerais et lingots"},
    {"luck", "Filon double", "+6 % de chance de minerai en double"},
    {"turbo", "Turbo", "+6 % de vitesse de forage"},
    {"eco", "Économe", "-7 % de consommation de carburant"},
    {"armor", "Blindage", "-7 % de dégâts subis"},
}};

namespace {
double jsRound(double x) {  // Math.round de JS
    const double f = std::floor(x);
    return x - f >= .5 ? f + 1 : f;
}
double statOf(Upgrade u, int l, int turbo) {
    switch (u) {
        case POWER: return 3 * std::pow(1.38, l) * (1 + .06 * turbo);
        case BLADE: return 12 * std::pow(1.42, l);
        case SPEED: return 4 + .5 * l;
        case TANK: return 60 + 28 * l;
        case HULL: return 50 + 35 * l;
        case CARGO: return 10 + 7 * l;
        case LIGHT: return 3.2 + .7 * l;
        case RADAR: return l ? 3 + 2 * l : 0;
        case SMELT: return 3 / (1 + .35 * l);
        case ARM: return 3 + .6 * l;
        default: return 0;
    }
}
}  // namespace

Effects effects(const Economy& e) {
    const int t = e.tal[TURBO];
    return {statOf(POWER, e.up[POWER], t), statOf(BLADE, e.up[BLADE], t), statOf(SPEED, e.up[SPEED], t), statOf(TANK, e.up[TANK], t),
            statOf(HULL, e.up[HULL], t),   static_cast<int>(statOf(CARGO, e.up[CARGO], t)), statOf(LIGHT, e.up[LIGHT], t),
            statOf(RADAR, e.up[RADAR], t), statOf(SMELT, e.up[SMELT], t), statOf(ARM, e.up[ARM], t)};
}

DrillStats drillStats(const Economy& e) {
    const Effects f = effects(e);
    DrillStats s;
    s.dps = f.dps;
    s.spd = f.spd;
    s.fuelMax = f.fuel;
    s.hullMax = f.hull;
    s.light = f.light;
    s.armReach = f.arm;
    s.eco = e.tal[ECO];
    s.armor = e.tal[ARMOR];
    return s;
}

double oreValue(const Economy& e, int i) { return ORES[static_cast<std::size_t>(i)].value * (1 + .08 * e.tal[PROS]); }

std::string effectText(Upgrade u, int l) {
    char buf[48];
    switch (u) {
        case POWER: std::snprintf(buf, sizeof buf, "force x%.1f", std::pow(1.38, l)); break;
        case BLADE: std::snprintf(buf, sizeof buf, "%d dégâts", static_cast<int>(jsRound(statOf(u, l, 0)))); break;
        case SPEED: std::snprintf(buf, sizeof buf, "%.1f cases/s", statOf(u, l, 0)); break;
        case TANK: std::snprintf(buf, sizeof buf, "%d L", static_cast<int>(statOf(u, l, 0))); break;
        case HULL: std::snprintf(buf, sizeof buf, "%d PV", static_cast<int>(statOf(u, l, 0))); break;
        case CARGO: std::snprintf(buf, sizeof buf, "%d places", static_cast<int>(statOf(u, l, 0))); break;
        case LIGHT: std::snprintf(buf, sizeof buf, "%.1f cases", statOf(u, l, 0)); break;
        case RADAR: l ? std::snprintf(buf, sizeof buf, "%d cases", static_cast<int>(statOf(u, l, 0))) : std::snprintf(buf, sizeof buf, "aucun"); break;
        case SMELT: std::snprintf(buf, sizeof buf, "%.1f s/minerai", statOf(u, l, 0)); break;
        case ARM: std::snprintf(buf, sizeof buf, "%.1f cases", statOf(u, l, 0)); break;
        default: buf[0] = 0;
    }
    return buf;
}

int cost(const Economy& e, Upgrade u) {
    const UpgradeDef& d = UPGRADES[static_cast<std::size_t>(u)];
    return static_cast<int>(jsRound(d.base * std::pow(d.growth, e.up[u])));
}

int xpNeed(int level) { return static_cast<int>(jsRound(25 * std::pow(1.3, level - 1))); }

int freePoints(const Economy& e) {
    int used = 0;
    for (int t : e.tal) used += t;
    return e.lvl - 1 - used;
}

int cargoCount(const Economy& e) {
    int n = 0;
    for (int c : e.cargo) n += c;
    return n;
}

double cargoValue(const Economy& e) {  // même ordre de somme que la version JS (minerais par id croissant)
    double v = 0;
    for (int i = 0; i < ORE_COUNT; ++i)
        if (e.cargo[i]) v += e.cargo[i] * oreValue(e, i);
    return v;
}

double barsValue(const Economy& e) {
    double v = 0;
    for (int i = 0; i < ORE_COUNT; ++i)
        if (e.bars[i]) v += e.bars[i] * oreValue(e, i) * BAR_FACTOR;
    return v;
}

int addXp(Economy& e, double xp) {
    e.xp += xp;
    int gained = 0;
    for (int need = xpNeed(e.lvl); e.xp >= need; need = xpNeed(e.lvl)) {
        e.xp -= need;
        ++e.lvl;
        ++gained;
    }
    return gained;
}

bool buy(Economy& e, Upgrade u) {
    const int c = cost(e, u);
    if (e.coins < c || e.up[u] >= UPGRADES[static_cast<std::size_t>(u)].max) return false;
    e.coins -= c;
    ++e.up[u];
    return true;
}

bool addTalent(Economy& e, Talent t) {
    if (freePoints(e) <= 0 || e.tal[t] >= TALENT_MAX) return false;
    ++e.tal[t];
    return true;
}

int sellAll(Economy& e) {
    const int v = static_cast<int>(jsRound(cargoValue(e) + barsValue(e)));
    if (!v) return 0;
    e.coins += v;
    std::fill(std::begin(e.cargo), std::end(e.cargo), 0);
    std::fill(std::begin(e.bars), std::end(e.bars), 0);
    return v;
}

void smeltAll(Economy& e) {
    for (int i = 0; i < ORE_COUNT; ++i)
        if (e.cargo[i] > 0) e.furnace.queue.emplace_back(static_cast<std::uint8_t>(ORE_FIRST + i), e.cargo[i]);
    std::fill(std::begin(e.cargo), std::end(e.cargo), 0);
}

void updateFurnace(Economy& e, std::int64_t nowMs) {  // horloge réelle : la fonderie avance aussi jeu fermé
    Furnace& f = e.furnace;
    double dt = std::clamp(static_cast<double>(nowMs - f.last) / 1000, 0.0, FURNACE_CATCHUP_MAX);
    f.last = nowMs;
    if (f.queue.empty()) {
        f.progress = 0;
        return;
    }
    const double per = effects(e).smelt;
    while (dt > 0 && !f.queue.empty()) {
        const double need = per - f.progress;
        if (dt >= need) {
            dt -= need;
            f.progress = 0;
            auto& it = f.queue.front();
            ++e.bars[it.first - ORE_FIRST];
            if (--it.second <= 0) f.queue.erase(f.queue.begin());
        } else {
            f.progress += dt;
            dt = 0;
        }
    }
}

double furnaceRemaining(const Economy& e) {
    int n = 0;
    for (const auto& it : e.furnace.queue) n += it.second;
    return std::max(0.0, n * effects(e).smelt - e.furnace.progress);
}

}  // namespace filon
