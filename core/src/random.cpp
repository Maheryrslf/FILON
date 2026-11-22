#include "filon/core/random.hpp"

#include <cmath>

namespace filon {
namespace {
// Math.imul de JS : produit sur 32 bits avec débordement
constexpr std::uint32_t mul(std::uint32_t a, std::uint32_t b) { return a * b; }
constexpr std::uint32_t u(std::int32_t v) { return static_cast<std::uint32_t>(v); }
}  // namespace

double Rng::next() {
    s_ += 0x6D2B79F5u;
    const std::uint32_t s = s_;
    std::uint32_t t = mul(s ^ (s >> 15), 1u | s);
    t = (t + mul(t ^ (t >> 7), 61u | t)) ^ t;
    return static_cast<double>(t ^ (t >> 14)) / 4294967296.0;
}

std::uint32_t hash2(std::int32_t x, std::int32_t y) {
    std::uint32_t h = mul(u(x), 374761393u) + mul(u(y), 668265263u);
    h = mul(h ^ (h >> 13), 1274126177u);
    return h ^ (h >> 16);
}

double noiseHash(std::int32_t x, std::int32_t y, std::int32_t s) {
    std::uint32_t h = mul(u(x), 374761393u) ^ mul(u(y), 668265263u) ^ mul(u(s), 1442695041u);
    h = mul(h ^ (h >> 13), 1274126177u);
    return static_cast<double>(h ^ (h >> 16)) / 4294967296.0;
}

double valueNoise(double x, double y, std::int32_t s) {
    const double fx0 = std::floor(x), fy0 = std::floor(y);
    const auto ix = static_cast<std::int32_t>(fx0), iy = static_cast<std::int32_t>(fy0);
    const double fx = x - fx0, fy = y - fy0;
    const double ux = fx * fx * (3 - 2 * fx), uy = fy * fy * (3 - 2 * fy);
    const double a = noiseHash(ix, iy, s), b = noiseHash(ix + 1, iy, s);
    const double c = noiseHash(ix, iy + 1, s), d = noiseHash(ix + 1, iy + 1, s);
    return a + (b - a) * ux + (c - a) * uy + (a - b - c + d) * ux * uy;
}

double fbm(double x, double y, std::int32_t s, int octaves) {
    double v = 0, amp = .5, norm = 0;
    for (int i = 0; i < octaves; ++i) {
        v += amp * valueNoise(x, y, s + i * 17);
        norm += amp;
        amp *= .5;
        x *= 2.03;
        y *= 2.03;
    }
    return v / norm;
}

}  // namespace filon
