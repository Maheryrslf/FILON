#include "filon/core/physics.hpp"

#include <algorithm>
#include <cmath>

namespace filon {

MoveResult moveBox(const Terrain& t, Body& b, double w, double h, double dx, double dy) {
    MoveResult r;
    const int n = std::max(1, static_cast<int>(std::ceil(std::max(std::fabs(dx), std::fabs(dy)) / .06)));
    const double sx = dx / n, sy = dy / n;
    for (int i = 0; i < n; ++i) {
        if (sx != 0 && !r.hx) {
            if (!t.boxHits(b.x + sx, b.y, w, h)) {
                b.x += sx;
            } else {
                bool climbed = false;
                if (b.grounded)
                    for (int k = 1; k <= 3; ++k)
                        if (!t.boxHits(b.x + sx, b.y - k / 8.0, w, h)) {  // pente : monte jusqu'à 3 pixels
                            b.x += sx;
                            b.y -= k / 8.0;
                            r.stepped += k / 8.0;
                            climbed = true;
                            break;
                        }
                if (!climbed) r.hx = true;
            }
        }
        if (sy != 0 && !r.hy) {
            if (!t.boxHits(b.x, b.y + sy, w, h)) b.y += sy;
            else r.hy = true;
        }
    }
    return r;
}

}  // namespace filon
