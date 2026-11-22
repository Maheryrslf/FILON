#pragma once
// Outils partagés par les tests de parité.
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>

namespace test {

inline double num(std::istringstream& in) {  // lecture exacte d'un nombre écrit par JS
    std::string s;
    in >> s;
    return std::strtod(s.c_str(), nullptr);
}

// sin, cos, atan2 et hypot de la bibliothèque C et du moteur JS peuvent différer sur le dernier bit
inline bool close(double a, double b) { return std::fabs(a - b) <= 1e-9 * (1 + std::fabs(b)); }

}  // namespace test
