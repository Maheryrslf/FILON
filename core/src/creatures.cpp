#include "filon/core/creatures.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace filon {

const std::array<FoeDef, NL> FOES = {{
    {"Larve", 24, 6, 1.6, 0xe8d9a8, 6, 4},
    {"Scorpide", 90, 12, 2.2, 0xe07a4a, 25, 12},
    {"Golem", 320, 24, 1.2, 0x8a93a8, 90, 35},
    {"Salamandre", 900, 38, 2.8, 0xff7a2a, 260, 90},
}};

const std::array<BossDef, NL> BOSSES = {{
    {"Mâchoire-Racine", 500, 14, 0xc9a86a, 0, 400, 60},
    {"Reine Scorpide", 2200, 24, 0xe0563a, 6, 2500, 200},
    {"Colosse de Basalte", 8000, 38, 0x6f7a94, 8, 9000, 600},
    {"Cœur de Magma", 26000, 55, 0xff5a1f, 12, 30000, 1500},
}};

namespace {

constexpr double PI = std::numbers::pi;
constexpr int OCT[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};  // E, SE, S, SO, O, NO, N, NE

double jsRound(double x) {  // Math.round
    const double f = std::floor(x);
    return x - f >= .5 ? f + 1 : f;
}
double jsSign(double x) { return x > 0 ? 1 : x < 0 ? -1 : 0; }
double dotN(double ax, double ay, double bx, double by) {
    const double l = std::hypot(ax, ay) * std::hypot(bx, by);
    return (ax * bx + ay * by) / (l ? l : 1);
}
bool inCh(const Chamber& c, int x, int y) { return x >= c.x1 && x <= c.x2 && y >= c.y1 && y <= c.y2; }

struct Bounds {
    double mnx, mxx, floor, ceil;
};
Bounds bounds(const Boss& b) { return {b.ch.x1 + BOSS_R, b.ch.x2 + 1 - BOSS_R, b.ch.y2 + 1 - BOSS_R, b.ch.y1 + BOSS_R}; }
void newGoal(Boss& b, Rng& rng) {
    const Bounds k = bounds(b);
    b.gx = k.mnx + rng.next() * (k.mxx - k.mnx);
}

void resetBoss(Creatures& c, Boss& b) {
    b.active = false;
    b.hp = b.max;
    b.x = (b.ch.x1 + b.ch.x2 + 1) / 2.0;
    b.y = b.ch.y2 + 1 - BOSS_R;
    b.vx = b.vy = 0;
    b.state = BossState::Roam;
    c.bullets.clear();
}

void hitFoe(Foe& f, Drill& d, double dmg, CreatureEvents& ev) {
    f.hp -= dmg;
    f.flash = .15;
    d.ramCd = .32;
    ev.hits.push_back({f.x + FW / 2, f.y, dmg, f.x + FW / 2, f.y + FH / 2, FOES[static_cast<std::size_t>(f.k)].color, f.hp <= 0, false});
    if (f.hp <= 0) {
        f.alive = false;
        ev.killedFoes.push_back(f.i);
    }
}

void hitBoss(Creatures& c, Boss& b, Drill& d, double dmg, CreatureEvents& ev) {
    b.hp -= dmg;
    b.flash = .12;
    d.ramCd = .3;
    ev.hits.push_back({b.x, b.y - 1.3, dmg, b.x, b.y, BOSSES[static_cast<std::size_t>(b.L)].color, b.hp <= 0, true});
    if (b.hp <= 0) {
        b.dead = true;
        b.active = false;
        c.bullets.clear();
        ev.bossKilled = b.L;
    }
}

void updateFoes(Creatures& c, Drill& d, const Terrain& t, const DrillStats& s, double dt, Rng& rng, DrillEvents& dev) {
    const double pcx = d.x + PW / 2, pcy = d.y + PH / 2;
    for (Foe& f : c.foes) {
        if (!f.alive) continue;
        const double fcx = f.x + FW / 2, fcy = f.y + FH / 2, dist = std::hypot(fcx - pcx, fcy - pcy);  // centre de gravité
        if (dist > 16) continue;
        const FoeDef& F = FOES[static_cast<std::size_t>(f.k)];
        f.flash = std::max(0.0, f.flash - dt);
        if ((f.wait -= dt) <= 0) {
            f.wait = .3 + rng.next() * .6;
            if (dist < 8) {  // point cardinal vers la foreuse ; N, NE, NO : saut
                const int o = (static_cast<int>(jsRound(std::atan2(pcy - fcy, pcx - fcx) / (PI / 4))) + 8) % 8;
                f.dir = OCT[o][0];
                if (OCT[o][1] < 0 && f.grounded) f.vy = -7.5;
            } else {
                const int o = static_cast<int>(std::floor(rng.next() * 8));
                f.dir = rng.next() < .4 ? 0 : OCT[o][0];
            }
        }
        f.vx = f.dir * F.spd;
        f.vy = std::min(14.0, f.vy + GRAVITY * dt);
        const MoveResult m = moveBox(t, f, FW, FH, f.vx * dt, f.vy * dt);
        if (m.hy) f.vy = 0;
        f.grounded = t.boxHits(f.x, f.y + .03, FW, FH);
        if (m.hx && f.grounded && rng.next() < .03) f.vy = -7.5;  // obstacle : tente de sauter par-dessus
        if (dist < .75 && hurt(d, s, F.dmg, false, dev)) resetActiveBosses(c);
    }
}

void updateBosses(Creatures& c, Drill& d, const DrillStats& s, double dt, Rng& rng, CreatureEvents& ev, DrillEvents& dev) {
    const double pcx = d.x + PW / 2, pcy = d.y + PH / 2;
    for (Boss& b : c.bosses) {
        if (b.dead) continue;
        const BossDef& B = BOSSES[static_cast<std::size_t>(b.L)];
        if (!b.active) {
            if (!inCh(b.ch, d.tx, d.ty)) continue;
            b.active = true;
            b.state = BossState::Roam;
            b.t = 1.2;
            newGoal(b, rng);
            ev.bossWoke = b.L;
        }
        const double dist = std::hypot(b.x - pcx, b.y - pcy);
        if (!inCh(b.ch, d.tx, d.ty) && dist > 10) {
            resetBoss(c, b);
            continue;
        }
        b.t -= dt;
        b.flash = std::max(0.0, b.flash - dt);
        const Bounds k = bounds(b);
        const double side = pcx - b.x < 0 ? -1 : 1;  // Math.sign(...) || 1
        if (b.state == BossState::Roam) {
            const double dx = b.gx - b.x;
            if (std::fabs(dx) < .25) newGoal(b, rng);
            b.vx = jsSign(dx) * (2 + b.L * .3);
            if (b.t <= 0) {
                b.state = BossState::Wind;
                b.t = .6;
                b.vx = 0;
            }
        } else if (b.state == BossState::Wind) {
            b.vx = 0;
            if (b.t <= 0) {
                if (pcy < b.y - 1.5 || rng.next() < .3) {
                    b.vy = -(10 + b.L);
                    b.vx = side * (3 + b.L * .5);
                    b.state = BossState::Jump;
                } else {
                    b.vx = side * (8 + b.L);
                    b.state = BossState::Charge;
                    b.t = .9;
                }
            }
        }
        b.vy += GRAVITY * dt;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        bool wall = false, landed = false;
        if (b.x < k.mnx) {
            b.x = k.mnx;
            wall = true;
        }
        if (b.x > k.mxx) {
            b.x = k.mxx;
            wall = true;
        }
        if (b.y < k.ceil) {
            b.y = k.ceil;
            b.vy = std::max(0.0, b.vy);
        }
        if (b.y >= k.floor) {
            landed = b.vy > 4;
            b.y = k.floor;
            b.vy = 0;
        }
        if ((b.state == BossState::Charge && (wall || b.t <= 0)) || (b.state == BossState::Jump && landed)) {
            b.state = BossState::Roam;
            b.vx = 0;
            b.t = std::max(.8, 1.6 + rng.next() * 1.2 - b.L * .15);
            newGoal(b, rng);
            if (wall || landed) {
                ev.bossLanded = true;
                ev.bossShake = std::max(ev.bossShake, landed ? .4 : .2);
            }
            const int n = B.shots ? B.shots + (b.hp < b.max / 2 ? 4 : 0) : 0;  // éclats projetés en cloche
            for (int i = 0; i < n; ++i) {
                const double a = -PI * (.12 + .76 * i / std::max(1, n - 1)), sp = 6 + rng.next() * 2.5;
                c.bullets.push_back({b.x, b.y - .5, std::cos(a) * sp, std::sin(a) * sp, 4, B.dmg * .6, b.L});
            }
            if (n) ev.shots = true;
        }
        if (dist < BOSS_R + .4 && hurt(d, s, B.dmg, false, dev)) resetActiveBosses(c);
    }
}

void updateBullets(Creatures& c, Drill& d, const Terrain& t, const DrillStats& s, double dt, DrillEvents& dev) {
    const double pcx = d.x + PW / 2, pcy = d.y + PH / 2;
    const std::vector<Bullet> cur = std::move(c.bullets);
    std::vector<Bullet> keep;
    for (Bullet u : cur) {
        u.vy += 9 * dt;
        u.x += u.vx * dt;
        u.y += u.vy * dt;
        u.life -= dt;
        if (u.life <= 0 || t.cellAt(u.x, u.y)) continue;
        if (std::hypot(u.x - pcx, u.y - pcy) < .5) {
            if (hurt(d, s, u.dmg, false, dev)) resetActiveBosses(c);
            continue;
        }
        keep.push_back(u);
    }
    c.bullets = std::move(keep);  // comme bullets = bullets.filter(...) en JS : remplace aussi un effacement fait pendant la boucle
}

}  // namespace

