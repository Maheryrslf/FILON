// Parité avec la version JS : valeurs calculées avec les fonctions de docs/reference-js/filon-v17.html (L211-218).
// Les doubles doivent être identiques au bit près.
#include "filon/core/random.hpp"

#include <cstdio>

static int failures = 0;

static void check(bool ok, const char* what) {
    if (!ok) {
        ++failures;
        std::printf("ÉCHEC : %s\n", what);
    }
}

int main() {
    {
        filon::Rng r(123456789u);
        const double expect[] = {0.2577907438389957, 0.9707721115555614, 0.7853280142880976,
                                 0.20616457983851433, 0.30307188746519387, 0.7470660470426083};
        for (double e : expect) check(r.next() == e, "rng(123456789)");
    }
    {
        filon::Rng r(2147483647u);
        const double expect[] = {0.4290980885270983, 0.12713524978607893, 0.3852774982806295};
        for (double e : expect) check(r.next() == e, "rng(2147483647)");
    }
    check(filon::hash2(3, 7) == 260807386u, "hash(3,7)");
    check(filon::hash2(-5, 100) == 3268209237u, "hash(-5,100)");
    check(filon::hash2(288, 2935) == 2717699074u, "hash(288,2935)");
    check(filon::hash2(-1, -1) == 731167729u, "hash(-1,-1)");
    check(filon::noiseHash(1, 2, 3) == 0.6394013063982129, "nh(1,2,3)");
    check(filon::noiseHash(-7, 40, 9973) == 0.9340460777748376, "nh(-7,40,9973)");
    check(filon::valueNoise(1.25, -3.5, 5) == 0.2765180474743829, "vnoise(1.25,-3.5,5)");
    check(filon::valueNoise(100.7, 33.1, 17) == 0.10646744330266132, "vnoise(100.7,33.1,17)");
    check(filon::fbm(12.3, 45.6, 42, 3) == 0.5760565470308052, "fbm(12.3,45.6,42,3)");
    check(filon::fbm(-2.2, 0.5, 7, 2) == 0.5555985129140529, "fbm(-2.2,0.5,7,2)");

    if (failures == 0) std::printf("Parité hasard/bruit : OK (19 valeurs identiques à la version JS)\n");
    return failures == 0 ? 0 : 1;
}
