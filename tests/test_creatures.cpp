// Parité des créatures et des gardiens : même scénario que tools/export_reference.js §9, image par image.
// Le hasard de la version JS (Math.random) y est remplacé par la même suite que Rng(4242).
#include <cinttypes>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "common.hpp"
#include "filon/core/creatures.hpp"
#include "filon/core/economy.hpp"
#include "filon/core/terrain.hpp"

using namespace filon;

static int failures = 0;

static void fail(const std::string& msg) {
    if (++failures <= 12) std::printf("ÉCHEC : %s\n", msg.c_str());
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

int main() {
    std::ifstream f(std::string(FILON_TEST_DATA_DIR) + "/creatures_1.txt");
    if (!f) {
        std::printf("ÉCHEC : creatures_1.txt introuvable\n");
        return 1;
    }
    Rng rng(4242);
    World w = generate(1);
    Terrain t(w);
    Creatures c = makeCreatures(w, {}, {}, rng);
    Economy e;
    e.up[HULL] = 6;
    e.tal[ARMOR] = 2;
    DrillStats st = drillStats(e);
    Drill d;
    auto place = [&](double x, double y) {
        d.x = x;
        d.y = y;
        d.tx = static_cast<int>(std::floor(d.x + PW / 2));
        d.ty = static_cast<int>(std::floor(d.y + PH / 2));
    };
    const FoeSpawn& f0 = w.foes[0];
    const Chamber c1 = chamber(1);
    place(f0.tx - 0.9, f0.ty + 1 - PH - 0.001);
    d.hull = st.hullMax;
    d.fuel = st.fuelMax;
    double coins = 0, xp = 0;
    std::vector<int> dead;
    std::array<int, NL> bossDone{};
    const double dt = 1.0 / 60;
    static const char* states[] = {"roam", "wind", "charge", "jump"};

    std::string line, kind;
    int frames = 0;
    while (std::getline(f, line)) {
        std::istringstream in(line);
        in >> kind;
        if (kind == "foes") {
            std::string a;
            int n;
            in >> n;
            if (n != static_cast<int>(w.foes.size())) fail("nombre de créatures");
        } else if (kind == "f") {
            int fr;
            in >> fr;
            const double ux = test::num(in), uy = test::num(in);
            if (fr == 700) {
                e.up[HULL] = 12;
                e.up[BLADE] = 12;
                st = drillStats(e);
                place(c1.x1 + 1.5, c1.y2 + 1 - PH - 0.001);
                d.hull = st.hullMax;
            }
            if (fr == 1300) place(19.4, DECK - PH - 0.001);
            if (fr == 1310) place(c1.x1 + 1.5, c1.y2 + 1 - PH - 0.001);
            d.ramCd -= dt;
            d.hurtCd -= dt;
            CreatureEvents ev;
            DrillEvents dev;
            if (fr % 2 == 0 && fr >= 300) ram(c, d, ux, uy, effects(e).ram, ev);
            updateCreatures(c, d, t, st, dt, rng, ev, dev);
            for (int i : ev.killedFoes) {
                const FoeDef& F = FOES[static_cast<std::size_t>(c.foes[static_cast<std::size_t>(i)].k)];
                coins += F.coins;
                xp += F.xp;
                dead.push_back(i);
            }
            if (ev.bossKilled >= 0) {
                const BossDef& B = BOSSES[static_cast<std::size_t>(ev.bossKilled)];
                coins += B.coins;
                xp += B.xp;
                bossDone[static_cast<std::size_t>(ev.bossKilled)] = 1;
                breakSeal(w, ev.bossKilled);
            }
            double sum = 0, bs = 0;
            int alive = 0;
            for (const Foe& o : c.foes)
                if (o.alive) {
                    ++alive;
                    sum += o.x + 3 * o.y + 7 * o.vx + 11 * o.vy + o.hp + o.wait + o.dir + (o.grounded ? 1 : 0) + o.flash;
                }
            for (const Bullet& u : c.bullets) bs += u.x + 2 * u.y + u.life;
            const double want[] = {test::num(in), test::num(in), test::num(in), test::num(in), test::num(in), test::num(in), test::num(in),
                                   test::num(in), test::num(in), test::num(in), test::num(in), test::num(in)};
            const double got[] = {d.x, d.y, d.hull, d.hurtCd, d.ramCd, coins, xp, static_cast<double>(dead.size()), static_cast<double>(alive), sum,
                                  static_cast<double>(c.bullets.size()), bs};
            static const char* names[] = {"x", "y", "coque", "hurtCd", "ramCd", "pièces", "xp", "tuées", "vivantes", "somme créatures", "éclats", "somme éclats"};
            for (int k = 0; k < 12; ++k)
                if (!test::close(got[k], want[k])) {
                    char b[200];
                    std::snprintf(b, sizeof b, "image %d, %s : %.17g au lieu de %.17g", fr, names[k], got[k], want[k]);
                    fail(b);
                }
            ++frames;
        } else if (kind == "b") {
            int L;
            in >> L;
            const Boss& b = c.bosses[static_cast<std::size_t>(L)];
            const double x = test::num(in), y = test::num(in), vx = test::num(in), vy = test::num(in), hp = test::num(in);
            std::string state;
            in >> state;
            const double tt = test::num(in), gx = test::num(in);
            int active, isDead;
            in >> active >> isDead;
            const double flash = test::num(in);
            if (!test::close(b.x, x) || !test::close(b.y, y) || !test::close(b.vx, vx) || !test::close(b.vy, vy) || !test::close(b.hp, hp) ||
                state != states[static_cast<int>(b.state)] || !test::close(b.t, tt) || !test::close(b.gx, gx) || b.active != (active == 1) ||
                b.dead != (isDead == 1) || !test::close(b.flash, flash)) {
                char m[260];
                std::snprintf(m, sizeof m, "image %d, gardien %d : (%.6g, %.6g) pv %.6g %s t %.6g au lieu de (%.6g, %.6g) pv %.6g %s t %.6g", frames - 1, L, b.x, b.y,
                              b.hp, states[static_cast<int>(b.state)], b.t, x, y, hp, state.c_str(), tt);
                fail(m);
            }
        } else if (kind == "end") {
            std::string deadList, hash;
            int bd[NL];
            in >> bd[0] >> bd[1] >> bd[2] >> bd[3] >> deadList >> hash;
            for (int L = 0; L < NL; ++L)
                if (bossDone[static_cast<std::size_t>(L)] != bd[L]) fail("gardien vaincu " + std::to_string(L));
            std::string mine;
            for (int i : dead) mine += (mine.empty() ? "" : ",") + std::to_string(i);
            if ((mine.empty() ? "-" : mine) != deadList) fail("créatures tuées : " + mine + " au lieu de " + deadList);
            if (fnv(w.tiles) != hash) fail("cases après le sceau brisé");
        }
    }
    std::printf("%s : %d images, %zu créatures tuées, gardiens %d%d%d%d, %.0f pièces\n", failures ? "ÉCHEC" : "OK", frames, dead.size(), bossDone[0],
                bossDone[1], bossDone[2], bossDone[3], coins);
    return failures ? 1 : 0;
}