Creatures makeCreatures(const World& w, const std::vector<int>& dead, const std::array<int, NL>& bossDone, Rng& rng) {
    Creatures c;
    c.foes.reserve(w.foes.size());
    for (const FoeSpawn& sp : w.foes) {
        Foe f;
        f.k = sp.kind;
        f.i = sp.index;
        f.x = sp.tx + (1 - FW) / 2;
        f.y = sp.ty + 1 - FH - .001;
        f.grounded = true;
        f.hp = FOES[static_cast<std::size_t>(sp.kind)].hp;
        f.wait = rng.next();
        c.foes.push_back(f);
    }
    for (int i : dead)
        for (Foe& f : c.foes)
            if (f.i == i) f.alive = false;
    for (int L = 0; L < NL; ++L) {
        Boss& b = c.bosses[static_cast<std::size_t>(L)];
        b.L = L;
        b.ch = chamber(L);
        b.max = BOSSES[static_cast<std::size_t>(L)].hp;
        resetBoss(c, b);
        b.dead = bossDone[static_cast<std::size_t>(L)] != 0;
    }
    return c;
}

bool ram(Creatures& c, Drill& d, double ux, double uy, double dmg, CreatureEvents& ev) {
    const double pcx = d.x + PW / 2, pcy = d.y + PH / 2;
    for (Foe& f : c.foes) {
        if (!f.alive) continue;
        const double dx = f.x + FW / 2 - pcx, dy = f.y + FH / 2 - pcy, dist = std::hypot(dx, dy);
        if (dist < 1.25 && (dist < .45 || dotN(dx, dy, ux, uy) > .6)) {
            if (d.ramCd <= 0) hitFoe(f, d, dmg, ev);
            return true;
        }
    }
    for (Boss& b : c.bosses) {
        if (!b.active) continue;
        const double dx = b.x - pcx, dy = b.y - pcy, h = std::hypot(dx, dy), dist = h ? h : 1;
        if (dist < 2.05 && dotN(dx, dy, ux, uy) > .35) {
            if (d.ramCd <= 0) hitBoss(c, b, d, dmg, ev);
            return true;
        }
    }
    return false;
}

void updateCreatures(Creatures& c, Drill& d, const Terrain& t, const DrillStats& s, double dt, Rng& rng, CreatureEvents& ev, DrillEvents& dev) {
    updateFoes(c, d, t, s, dt, rng, dev);
    updateBosses(c, d, s, dt, rng, ev, dev);
    updateBullets(c, d, t, s, dt, dev);
}

void resetActiveBosses(Creatures& c) {
    for (Boss& b : c.bosses)
        if (b.active) resetBoss(c, b);
    c.bullets.clear();
}

void breakSeal(World& w, int layer) {
    const int y0 = SURF + layer * LH;
    for (int y = y0 + LH - 2; y < y0 + LH; ++y)
        for (int x = 1; x < W - 1; ++x)
            if (w.tiles[static_cast<std::size_t>(y * W + x)] == SEAL) w.tiles[static_cast<std::size_t>(y * W + x)] = ROCK;
}

}  // namespace filon
