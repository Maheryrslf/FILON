#include "filon/core/texture.hpp"

#include <algorithm>
#include <cmath>

#include "filon/core/random.hpp"

namespace filon {
namespace {

// Teintes par couche : [meuble, roche, roche dure] x [sombre, clair] x RGB
constexpr int PAL[NL][3][2][3] = {
    {{{66, 46, 30}, {140, 102, 68}}, {{78, 62, 48}, {146, 124, 100}}, {{50, 45, 41}, {112, 102, 94}}},
    {{{104, 48, 30}, {178, 98, 64}}, {{98, 54, 40}, {166, 108, 82}}, {{66, 38, 32}, {128, 80, 68}}},
    {{{60, 60, 64}, {124, 122, 124}}, {{38, 40, 46}, {94, 98, 106}}, {{24, 26, 30}, {72, 74, 82}}},
    {{{68, 32, 24}, {132, 72, 52}}, {{50, 25, 21}, {112, 58, 46}}, {{32, 17, 15}, {88, 45, 37}}},
};
constexpr int BEDP[2][3] = {{14, 12, 11}, {44, 40, 38}};

int ifloor(double v) { return static_cast<int>(std::floor(v)); }
double jsRound(double x) {  // Math.round de JS : moitiés arrondies vers le haut
    const double f = std::floor(x);
    return x - f >= .5 ? f + 1 : f;
}
std::uint8_t toU8(double x) {  // écriture dans un Uint8ClampedArray : bornage, puis arrondi au pair le plus proche
    if (!(x > 0)) return 0;
    if (x >= 255) return 255;
    const double f = std::floor(x), d = x - f;
    if (d < .5) return static_cast<std::uint8_t>(f);
    if (d > .5) return static_cast<std::uint8_t>(f + 1);
    return static_cast<std::uint8_t>(static_cast<int>(f) % 2 == 0 ? f : f + 1);
}
double lerp(int a, int b, double k) { return a + (b - a) * k; }
std::uint8_t original(const World& w, int x, int y) {
    return (x < 0 || x >= W || y < 0 || y >= H) ? std::uint8_t{BED} : w.original[static_cast<std::size_t>(y * W + x)];
}
double channel(std::uint32_t rgb, int c) { return static_cast<double>((rgb >> (16 - 8 * c)) & 0xff); }

}  // namespace

std::uint32_t tileColor(std::uint8_t t, int y) {
    if (isOre(t)) return ORES[t - ORE_FIRST].color;
    const int* p = PAL[std::max(0, layerOf(y))][t == SOFT ? 0 : t == HARD ? 2 : 1][1];
    return static_cast<std::uint32_t>(p[0]) << 16 | static_cast<std::uint32_t>(p[1]) << 8 | static_cast<std::uint32_t>(p[2]);
}

Rgb texel(const World& w, int s, double u, double v, double hc, double sh) {
    const int ty = ifloor(v / TS), tx = ifloor(u / TS), L = layerOf(ty);
    double r, g, b;
    // frontières entre matériaux ondulées : pas de grille visible
    const double wu = u + (fbm(u * .03, v * .03, s + 5, 2) - .5) * 44, wv = v + (fbm(u * .03 + 7.3, v * .03, s + 9, 2) - .5) * 44;
    const std::uint8_t mt = original(w, ifloor(wu / TS), ifloor(wv / TS));
    if (tx == 0 || tx == W - 1 || ty == H - 1 || mt == BED) {
        const double k = std::clamp(hc * .7 + valueNoise(u * .4, v * .4, s + 21) * .3, 0.0, 1.0);
        r = lerp(BEDP[0][0], BEDP[1][0], k);
        g = lerp(BEDP[0][1], BEDP[1][1], k);
        b = lerp(BEDP[0][2], BEDP[1][2], k);
    } else {
        const int kind = mt == SOFT ? 0 : (mt == HARD || mt == LAVA || mt == SEAL) ? 2 : 1;
        const auto& p = PAL[L][kind];
        double k;
        if (kind == 0) {  // sédiments en strates + cailloux
            k = hc * .65 + (.5 + .5 * std::sin(v * .21 + hc * 7)) * .35;
            if (valueNoise(u * .22, v * .22, s + 3) > .8) k += .18;
        } else if (kind == 1) {  // roche + fissures
            k = hc;
            const double cr = std::fabs(fbm(u * .03, v * .045, s + 11, 3) - .5);
            if (cr < .008) k -= .35;
            else if (cr < .02) k -= .08;
        } else {  // grain de granite
            const double gr = valueNoise(u * .55, v * .55, s + 13);
            k = hc * .55 + gr * .45;
            if (gr > .82) k += .25;
            else if (gr < .14) k -= .3;
        }
        k = jsRound(std::clamp(k, 0.0, 1.0) * 6) / 6;
        r = lerp(p[0][0], p[1][0], k);
        g = lerp(p[0][1], p[1][1], k);
        b = lerp(p[0][2], p[1][2], k);
        if (L == 3 && kind == 2 && valueNoise(u * .5, v * .5, s + 29) > .9) {  // braises
            r = 225;
            g = 95;
            b = 35;
            sh = 0;
        }
    }
    const std::uint8_t ot = w.at(tx, ty);
    if (isOre(ot)) {  // pépites incrustées (disparaissent une fois le filon extrait)
        const std::uint32_t hh = hash2(tx, ty);
        double best = 9;
        for (int q = 0; q < 3; ++q) {
            const double sx = tx * TS + 7 + static_cast<int>((hh >> (q * 7)) % 18);
            const double sy = ty * TS + 7 + static_cast<int>((hh >> (q * 7 + 4)) % 18);
            const double rad = (5 + static_cast<int>((hh >> (q * 5 + 2)) % 5)) * (.75 + .5 * valueNoise(u * .25, v * .25, s + 31 + q));
            const double dx = u - sx, dy = v - sy, dd = std::sqrt(dx * dx + dy * dy) / rad;
            if (dd < best) best = dd;
        }
        if (best < 1) {
            const OreDef& o = ORES[ot - ORE_FIRST];
            const double f = std::clamp(.3 + sh * 2.5 + (1 - best) * .5, 0.0, 1.0);
            r = channel(o.color, 0) + (channel(o.glint, 0) - channel(o.color, 0)) * f;
            g = channel(o.color, 1) + (channel(o.glint, 1) - channel(o.color, 1)) * f;
            b = channel(o.color, 2) + (channel(o.glint, 2) - channel(o.color, 2)) * f;
            if (best > .8) {
                r *= .5;
                g *= .5;
                b *= .5;
            } else if (valueNoise(u * .9, v * .9, s + 37) > .88) {
                r = g = b = 255;
            }
            sh *= .3;
        }
    }
    // relief éclairé du haut à gauche + nuance propre à chaque pixel, par paliers
    const double jit = (static_cast<double>(hash2(ifloor(u / PX), ifloor(v / PX)) & 255) / 255 - .5) * .24;
    const double lf = jsRound(std::clamp(1 + sh, .62, 1.4) * (1 + jit) * 10) / 10;
    r *= lf;
    g *= lf;
    b *= lf;
    if (ty == SURF) {  // herbe
        const double gv = v - SURF * TS, gt = 4 + valueNoise(u * .3, .5, s + 41) * 4;
        if (gv < gt) {
            const double k2 = valueNoise(u * .8, v * .8, s + 43);
            r = 62 + 40 * k2;
            g = 112 + 50 * k2;
            b = 36 + 18 * k2;
        } else if (gv < gt + 3) {
            r *= .55;
            g *= .55;
            b *= .55;
        }
    }
    return {r, g, b};
}

void shadePixel(Chunk& c, const Terrain& t, int sx, int sy) {
    const int i = sx - c.cx * CHUNK_W, j = sy - c.cy * CHUNK_H;
    if (i < 0 || i >= CHUNK_W || j < 0 || j >= CHUNK_H) return;
    const std::size_t o = static_cast<std::size_t>((j * CHUNK_W + i) * 4);
    if (c.base[o + 3] == 0) return;  // ciel
    // pixel plein exposé par le haut : plus clair ; pixel creusé : paroi du fond, plus sombre sous un surplomb
    const double k = t.cellSolid(sx, sy) ? (t.cellSolid(sx, sy - 1) || sy == SURF * 8 ? 1 : 1.18) : (t.cellSolid(sx, sy - 1) ? .17 : .27);
    for (int ch = 0; ch < 3; ++ch) c.rgba[o + static_cast<std::size_t>(ch)] = toU8(c.base[o + static_cast<std::size_t>(ch)] * k);
    c.rgba[o + 3] = 255;
}

Chunk buildChunk(const World& w, const Terrain& t, int s, int cx, int cy) {
    Chunk c;
    c.cx = cx;
    c.cy = cy;
    c.base.assign(static_cast<std::size_t>(CHUNK_W * CHUNK_H * 4), 0);
    const double u0 = cx * CW * TS + PX / 2.0, v0 = cy * CH * TS + PX / 2.0, rel = 1.6;
    const int hw = CHUNK_W + 2;
    std::vector<float> hf(static_cast<std::size_t>(hw * (CHUNK_H + 2)), 0.f);  // relief en 32 bits, comme le Float32Array de JS
    for (int j = 0; j < CHUNK_H + 2; ++j) {
        const double v = v0 + (j - 1) * PX;
        if (v < SURF * TS - PX * 2) continue;
        for (int i = 0; i < hw; ++i) hf[static_cast<std::size_t>(j * hw + i)] = static_cast<float>(fbm((u0 + (i - 1) * PX) * .05, v * .05, s, 3));
    }
    auto HF = [&](int i, int j) { return static_cast<double>(hf[static_cast<std::size_t>(j * hw + i)]); };
    for (int j = 0; j < CHUNK_H; ++j) {
        const double v = v0 + j * PX;
        const int ty = ifloor(v / TS);
        if (ty < SURF || ty >= H) continue;
        for (int i = 0; i < CHUNK_W; ++i) {
            const double u = u0 + i * PX;
            if (ifloor(u / TS) >= W) continue;
            const Rgb col = texel(w, s, u, v, HF(i + 1, j + 1), (HF(i, j) - HF(i + 2, j + 2)) * rel);
            const std::size_t o = static_cast<std::size_t>((j * CHUNK_W + i) * 4);
            c.base[o] = toU8(col.r);
            c.base[o + 1] = toU8(col.g);
            c.base[o + 2] = toU8(col.b);
            c.base[o + 3] = 255;
        }
    }
    c.rgba = c.base;
    for (int j = 0; j < CHUNK_H; ++j)
        for (int i = 0; i < CHUNK_W; ++i) shadePixel(c, t, cx * CHUNK_W + i, cy * CHUNK_H + j);
    return c;
}

void recolorTile(Chunk& c, const World& w, const Terrain& t, int s, int tx, int ty) {
    for (int sy = ty * 8; sy < ty * 8 + 8; ++sy)
        for (int sx = tx * 8; sx < tx * 8 + 8; ++sx) {
            const int i = sx - c.cx * CHUNK_W, j = sy - c.cy * CHUNK_H;
            if (i < 0 || i >= CHUNK_W || j < 0 || j >= CHUNK_H) continue;
            const double u = sx * PX + PX / 2.0, v = sy * PX + PX / 2.0;
            const double hc = fbm(u * .05, v * .05, s, 3);
            const double sh = (fbm((u - PX) * .05, (v - PX) * .05, s, 3) - fbm((u + PX) * .05, (v + PX) * .05, s, 3)) * 1.6;
            const Rgb col = texel(w, s, u, v, hc, sh);
            const std::size_t o = static_cast<std::size_t>((j * CHUNK_W + i) * 4);
            c.base[o] = toU8(col.r);
            c.base[o + 1] = toU8(col.g);
            c.base[o + 2] = toU8(col.b);
            shadePixel(c, t, sx, sy);
        }
}

}  // namespace filon
