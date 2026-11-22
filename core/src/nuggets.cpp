#include "filon/core/nuggets.hpp"

#include <algorithm>
#include <cmath>

#include "filon/core/drill.hpp"

namespace filon {

NuggetUpdate updateNuggets(std::vector<Nugget>& list, const Terrain& t, double pcx, double pcy, int& cargoCount, int cargoMax, double dt) {
    constexpr double r = .15;  // rayon d'une pépite
    NuggetUpdate res;
    std::vector<Nugget> keep;
    keep.reserve(list.size());
    for (Nugget n : list) {
        const double dx = pcx - n.x, dy = pcy - n.y;
        double d = std::hypot(dx, dy);
        if (d == 0) d = 1;
        if (d < .6) {  // au contact : ramassée s'il reste de la place
            if (cargoCount < cargoMax) {
                res.collected.push_back(n.id);
                ++cargoCount;
                continue;
            }
            res.cargoFull = true;
        }
        if (d < 1.6 && cargoCount < cargoMax) {  // aimant de la foreuse
            n.vx += dx / d * 14 * dt;
            n.vy += dy / d * 14 * dt;
        }
        n.vy = std::min(12.0, n.vy + GRAVITY * dt);
        n.vx *= .985;
        double nx = n.x + n.vx * dt;
        if (t.cellAt(nx + ((n.vx > 0) - (n.vx < 0)) * r, n.y)) {
            n.vx *= -.35;
            nx = n.x;
        }
        n.x = nx;
        double ny = n.y + n.vy * dt;
        if (t.cellAt(n.x, ny + (n.vy > 0 ? r : -r))) {  // rebond, amorti
            if (n.vy > 0) n.vx *= .6;
            n.vy *= -.3;
            if (std::fabs(n.vy) < .8) n.vy = 0;
            ny = n.y;
        }
        n.y = ny;
        if (n.vy == 0 && t.cellAt(n.x, n.y + r + .05)) {  // posée : roule le long des pentes
            if (!t.cellAt(n.x - .2, n.y + r + .05)) n.vx -= 5 * dt;
            else if (!t.cellAt(n.x + .2, n.y + r + .05)) n.vx += 5 * dt;
        }
        keep.push_back(n);
    }
    list.swap(keep);
    return res;
}

}  // namespace filon
