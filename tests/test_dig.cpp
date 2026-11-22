// Parité du terrain en pixels et du forage avec la version JS.
// Rejoue tests/data/dig_1.txt (produit par tools/export_reference.js) et compare chaque résultat.
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "filon/core/terrain.hpp"

static int failures = 0;

static void fail(const std::string& msg) {
    if (++failures <= 10) std::printf("ÉCHEC : %s\n", msg.c_str());
}

static std::string fnv(const std::vector<std::uint8_t>& bytes) {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (std::uint8_t b : bytes) {
        h ^= b;
        h *= 0x100000001b3ULL;
    }
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016" PRIx64, h);
    return buf;
}

static double num(std::istringstream& in) {  // lecture exacte d'un nombre écrit par JS
    std::string s;
    in >> s;
    return std::strtod(s.c_str(), nullptr);
}

int main() {
    filon::World world = filon::generate(1);
    filon::Terrain terrain(world);
    std::ifstream file(std::string(FILON_TEST_DATA_DIR) + "/dig_1.txt");
    if (!file) {
        std::printf("ÉCHEC : dig_1.txt introuvable\n");
        return 1;
    }

    std::string line;
    int digs = 0;
    std::size_t freedIdx = 0;
    while (std::getline(file, line)) {
        std::istringstream in(line);
        std::string kind;
        in >> kind;
        if (kind == "dig") {
            const double cx = num(in), cy = num(in), r = num(in), b = num(in);
            const int removed = static_cast<int>(num(in)), left = static_cast<int>(num(in));
            const double budget = num(in);
            const int blocked = static_cast<int>(num(in)), t = static_cast<int>(num(in)), tx = static_cast<int>(num(in)),
                      ty = static_cast<int>(num(in));
            const filon::DigResult res = terrain.dig(cx, cy, r, b);
            ++digs;
            if (res.removed != removed || res.left != left || res.budget != budget || res.blocked != blocked || res.t != t || res.tx != tx ||
                res.ty != ty) {
                char buf[256];
                std::snprintf(buf, sizeof buf, "coup n°%d en (%g, %g) : C++ %d/%d/%.17g, JS %d/%d/%.17g", digs, cx, cy, res.removed, res.left,
                              res.budget, removed, left, budget);
                fail(buf);
            }
        } else if (kind == "xp") {
            const double xp = num(in);
            if (terrain.xpGained != xp) fail("XP : C++ " + std::to_string(terrain.xpGained) + ", JS " + std::to_string(xp));
        } else if (kind == "freed") {
            const int id = static_cast<int>(num(in));
            const double x = num(in), y = num(in);
            if (freedIdx >= terrain.freed.size() || terrain.freed[freedIdx].id != id || terrain.freed[freedIdx].x != x ||
                terrain.freed[freedIdx].y != y)
                fail("pépite n°" + std::to_string(freedIdx) + " différente");
            ++freedIdx;
        } else if (kind == "fnv") {
            std::string what, expect;
            in >> what >> expect;
            if (what == "hatch_open") terrain.setHatch(true);
            if (what == "hatch_closed") terrain.setHatch(false);
            const std::string got = what == "remaining" ? fnv(terrain.remaining()) : what == "tiles" ? fnv(world.tiles) : fnv(terrain.mask());
            if (got != expect) fail("empreinte " + what + " : C++ " + got + ", JS " + expect);
        }
    }
    if (freedIdx != terrain.freed.size()) fail("nombre de pépites : C++ " + std::to_string(terrain.freed.size()) + ", JS " + std::to_string(freedIdx));

    if (failures == 0)
        std::printf("Parité du forage : OK (%d coups de foret, %zu pépites, XP %.5f, masque et trappe identiques)\n", digs, freedIdx,
                    terrain.xpGained);
    return failures == 0 ? 0 : 1;
}
