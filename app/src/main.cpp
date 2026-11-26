// Filon : la foreuse dans la mine (terrain en pixels, physique, tourelle, forage, pépites, bras d'accroche, économie,
// créatures et gardiens).
// Souris : la foreuse suit le curseur ; clic gauche maintenu : forer ; clic droit maintenu : bras d'accroche.
// Clavier : flèches ou ZQSD pour bouger, Espace pour forer, Maj pour le bras.
// Échap ou B : menu de la base (le jeu est en pause pendant le menu). N deux fois : nouveau monde, progression gardée.
// Sauvegarde au format de la version JS dans $XDG_DATA_HOME/filon/save.json (sinon ~/.local/share/filon/save.json) :
// toutes les 5 s de jeu, à la base, après un achat ou une vente, à la fermeture du menu et de la fenêtre.
// --capture / --capture-menu / --capture-boss / --capture-base fichier.png : parcours d'essai (ou salle du 2e gardien, ou pont
// de la base), image de l'écran, puis fin.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "base_menu.hpp"
#include "creature_view.hpp"
#include "drill_view.hpp"
#include "filon/core/creatures.hpp"
#include "filon/core/drill.hpp"
#include "filon/core/economy.hpp"
#include "filon/core/nuggets.hpp"
#include "filon/core/random.hpp"
#include "filon/core/save.hpp"
#include "filon/core/synth.hpp"
#include "filon/core/texture.hpp"
#include "raylib.h"
#include "rlgl.h"
#include "scene_view.hpp"
#include "terrain_view.hpp"

