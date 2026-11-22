// Parité des pépites avec la version JS : chute, rebonds, cratères, soute pleine, aimant et ramassage.
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "common.hpp"
#include "filon/core/drill.hpp"
#include "filon/core/nuggets.hpp"

int main() {
    std::ifstream file(std::string(FILON_TEST_DATA_DIR) + "/nuggets_1.txt");
    if (!file) {
        std::printf("ÉCHEC : nuggets_1.txt introuvable\n");
        return 1;
    }
    filon::World world = filon::generate(1);
    filon::Terrain terrain(world);
    for (double x : {24.0, 27.0, 30.0}) terrain.dig(x, 6.1, .47, 50);  // mêmes cratères que la référence

    std::vector<filon::Nugget> list;
    int cargoMax = 0, cargo = 0, frame = 0, pickups = 0;
    double px = 0, py = 0, maxDiff = 0;
    const double dt = 1.0 / 60;
    for (std::string line; std::getline(file, line);) {
        std::istringstream in(line);
        std::string kind;
        in >> kind;
        if (kind == "cargo") {
            cargoMax = static_cast<int>(test::num(in));
        } else if (kind == "spawn") {
            filon::Nugget n{};
            n.id = static_cast<std::uint8_t>(test::num(in));
            n.x = test::num(in);
            n.y = test::num(in);
            n.vx = test::num(in);
            n.vy = test::num(in);
            list.push_back(n);
        } else if (kind == "phase") {
            std::string name, frames, json;
            in >> name >> frames;
            px = test::num(in);
            py = test::num(in);
            in >> json;
            cargo = 0;  // somme des valeurs du JSON de la soute, par exemple {"10":10}
            for (std::size_t p = json.find(':'); p != std::string::npos; p = json.find(':', p + 1)) cargo += std::atoi(json.c_str() + p + 1);
        } else if (kind == "n") {
            ++frame;
            const filon::NuggetUpdate up = filon::updateNuggets(list, terrain, px + filon::PW / 2, py + filon::PH / 2, cargo, cargoMax, dt);
            pickups += static_cast<int>(up.collected.size());
            const int count = static_cast<int>(test::num(in)), total = static_cast<int>(test::num(in));
            if (count != static_cast<int>(list.size()) || total != cargo) {
                std::printf("ÉCHEC à l'image %d : %zu pépites et soute %d en C++, %d et %d en JS\n", frame, list.size(), cargo, count, total);
                return 1;
            }
            for (const filon::Nugget& n : list) {
                const double id = test::num(in), x = test::num(in), y = test::num(in), vx = test::num(in), vy = test::num(in);
                const double got[] = {static_cast<double>(n.id), n.x, n.y, n.vx, n.vy}, ref[] = {id, x, y, vx, vy};
                for (int i = 0; i < 5; ++i) {
                    maxDiff = std::max(maxDiff, std::fabs(got[i] - ref[i]));
                    if (!test::close(got[i], ref[i])) {
                        std::printf("ÉCHEC à l'image %d : pépite %d, valeur %d : C++ %.17g, JS %.17g\n", frame, static_cast<int>(id), i, got[i], ref[i]);
                        return 1;
                    }
                }
            }
        }
    }
    std::printf("Parité des pépites : OK (%d images, %d pépite(s) ramassée(s), %zu au sol, écart maximal %.1e)\n", frame, pickups, list.size(), maxDiff);
    return 0;
}
