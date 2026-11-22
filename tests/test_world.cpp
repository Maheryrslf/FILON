// Parité de la génération du monde avec la version JS.
// Données de référence : tests/data, produites par tools/export_reference.js.
#include "filon/core/world.hpp"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

static int failures = 0;

static std::vector<unsigned char> readBin(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}

static void checkSeed(std::uint32_t seed) {
    const std::string dir = FILON_TEST_DATA_DIR;
    const auto ref = readBin(dir + "/world_" + std::to_string(seed) + ".bin");
    const filon::World w = filon::generate(seed);

    if (ref.size() != w.tiles.size()) {
        ++failures;
        std::printf("ÉCHEC graine %u : %zu cases de référence, %zu générées\n", seed, ref.size(), w.tiles.size());
        return;
    }
    for (std::size_t i = 0; i < ref.size(); ++i)
        if (ref[i] != w.tiles[i]) {
            ++failures;
            std::printf("ÉCHEC graine %u : première différence en x=%zu y=%zu (JS %d, C++ %d)\n", seed, i % filon::W, i / filon::W,
                        ref[i], w.tiles[i]);
            return;
        }

    std::ifstream f(dir + "/foes_" + std::to_string(seed) + ".txt");
    std::size_t n = 0;
    int k, idx, tx, ty;
    while (f >> k >> idx >> tx >> ty) {
        if (n >= w.foes.size() || w.foes[n].kind != k || w.foes[n].index != idx || w.foes[n].tx != tx || w.foes[n].ty != ty) {
            ++failures;
            std::printf("ÉCHEC graine %u : créature n°%zu différente\n", seed, n);
            return;
        }
        ++n;
    }
    if (n != w.foes.size()) {
        ++failures;
        std::printf("ÉCHEC graine %u : %zu créatures de référence, %zu générées\n", seed, n, w.foes.size());
        return;
    }
    std::printf("graine %u : %zu cases et %zu créatures identiques\n", seed, ref.size(), n);
}

int main() {
    for (std::uint32_t seed : {1u, 123456789u, 2147483647u}) checkSeed(seed);

    // Quelques valeurs de dureté (fiche §2.5)
    if (filon::tileHP(filon::SOFT, 10) != 1 || filon::tileHP(filon::HARD, 100) != 4 * 2.4 ||
        filon::tileHP(filon::ORE_FIRST + 8, 300) != 2 * 3 * 12.0) {
        ++failures;
        std::printf("ÉCHEC : tileHP\n");
    }
    if (failures == 0) std::printf("Parité du monde : OK\n");
    return failures == 0 ? 0 : 1;
}
