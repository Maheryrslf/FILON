#include "scene_view.hpp"

#include <algorithm>
#include <cmath>

#include "filon/core/random.hpp"
#include "filon/core/terrain.hpp"
#include "raylib.h"
#include "rlgl.h"

using namespace filon;

namespace {

constexpr int SPAWN = 17;  // case du panneau MINE
const Color INK{42, 22, 6, 255}, AMBER{255, 177, 59, 255}, CREAM{243, 230, 216, 255};

Color rgba(int r, int g, int b, double a) {
    return Color{static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), static_cast<unsigned char>(std::lround(255 * std::clamp(a, 0.0, 1.0)))};
}
void rect(double x, double y, double w, double h, Color c) {
    DrawRectangleRec({static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)}, c);
}
void label(const char* text, double cx, double baseline, int size, Color c) {  // texte centré, ligne de base comme fillText
    DrawText(text, static_cast<int>(cx) - MeasureText(text, size) / 2, static_cast<int>(baseline) - size + 1, size, c);
}
void building(double x, double w, double h, Color c, const char* name, double gy) {
    rect(x, gy - h, w, h, c);
    rect(x, gy - h, w, 5, rgba(0, 0, 0, .25));
    rect(x - 4, gy - h - 7, w + 8, 8, Color{43, 33, 28, 255});
    rect(x + w / 2 - 9, gy - 26, 18, 26, Color{29, 22, 18, 255});
    label(name, x + w / 2, gy - h + 17, 10, CREAM);
}

}  // namespace

void drawSurface(double camX, double vwT, double time, bool hatchOpen, bool furnaceBusy) {
    const double gy = SURF * TS, dy = DECK * TS;
    // collines au loin
    const Color hill{167, 122, 88, 255};
    const int xa = static_cast<int>(std::floor(camX / TS)) - 1, xb = static_cast<int>(std::ceil((camX + vwT) / TS)) + 1;
    auto top = [&](int x) { return static_cast<float>(gy - TS * (1.3 + std::sin(x * .45) * .6 + std::sin(x * .17) * .8)); };
    for (int x = xa; x < xb; ++x) {
        const float X0 = static_cast<float>(x * TS), X1 = static_cast<float>((x + 1) * TS), G = static_cast<float>(gy);
        DrawTriangle({X0, G}, {X1, top(x + 1)}, {X0, top(x)}, hill);
        DrawTriangle({X0, G}, {X1, G}, {X1, top(x + 1)}, hill);
    }
    // colonne d'accostage : faisceau du sol jusqu'à la trappe
    DrawRectangleGradientV(static_cast<int>(GX1 * TS), static_cast<int>(dy), static_cast<int>((GX2 - GX1) * TS), static_cast<int>(gy - dy),
                           rgba(255, 200, 90, .32 + .08 * std::sin(time * 3)), rgba(255, 200, 90, .04));
    // moteurs de sustentation
    for (double ex : {DX1 + 1.2, DX1 + 4.5, DX2 - 4.5, DX2 - 1.2}) {
        rect(ex * TS - 8, dy + 16, 16, 9, Color{58, 63, 70, 255});
        const float fl = static_cast<float>(7 + std::sin(time * 20 + ex) * 3), X = static_cast<float>(ex * TS), Y = static_cast<float>(dy + 25);
        DrawTriangle({X - 6, Y}, {X, Y + fl}, {X + 6, Y}, rgba(120, 200, 255, .75));
    }
    // pont métallique, de part et d'autre de la trappe
    for (const auto& [a, b] : {std::pair{DX1, GX1}, std::pair{GX2, DX2}}) {
        rect(a * TS, dy, (b - a) * TS, 16, Color{74, 82, 91, 255});
        rect(a * TS, dy + 12, (b - a) * TS, 4, Color{46, 50, 56, 255});
        for (double x = a * TS + 2; x < b * TS - 12; x += 16) rect(x, dy + 3, 12, 6, Color{58, 64, 72, 255});
    }
    for (double x : {GX1 * TS - 6, GX2 * TS})
        for (int k = 0; k < 2; ++k) rect(x, dy + k * 8, 6, 8, k ? INK : AMBER);
    const Color hatch{91, 99, 108, 255};
    if (hatchOpen) {  // battants ouverts
        rect(GX1 * TS, dy, 4, 18, hatch);
        rect(GX2 * TS - 4, dy, 4, 18, hatch);
    } else {
        rect(GX1 * TS, dy, (GX2 - GX1) * TS, 14, hatch);
        for (double x = GX1 * TS + 3; x < GX2 * TS - 4; x += 8) rect(x, dy + 3, 4, 8, INK);
    }
    for (const auto& [x, dir] : {std::pair{DX1 * TS, 1}, std::pair{DX2 * TS - 8, -1}}) {  // garde-corps
        for (int k = 0; k < 4; ++k) rect(x, dy - TS + k * 8, 8, 8, k % 2 ? INK : AMBER);
        const double rx = dir > 0 ? x : x - TS * .9 + 8;
        rect(rx, dy - TS, TS * .9, 3, Color{138, 147, 157, 255});
        rect(rx, dy - TS * .55, TS * .9, 3, Color{138, 147, 157, 255});
    }
    // bâtiments sur le pont
    building(5.4 * TS, 3.6 * TS, 2.2 * TS, Color{75, 90, 106, 255}, "ATELIER", dy);
    building(9.4 * TS, 2.6 * TS, 2 * TS, Color{122, 70, 50, 255}, "FONDERIE", dy);
    rect(11.2 * TS, dy - 3.2 * TS, .5 * TS, 1.3 * TS, Color{58, 42, 34, 255});  // cheminée
    if (furnaceBusy) rect(9.7 * TS, dy - 1.1 * TS, .6 * TS, .45 * TS, rgba(255, 150, 40, .6 + .3 * std::sin(time * 6)));
    building(15 * TS, 3.6 * TS, 1.9 * TS, Color{93, 107, 58, 255}, "COMPTOIR", dy);
    for (int i = 0; i < 8; ++i) {  // auvent rayé
        const double w = (3.6 * TS + 8) / 8;
        rect(15 * TS - 4 + i * w, dy - 1.9 * TS - 13, w, 9, i % 2 ? CREAM : Color{208, 83, 59, 255});
    }
    rect(19.3 * TS, dy - 30, 10, 30, Color{208, 83, 59, 255});  // pompe à carburant
    rect(19.3 * TS + 2, dy - 26, 6, 6, CREAM);
    // panneau MINE au sol
    rect(SPAWN * TS + 14, gy - 24, 4, 24, Color{90, 64, 48, 255});
    rect(SPAWN * TS + 4, gy - 34, 24, 12, AMBER);
    label("MINE", SPAWN * TS + 16, gy - 25, 10, INK);
}

