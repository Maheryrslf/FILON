// Banc d'essai du terrain : textures de terre en pixels, forage à la souris.
// Clic gauche maintenu : forer. Flèches ou clic droit glissé : se déplacer. Molette : zoom.
// T : ouvrir/fermer la trappe. N : nouveau monde. Échap : quitter.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>

#include "filon/core/texture.hpp"
#include "raylib.h"
#include "rlgl.h"
#include "terrain_view.hpp"

namespace {

struct Game {
    filon::World world;
    filon::Terrain terrain;
    TerrainView view;
    double xp = 0;
    int nuggets = 0;
    explicit Game(std::uint32_t seed)
        : world(filon::generate(seed)), terrain(world), view(world, terrain, filon::textureSeed(seed)) {}
};

}  // namespace

int main(int argc, char** argv) {
    // --capture fichier.png : creuse une galerie d'essai, enregistre une image de l'écran et quitte (vérification automatique)
    const char* capture = argc == 3 && std::string(argv[1]) == "--capture" ? argv[2] : nullptr;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Filon");
    SetTargetFPS(60);

    std::uint32_t seed = 1;
    auto game = std::make_unique<Game>(seed);
    float scale = 2.f;                              // pixels écran par pixel du monde
    float camX = 0, camY = (filon::SURF - 6) * filon::TS;  // coin haut-gauche, en pixels du monde
    int frame = 0;
    if (capture) {
        scale = 3.f;
        camX = 10 * filon::TS;
        camY = 1 * filon::TS;
        for (int k = 0; k < 30; ++k) game->terrain.dig(17.5 + k * .1, 6.3 + k * .12, .47, 50);
    }

    while (!WindowShouldClose()) {
        const bool shoot = capture && ++frame == 4;
        const float dt = std::min(GetFrameTime(), 0.05f);
        if (IsKeyPressed(KEY_N)) {
            seed = static_cast<std::uint32_t>(GetRandomValue(1, 2147483646));
            game = std::make_unique<Game>(seed);
        }
        if (IsKeyPressed(KEY_T)) game->terrain.setHatch(!game->terrain.hatchOpen());

        // caméra
        const float speed = 600 / scale;
        if (IsKeyDown(KEY_LEFT)) camX -= speed * dt;
        if (IsKeyDown(KEY_RIGHT)) camX += speed * dt;
        if (IsKeyDown(KEY_UP)) camY -= speed * dt;
        if (IsKeyDown(KEY_DOWN)) camY += speed * dt;
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            const Vector2 d = GetMouseDelta();
            camX -= d.x / scale;
            camY -= d.y / scale;
        }
        const float wheel = GetMouseWheelMove();
        if (wheel != 0) {  // zoom autour du curseur
            const Vector2 m = GetMousePosition();
            const float wx = camX + m.x / scale, wy = camY + m.y / scale;
            scale = std::clamp(scale * (wheel > 0 ? 1.25f : 0.8f), 0.5f, 6.f);
            camX = wx - m.x / scale;
            camY = wy - m.y / scale;
        }
        camY = std::clamp(camY, -4.f * filon::TS, static_cast<float>(filon::H * filon::TS) - GetScreenHeight() / scale);

        // forage au curseur : disque de 0,47 case, budget du foret de base (3 PV par seconde) x 10 pour l'essai
        const Vector2 m = GetMousePosition();
        const double mx = (camX + m.x / scale) / filon::TS, my = (camY + m.y / scale) / filon::TS;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            game->terrain.dig(mx, my, .47, 30.0 * dt);
            game->xp += game->terrain.xpGained;
            game->nuggets += static_cast<int>(game->terrain.freed.size());
            game->terrain.xpGained = 0;
            game->terrain.freed.clear();
        }
        game->view.sync(game->terrain);

        BeginDrawing();
        ClearBackground(Color{13, 11, 10, 255});
        const float skyBottom = (filon::SURF * filon::TS - camY) * scale;
        if (skyBottom > 0) DrawRectangleGradientV(0, 0, GetScreenWidth(), static_cast<int>(skyBottom), Color{47, 93, 138, 255}, Color{242, 183, 122, 255});
        game->view.draw(camX, camY, scale, GetScreenWidth(), GetScreenHeight());
        // pont de la base, dessiné d'après le masque (la trappe s'ouvre avec T)
        for (int sx = static_cast<int>(filon::DX1 * 8); sx < filon::DX2 * 8; ++sx)
            for (int sy = static_cast<int>(filon::DECK * 8) - 8; sy < filon::DECK * 8 + 4; ++sy)
                if (game->terrain.cellSolid(sx, sy))
                    DrawRectangle(static_cast<int>((sx * filon::PX - camX) * scale), static_cast<int>((sy * filon::PX - camY) * scale),
                                  static_cast<int>(std::ceil(filon::PX * scale)), static_cast<int>(std::ceil(filon::PX * scale)), Color{90, 99, 108, 255});
        DrawCircleLines(static_cast<int>(m.x), static_cast<int>(m.y), .47f * filon::TS * scale, Color{255, 177, 59, 200});

        DrawRectangle(10, 10, 560, 112, Color{0, 0, 0, 160});
        DrawText(("Graine " + std::to_string(seed) + "   profondeur " + std::to_string(std::max(0, static_cast<int>(my) - filon::SURF + 1) * 2) + " m")
                     .c_str(),
                 20, 18, 20, RAYWHITE);
        DrawText(("XP " + std::to_string(static_cast<int>(game->xp)) + "   pépites libérées " + std::to_string(game->nuggets)).c_str(), 20, 44, 20,
                 Color{255, 177, 59, 255});
        DrawText("Clic gauche : forer   Clic droit / flèches : bouger   Molette : zoom", 20, 72, 16, LIGHTGRAY);
        DrawText("T : trappe   N : nouveau monde   Échap : quitter", 20, 94, 16, LIGHTGRAY);
        DrawFPS(GetScreenWidth() - 100, 20);
        if (shoot) {  // envoie les dessins en attente à la carte graphique, puis lit l'image avant EndDrawing
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
