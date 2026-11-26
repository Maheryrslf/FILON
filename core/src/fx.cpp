#include "filon/core/fx.hpp"

#include <algorithm>
#include <cmath>

#include "filon/core/terrain.hpp"

namespace filon {

void Fx::burst(double x, double y, std::uint32_t color, int n, double sp) {
    for (int i = 0; i < n && parts_.size() < MAX; ++i) {
        const double a = rng_.next() * 6.283, s = rng_.next() * sp;
        parts_.push_back({x, y, std::cos(a) * s, std::sin(a) * s - 1, .4 + rng_.next() * .4, 6, 3, color, 1});
    }
}

void Fx::shake(double amplitude) {
    shakeA_ = std::max(shakeA_, amplitude);
    shakeT_ = .25;
}

void Fx::drill(const Drill& d) {
    if (d.sliding && rng_.next() < .35 && parts_.size() < MAX) {  // poussière soulevée par la glissade
        const double dir = (d.slope > 0 ? -1 : 1) * d.side;
        burst(d.x + PW / 2 - dir * .35, d.y + PH, 0xb59a80, 1, 1.5);
    }
    if (d.thrust && parts_.size() < MAX && rng_.next() < .7) {  // flammes du propulseur, sous la caisse
        const double vx = (rng_.next() - .5) * .6, vy = 2 + rng_.next() * 2, life = .2 + rng_.next() * .2;
        parts_.push_back({d.x + PW / 2 - d.side * .33, d.y + PH - .15, vx, vy, life, 0, 3, rng_.next() < .5 ? 0xffb13bu : 0xff6a2au, 1});
    }
}

void Fx::update(double dt, bool furnaceBusy) {
    for (Particle& p : parts_) {
        p.vy += p.g * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.life -= dt;
    }
    std::erase_if(parts_, [](const Particle& p) { return p.life <= 0; });
    if (furnaceBusy && rng_.next() < .15 && parts_.size() < MAX)  // fumée de la cheminée de la fonderie
        parts_.push_back({11.45 + rng_.next() * .3, DECK - 3.3, .2 + rng_.next() * .3, -.6, 2.2, 0, 6, 0xc8beb4, .35});
    if (shakeT_ > 0) shakeT_ -= dt;
    else shakeA_ = 0;
}

void Fx::shakeOffset(double& ox, double& oy) {
    ox = oy = 0;
    if (shakeT_ <= 0) return;
    ox = (rng_.next() - .5) * shakeA_ * TS;
    oy = (rng_.next() - .5) * shakeA_ * TS;
}

}  // namespace filon
