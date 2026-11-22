#include "filon/core/drill.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace filon {
namespace {

constexpr double PI = 3.141592653589793;
constexpr double D45 = PI / 4;          // débattement de la tête par rapport à la caisse
constexpr double TIP = PI * .28;        // inclinaison maximale de la caisse (50,4°)
constexpr double SLIDE = 50 * PI / 180;  // au-delà, les chenilles glissent

double sgn(double v) { return (v > 0) - (v < 0); }

// Pente réelle du sol sous les deux chenilles (positif = nez vers le haut) ; vide si une marche ou le vide
std::optional<double> groundSlope(const Drill& d, const Terrain& t) {
    if (!d.grounded) return std::nullopt;
    auto fl = [&](double x) -> std::optional<double> {
        for (int k = 0; k < 12; ++k) {
            const double y = d.y + PH - .2 + k / 8.0;
            if (t.cellAt(x, y)) return y;
        }
        return std::nullopt;
    };
    const double xa = d.x + .08, xm = d.x + PW / 2, xb = d.x + PW - .08;
    const auto a = fl(xa), m = fl(xm), b = fl(xb);
    if (!m) return std::nullopt;
    if (a && b) {
        if (std::fabs(*m - (*a + *b) / 2) > .16) return std::nullopt;  // milieu non aligné : une marche
        return std::atan2((*a - *b) * d.side, xb - xa);
    }
    if (a) return std::atan2((*a - *m) * d.side, xm - xa);  // pente raide : mesurée avec le milieu
    if (b) return std::atan2((*m - *b) * d.side, xb - xm);
    return std::nullopt;
}

void aimHead(Drill& d, const Terrain& t, double dt, std::optional<double> ta) {
    const auto sl = groundSlope(d, t);
    d.slope = sl.value_or(0);
    const double tTilt = sl ? std::clamp(*sl, -TIP, TIP) : 0;  // seule la pente réelle incline la caisse
    if (ta) {
        const double c = std::cos(*ta);
        if (std::fabs(c) > .05) d.yawT = sgn(c) * d.side > 0 ? 1 : -1;  // tête vers l'avant ou l'arrière
        const double pz = std::atan2(-std::sin(*ta), std::fabs(c));      // élévation voulue
        d.thT = std::clamp(pz - d.tilt * d.yawT, -D45, D45);
    }
    d.th += std::clamp(d.thT - d.th, -4 * dt, 4 * dt);
    d.yaw += std::clamp(d.yawT - d.yaw, -4 * dt, 4 * dt);
    d.tilt += std::clamp(tTilt - d.tilt, -3 * dt, 3 * dt);
    const double ys = d.yaw >= 0 ? 1 : -1, e = d.th + d.tilt * ys;
    d.ang = std::atan2(-std::sin(e), d.side * ys * std::cos(e));
    d.sliding = sl && std::fabs(*sl) > SLIDE;
}

void explore(const Drill& d, std::vector<std::uint8_t>& explored, double r) {
    const int R = static_cast<int>(std::ceil(r));
    const int x0 = static_cast<int>(std::floor(d.x + PW / 2)), y0 = static_cast<int>(std::floor(d.y + PH / 2));
    for (int dy = -R; dy <= R; ++dy) {
        const int y = y0 + dy;
        if (y < 0 || y >= H) continue;
        for (int dx = -R; dx <= R; ++dx) {
            const int x = x0 + dx;
            if (x >= 0 && x < W && dx * dx + dy * dy <= r * r) explored[static_cast<std::size_t>(y * W + x)] = 1;
        }
    }
}

bool boxInTile(const Drill& d, const World& w, std::uint8_t tile) {
    for (int y = static_cast<int>(std::floor(d.y)); y <= static_cast<int>(std::floor(d.y + PH)); ++y)
        for (int x = static_cast<int>(std::floor(d.x)); x <= static_cast<int>(std::floor(d.x + PW)); ++x)
            if (w.at(x, y) == tile) return true;
    return false;
}

void strand(Drill& d, const DrillStats& s, bool fuel, DrillEvents& ev) {
    (fuel ? ev.strandedFuel : ev.strandedHull) = true;  // le jeu retire la soute (moitié ou totalité)
    spawnOnDeck(d, s);
    d.arm = Arm{};
    d.sliding = false;
    d.tilt = 0;
    d.under = false;
}

void hurt(Drill& d, const DrillStats& s, double dmg, bool force, DrillEvents& ev) {
    if (d.hurtCd > 0 && !force) return;
    const double real = dmg * (1 - .07 * s.armor);
    d.hull -= real;
    d.hurtCd = .6;
    ev.damage += real;
    if (d.hull <= 0) strand(d, s, false, ev);
}

// ---------- Bras d'accroche ----------
void armBack(Arm& a, double ox, double oy) {  // le bras rentre ; s'il était planté, il part de sa prise
    if (a.state == ArmState::Hooked) {
        a.len = std::hypot(a.hx - ox, a.hy - oy);
        if (a.len == 0) a.len = .01;
        a.ax = (a.hx - ox) / a.len;
        a.ay = (a.hy - oy) / a.len;
    }
    a.state = ArmState::Back;
}

void updArm(Drill& d, const Terrain& t, const DrillStats& s, const DrillInput& in, double dt, DrillEvents& ev) {
    const double ox = d.x + PW / 2, oy = d.y + .15;  // épaule du bras, en haut de la caisse
    Arm& a = d.arm;
    if (!in.grab) {
        if (a.state != ArmState::None) {
            if (a.state != ArmState::Back) armBack(a, ox, oy);
            a.len -= 20 * dt;
            if (a.len <= 0) a = Arm{};
        }
        return;
    }
    if (a.state == ArmState::None) {  // visée : vers le curseur, sinon la direction saisie, sinon vers le haut
        double ax = 0, ay = -1;
        const double cd = std::hypot(in.cdx, in.cdy);
        if (in.cursor && cd > .3) {
            ax = in.cdx / cd;
            ay = in.cdy / cd;
        } else if (in.ix || in.iy) {
            const double dd = std::hypot(in.ix, in.iy);
            ax = in.ix / dd;
            ay = in.iy / dd;
        }
        a = Arm{ArmState::Out, 0, ax, ay, 0, 0, 0};
    }
    if (a.state == ArmState::Out) {
        a.len += 18 * dt;
        const double tx = ox + a.ax * a.len, ty = oy + a.ay * a.len;
        if (t.cellAt(tx, ty)) {  // prise : premier pixel de terre touché
            a.state = ArmState::Hooked;
            a.hx = tx;
            a.hy = ty;
            a.rope = std::hypot(d.x + PW / 2 - tx, d.y + PH / 2 - ty);
            ev.armHooked = true;
        } else if (a.len >= s.armReach) {
            a.state = ArmState::Back;
        }
    } else if (a.state == ArmState::Back) {
        a.len -= 20 * dt;
        if (a.len <= 0) a = Arm{};
    } else if (!t.cellAt(a.hx, a.hy)) {  // la prise a été creusée : le bras lâche
        armBack(a, ox, oy);
    } else {
        if (in.iy < 0) a.rope = std::max(.9, a.rope - 3 * dt);  // treuil
        else if (in.iy > 0 && !in.drill) a.rope = std::min(s.armReach + .5, a.rope + 3 * dt);  // rappel
        const double dx = d.x + PW / 2 - a.hx, dy = d.y + PH / 2 - a.hy;
        double dist = std::hypot(dx, dy);
        if (dist == 0) dist = 1;
        if (dist > a.rope) {  // pendule : on ramène la foreuse sur le cercle et on retire la vitesse qui l'en éloigne
            const double nx = a.hx + dx / dist * a.rope - PW / 2, ny = a.hy + dy / dist * a.rope - PH / 2;
            if (!t.boxHits(nx, ny, PW, PH)) {
                d.x = nx;
                d.y = ny;
            }
            const double ux = dx / dist, uy = dy / dist, vr = d.vx * ux + d.vy * uy;
            if (vr > 0) {
                d.vx -= vr * ux;
                d.vy -= vr * uy;
            }
            d.vx *= .995;
        }
    }
}

}  // namespace