void drawLava(const World& w, const std::vector<std::uint8_t>& explored, int x0, int x1, int y0, int y1, double time) {
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (w.at(x, y) != LAVA || !explored[static_cast<std::size_t>(y * W + x)]) continue;
            const std::uint32_t h = hash2(x, y);
            const double X = x * TS, Y = y * TS, k = .5 + .5 * std::sin(time * 2.5 + h % 7), top = Y + 8 + std::sin(time * 3 + x) * 1.5;
            DrawRectangleGradientV(static_cast<int>(X), static_cast<int>(top), TS, static_cast<int>(std::ceil(Y + TS - top)),
                                   Color{255, static_cast<unsigned char>(std::lround(170 + 50 * k)), 70, 255}, Color{194, 49, 14, 255});
            rect(X + (h % 22) + 2, top + 2 + k * 3, 5, 2, rgba(255, 240, 160, .8));
        }
}

void drawRadar(const World& w, const Drill& d, int radius, double light, double time) {
    if (!radius) return;
    for (int y = std::max(SURF, d.ty - radius); y <= std::min(H - 1, d.ty + radius); ++y)
        for (int x = std::max(0, d.tx - radius); x <= std::min(W - 1, d.tx + radius); ++x) {
            const std::uint8_t t = w.at(x, y);
            const double dist = std::hypot(x - d.tx, y - d.ty);
            if (!isOre(t) || dist > radius || dist < light - .5) continue;
            const float X = x * TS + 16.f, Y = y * TS + 16.f, s = static_cast<float>(4 + std::sin(time * 4 + x + y) * 1.2);
            const std::uint32_t g = ORES[t - ORE_FIRST].glint;
            const Color c = rgba(static_cast<int>(g >> 16 & 255), static_cast<int>(g >> 8 & 255), static_cast<int>(g & 255), .75);
            DrawTriangle({X, Y - s}, {X - s, Y}, {X + s, Y}, c);
            DrawTriangle({X - s, Y}, {X, Y + s}, {X + s, Y}, c);
        }
}

void drawDarkness(float sx, float sy, float sg, float r, bool bossAwake) {
    const int vw = GetScreenWidth(), vh = GetScreenHeight();
    if (sg >= vh) return;
    const int top = static_cast<int>(std::floor(sg));
    BeginScissorMode(0, top, vw, vh - top);
    // dégradé radial comme dans la version JS : transparent jusqu'à 0,25 r, 0,2 à 0,7 r, puis 0,66 (0,45 pendant un gardien)
    const unsigned char edge = bossAwake ? 115 : 168;
    const float rings[4] = {r * .25f, r * .7f, r, r + 2.f * (vw + vh)};
    const unsigned char alpha[4] = {0, 51, edge, edge};
    constexpr int N = 96;
    rlBegin(RL_TRIANGLES);
    for (int ring = 0; ring < 3; ++ring)
        for (int i = 0; i < N; ++i) {
            const float a0 = i * 2 * PI / N, a1 = (i + 1) * 2 * PI / N;
            const float ri = rings[ring], ro = rings[ring + 1];
            const Vector2 i0{sx + std::cos(a0) * ri, sy + std::sin(a0) * ri}, i1{sx + std::cos(a1) * ri, sy + std::sin(a1) * ri};
            const Vector2 o0{sx + std::cos(a0) * ro, sy + std::sin(a0) * ro}, o1{sx + std::cos(a1) * ro, sy + std::sin(a1) * ro};
            auto v = [&](Vector2 p, unsigned char a) {
                rlColor4ub(5, 4, 4, a);
                rlVertex2f(p.x, p.y);
            };
            v(i0, alpha[ring]);
            v(o0, alpha[ring + 1]);
            v(o1, alpha[ring + 1]);
            v(i0, alpha[ring]);
            v(o1, alpha[ring + 1]);
            v(i1, alpha[ring]);
        }
    rlEnd();
    EndScissorMode();
}
