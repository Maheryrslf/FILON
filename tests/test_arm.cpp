// Parité du bras d'accroche avec la version JS, image par image : prise en tombant, rappel, treuil,
// balancement, lâcher, puis visée hors de portée (bras qui sort et rentre pendant que le propulseur monte).
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "common.hpp"
#include "filon/core/drill.hpp"

int main() {
    std::ifstream file(std::string(FILON_TEST_DATA_DIR) + "/arm_1.txt");
    if (!file) {
        std::printf("ÉCHEC : arm_1.txt introuvable\n");
        return 1;
    }
    filon::DrillStats stats;
    filon::Drill d;
    struct Seg {
        int frames;
        filon::DrillInput in;
    };
    std::vector<Seg> segs;
    std::vector<std::string> frames;
    for (std::string line; std::getline(file, line);) {
        std::istringstream in(line);
        std::string kind;
        in >> kind;
        if (kind == "stats") {
            stats.dps = test::num(in);
            stats.spd = test::num(in);
            stats.fuelMax = test::num(in);
            stats.hullMax = test::num(in);
            stats.light = test::num(in);
            stats.armReach = test::num(in);
        } else if (kind == "start") {
            d.x = test::num(in);
            d.y = test::num(in);
        } else if (kind == "seg") {
            Seg s{};
            s.frames = static_cast<int>(test::num(in));
            s.in.ix = static_cast<int>(test::num(in));
            s.in.iy = static_cast<int>(test::num(in));
            s.in.cursor = test::num(in) != 0;
            s.in.cdx = test::num(in);
            s.in.cdy = test::num(in);
            s.in.grab = test::num(in) != 0;
            segs.push_back(s);
        } else if (kind == "f") {
            frames.push_back(line);
        }
    }
    d.fuel = stats.fuelMax;
    d.hull = stats.hullMax;

    filon::World world = filon::generate(1);
    filon::Terrain terrain(world);
    std::vector<std::uint8_t> explored(static_cast<std::size_t>(filon::W * filon::H), 0);
    const double dt = 1.0 / 60;
    std::size_t fi = 0;
    double maxDiff = 0;
    int hookedFrames = 0;
    static const char* names[] = {"x", "y", "vx", "vy", "au sol", "coque", "carburant", "propulseur", "état du bras", "longueur",
                                  "visée x", "visée y", "prise x", "prise y", "câble"};
    for (const Seg& s : segs)
        for (int k = 0; k < s.frames; ++k) {
            filon::DrillEvents ev;
            filon::updateDrill(d, terrain, world, explored, stats, s.in, dt, ev);
            std::istringstream in(frames.at(fi++));
            std::string tag;
            in >> tag;
            const int f = static_cast<int>(test::num(in));
            double ref[15];
            for (double& v : ref) v = test::num(in);
            const filon::Arm& a = d.arm;
            const double got[] = {d.x, d.y, d.vx, d.vy, static_cast<double>(d.grounded), d.hull, d.fuel, static_cast<double>(d.thrust),
                                  static_cast<double>(static_cast<int>(a.state)), a.len, a.ax, a.ay, a.hx, a.hy, a.rope};
            if (a.state == filon::ArmState::Hooked) ++hookedFrames;
            for (int i = 0; i < 15; ++i) {
                maxDiff = std::max(maxDiff, std::fabs(got[i] - ref[i]));
                if (!test::close(got[i], ref[i])) {
                    std::printf("ÉCHEC à l'image %d : %s vaut %.17g en C++, %.17g en JS\n", f, names[i], got[i], ref[i]);
                    return 1;
                }
            }
        }
    std::printf("Parité du bras : OK (%zu images dont %d accroché, coque finale %.1f, écart maximal %.1e)\n", fi, hookedFrames, d.hull, maxDiff);
    return 0;
}
