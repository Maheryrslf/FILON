#pragma once
// Hasard et bruit identiques à la version JS (fiche technique §2.7).
#include <cstdint>

namespace filon {

// Générateur mulberry32 : même graine => même suite que rng() en JS.
class Rng {
public:
    explicit Rng(std::uint32_t seed) : s_(seed) {}
    double next();  // dans [0, 1)
private:
    std::uint32_t s_;
};

std::uint32_t hash2(std::int32_t x, std::int32_t y);                 // hash() JS
double noiseHash(std::int32_t x, std::int32_t y, std::int32_t s);    // nh() JS
double valueNoise(double x, double y, std::int32_t s);               // vnoise() JS
double fbm(double x, double y, std::int32_t s, int octaves);         // fbm() JS

}  // namespace filon