bool onDeck(const Drill& d) { return d.grounded && std::fabs(d.y + PH - DECK) < .06 && d.x + PW > DX1 && d.x < DX2; }
bool inDock(const Drill& d) {
    const double cx = d.x + PW / 2;
    return cx > GX1 - .05 && cx < GX2 + .05 && d.y + PH / 2 > DECK - 1.6 && d.y < SURF;
}
bool hookedUnderBase(const Drill& d) {
    const Arm& a = d.arm;
    return a.state == ArmState::Hooked && a.hy > DECK - .1 && a.hy < DECK + .6 && a.hx > DX1 && a.hx < DX2;
}
bool atBase(const Drill& d) { return onDeck(d) || hookedUnderBase(d); }

void spawnOnDeck(Drill& d, const DrillStats& s) {
    d.x = 19.4;
    d.y = DECK - PH - .001;
    d.vx = d.vy = 0;
    d.fuel = s.fuelMax;
    d.hull = s.hullMax;
}

void updateDrill(Drill& d, Terrain& t, const World& w, std::vector<std::uint8_t>& explored, const DrillStats& s, const DrillInput& in,
                 double dt, DrillEvents& ev) {
    d.ramCd -= dt;
    d.hurtCd -= dt;
    const int ix = in.ix, iy = in.iy;
    if (ix && ix != d.side) {  // demi-tour : la tête garde son orientation dans le monde
        d.side = ix;
        d.yaw = -d.yaw;
        d.yawT = -d.yawT;
        d.tilt = -d.tilt;
    }
    bool busy = false;

    // tête de forage : vers le curseur, sinon la direction saisie
    std::optional<double> ta;
    if (in.cursor && std::hypot(in.cdx, in.cdy) > .3) ta = std::atan2(in.cdy, in.cdx);
    else if (ix || iy) ta = std::atan2(iy, ix);
    aimHead(d, t, dt, ta);

    const double ux = std::cos(d.ang), uy = std::sin(d.ang);
    d.drilling = false;
    if (in.drill) {  // le foret creuse un disque dans l'axe de la tête
        const double tx = d.x + PW / 2 + ux * .45, ty = d.y + PH / 2 + uy * .45 - (std::fabs(uy) < .38 ? .05 : 0);
        d.digAcc = std::min(d.digAcc + s.dps * dt, 50.0);
        DigResult r = t.dig(tx, ty, .47, d.digAcc);
        if (std::fabs(ux) > .38 && std::fabs(uy) > .38) {  // en biais : on creuse aussi le côté, d'où une galerie en pente
            const DigResult r2 = t.dig(d.x + PW / 2 + sgn(ux) * .45, d.y + PH / 2 - .05, .47, r.budget);
            const bool first = r.t != 0;
            r = DigResult{r2.budget, r.removed + r2.removed, r.left + r2.left, r2.blocked, first ? r.t : r2.t, first ? r.tx : r2.tx, first ? r.ty : r2.ty};
        }
        d.digAcc = r.left ? r.budget : 0;
        if (r.removed || r.left) {
            busy = true;
            d.drilling = true;
            if ((d.digSnd -= dt) <= 0) {
                d.digSnd = .09;
                ev.digSound = true;
                ev.digX = tx;
                ev.digY = ty;
                ev.digTile = r.t;
            }
            const int k = r.ty * W + r.tx;
            if (tileHP(r.t, r.ty) / s.dps > 7 && d.warned != k) {
                d.warned = k;
                ev.hardRock = true;
            }
        } else if (r.blocked) {
            ev.bump = true;
            ev.sealBump = r.blocked == SEAL;
        }
    }

    {  // trappe : s'ouvre pour laisser monter la foreuse, ou quand on vise vers le bas en étant dessus ; jamais fermée sur elle
        const double cx = d.x + PW / 2;
        const bool overHatch = cx > GX1 && cx < GX2;
        const bool open = (d.y + PH > DECK + .02 && inDock(d)) || (onDeck(d) && overHatch && iy > 0);
        const bool inOpening = d.x < GX2 && d.x + PW > GX1 && d.y < DECK + .5 && d.y + PH > DECK;
        if (open != t.hatchOpen() && (open || !inOpening)) {
            t.setHatch(open);
            ev.hatchToggled = true;
        }
    }

    // déplacement : roule au sol et monte les pentes douces, contrôle réduit en l'air, propulseur
    const double tv = ix * s.spd * (d.grounded ? 1 : .8);
    const double acc = (d.grounded ? (d.sliding ? 8 : 32) : ix ? 16 : 0) * dt;  // en l'air sans commande : pas de freinage
    d.vx += std::clamp(tv - d.vx, -acc, acc);
    if (d.sliding) {  // glissade vers le bas de la pente, sans dégâts
        const double dir = (d.slope > 0 ? -1 : 1) * d.side;
        d.vx = std::clamp(d.vx + dir * GRAVITY * std::sin(std::fabs(d.slope)) * .8 * dt, -7.0, 7.0);
    }
    // dans la mine ou dans la colonne d'accostage ; bras accroché : le haut actionne le treuil, pas le propulseur
    d.thrust = iy < 0 && d.fuel > 0 && d.arm.state != ArmState::Hooked && (d.y + PH > SURF + .25 || inDock(d));
    if (d.thrust && inDock(d) && d.y + PH > DECK) {  // se centre sous la trappe en montant
        const double gx = (GX1 + GX2) / 2 - PW / 2;
        d.x += std::clamp(gx - d.x, -3 * dt, 3 * dt);
    }
    d.vy = std::clamp(d.vy + (GRAVITY - (d.thrust ? THRUST : 0)) * dt, -6.0, 14.0);
    if (d.y + d.vy * dt < DECK - 2.5) d.vy = std::max(0.0, d.vy);
    const double vy0 = d.vy;
    const MoveResult mv = moveBox(t, d, PW, PH, d.vx * dt, d.vy * dt);
    if (mv.stepped > 0) d.vis = std::min(.5, d.vis + mv.stepped);
    if (mv.hx) d.vx = 0;
    double impact = 0;
    if (mv.hy) {
        if (vy0 > 0) impact = vy0;
        d.vy = 0;
    }
    updArm(d, t, s, in, dt, ev);
    if (d.arm.state == ArmState::Hooked) impact *= .4;  // le bras accroché amortit la chute

    // après le déplacement
    d.vis *= std::exp(-dt * 14);
    const double air = d.air;
    d.grounded = t.boxHits(d.x, d.y + .03, PW, PH);
    d.air = d.grounded ? 0 : air + dt;
    if (impact > 6) {
        ev.landImpact = impact;
        if (impact > 12.5 && air > .3) {  // vraie chute seulement
            hurt(d, s, (impact - 12.5) * 8, true, ev);
            ev.fellHard = true;
        }
    }
    d.odo += std::fabs(d.vx) * dt;
    d.tx = static_cast<int>(std::floor(d.x + PW / 2));
    d.ty = static_cast<int>(std::floor(d.y + PH / 2));
    const bool inMine = d.y + PH > SURF + .05;
    d.under = inMine;
    if (atBase(d)) {
        d.fuel = s.fuelMax;
        d.hull = s.hullMax;
        if (d.away > 3) ev.arrivedAtBase = true;
        d.away = 0;
    } else {
        d.away += dt;
        d.fuel -= (d.thrust ? 2.2 : inMine && (busy || std::fabs(d.vx) > .3) ? 1 : 0) * (1 - .07 * s.eco) * dt;
        if (d.fuel <= 0) {
            strand(d, s, true, ev);
            return;
        }
    }
    ev.depth = static_cast<int>(std::floor(d.y + PH)) - SURF + 1;
    if (boxInTile(d, w, LAVA)) hurt(d, s, 20, false, ev);
    d.spin += dt * (busy ? 14 : 2);
    explore(d, explored, s.light);
}

}  // namespace filon
