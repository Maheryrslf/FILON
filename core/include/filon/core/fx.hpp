#pragma once
// Effets visuels : particules (éclats, poussière, flammes, fumée) et secousses de l'écran, comme burst(), shake()
// et updParticles() de la version JS. Hasard séparé de celui du jeu : les effets ne changent pas la partie.
#include <cstdint>
#include <vector>

#include "filon/core/drill.hpp"
#include "filon/core/random.hpp"

namespace filon {

struct Particle {
    double x, y, vx, vy;  // en cases
    double life;          // secondes restantes
    double g;             // gravité propre (cases/s²)
    double size;          // côté du carré, en pixels du monde
    std::uint32_t color;  // 0xRRGGBB
    double alpha;         // opacité de base (fumée translucide)
};

class Fx {
public:
    static constexpr std::size_t MAX = 500;  // plafond de particules, comme en JS

    explicit Fx(std::uint32_t seed) : rng_(seed) {}

    // n éclats autour de (x, y), vitesse jusqu'à sp cases/s, légèrement projetés vers le haut
    void burst(double x, double y, std::uint32_t color, int n, double sp = 3);
    void shake(double amplitude);           // secousse de 0,25 s (la plus forte l'emporte)

    // Effets continus, à appeler à chaque image de jeu
    void drill(const Drill& d);             // flammes du propulseur, poussière de glissade
    void update(double dt, bool furnaceBusy);  // mouvement, fin de vie, fumée de la cheminée, fin des secousses

    // Décalage de la caméra dû aux secousses, en pixels du monde (tiré à chaque appel)
    void shakeOffset(double& ox, double& oy);

    const std::vector<Particle>& particles() const { return parts_; }

private:
    Rng rng_;
    std::vector<Particle> parts_;
    double shakeT_ = 0, shakeA_ = 0;
};

}  // namespace filon
