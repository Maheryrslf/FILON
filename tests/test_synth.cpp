// Synthétiseur : contrôles du signal produit (pas de carte son nécessaire).
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "filon/core/synth.hpp"

using namespace filon;

static int failures = 0;
static void check(bool ok, const std::string& msg) {
    if (!ok && ++failures <= 20) std::printf("ÉCHEC : %s\n", msg.c_str());
}

static std::vector<float> run(Synth& s, double seconds, int rate) {
    std::vector<float> out(static_cast<std::size_t>(seconds * rate));
    for (std::size_t i = 0; i < out.size(); i += 256) s.render(out.data() + i, static_cast<int>(std::min<std::size_t>(256, out.size() - i)));
    return out;
}
static double peak(const std::vector<float>& v, std::size_t a, std::size_t b) {
    double p = 0;
    for (std::size_t i = a; i < b && i < v.size(); ++i) p = std::max(p, static_cast<double>(std::fabs(v[i])));
    return p;
}
static int crossings(const std::vector<float>& v, std::size_t a, std::size_t b) {
    int n = 0;
    for (std::size_t i = a + 1; i < b; ++i) n += (v[i - 1] < 0) != (v[i] < 0);
    return n;
}

int main() {
    const int R = 8000;
    {  // note simple : volume qui part de v·gain et s'éteint, silence après l'arrêt
        Synth s(R);
        s.setVolumes(0, 1);
        s.tone(440, .1, Wave::Sine, .5);
        const auto o = run(s, .3, R);
        check(std::fabs(peak(o, 0, 40) - .5 * .6) < .03, "attaque de la note");
        check(peak(o, 750, 800) < .002, "note éteinte à la fin de sa rampe");
        check(peak(o, 1100, o.size()) == 0, "silence après l'arrêt");
        const int c = crossings(o, 0, 400);  // 440 Hz sur 50 ms : ~44 passages par zéro
        check(c > 40 && c < 48, "fréquence de 440 Hz (" + std::to_string(c) + " passages)");
    }
    {  // glissement : la fréquence descend de 1000 à 500 Hz
        Synth s(R);
        s.setVolumes(0, 1);
        s.tone(1000, .5, Wave::Sine, 1, -500);
        const auto o = run(s, .5, R);
        check(crossings(o, 0, 400) > crossings(o, 3600, 4000) * 1.5, "glissement de fréquence");
    }
    {  // bruit : 0,3 s au plus, même pour une durée plus longue (tampon joué sans boucle, comme en JS)
        Synth s(R);
        s.setVolumes(0, 1);
        s.noise(.9, .35, 400);
        const auto o = run(s, 1, R);
        check(peak(o, 0, 800) > .01, "bruit audible");
        check(peak(o, 2600, o.size()) < .001, "bruit limité à 0,3 s (reste la traîne du filtre)");
    }
    {  // tous les bruitages : signal fini, borné, non nul ; rien si le volume des effets est nul
        for (int k = 0; k <= static_cast<int>(Sfx::Shoot); ++k) {
            Synth s(R, 7);
            s.setVolumes(0, 1);
            s.sfx(static_cast<Sfx>(k));
            const auto o = run(s, 1.5, R);
            bool finite = true;
            for (float x : o) finite = finite && std::isfinite(x) && std::fabs(x) <= 1;
            check(finite && peak(o, 0, o.size()) > .005, "bruitage " + std::to_string(k));
        }
        Synth s(R);
        s.setVolumes(1, 0);
        s.sfx(Sfx::Win);
        check(peak(run(s, 1, R), 0, R) == 0, "effets coupés");
    }
    {  // musique : planifiée au fil des images, différente pendant un gardien, muette à volume nul
        auto play = [&](bool boss, double vol) {
            Synth s(R);
            s.setVolumes(vol, 1);
            std::vector<float> all;
            for (int f = 0; f < 120; ++f) {  // 2 s à 60 images/s
                s.music(boss);
                const auto o = run(s, 1.0 / 60, R);
                all.insert(all.end(), o.begin(), o.end());
            }
            double e = 0;
            for (float x : all) e += x * x;
            return e;
        };
        const double calm = play(false, 1), boss = play(true, 1);
        check(calm > 1, "musique audible");
        check(boss > 1 && boss != calm, "musique de combat audible et différente");
        check(play(false, 0) == 0, "musique coupée");
    }
    std::printf("%s : synthétiseur\n", failures ? "ÉCHEC" : "OK");
    return failures ? 1 : 0;
}
