// Aperçu du monde généré par filon-core : carte de toutes les cases, créatures en rouge.
// Molette ou flèches : défiler. N : nouveau monde. Échap : quitter.
#include <cstdint>
#include <string>

#include "filon/core/world.hpp"
#include "raylib.h"

namespace {

Color hex(std::uint32_t c) {
    return Color{static_cast<unsigned char>(c >> 16), static_cast<unsigned char>(c >> 8), static_cast<unsigned char>(c), 255};
}
Color scale(Color c, float k) {
    auto f = [k](unsigned char v) { const float r = v * k; return static_cast<unsigned char>(r > 255 ? 255 : r); };
    return Color{f(c.r), f(c.g), f(c.b), 255};
}

constexpr std::uint32_t LAYER_ROCK[filon::NL] = {0x5d412b, 0x7c4330, 0x3e414d, 0x52231c};

Color tileColor(std::uint8_t t, int y) {
    using namespace filon;
    if (y < SURF) return Color{47, 93, 138, 255};
    const Color rock = hex(LAYER_ROCK[layerOf(y)]);
    switch (t) {
        case AIR: return Color{28, 19, 12, 255};
        case SOFT: return scale(rock, 1.3f);
        case ROCK: return rock;
        case HARD: return scale(rock, .7f);
        case BED: return Color{13, 11, 10, 255};
        case SEAL: return Color{120, 70, 180, 255};
        case LAVA: return Color{255, 110, 30, 255};
        default: return hex(ORES[t - ORE_FIRST].glint);
    }
}

Texture2D mapTexture(const filon::World& w) {
    Image img = GenImageColor(filon::W, filon::H, BLACK);
    for (int y = 0; y < filon::H; ++y)
        for (int x = 0; x < filon::W; ++x) ImageDrawPixel(&img, x, y, tileColor(w.at(x, y), y));
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

}  // namespace

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Filon");
    SetTargetFPS(60);

    std::uint32_t seed = 1;
    filon::World world = filon::generate(seed);
    Texture2D map = mapTexture(world);
    constexpr float S = 6;  // pixels écran par case
    float scroll = 0;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_N)) {
            seed = static_cast<std::uint32_t>(GetRandomValue(1, 2147483646));
            world = filon::generate(seed);
            UnloadTexture(map);
            map = mapTexture(world);
            scroll = 0;
        }
        scroll -= GetMouseWheelMove() * 60;
        if (IsKeyDown(KEY_DOWN)) scroll += 12;
        if (IsKeyDown(KEY_UP)) scroll -= 12;
        const float maxScroll = filon::H * S - GetScreenHeight() + 40;
        scroll = scroll < 0 ? 0 : scroll > maxScroll ? maxScroll : scroll;

        BeginDrawing();
        ClearBackground(Color{18, 13, 11, 255});
        const float mx = 40, my = 20 - scroll;
        DrawTextureEx(map, Vector2{mx, my}, 0, S, WHITE);
        for (const auto& f : world.foes) DrawCircle(static_cast<int>(mx + (f.tx + .5f) * S), static_cast<int>(my + (f.ty + .5f) * S), 3, RED);
        for (int L = 0; L < filon::NL; ++L) {
            const int y = static_cast<int>(my + (filon::SURF + L * filon::LH) * S);
            DrawText(filon::LAYERS[L].name, static_cast<int>(mx + filon::W * S + 12), y + 4, 18, RAYWHITE);
        }

        const int px = 420;
        DrawText("FILON", px, 30, 64, Color{255, 177, 59, 255});
        DrawText(("Monde de la graine " + std::to_string(seed)).c_str(), px, 105, 22, RAYWHITE);
        DrawText((std::to_string(filon::W) + " x " + std::to_string(filon::H) + " cases, " + std::to_string(world.foes.size()) +
                  " créatures (points rouges)")
                     .c_str(),
                 px, 135, 18, LIGHTGRAY);
        DrawText("Molette ou flèches : défiler    N : nouveau monde    Échap : quitter", px, 165, 18, LIGHTGRAY);
        for (int i = 0; i < filon::ORE_COUNT; ++i) {
            DrawRectangle(px, 210 + i * 26, 18, 18, hex(filon::ORES[i].glint));
            DrawText(filon::ORES[i].name, px + 28, 210 + i * 26, 18, RAYWHITE);
        }
        DrawFPS(GetScreenWidth() - 100, 20);
        EndDrawing();
    }

    UnloadTexture(map);
    CloseWindow();
    return 0;
}
