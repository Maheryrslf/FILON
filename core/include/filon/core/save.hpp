#pragma once
// Sauvegarde au format de la version JS (clé filon-save-v1, champ v = 1), même ordre des champs :
// une sauvegarde du navigateur se relit ici, et une sauvegarde d'ici se relit dans le navigateur.
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "filon/core/economy.hpp"
#include "filon/core/nuggets.hpp"

namespace filon {

struct Settings {
    double music = .45, sfx = .8;
    bool vib = true;
    std::string stick = "dyn";
    bool shake = true;  // secousses de l'écran ; propre à la version C++, écrit dans le JSON seulement quand il est coupé
};

struct Progress {  // ce qui n'appartient ni à l'économie ni au monde
    std::array<int, 4> boss{};  // gardiens vaincus, par couche
    std::vector<int> dead;      // créatures tuées (indice de création)
    double maxDepth = 0;        // profondeur maximale atteinte, en cases
    double play = 0;            // temps de jeu, en secondes
    int seenL = 0;              // dernière couche annoncée
    int fallTip = 0;            // conseil du bras déjà donné
    Settings set;
};

struct SaveData {
    std::uint32_t seed = 1;
    Economy eco;
    Progress prog;
    std::optional<std::array<double, 2>> pos;  // foreuse
    std::optional<double> fuel, hull;
    std::vector<std::uint8_t> tiles;     // W * H, vide si absent ou de mauvaise taille
    std::vector<std::uint8_t> explored;  // W * H, idem
    std::vector<std::uint8_t> mask;      // masque en bits (MW * MH / 8 octets, bit de poids faible d'abord), idem
    std::vector<Nugget> nuggets;         // positions au centième, vitesse nulle
};

std::string toJson(const SaveData& s);
std::optional<SaveData> fromJson(std::string_view text);  // nullopt si illisible ou si v != 1 ; champs manquants : valeurs neuves

double toFixed2(double v);  // +v.toFixed(2) en JS (moitiés exactes arrondies en s'éloignant de zéro)

// Fichier : écrit à côté puis renommé, pour ne jamais laisser une sauvegarde à moitié écrite
bool writeFileAtomic(const std::string& path, const std::string& text);
std::optional<std::string> readFile(const std::string& path);

}  // namespace filon
