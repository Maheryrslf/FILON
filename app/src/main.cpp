// Filon : la foreuse dans la mine (terrain en pixels, physique, tourelle, forage, pépites, bras d'accroche).
// Souris : la foreuse suit le curseur ; clic gauche maintenu : forer ; clic droit maintenu : bras d'accroche.
// Clavier : flèches ou ZQSD pour bouger, Espace pour forer, Maj pour le bras. N : nouveau monde. Échap : quitter.
// --capture fichier.png : parcours d'essai, image de l'écran, puis fin (vérification automatique).
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "drill_view.hpp"
#include "filon/core/drill.hpp"
#include "filon/core/nuggets.hpp"
#include "filon/core/random.hpp"
#include "filon/core/texture.hpp"
#include "raylib.h"
#include "rlgl.h"
#include "terrain_view.hpp"

namespace {

using namespace filon;

struct Game {
    World world;
    Terrain terrain;
    TerrainView view;
    std::vector<std::uint8_t> explored;
    DrillStats stats;
    Drill drill;
    std::vector<Nugget> nuggets;
    int cargo[ORE_COUNT] = {};  // minerais dans la soute, par type
    int cargoMax = 10;
    Rng rng;                    // effets seulement (vitesse de départ des pépites)
    int cargoCount() const {
        int n = 0;
        for (int c : cargo) n += c;
        return n;
    }
    explicit Game(std::uint32_t seed)
        : world(generate(seed)), terrain(world), view(world, terrain, textureSeed(seed)), explored(static_cast<std::size_t>(W * H), 0),
          rng(seed ^ 0x9e3779b9u) {
        for (int x = 0; x < W; ++x) explored[static_cast<std::size_t>(SURF * W + x)] = 1;
        spawnOnDeck(drill, stats);
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
    const char* capture = argc == 3 && std::string(argv[1]) == "--capture" ? argv[2] : nullptr;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Filon");
    SetTargetFPS(60);

    std::uint32_t seed = 1;
    auto game = std::make_unique<Game>(seed);
    double camX = 0, camY = 0, time = 0;
    bool camInit = false;
    std::string toast;
    double toastT = 0;
    int frame = 0;
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
        const float scale = std::clamp(std::min(GetScreenWidth(), GetScreenHeight()) / (10.5f * TS), .9f, 2.2f);
        Drill& d = game->drill;

        // commandes : clavier d'abord, sinon la souris (zone morte de 0,6 case autour de la foreuse)
        DrillInput in;
        const Vector2 m = GetMousePosition();
        const double pcx = d.x + PW / 2, pcy = d.y + PH / 2;
        in.cursor = IsCursorOnScreen();
        in.cdx = (camX + m.x / scale) / TS - pcx;
        in.cdy = (camY + m.y / scale) / TS - pcy;
        in.ix = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) - (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
        in.iy = (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) - (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
        if (!in.ix && !in.iy && in.cursor && std::hypot(in.cdx, in.cdy) > .6) octant(in.cdx, in.cdy, in.ix, in.iy);
        in.drill = IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_SPACE);
        in.grab = IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
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

        DrillEvents ev;
        updateDrill(d, game->terrain, game->world, game->explored, game->stats, in, dt, ev);
        game->view.sync(game->terrain);
        game->terrain.xpGained = 0;
        for (const FreedOre& f : game->terrain.freed)  // une pépite jaillit de chaque filon dégagé
            game->nuggets.push_back({f.id, f.x, f.y, (game->rng.next() - .5) * 2.5, -2 - game->rng.next() * 2});
        game->terrain.freed.clear();
        if (ev.strandedFuel)  // panne sèche : la moitié de la soute est perdue ; destruction : toute la soute
            for (int& c : game->cargo) c /= 2;
        if (ev.strandedHull)
            for (int& c : game->cargo) c = 0;
        {
            int n = game->cargoCount();
            const NuggetUpdate up = updateNuggets(game->nuggets, game->terrain, d.x + PW / 2, d.y + PH / 2, n, game->cargoMax, dt);
            for (std::uint8_t id : up.collected) {
                ++game->cargo[id - ORE_FIRST];
                toast = std::string("+1 ") + ORES[id - ORE_FIRST].name;
                toastT = 1.2;
            }
            static double lastFull = -9;
            if (up.cargoFull && time - lastFull > 2.5) {
                lastFull = time;
                toast = "Soute pleine : remonte vendre ou fondre.";
                toastT = 2.8;
            }
        }
        if (ev.strandedFuel) toast = "Panne sèche ! Remorquée à la base, la moitié de la soute est perdue.";
        if (ev.strandedHull) toast = "Foreuse détruite ! Rapatriée à la base, la soute est perdue.";
        if (ev.hardRock) toast = "Roche très dure : il faudra un meilleur foret.";
        if (ev.sealBump) toast = "Roche scellée : bats le gardien de cette couche.";
        if (ev.arrivedAtBase) toast = "Base : plein et réparations faits.";
        if (ev.strandedFuel || ev.strandedHull || ev.hardRock || ev.sealBump || ev.arrivedAtBase) toastT = 2.8;
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
        for (const Nugget& n : game->nuggets) {  // pépites : 2 x 2 pixels de terre, reflet et ombre
            const OreDef& o = ORES[n.id - ORE_FIRST];
            const float X = std::round(static_cast<float>(n.x * TS) / 2) * 2 - 4, Y = std::round(static_cast<float>(n.y * TS) / 2) * 2 - 4;
            DrawRectangleRec({X, Y, 8, 8}, Color{static_cast<unsigned char>(o.color >> 16), static_cast<unsigned char>(o.color >> 8), static_cast<unsigned char>(o.color), 255});
            DrawRectangleRec({X, Y, 4, 4}, Color{static_cast<unsigned char>(o.glint >> 16), static_cast<unsigned char>(o.glint >> 8), static_cast<unsigned char>(o.glint), 255});
            DrawRectangleRec({X + 4, Y + 4, 4, 4}, Color{0, 0, 0, 89});
        }
        drawArm(d);
        drawDrill(d, time);
        EndMode2D();

        bar(12, 12, 300, d.fuel / game->stats.fuelMax, Color{255, 177, 59, 255}, "CARBURANT " + std::to_string(static_cast<int>(std::ceil(std::max(0.0, d.fuel)))));
        bar(12, 36, 300, d.hull / game->stats.hullMax, Color{255, 90, 74, 255}, "COQUE " + std::to_string(static_cast<int>(std::ceil(std::max(0.0, d.hull)))));
        DrawText(d.under ? (std::to_string(std::max(0, ev.depth) * 2) + " m").c_str() : "Surface", 330, 14, 20, RAYWHITE);
        DrawText(("Soute " + std::to_string(game->cargoCount()) + "/" + std::to_string(game->cargoMax) + "    Graine " + std::to_string(seed)).c_str(), 330, 38,
                 16, game->cargoCount() >= game->cargoMax ? Color{255, 90, 74, 255} : LIGHTGRAY);
        if (toastT > 0) {
            const int tw = MeasureText(toast.c_str(), 18);
            DrawRectangle(GetScreenWidth() / 2 - tw / 2 - 14, 80, tw + 28, 32, Color{18, 13, 11, 235});
            DrawText(toast.c_str(), GetScreenWidth() / 2 - tw / 2, 87, 18, RAYWHITE);
        }
        DrawText("Souris : diriger   Clic gauche / Espace : forer   Clic droit / Maj : bras   Flèches / ZQSD : bouger   N : nouveau monde", 12, GetScreenHeight() - 24, 16,
                 Color{179, 157, 138, 255});
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