namespace {

using namespace filon;

std::int64_t nowMs() {  // horloge réelle, pour la fonderie qui avance aussi jeu fermé
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string savePath() {
    if (const char* x = std::getenv("XDG_DATA_HOME"); x && *x) return std::string(x) + "/filon/save.json";
    if (const char* h = std::getenv("HOME"); h && *h) return std::string(h) + "/.local/share/filon/save.json";
    return "filon-save.json";
}

World restoredWorld(const SaveData& s) {  // monde régénéré d'après la graine, puis les cases de la sauvegarde
    World w = generate(s.seed);
    if (!s.tiles.empty()) w.tiles = s.tiles;
    return w;
}

struct Game {  // une partie, restaurée comme boot() en JS
    std::uint32_t seed;
    World world;
    Terrain terrain;
    TerrainView view;
    std::vector<std::uint8_t> explored;
    Economy eco;
    Progress prog;
    Drill drill;
    std::vector<Nugget> nuggets;
    Rng rng;  // hasard du jeu : pépites, filon double, créatures, gardiens
    Creatures creatures;
    explicit Game(const SaveData& s)
        : seed(s.seed), world(restoredWorld(s)), terrain(world, s.mask), view(world, terrain, textureSeed(s.seed)),
          explored(s.explored.empty() ? std::vector<std::uint8_t>(static_cast<std::size_t>(W * H), 0) : s.explored), eco(s.eco), prog(s.prog),
          nuggets(s.nuggets), rng(s.seed ^ 0x9e3779b9u), creatures(makeCreatures(world, s.prog.dead, s.prog.boss, rng)) {
        for (int x = 0; x < W; ++x) explored[static_cast<std::size_t>(SURF * W + x)] = 1;
        if (!eco.furnace.last) eco.furnace.last = nowMs();
        const DrillStats st = drillStats(eco);
        spawnOnDeck(drill, st);
        if (s.pos && (*s.pos)[1] >= DECK - 3 && !terrain.boxHits((*s.pos)[0], (*s.pos)[1], PW, PH)) {  // sinon retour sur le pont
            drill.x = (*s.pos)[0];
            drill.y = (*s.pos)[1];
        }
        drill.under = drill.y + PH > SURF + .05;
        drill.fuel = s.fuel.value_or(st.fuelMax);
        drill.hull = s.hull.value_or(st.hullMax);
    }
    SaveData snapshot() const {
        SaveData s;
        s.seed = seed;
        s.eco = eco;
        s.prog = prog;
        s.pos = std::array<double, 2>{drill.x, drill.y};
        s.fuel = drill.fuel;
        s.hull = drill.hull;
        s.tiles = world.tiles;
        s.explored = explored;
        s.mask = terrain.packMask();
        s.nuggets = nuggets;
        return s;
    }
};

struct Float {  // chiffre qui monte au-dessus d'un coup porté
    double x, y;
    std::string text;
    Color color;
    double life;
};

Synth* gSynth = nullptr;  // lu par le fil audio de raylib
void audioCallback(void* buffer, unsigned int frames) {
    if (gSynth) gSynth->render(static_cast<float*>(buffer), static_cast<int>(frames));
}

std::uint32_t randomSeed() { return static_cast<std::uint32_t>(GetRandomValue(0, 2147483646)); }

SaveData freshSave(std::uint32_t seed) {
    SaveData s;
    s.seed = seed;
    return s;
}

// 8 directions : la plus proche de l'angle (dx, dy)
void octant(double dx, double dy, int& ix, int& iy) {
    static constexpr int OCT[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
    const int o = (static_cast<int>(std::lround(std::atan2(dy, dx) / (PI / 4))) + 8) % 8;
    ix = OCT[o][0];
    iy = OCT[o][1];
}

void bar(int x, int y, int w, double frac, Color c, const std::string& label) {
    DrawRectangle(x, y, w, 18, Color{0, 0, 0, 153});
    DrawRectangle(x, y, static_cast<int>(w * std::clamp(frac, 0.0, 1.0)), 18, c);
    DrawRectangleLines(x, y, w, 18, Color{67, 51, 42, 255});
    DrawText(label.c_str(), x + 7, y + 4, 10, RAYWHITE);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string mode = argc == 3 ? argv[1] : "";
    const char* capture = (mode == "--capture" || mode == "--capture-menu" || mode == "--capture-boss" || mode == "--capture-base") ? argv[2] : nullptr;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Filon");
    rlDisableBackfaceCulling();  // jeu en 2D : triangles tracés dans les deux sens (retournements, dégradés)
    SetExitKey(KEY_NULL);  // Échap ouvre le menu de la base
    SetTargetFPS(60);

    // son : synthétisé à la volée par le fil audio (rien pendant les captures d'essai)
    Synth synth(44100, randomSeed());
    AudioStream stream{};
    if (!capture) {
        InitAudioDevice();
        if (IsAudioDeviceReady()) {
            SetAudioStreamBufferSizeDefault(1024);
            stream = LoadAudioStream(44100, 32, 1);
            gSynth = &synth;
            SetAudioStreamCallback(stream, audioCallback);
            PlayAudioStream(stream);
        }
    }
    auto play = [&](Sfx s) {
        if (gSynth) synth.sfx(s);
    };
    std::string toast;
    double toastT = 0;
    auto say = [&](const std::string& m, double t = 2.8) {
        toast = m;
        toastT = t;
    };
    // le parcours d'essai part toujours du même monde neuf et ne touche pas à la sauvegarde
    const std::string path = capture ? "" : savePath();
    std::unique_ptr<Game> game;
    if (!capture) {
        if (const auto text = readFile(path)) {
            if (const auto s = fromJson(*text)) {
                game = std::make_unique<Game>(*s);
                say("Partie reprise.");
            } else {  // illisible : mise de côté plutôt qu'écrasée
                std::rename(path.c_str(), (path + ".illisible").c_str());
                say("Sauvegarde illisible, mise de côté (save.json.illisible). Nouvelle partie.", 5);
            }
        }
    }
    if (!game) game = std::make_unique<Game>(freshSave(capture ? 1 : randomSeed()));
    auto save = [&] {
        if (!path.empty() && !writeFileAtomic(path, toJson(game->snapshot()))) say("Impossible d'enregistrer la partie dans " + path, 4);
    };
    BaseMenu menu;
    double camX = 0, camY = 0, time = 0, lastFull = -9, lastSave = 0, confirmWorld = -9, lastBump = -9;
    bool camInit = false;
    int frame = 0;
    std::vector<Float> floats;
    if (capture) {  // le parcours d'essai part du sol, pas du pont
        game->drill.x = 17.11;
        game->drill.y = SURF - PH - .001;
    }
    const bool bossTest = mode == "--capture-boss", baseTest = mode == "--capture-base";
    if (baseTest) spawnOnDeck(game->drill, drillStats(game->eco));
    if (bossTest) {  // dans la salle du 2e gardien, coque renforcée, sans bouger
        const Chamber c1 = chamber(1);
        game->drill.x = c1.x1 + 1.5;
        game->drill.y = c1.y2 + 1 - PH - .001;
        game->drill.hull = 1e6;
        for (int y = c1.y1; y <= c1.y2; ++y)
            for (int x = c1.x1; x <= c1.x2; ++x) game->explored[static_cast<std::size_t>(y * W + x)] = 1;
        for (int k = 0; k < NL; ++k)  // une créature de chaque couche, pour voir leur dessin
            for (Foe& fo : game->creatures.foes)
                if (fo.k == k) {
                    fo.x = c1.x1 + 4 + 2.5 * k;
                    fo.y = c1.y2 + 1 - FH - .001;
                    break;
                }
    }

    while (!WindowShouldClose()) {
        const double dt = capture ? 1.0 / 60 : std::min<double>(GetFrameTime(), .05);
        time += dt;
        ++frame;
        if (IsKeyPressed(KEY_N)) {
            if (time - confirmWorld < 3) {  // comme newWorld() en JS : garde pièces, améliorations, talents, niveau, soute, lingots et réglages
                SaveData s = game->snapshot();
                s.seed = randomSeed();
                s.prog.dead.clear();
                s.prog.boss = {};
                s.prog.seenL = 0;
                s.pos.reset();
                s.fuel.reset();
                s.hull.reset();
                s.tiles.clear();
                s.explored.clear();
                s.mask.clear();
                s.nuggets.clear();
                game = std::make_unique<Game>(s);
                camInit = false;
                confirmWorld = -9;
                save();
                say("Nouveau monde ! Ta progression est gardée.");
            } else {
                confirmWorld = time;
                say("Appuie encore sur N pour un nouveau monde (ta progression est gardée).", 3);
            }
        }
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_B)) {
            menu.open = !menu.open;
            if (!menu.open) save();
        }
        const float scale = std::clamp(std::min(GetScreenWidth(), GetScreenHeight()) / (10.5f * TS), .9f, 2.2f);
        Drill& d = game->drill;
        Economy& eco = game->eco;
        const DrillStats stats = drillStats(eco);
        const Rectangle menuBtn{static_cast<float>(GetScreenWidth() - 120), 40, 104, 40};
        const bool overUi = menu.open || CheckCollisionPointRec(GetMousePosition(), menuBtn);
        updateFurnace(eco, nowMs());

        DrillEvents ev;
        if (!menu.open) {  // le menu met le jeu en pause
            // commandes : clavier d'abord, sinon la souris (zone morte de 0,6 case autour de la foreuse)
            DrillInput in;
            const Vector2 m = GetMousePosition();
            const double pcx = d.x + PW / 2, pcy = d.y + PH / 2;
            in.cursor = IsCursorOnScreen() && !overUi;
            in.cdx = (camX + m.x / scale) / TS - pcx;
            in.cdy = (camY + m.y / scale) / TS - pcy;
            in.ix = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) - (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
            in.iy = (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) - (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
            if (!in.ix && !in.iy && in.cursor && std::hypot(in.cdx, in.cdy) > .6) octant(in.cdx, in.cdy, in.ix, in.iy);
            in.drill = (!overUi && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) || IsKeyDown(KEY_SPACE);
            in.grab = (!overUi && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) || IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            if (bossTest || baseTest) in = DrillInput{};
            else if (capture) {  // parcours d'essai : rampe vers la droite en forant, puis bras d'accroche vers le haut à gauche
                in = DrillInput{};
                in.cursor = true;
                if (frame < 240) {
                    in.ix = 1;
                    in.iy = 1;
                    in.cdx = in.cdy = .7;
                    in.drill = true;
                } else {
                    in.cdx = -1;
                    in.cdy = -1.2;
                    in.grab = true;
                }
            }

            CreatureEvents cev;
            const double ramDmg = effects(eco).ram;
            updateDrill(d, game->terrain, game->world, game->explored, stats, in, dt, ev,
                        [&](Drill& dr, double ux, double uy) { return ram(game->creatures, dr, ux, uy, ramDmg, cev); });
            if (ev.strandedFuel || ev.strandedHull) resetActiveBosses(game->creatures);
            updateCreatures(game->creatures, d, game->terrain, stats, dt, game->rng, cev, ev);
            for (const CreatureEvents::Hit& h : cev.hits) floats.push_back({h.x, h.y, "-" + std::to_string(static_cast<long long>(std::lround(h.dmg))), WHITE, 1.2});
            int gained = 0;
            for (int i : cev.killedFoes) {
                const Foe& fo = game->creatures.foes[static_cast<std::size_t>(i)];
                const FoeDef& F = FOES[static_cast<std::size_t>(fo.k)];
                eco.coins += F.coins;
                gained += addXp(eco, F.xp);
                game->prog.dead.push_back(fo.i);
                floats.push_back({fo.x + FW / 2, fo.y + .2, "+" + std::to_string(F.coins), Color{255, 177, 59, 255}, 1.2});
            }
            if (cev.bossWoke >= 0) say(std::string(BOSSES[static_cast<std::size_t>(cev.bossWoke)].name) + " se réveille !");
            if (cev.bossKilled >= 0) {
                const int L = cev.bossKilled;
                const BossDef& B = BOSSES[static_cast<std::size_t>(L)];
                eco.coins += B.coins;
                gained += addXp(eco, B.xp);
                game->prog.boss[static_cast<std::size_t>(L)] = 1;
                breakSeal(game->world, L);
                say(std::string(B.name) + (L < NL - 1 ? " vaincu ! +" + std::to_string(B.coins) + " pièces. Le sceau est brisé."
                                                      : " vaincu ! Tu as atteint le fond du monde. Bravo !"), 4);
            }
            if (gained) say("Niveau " + std::to_string(eco.lvl) + " ! +" + std::to_string(gained) + " point de talent");
            if (!cev.killedFoes.empty() || cev.bossKilled >= 0) save();
            // bruitages, aux mêmes moments que dans la version JS
            if (ev.digSound) play(Sfx::Dig);
            if (!cev.hits.empty()) play(Sfx::Hit);
            if (!cev.killedFoes.empty()) play(Sfx::Kill);
            if (cev.bossWoke >= 0) play(Sfx::Roar);
            if (cev.bossKilled >= 0) play(Sfx::Win);
            if (cev.bossLanded || ev.landImpact > 0) play(Sfx::Land);
            if (cev.shots) play(Sfx::Shoot);
            if (ev.damage > 0) play(Sfx::Hurt);
            if (ev.strandedFuel || ev.strandedHull) play(Sfx::Fail);
            if (ev.hatchToggled || ev.armHooked) play(Sfx::Bump);
            if (ev.bump && time - lastBump > 2) {  // foret bloqué par la roche mère ou un sceau
                lastBump = time;
                play(Sfx::Bump);
            }
            if (gained) play(Sfx::Lvl);
            Progress& pr = game->prog;
            pr.play += dt;
            if (ev.depth > pr.maxDepth) pr.maxDepth = ev.depth;
            if (const int L = layerOf(static_cast<int>(std::floor(d.y + PH / 2))); L > pr.seenL) {
                pr.seenL = L;
                say("Couche " + std::to_string(L + 1) + " : " + LAYERS[static_cast<std::size_t>(L)].name);
            }
            if (ev.fellHard && !pr.fallTip) {
                pr.fallTip = 1;
                say("Chute rude : garde le propulseur appuyé pour freiner, ou accroche le bras.");
            }
            game->view.sync(game->terrain);
            if (const int lv = addXp(eco, game->terrain.xpGained)) {
                say("Niveau " + std::to_string(eco.lvl) + " ! +" + std::to_string(lv) + " point de talent");
                play(Sfx::Lvl);
            }
            game->terrain.xpGained = 0;
            for (const FreedOre& f : game->terrain.freed) {  // une pépite jaillit de chaque filon dégagé (deux avec Filon double)
                const int n = 1 + (game->rng.next() < .06 * eco.tal[LUCK] ? 1 : 0);
                for (int k = 0; k < n; ++k) game->nuggets.push_back({f.id, f.x, f.y, (game->rng.next() - .5) * 2.5, -2 - game->rng.next() * 2});
            }
            game->terrain.freed.clear();
            if (ev.strandedFuel)  // panne sèche : la moitié de la soute est perdue ; destruction : toute la soute
                for (int& c : eco.cargo) c /= 2;
            if (ev.strandedHull)
                for (int& c : eco.cargo) c = 0;
            int n = cargoCount(eco);
            const NuggetUpdate up = updateNuggets(game->nuggets, game->terrain, d.x + PW / 2, d.y + PH / 2, n, effects(eco).cargo, dt);
            for (std::uint8_t id : up.collected) {
                ++eco.cargo[id - ORE_FIRST];
                say(std::string("+1 ") + ORES[id - ORE_FIRST].name, 1.2);
            }
            if (!up.collected.empty()) play(Sfx::Ore);
            if (up.cargoFull && time - lastFull > 2.5) {
                lastFull = time;
                say("Soute pleine : remonte vendre ou fondre.");
                play(Sfx::Bump);
            }
            if (ev.strandedFuel) say("Panne sèche ! Remorquée à la base, la moitié de la soute est perdue.");
            if (ev.strandedHull) say("Foreuse détruite ! Rapatriée à la base, la soute est perdue.");
            if (ev.hardRock) say("Roche très dure : améliore ton foret à l'atelier.");
            if (ev.sealBump) say("Roche scellée : bats le gardien de cette couche.");
            if (ev.arrivedAtBase) say("Base : plein et réparations faits. Échap ou B pour vendre et améliorer.");
            if (ev.arrivedAtBase || time - lastSave > 5) {
                lastSave = time;
                save();
            }
        }
        toastT -= dt;
        if (gSynth) {  // volumes du menu Options, musique plus rapide pendant un combat de gardien
            synth.setVolumes(game->prog.set.music, game->prog.set.sfx);
            synth.music(std::any_of(game->creatures.bosses.begin(), game->creatures.bosses.end(), [](const Boss& b) { return b.active; }));
        }
        if (!menu.open) {
            for (Float& fl : floats) {
                fl.y -= .9 * dt;
                fl.life -= dt;
            }
            std::erase_if(floats, [](const Float& fl) { return fl.life <= 0; });
        }

        // caméra amortie (plus douce à la verticale, à cause des marches de pixels)
        const double vwT = GetScreenWidth() / scale, vhT = GetScreenHeight() / scale;
        double tx = (d.x + PW / 2) * TS - vwT / 2, ty = (d.y + PH / 2) * TS - vhT * .45;
        tx = W * TS <= vwT ? (W * TS - vwT) / 2 : std::clamp(tx, 0.0, W * TS - vwT);
        ty = std::clamp(ty, -3.0 * TS, H * TS - vhT);
        if (!camInit || std::fabs(tx - camX) > 6 * TS || std::fabs(ty - camY) > 6 * TS) {
            camX = tx;
            camY = ty;
            camInit = true;
        } else {
            camX += (tx - camX) * (1 - std::exp(-dt * 10));
            camY += (ty - camY) * (1 - std::exp(-dt * 6));
        }

        BeginDrawing();
        ClearBackground(Color{13, 11, 10, 255});
        const float skyBottom = static_cast<float>((SURF * TS - camY) * scale);
        if (skyBottom > 0) DrawRectangleGradientV(0, 0, GetScreenWidth(), static_cast<int>(skyBottom), Color{47, 93, 138, 255}, Color{242, 183, 122, 255});
        Camera2D cam{};
        cam.target = {static_cast<float>(camX), static_cast<float>(camY)};
        cam.zoom = scale;
        if (skyBottom > 0) {  // décor de la surface et base flottante, derrière le terrain
            BeginMode2D(cam);
            drawSurface(camX, vwT, time, game->terrain.hatchOpen(), !eco.furnace.queue.empty());
            EndMode2D();
        }
        game->view.draw(static_cast<float>(camX), static_cast<float>(camY), scale, GetScreenWidth(), GetScreenHeight());

        BeginMode2D(cam);
        // brouillard : cases jamais éclairées
        const int x0 = std::max(0, static_cast<int>(camX / TS)), x1 = std::min(W - 1, static_cast<int>((camX + vwT) / TS) + 1);
        const int y0 = std::max(SURF, static_cast<int>(camY / TS)), y1 = std::min(H - 1, static_cast<int>((camY + vhT) / TS) + 1);
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
                if (!game->explored[static_cast<std::size_t>(y * W + x)]) DrawRectangle(x * TS, y * TS, TS, TS, Color{5, 4, 4, 255});
        drawLava(game->world, game->explored, x0, x1, y0, y1, time);
        drawSeals(game->world, game->explored, x0, x1, y0, y1, time);
        for (const Foe& fo : game->creatures.foes)
            if (fo.alive && fo.x >= x0 - 1 && fo.x <= x1 + 1 && fo.y >= y0 - 1 && fo.y <= y1 + 1) drawFoe(fo, d, time);
        bool bossAwake = false;
        for (const Boss& bo : game->creatures.bosses) {
            if (!bo.dead && bo.y >= y0 - 3 && bo.y <= y1 + 3) drawBoss(bo, d, time);
            bossAwake = bossAwake || bo.active;
        }
        drawBullets(game->creatures);
        for (const Nugget& nu : game->nuggets) {  // pépites : 2 x 2 pixels de terre, reflet et ombre
            const OreDef& o = ORES[nu.id - ORE_FIRST];
            const float X = std::round(static_cast<float>(nu.x * TS) / 2) * 2 - 4, Y = std::round(static_cast<float>(nu.y * TS) / 2) * 2 - 4;
            DrawRectangleRec({X, Y, 8, 8}, Color{static_cast<unsigned char>(o.color >> 16), static_cast<unsigned char>(o.color >> 8), static_cast<unsigned char>(o.color), 255});
            DrawRectangleRec({X, Y, 4, 4}, Color{static_cast<unsigned char>(o.glint >> 16), static_cast<unsigned char>(o.glint >> 8), static_cast<unsigned char>(o.glint), 255});
            DrawRectangleRec({X + 4, Y + 4, 4, 4}, Color{0, 0, 0, 89});
        }
        drawArm(d);
        drawDrill(d, time);
        EndMode2D();

        // obscurité autour de la foreuse, sous la surface, puis radar et chiffres par-dessus
        drawDarkness(static_cast<float>(((d.x + PW / 2) * TS - camX) * scale), static_cast<float>(((d.y + PH / 2) * TS - camY) * scale),
                     std::max(0.f, skyBottom), static_cast<float>(stats.light * TS * scale), bossAwake);
        BeginMode2D(cam);
        drawRadar(game->world, d, static_cast<int>(effects(eco).radar), stats.light, time);
        for (const Float& fl : floats) {
            const unsigned char a = static_cast<unsigned char>(255 * std::clamp(fl.life * 1.5, 0.0, 1.0));
            const int fx = static_cast<int>(fl.x * TS) - MeasureText(fl.text.c_str(), 12) / 2, fy = static_cast<int>(fl.y * TS);
            DrawText(fl.text.c_str(), fx + 1, fy + 1, 12, Color{0, 0, 0, a});
            DrawText(fl.text.c_str(), fx, fy, 12, Color{fl.color.r, fl.color.g, fl.color.b, a});
        }
        EndMode2D();

        if (!menu.open) drawBossBar(game->creatures);
        // relevés
        bar(12, 12, 300, d.fuel / stats.fuelMax, Color{255, 177, 59, 255}, "CARBURANT " + std::to_string(static_cast<int>(std::ceil(std::max(0.0, d.fuel)))));
        bar(12, 36, 300, d.hull / stats.hullMax, Color{255, 90, 74, 255}, "COQUE " + std::to_string(static_cast<int>(std::ceil(std::max(0.0, d.hull)))));
        const int depth = static_cast<int>(std::floor(d.y + PH)) - SURF + 1;  // calculée ici : la simulation est en pause pendant le menu
        DrawText(d.under ? (std::to_string(std::max(0, depth) * 2) + " m").c_str() : "Surface", 330, 14, 20, RAYWHITE);
        const int cargoMax = effects(eco).cargo;
        DrawText(("Soute " + std::to_string(cargoCount(eco)) + "/" + std::to_string(cargoMax)).c_str(), 330, 38, 16,
                 cargoCount(eco) >= cargoMax ? Color{255, 90, 74, 255} : LIGHTGRAY);
        DrawText((std::to_string(static_cast<long long>(eco.coins)) + " pièces   Niv. " + std::to_string(eco.lvl)).c_str(), 450, 38, 16, Color{255, 177, 59, 255});
        // bouton Base / Menu, en haut à droite
        const bool base = atBase(d);
        const bool hover = CheckCollisionPointRec(GetMousePosition(), menuBtn);
        DrawRectangleRounded(menuBtn, .5f, 8, base ? Color{255, 177, 59, 255} : (hover ? Color{60, 46, 38, 240} : Color{30, 22, 19, 225}));
        const char* label = base ? "BASE" : "MENU";
        DrawText(label, static_cast<int>(menuBtn.x + (menuBtn.width - MeasureText(label, 18)) / 2), static_cast<int>(menuBtn.y + 11), 18,
                 base ? Color{42, 22, 6, 255} : RAYWHITE);
        if (!menu.open && hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) menu.open = true;

        if (capture && mode == "--capture-menu" && frame == 290) {  // menu d'essai : un peu de minerai et de pièces
            eco.coins = 1234;
            eco.cargo[1] = 3;
            eco.cargo[4] = 2;
            eco.bars[2] = 4;
            menu.open = true;
            menu.tab = 2;
        }
        const bool wasOpen = menu.open;
        if (const std::string m = menu.draw(eco, d, base, game->prog); !m.empty()) {
            say(m);
            save();  // vente, fonte, achat
        }
        if (menu.action == MenuAction::Sell) play(Sfx::Sell);
        if (menu.action == MenuAction::Smelt || menu.action == MenuAction::Buy) play(Sfx::Buy);
        if (wasOpen && !menu.open) save();
        if (toastT > 0) {
            const int tw = MeasureText(toast.c_str(), 18);
            DrawRectangle(GetScreenWidth() / 2 - tw / 2 - 14, 80, tw + 28, 32, Color{18, 13, 11, 235});
            DrawText(toast.c_str(), GetScreenWidth() / 2 - tw / 2, 87, 18, RAYWHITE);
        }
        if (!menu.open)
            DrawText("Souris : diriger   Clic gauche / Espace : forer   Clic droit / Maj : bras   Échap / B : base   N N : nouveau monde", 12,
                     GetScreenHeight() - 24, 16, Color{179, 157, 138, 255});
        DrawFPS(GetScreenWidth() - 100, 12);
        const bool shoot = capture && frame == 300;
        if (shoot) {
            rlDrawRenderBatchActive();
            TakeScreenshot(capture);
        }
        EndDrawing();
        if (shoot) break;
    }

    save();
    if (gSynth) {
        StopAudioStream(stream);
        UnloadAudioStream(stream);
        gSynth = nullptr;
        CloseAudioDevice();
    }
    game.reset();  // libère les textures avant de fermer la fenêtre
    CloseWindow();
    return 0;
}
