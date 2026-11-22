// Parité des textures de terre avec la version JS, pixel par pixel.
// Rejoue le forage de dig_1.txt, puis compare les blocs listés dans chunks_1.txt à chunks_1.bin.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "filon/core/texture.hpp"

int main() {
    const std::string dir = FILON_TEST_DATA_DIR;
    filon::World world = filon::generate(1);
    filon::Terrain terrain(world);

    {  // même forage que la référence
        std::ifstream f(dir + "/dig_1.txt");
        std::string line;
        while (std::getline(f, line)) {
            std::istringstream in(line);
            std::string kind, a, b, c, d;
            in >> kind;
            if (kind != "dig") continue;
            in >> a >> b >> c >> d;
            terrain.dig(std::strtod(a.c_str(), nullptr), std::strtod(b.c_str(), nullptr), std::strtod(c.c_str(), nullptr),
                        std::strtod(d.c_str(), nullptr));
        }
        terrain.setHatch(true);
        terrain.setHatch(false);
    }

    std::ifstream binf(dir + "/chunks_1.bin", std::ios::binary);
    const std::vector<unsigned char> ref{std::istreambuf_iterator<char>(binf), {}};
    std::ifstream list(dir + "/chunks_1.txt");
    const std::size_t size = filon::CHUNK_W * filon::CHUNK_H * 4;
    int cx, cy, n = 0, failures = 0;
    while (list >> cx >> cy) {
        const filon::Chunk c = filon::buildChunk(world, terrain, filon::textureSeed(1), cx, cy);
        int diff = 0, first = -1;
        for (std::size_t i = 0; i < size; ++i)
            if (c.rgba[i] != ref[n * size + i]) {
                if (first < 0) first = static_cast<int>(i);
                ++diff;
            }
        if (diff) {
            ++failures;
            const int px = first / 4;
            std::printf("ÉCHEC bloc (%d,%d) : %d octets différents ; premier au pixel (%d,%d) canal %d : C++ %d, JS %d\n", cx, cy, diff,
                        px % filon::CHUNK_W, px / filon::CHUNK_W, first % 4, c.rgba[static_cast<std::size_t>(first)], ref[n * size + static_cast<std::size_t>(first)]);
        }
        ++n;
    }
    if (n == 0 || ref.size() != n * size) {
        std::printf("ÉCHEC : données de référence incomplètes\n");
        return 1;
    }
    if (failures == 0) std::printf("Parité des textures : OK (%d blocs de 96 x 64 pixels identiques)\n", n);
    return failures == 0 ? 0 : 1;
}
