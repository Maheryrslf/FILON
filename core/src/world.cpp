#include "filon/core/world.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "filon/core/random.hpp"

namespace filon {

const std::array<LayerDef, NL> LAYERS = {{
    {"Terres brunes", 1},
    {"Grès rouge", 2.4},
    {"Basalte", 5.5},
    {"Cœur ardent", 12},
}};

const std::array<OreDef, ORE_COUNT> ORES = {{
    {"Charbon", 0x1c1a1a, 0x6a6360, 3, 1},
    {"Cuivre", 0xc96f34, 0xf0a46a, 7, 1.2},
    {"Fer", 0xa99787, 0xe1d2c4, 13, 1.5},
    {"Argent", 0xcfd8e3, 0xffffff, 24, 1.6},
    {"Or", 0xf2b822, 0xffe58a, 42, 1.8},
    {"Rubis", 0xd42a48, 0xff8aa0, 75, 2},
    {"Émeraude", 0x1fbf78, 0x8fffc8, 130, 2.2},
    {"Diamant", 0x8fe8ff, 0xe8fbff, 230, 2.5},
    {"Astralite", 0xa85cff, 0xe2c4ff, 400, 3},
}};

namespace {
constexpr double ORE_RATE = .1;
// Poids de tirage des minerais par couche
constexpr int ORE_W[NL][ORE_COUNT] = {
    {40, 30, 18, 0, 0, 0, 0, 0, 0},
    {10, 20, 30, 25, 12, 0, 0, 0, 0},
    {0, 0, 10, 25, 30, 22, 10, 0, 0},
    {0, 0, 0, 5, 15, 25, 30, 18, 8},
};

int randInt(Rng& r, int n) { return static_cast<int>(std::floor(r.next() * n)); }  // Math.floor(r()*n)

int pickW(const int (&w)[ORE_COUNT], Rng& r) {
    double s = 0;
    for (int v : w) s += v;
    double x = r.next() * s;
    for (int i = 0; i < ORE_COUNT; ++i) {
        x -= w[i];
        if (x < 0) return i;
    }
    return ORE_COUNT - 1;
}
}  // namespace

double tileHP(std::uint8_t t, int y) {
    const double m = LAYERS[static_cast<std::size_t>(std::max(0, layerOf(y)))].mult;
    if (t == SOFT) return m;
    if (t == ROCK) return 2 * m;
    if (t == HARD) return 4 * m;
    if (isOre(t)) return 2 * ORES[t - ORE_FIRST].hp * m;
    return std::numeric_limits<double>::infinity();
}

Chamber chamber(int layer) {
    const int y0 = SURF + layer * LH;
    return {5, W - 6, y0 + LH - 14, y0 + LH - 3};
}

World generate(std::uint32_t seed) {
    Rng r(seed);
    World w;
    w.seed = seed;
    auto& t = w.tiles;
    t.assign(static_cast<std::size_t>(W * H), AIR);
    auto cell = [&](int x, int y) -> std::uint8_t& { return t[static_cast<std::size_t>(y * W + x)]; };

    // 1. Remplissage : un tirage par case, bords compris
    for (int y = SURF; y < H; ++y) {
        const int L = layerOf(y);
        const double d = static_cast<double>(y - SURF - L * LH) / LH;
        for (int x = 0; x < W; ++x) {
            const double n = r.next();
            cell(x, y) = (x == 0 || x == W - 1 || y == H - 1) ? BED : n < .5 - .25 * d ? SOFT : n < .9 - .15 * d ? ROCK : HARD;
        }
    }

    for (int L = 0; L < NL; ++L) {
        const int y0 = SURF + L * LH, ylo = y0 + 4, yhi = y0 + LH - 17;

        // 2. Grottes : vers aléatoires
        for (int k = 0; k < 9 + L * 2; ++k) {
            int x = 2 + randInt(r, W - 4);
            int y = ylo + 3 + randInt(r, yhi - ylo - 3);
            const int len = 10 + randInt(r, 22);
            for (int i = 0; i < len; ++i) {
                static constexpr int OFF[3][2] = {{0, 0}, {1, 0}, {0, 1}};
                for (const auto& o : OFF) {
                    const int X = x + o[0], Y = y + o[1];
                    if (X > 0 && X < W - 1 && Y >= ylo && Y <= yhi) cell(X, Y) = AIR;
                }
                if (r.next() < .5) x += r.next() < .5 ? -1 : 1;
                else y += r.next() < .6 ? 1 : -1;
                x = std::clamp(x, 1, W - 3);
                y = std::clamp(y, ylo, yhi - 1);
            }
        }

        // 3. Lave sur le sol des grottes (couches 2 et 3) ; le tirage n'a lieu que si les conditions sont réunies
        if (L >= 2)
            for (int y = ylo; y <= yhi; ++y)
                for (int x = 1; x < W - 1; ++x)
                    if (cell(x, y) != AIR && cell(x, y) != BED && cell(x, y - 1) == AIR && r.next() < (L == 3 ? .35 : .08))
                        cell(x, y) = LAVA;

        // 4. Filons
        const int n = static_cast<int>(std::floor(W * LH * ORE_RATE / 3.5));
        for (int k = 0; k < n; ++k) {
            const auto id = static_cast<std::uint8_t>(ORE_FIRST + pickW(ORE_W[L], r));
            int x = 1 + randInt(r, W - 2);
            int y = y0 + 1 + randInt(r, LH - 5);
            const int len = 2 + randInt(r, 4);
            for (int i = 0; i < len; ++i) {
                auto& c = cell(x, y);
                if (c == SOFT || c == ROCK || c == HARD) c = id;
                if (r.next() < .5) x += r.next() < .5 ? -1 : 1;
                else y += r.next() < .5 ? -1 : 1;
                x = std::clamp(x, 1, W - 2);
                y = std::clamp(y, y0, y0 + LH - 3);
            }
        }

        // 5. Salle du gardien et sceau
        const Chamber c = chamber(L);
        for (int y = c.y1; y <= c.y2; ++y)
            for (int x = c.x1; x <= c.x2; ++x) cell(x, y) = AIR;
        for (int y = c.y2 + 1; y < y0 + LH; ++y)
            for (int x = 1; x < W - 1; ++x) cell(x, y) = L < NL - 1 ? SEAL : HARD;

        // 6. Créatures, posées au sol
        int placed = 0, tries = 0;
        while (placed < 7 + L * 3 && tries++ < 900) {
            const int x = 1 + randInt(r, W - 2), y = ylo + randInt(r, yhi - ylo);
            if (cell(x, y) != AIR) continue;
            int yy = y;
            while (cell(x, yy + 1) == AIR) ++yy;
            const bool taken = std::any_of(w.foes.begin(), w.foes.end(), [&](const FoeSpawn& f) { return f.tx == x && f.ty == yy; });
            if (!taken) {
                w.foes.push_back({L, static_cast<int>(w.foes.size()), x, yy});
                ++placed;
            }
        }
    }
    return w;
}

}  // namespace filon
