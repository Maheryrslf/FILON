// Parité de la sauvegarde : une vraie sauvegarde écrite par la version JS (tools/export_reference.js, §8)
// doit se relire ici, redonner le même terrain, et se réécrire octet pour octet.
#include <cinttypes>
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "filon/core/json.hpp"
#include "filon/core/save.hpp"
#include "filon/core/terrain.hpp"

using namespace filon;

static int failures = 0;

static void check(bool ok, const std::string& msg) {
    if (!ok && ++failures <= 20) std::printf("ÉCHEC : %s\n", msg.c_str());
}

static std::string fnv(const std::vector<std::uint8_t>& bytes) {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (std::uint8_t b : bytes) {
        h ^= b;
        h *= 0x100000001b3ULL;
    }
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016" PRIx64, h);
    return buf;
}

int main() {
    const std::string dir = FILON_TEST_DATA_DIR;
    const auto text = readFile(dir + "/save_js.json");
    check(text.has_value(), "save_js.json introuvable");
    if (!text) return 1;
    std::map<std::string, std::string> want;
    {
        std::ifstream f(dir + "/save_js.txt");
        std::string line, kind, what, val;
        while (std::getline(f, line)) {  // « fnv <quoi> <empreinte> » ; la ligne « key ... » est ignorée
            std::istringstream in(line);
            if (in >> kind >> what >> val) want[what] = val;
        }
    }

    // lecture
    const auto s = fromJson(*text);
    check(s.has_value(), "sauvegarde JS illisible");
    if (!s) return 1;
    const Economy& e = s->eco;
    check(s->seed == 123456789, "graine");
    check(e.coins == 4321.5 && e.xp == 17.25 && e.lvl == 3, "pièces, xp, niveau");
    check(e.cargo[0] == 4 && e.cargo[3] == 1 && e.bars[1] == 2 && e.bars[8] == 1, "soute et lingots");
    check(e.furnace.queue.size() == 2 && e.furnace.queue[0] == std::pair<std::uint8_t, int>{12, 3} && e.furnace.progress == .75 && e.furnace.last == 1700000123456,
          "fonderie");
    check(e.up[POWER] == 3 && e.up[TANK] == 2 && e.up[ARM] == 1 && e.tal[LUCK] == 2, "améliorations et talents");
    check(s->prog.boss[0] == 1 && s->prog.dead.size() == 3 && s->prog.maxDepth == 37 && s->prog.play == 1234.5678, "progression");
    check(!s->prog.set.vib && s->prog.set.music == .3 && s->prog.set.stick == "dyn", "réglages");
    check(s->pos && (*s->pos)[0] == 18.83125 && s->fuel == 57.123456789 && s->hull == 41.5, "foreuse");
    check(s->nuggets.size() == 3 && s->nuggets[1].x == 20.13 && s->nuggets[1].y == 40.34 && s->nuggets[2].x == 1, "pépites");

    // terrain restauré comme boot() en JS
    World w = generate(s->seed);
    check(s->tiles.size() == w.tiles.size(), "taille des cases");
    w.tiles = s->tiles;
    check(fnv(w.tiles) == want["tiles"], "empreinte des cases");
    check(fnv(s->explored) == want["explored"], "empreinte de l'exploration");
    const Terrain t(w, s->mask);
    check(fnv(t.remaining()) == want["remaining"], "pixels restants par case");
    check(t.packMask() == s->mask, "masque relu puis recompressé");

    // réécriture octet pour octet, puis aller-retour d'une sauvegarde modifiée
    const std::string out = toJson(*s);
    check(out == *text, "réécriture différente de la sauvegarde JS");
    if (out != *text)
        for (std::size_t i = 0; i < out.size() && i < text->size(); ++i)
            if (out[i] != (*text)[i]) {
                std::printf("  premier écart à l'octet %zu : ...%s\n", i, out.substr(i > 40 ? i - 40 : 0, 80).c_str());
                break;
            }
    SaveData m = *s;
    m.eco.coins = 1e21;
    m.prog.play = 1e-7;
    m.pos.reset();
    m.fuel.reset();
    m.mask.clear();
    m.nuggets.push_back({18, -0.125, 2.675, 3, 4});
    m.prog.set.shake = false;
    const auto back = fromJson(toJson(m));
    check(back && toJson(*back) == toJson(m), "aller-retour");
    check(back && !back->pos && !back->fuel && back->mask.empty() && back->nuggets.back().x == -0.13, "valeurs absentes et arrondi négatif");
    check(back && !back->prog.set.shake && toJson(m).find(",\"shake\":false}") != std::string::npos, "secousses coupées gardées");
    check(s->prog.set.shake && toJson(*s).find("shake") == std::string::npos, "secousses actives par défaut, champ absent");

    // nombres écrits comme String(v) en JS
    const std::pair<double, const char*> nums[] = {{1e21, "1e+21"}, {1e20, "100000000000000000000"}, {1e-7, "1e-7"}, {1e-6, "0.000001"},
                                                   {-0.5, "-0.5"}, {123.456, "123.456"}, {-0.0, "0"}, {1.5e300, "1.5e+300"},
                                                   {0.1 + 0.2, "0.30000000000000004"}, {5e-324, "5e-324"}};
    for (const auto& [v, js] : nums) check(json::number(v) == js, std::string("nombre ") + js + " écrit " + json::number(v));

    // fichiers abîmés ou étrangers : refusés sans planter
    for (const char* bad : {"", "{", "null", "[1]", "{\"v\":2}", "{\"v\":1,}", "{\"v\":1} x", "\"\\ud800\""})
        check(!fromJson(bad), std::string("refus attendu : ") + bad);
    const auto partial = fromJson("{\"v\":1,\"seed\":7,\"tiles\":\"pas du base64!\",\"up\":{\"power\":999},\"nug\":[[3,1,1]]}");
    check(partial && partial->seed == 7 && partial->tiles.empty() && partial->eco.up[POWER] == UPGRADES[POWER].max && partial->nuggets.empty(),
          "sauvegarde partielle ou abîmée");

    std::printf("%s : sauvegarde JS de %zu caractères relue et réécrite\n", failures ? "ÉCHEC" : "OK", text->size());
    return failures ? 1 : 0;
}
