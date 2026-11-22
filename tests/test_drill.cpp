// Parité de la foreuse avec la version JS, image par image.
// Rejoue le parcours de tests/data/drill_1.txt (commandes scriptées) et compare l'état complet à chaque image.
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "filon/core/drill.hpp"

namespace {

double num(std::istringstream& in) {  // lecture exacte d'un nombre écrit par JS
    std::string s;
    in >> s;
    return std::strtod(s.c_str(), nullptr);
}

std::string fnv(const std::vector<std::uint8_t>& bytes) {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (std::uint8_t b : bytes) {
        h ^= b;
        h *= 0x100000001b3ULL;
    }
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016" PRIx64, h);
    return buf;
}

struct Seg {
    int frames;
    filon::DrillInput in;
};

}  // namespace

int main() {
    std::ifstream file(std::string(FILON_TEST_DATA_DIR) + "/drill_1.txt");
    if (!file) {
        std::printf("ÉCHEC : drill_1.txt introuvable\n");
        return 1;
    }
    filon::DrillStats stats;
    std::vector<Seg> segs;
    std::vector<std::string> frames, fnvs;
    for (std::string line; std::getline(file, line);) {
        std::istringstream in(line);
        std::string kind;
        in >> kind;
        if (kind == "stats") {
            stats.dps = num(in);
            stats.spd = num(in);
            stats.fuelMax = num(in);
            stats.hullMax = num(in);
            stats.light = num(in);
        } else if (kind == "seg") {
            Seg s{};
            s.frames = static_cast<int>(num(in));
            s.in.ix = static_cast<int>(num(in));
            s.in.iy = static_cast<int>(num(in));
            s.in.cursor = num(in) != 0;
            s.in.cdx = num(in);
            s.in.cdy = num(in);
            s.in.drill = num(in) != 0;
            segs.push_back(s);
        } else if (kind == "f") {
            frames.push_back(line);
        } else if (kind == "fnv") {
            fnvs.push_back(line);
        }
    }

    filon::World world = filon::generate(1);
    filon::Terrain terrain(world);
    std::vector<std::uint8_t> explored(static_cast<std::size_t>(filon::W * filon::H), 0);
    for (int x = 0; x < filon::W; ++x) explored[static_cast<std::size_t>(filon::SURF * filon::W + x)] = 1;
    filon::Drill d;
    d.x = 17.11;
    d.y = filon::SURF - filon::PH - 0.001;
    d.fuel = stats.fuelMax;
    d.hull = stats.hullMax;

    const double dt = 1.0 / 60;
    std::size_t fi = 0;
    double maxDiff = 0;
    for (const Seg& s : segs)
        for (int k = 0; k < s.frames; ++k) {
            filon::DrillEvents ev;
            filon::updateDrill(d, terrain, world, explored, stats, s.in, dt, ev);
            if (fi >= frames.size()) {
                std::printf("ÉCHEC : plus d'images simulées que de références\n");
                return 1;
            }
            std::istringstream in(frames[fi++]);
            std::string tag;
            in >> tag;
            const int f = static_cast<int>(num(in));
            const double ref[] = {num(in), num(in), num(in), num(in), num(in), num(in), num(in), num(in), num(in), num(in), num(in), num(in), num(in), num(in), num(in)};
            const double got[] = {d.x,      d.y,     d.vx,     d.vy,   static_cast<double>(d.grounded), static_cast<double>(d.side), d.ang, d.th,
                                  d.tilt,   d.yaw,   d.fuel,   d.hull, static_cast<double>(d.thrust),   static_cast<double>(d.sliding), d.digAcc};
            static const char* names[] = {"x", "y", "vx", "vy", "au sol", "sens", "angle", "tête", "inclinaison", "avant/arrière", "carburant", "coque",
                                          "propulseur", "glissade", "budget"};
            // sin, cos et atan2 de la bibliothèque C et du moteur JS peuvent différer sur le dernier bit :
            // tolérance de 1e-9, très en dessous de tout écart de logique (les booléens restent comparés exactement)
            for (int i = 0; i < 15; ++i) {
                if (std::fabs(got[i] - ref[i]) > maxDiff) maxDiff = std::fabs(got[i] - ref[i]);
                if (std::fabs(got[i] - ref[i]) > 1e-9 * (1 + std::fabs(ref[i]))) {
                    std::printf("ÉCHEC à l'image %d : %s vaut %.17g en C++, %.17g en JS\n", f, names[i], got[i], ref[i]);
                    return 1;
                }
            }
        }

    int failures = 0;
    for (const std::string& line : fnvs) {
        std::istringstream in(line);
        std::string tag, what, expect;
        in >> tag >> what >> expect;
        const std::string got = what == "mask" ? fnv(terrain.mask()) : fnv(explored);
        if (got != expect) {
            ++failures;
            std::printf("ÉCHEC : empreinte %s : C++ %s, JS %s\n", what.c_str(), got.c_str(), expect.c_str());
        }
    }
    if (failures == 0)
        std::printf("Parité de la foreuse : OK (%zu images, écart maximal %.1e ; arrivée en %.2f, %.2f ; terrain et exploration identiques au bit près)\n",
                    fi, maxDiff, d.x, d.y);
    return failures == 0 ? 0 : 1;
}
