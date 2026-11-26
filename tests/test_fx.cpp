// Effets visuels : plafond de particules, chute et fin de vie, fumée, flammes, secousses qui s'arrêtent.
#include <cmath>
#include <cstdio>
#include <string>

#include "filon/core/fx.hpp"

using namespace filon;

static int failures = 0;
static void check(bool ok, const std::string& msg) {
    if (!ok && ++failures <= 20) std::printf("ÉCHEC : %s\n", msg.c_str());
}

int main() {
    {  // éclats : plafond de 500, projetés puis retombant, tous éteints après 0,8 s
        Fx fx(1);
        fx.burst(10, 20, 0xffffff, 600);
        check(fx.particles().size() == Fx::MAX, "plafond de particules");
        for (int i = 0; i < 20; ++i) fx.update(1.0 / 60, false);
        double vy = 0;
        for (const Particle& p : fx.particles()) vy += p.vy;
        check(vy / fx.particles().size() > -1 + 6 * 20 / 60.0 - .5, "gravité des éclats");
        for (int i = 0; i < 40; ++i) fx.update(1.0 / 60, false);
        check(fx.particles().empty(), "éclats éteints après 0,8 s");
    }
    {  // fumée de la cheminée seulement quand le four tourne, translucide, qui monte
        Fx fx(2);
        for (int i = 0; i < 120; ++i) fx.update(1.0 / 60, false);
        check(fx.particles().empty(), "pas de fumée four éteint");
        for (int i = 0; i < 120; ++i) fx.update(1.0 / 60, true);
        check(!fx.particles().empty() && fx.particles()[0].alpha < .5 && fx.particles()[0].vy < 0, "fumée quand le four tourne");
    }
    {  // flammes sous la foreuse quand le propulseur pousse
        Fx fx(3);
        Drill d;
        d.x = 5;
        d.y = 10;
        d.thrust = true;
        for (int i = 0; i < 30; ++i) fx.drill(d);
        check(fx.particles().size() > 10, "flammes du propulseur");
        for (const Particle& p : fx.particles()) check(p.vy > 0 && p.y > d.y, "flamme vers le bas");
    }
    {  // secousse : la plus forte l'emporte, s'arrête après 0,25 s
        Fx fx(4);
        fx.shake(.6);
        fx.shake(.2);
        double ox, oy, m = 0;
        for (int i = 0; i < 10; ++i) {
            fx.shakeOffset(ox, oy);
            m = std::fmax(m, std::fmax(std::fabs(ox), std::fabs(oy)));
        }
        check(m > .2 * .5 * 32 && m <= .6 * .5 * 32, "amplitude de la secousse");
        for (int i = 0; i < 20; ++i) fx.update(1.0 / 60, false);
        fx.shakeOffset(ox, oy);
        check(ox == 0 && oy == 0, "secousse terminée");
    }
    std::printf("%s : effets visuels\n", failures ? "ÉCHEC" : "OK");
    return failures ? 1 : 0;
}
