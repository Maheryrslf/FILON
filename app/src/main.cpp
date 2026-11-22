// Squelette : ouvre la fenêtre et affiche une texture de bruit calculée par filon-core,
// pour vérifier que le cœur et raylib sont bien reliés. Échap pour quitter.
#include "filon/core/random.hpp"
#include "raylib.h"

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Filon");
    SetTargetFPS(60);

    // 96 x 64 pixels de terre (un bloc de texture), teintés comme les Terres brunes
    Image img = GenImageColor(96, 64, BLACK);
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 96; ++x) {
            const double k = filon::fbm(x * 4 * .05, y * 4 * .05, 1, 3);
            ImageDrawPixel(&img, x, y, Color{static_cast<unsigned char>(78 + 68 * k), static_cast<unsigned char>(62 + 62 * k),
                                             static_cast<unsigned char>(48 + 52 * k), 255});
        }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(Color{18, 13, 11, 255});
        DrawText("FILON", 40, 30, 64, Color{255, 177, 59, 255});
        DrawText("Squelette C++ : raylib 6.0 + filon-core", 40, 105, 20, RAYWHITE);
        DrawTextureEx(tex, Vector2{40, 150}, 0, 6, WHITE);
        DrawFPS(GetScreenWidth() - 100, 20);
        EndDrawing();
    }

    UnloadTexture(tex);
    CloseWindow();
    return 0;
}
