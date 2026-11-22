// Filon : la foreuse dans la mine (terrain en pixels, physique, tourelle, forage, pépites, bras d'accroche, économie).
// Souris : la foreuse suit le curseur ; clic gauche maintenu : forer ; clic droit maintenu : bras d'accroche.
// Clavier : flèches ou ZQSD pour bouger, Espace pour forer, Maj pour le bras.
// Échap ou B : menu de la base (le jeu est en pause pendant le menu). N : nouveau monde. Fermer la fenêtre pour quitter.
// --capture fichier.png / --capture-menu fichier.png : parcours d'essai, image de l'écran, puis fin (vérification automatique).
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base_menu.hpp"
#include "drill_view.hpp"
#include "filon/core/drill.hpp"
#include "filon/core/economy.hpp"
#include "filon/core/nuggets.hpp"
#include "filon/core/random.hpp"
#include "filon/core/texture.hpp"
#include "raylib.h"
#include "rlgl.h"
#include "terrain_view.hpp"

namespace {

using namespace filon;

std::int64_t nowMs() {  // horloge réelle, pour la fonderie qui avance aussi jeu fermé
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

struct Game {
    World world;
    Terrain terrain;
    TerrainView view;
    std::vector<std::uint8_t> explored;
    Economy eco;
    Drill drill;
    std::vector<Nugget> nuggets;
    Rng rng;  // effets et chance (vitesse des pépites, filon double)
    explicit Game(std::uint32_t seed)
        : world(generate(seed)), terrain(world), view(world, terrain, textureSeed(seed)), explored(static_cast<std::size_t>(W * H), 0),
          rng(seed ^ 0x9e3779b9u) {
        for (int x = 0; x < W; ++x) explored[static_cast<std::size_t>(SURF * W + x)] = 1;
        eco.furnace.last = nowMs();
        spawnOnDeck(drill, drillStats(eco));
    }
};

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
    const char* capture = (mode == "--capture" || mode == "--capture-menu") ? argv[2] : nullptr;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Filon");
    SetExitKey(KEY_NULL);  // Échap ouvre le menu de la base
    SetTargetFPS(60);

    std::uint32_t seed = 1;
    auto game = std::make_unique<Game>(seed);
    BaseMenu menu;
    double camX = 0, camY = 0, time = 0, lastFull = -9;
    bool camInit = false;
    std::string toast;
    double toastT = 0;
    int frame = 0;
    auto say = [&](const std::string& m, double t = 2.8) {
        toast = m;
        toastT = t;
    };
    if (capture) {  // le parcours d'essai part du sol, pas du pont
        game->drill.x = 17.11;
        game->drill.y = SURF - PH - .001;
    }

    while (!WindowShouldClose()) {
        const double dt = capture ? 1.0 / 60 : std::min<double>(GetFrameTime(), .05);
        time += dt;
        ++frame;
        if (IsKeyPressed(KEY_N)) {
            seed = static_cast<std::uint32_t>(GetRandomValue(1, 2147483646));
            game = std::make_unique<Game>(seed);
            camInit = false;
        }
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_B)) menu.open = !menu.open;
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
            if (capture) {  // parcours d'essai : rampe vers la droite en forant, puis bras d'accroche vers le haut à gauche
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

            updateDrill(d, game->terrain, game->world, game->explored, stats, in, dt, ev);
            game->view.sync(game->terrain);
            if (const int lv = addXp(eco, game->terrain.xpGained)) say("Niveau " + std::to_string(eco.lvl) + " ! +" + std::to_string(lv) + " point de talent");
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
            if (up.cargoFull && time - lastFull > 2.5) {
                lastFull = time;
                say("Soute pleine : remonte vendre ou fondre.");
            }
            if (ev.strandedFuel) say("Panne sèche ! Remorquée à la base, la moitié de la soute est perdue.");
            if (ev.strandedHull) say("Foreuse détruite ! Rapatriée à la base, la soute est perdue.");
            if (ev.hardRock) say("Roche très dure : améliore ton foret à l'atelier.");
            if (ev.sealBump) say("Roche scellée : bats le gardien de cette couche.");
            if (ev.arrivedAtBase) say("Base : plein et réparations faits. Échap ou B pour vendre et améliorer.");
        }
        toastT -= dt;

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
        game->view.draw(static_cast<float>(camX), static_cast<float>(camY), scale, GetScreenWidth(), GetScreenHeight());

        Camera2D cam{};
        cam.target = {static_cast<float>(camX), static_cast<float>(camY)};
        cam.zoom = scale;
        BeginMode2D(cam);
        // brouillard : cases jamais éclairées
        const int x0 = std::max(0, static_cast<int>(camX / TS)), x1 = std::min(W - 1, static_cast<int>((camX + vwT) / TS) + 1);
        const int y0 = std::max(SURF, static_cast<int>(camY / TS)), y1 = std::min(H - 1, static_cast<int>((camY + vhT) / TS) + 1);
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
                if (!game->explored[static_cast<std::size_t>(y * W + x)]) DrawRectangle(x * TS, y * TS, TS, TS, Color{5, 4, 4, 255});
        // pont de la base, d'après le masque
        for (int sx = static_cast<int>(DX1 * 8); sx < DX2 * 8; ++sx)
            for (int sy = static_cast<int>(DECK * 8) - 8; sy < DECK * 8 + 4; ++sy)
                if (game->terrain.cellSolid(sx, sy)) DrawRectangle(sx * PX, sy * PX, PX, PX, Color{90, 99, 108, 255});
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
        if (const std::string m = menu.draw(eco, d, base); !m.empty()) say(m);
        if (toastT > 0) {
            const int tw = MeasureText(toast.c_str(), 18);
            DrawRectangle(GetScreenWidth() / 2 - tw / 2 - 14, 80, tw + 28, 32, Color{18, 13, 11, 235});
            DrawText(toast.c_str(), GetScreenWidth() / 2 - tw / 2, 87, 18, RAYWHITE);
        }
        if (!menu.open)
            DrawText("Souris : diriger   Clic gauche / Espace : forer   Clic droit / Maj : bras   Échap / B : base   N : nouveau monde", 12,
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

    game.reset();  // libère les textures avant de fermer la fenêtre
    CloseWindow();
    return 0;
}
