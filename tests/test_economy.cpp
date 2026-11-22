// Parité de l'économie avec la version JS : coûts, effets, XP, achats, fonderie (horloge simulée), vente.
#include <cstdio>
#include <fstream>
#include <string>

#include "common.hpp"
#include "filon/core/economy.hpp"

namespace {
int failures = 0;
void fail(const std::string& m) {
    if (++failures <= 15) std::printf("ÉCHEC : %s\n", m.c_str());
}
filon::Upgrade upgradeOf(const std::string& key) {
    for (int i = 0; i < filon::UPGRADE_COUNT; ++i)
        if (key == filon::UPGRADES[static_cast<std::size_t>(i)].key) return static_cast<filon::Upgrade>(i);
    fail("amélioration inconnue " + key);
    return filon::POWER;
}
}  // namespace

int main() {
    std::ifstream file(std::string(FILON_TEST_DATA_DIR) + "/economy.txt");
    if (!file) {
        std::printf("ÉCHEC : economy.txt introuvable\n");
        return 1;
    }
    filon::Economy cfg;  // configuration courante pour les lignes « stat »
    filon::Economy e;    // scénario à la base
    std::int64_t now = 1700000000000LL;
    e.furnace.last = now;
    int costs = 0;
    for (std::string line; std::getline(file, line);) {
        std::istringstream in(line);
        std::string kind;
        in >> kind;
        if (kind == "cost") {
            std::string key;
            int lvl, ref;
            in >> key >> lvl >> ref;
            filon::Economy t;
            const filon::Upgrade u = upgradeOf(key);
            t.up[u] = lvl;
            ++costs;
            if (filon::cost(t, u) != ref) fail(key + " niveau " + std::to_string(lvl) + " : C++ " + std::to_string(filon::cost(t, u)) + ", JS " + std::to_string(ref));
        } else if (kind == "config") {
            int id;
            in >> id;
            cfg = filon::Economy{};
            for (int& u : cfg.up) in >> u;
            for (int& t : cfg.tal) in >> t;
        } else if (kind == "stat") {
            int id;
            in >> id;
            const filon::Effects f = filon::effects(cfg);
            const double got[] = {f.dps, f.ram, f.spd, f.fuel, f.hull, static_cast<double>(f.cargo), f.light, f.radar, f.smelt, f.arm};
            for (int i = 0; i < 10; ++i)
                if (const double r = test::num(in); !test::close(got[i], r)) fail("config " + std::to_string(id) + " effet " + std::to_string(i));
            for (int i = 0; i < filon::ORE_COUNT; ++i)
                if (const double r = test::num(in); !test::close(filon::oreValue(cfg, i), r)) fail("config " + std::to_string(id) + " valeur " + std::to_string(i));
        } else if (kind == "xpneed") {
            int l, ref;
            in >> l >> ref;
            if (filon::xpNeed(l) != ref) fail("xpNeed(" + std::to_string(l) + ")");
        } else if (kind == "addxp") {
            const double xp = test::num(in);
            filon::addXp(e, xp);
            const int lvl = static_cast<int>(test::num(in));
            const double rest = test::num(in);
            const int pts = static_cast<int>(test::num(in));
            if (e.lvl != lvl || e.xp != rest || filon::freePoints(e) != pts) fail("XP : niveau " + std::to_string(e.lvl) + ", attendu " + std::to_string(lvl));
            e.coins = 5000;
        } else if (kind == "buy") {
            std::string key;
            in >> key;
            filon::buy(e, upgradeOf(key));
            const double coins = test::num(in);
            const int lvl = static_cast<int>(test::num(in));
            if (e.coins != coins || e.up[upgradeOf(key)] != lvl) fail("achat " + key + " : " + std::to_string(e.coins) + " pièces");
        } else if (kind == "smelt") {
            e.cargo[0] = 3;  // { 10: 3, 11: 2, 14: 5 }
            e.cargo[1] = 2;
            e.cargo[4] = 5;
            filon::smeltAll(e);
        } else if (kind == "furnace") {
            std::string queue;
            std::int64_t step;
            in >> step >> queue;
            now += step;
            filon::updateFurnace(e, now);
            std::string got;
            for (const auto& [id, n] : e.furnace.queue) got += (got.empty() ? "" : ",") + std::to_string(id) + ":" + std::to_string(n);
            if (got.empty()) got = "-";
            const double p = test::num(in);
            if (got != queue || !test::close(e.furnace.progress, p)) fail("fonderie après " + std::to_string(step) + " ms : " + got + " / " + queue);
            for (int i = 0; i < filon::ORE_COUNT; ++i)
                if (static_cast<int>(test::num(in)) != e.bars[i]) fail("lingots de " + std::string(filon::ORES[static_cast<std::size_t>(i)].name));
        } else if (kind == "sell") {
            e.cargo[2] = 4;  // { 12: 4, 16: 1 }
            e.cargo[6] = 1;
            const int gained = filon::sellAll(e);
            const int refGain = static_cast<int>(test::num(in));
            const double total = test::num(in);
            if (gained != refGain || e.coins != total) fail("vente : " + std::to_string(gained) + " pièces, attendu " + std::to_string(refGain));
        }
    }
    if (failures == 0)
        std::printf("Parité de l'économie : OK (%d coûts, 3 configurations d'effets, 60 niveaux d'XP, achats, fonderie et vente identiques)\n", costs);
    return failures == 0 ? 0 : 1;
}
