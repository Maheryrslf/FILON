#include "creature_view.hpp"

#include <algorithm>
#include <cmath>

#include "raylib.h"
#include "rlgl.h"

using namespace filon;

namespace {

Color hex(std::uint32_t c, unsigned char a = 255) {
    return Color{static_cast<unsigned char>(c >> 16), static_cast<unsigned char>(c >> 8), static_cast<unsigned char>(c), a};
}
void circle(float x, float y, float r, Color c) { DrawCircleV({x, y}, r, c); }
void eyes(float x, float s) {
    circle(x, -s, 2.6f, WHITE);
    circle(x + .9f, -s, 1.3f, Color{17, 17, 17, 255});
    circle(x, s, 2.6f, WHITE);
    circle(x + .9f, s, 1.3f, Color{17, 17, 17, 255});
}
void ellipse(float rx, float ry, Color c) {  // ellipse centrée sur l'origine courante
    rlPushMatrix();
    rlScalef(rx, ry, 1);
    DrawCircleV({0, 0}, 1, c);
    rlPopMatrix();
}
void tri(Vector2 a, Vector2 b, Vector2 c, Color col) { DrawTriangle(a, b, c, col); }

}  // namespace

void drawSeals(const World& w, const std::vector<std::uint8_t>& explored, int x0, int x1, int y0, int y1, double time) {
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (w.at(x, y) != SEAL || !explored[static_cast<std::size_t>(y * W + x)]) continue;
            const float X = static_cast<float>(x * TS), Y = static_cast<float>(y * TS), h = TS / 2.f;
            DrawRectangle(x * TS, y * TS, TS, TS, Color{70, 30, 110, 89});
            const Color c{176, 123, 255, static_cast<unsigned char>(255 * (.45 + .3 * std::sin(time * 3 + x * .7)))};
            const Vector2 p[5] = {{X + 4, Y + h}, {X + h, Y + 5}, {X + TS - 4, Y + h}, {X + h, Y + TS - 5}, {X + 4, Y + h}};
            for (int i = 0; i < 4; ++i) DrawLineEx(p[i], p[i + 1], 2, c);
        }
}

void drawFoe(const Foe& f, const Drill& d, double time) {
    const FoeDef& F = FOES[static_cast<std::size_t>(f.k)];
    const float X = static_cast<float>((f.x + FW / 2) * TS);
    const float Y = static_cast<float>((f.y + FH) * TS - 10 + (f.vx != 0 ? std::sin(time * 12 + f.i) * 1.2 : 0));
    const Color col = f.flash > 0 ? WHITE : hex(F.color);
    rlDisableBackfaceCulling();
    rlPushMatrix();
    rlTranslatef(X, Y, 0);
    if (d.x < f.x) rlScalef(-1, 1, 1);  // regarde la foreuse
    if (f.k == 0) {  // larve : trois anneaux
        for (int i = 2; i >= 0; --i) circle(-i * 7.f, 8.f - (7 - i), 7.f - i, i % 2 ? Color{205, 189, 140, 255} : col);
        eyes(3, 3);
    } else if (f.k == 1) {  // scorpide : dard relevé, pince, pattes
        rlTranslatef(0, 7, 0);
        DrawSplineSegmentBezierQuadratic({-8, 0}, {-18, -4}, {-14, -12}, 3, col);
        circle(10, -2, 4, col);
        ellipse(11, 6, col);
        for (float lx : {-6.f, -1.f, 4.f}) DrawLineEx({lx, 3}, {lx - 2, 9}, 1.5f, col);
        eyes(6, 2);
    } else if (f.k == 2) {  // golem : bloc de pierre aux yeux bleus
        DrawRectangleRec({-11, -12, 22, 24}, col);
        DrawRectangleRec({-11, -2, 22, 3}, Color{0, 0, 0, 64});
        DrawRectangleRec({-2, -12, 3, 10}, Color{0, 0, 0, 64});
        DrawRectangleRec({-9, 12, 7, 4}, Color{95, 102, 120, 255});
        DrawRectangleRec({2, 12, 7, 4}, Color{95, 102, 120, 255});
        circle(6, -5, 2.5f, Color{127, 230, 255, 255});
        circle(6, 4, 2.5f, Color{127, 230, 255, 255});
    } else {  // salamandre : queue, corps, taches dorées
        rlTranslatef(0, 9, 0);
        tri({-10, 0}, {-20, -3}, {-20, 2}, col);
        rlPushMatrix();
        rlTranslatef(0, -1, 0);
        ellipse(13, 5, col);
        rlPopMatrix();
        DrawRectangleRec({-8, 2, 3, 4}, col);
        DrawRectangleRec({6, 2, 3, 4}, col);
        circle(-4, -2, 1.8f, Color{255, 211, 74, 255});
        circle(3, 0, 1.8f, Color{255, 211, 74, 255});
        eyes(8, 2);
    }
    rlPopMatrix();
    rlEnableBackfaceCulling();
    if (f.hp < F.hp) {
        DrawRectangleRec({X - 12, Y - 19, 24, 4}, Color{0, 0, 0, 153});
        DrawRectangleRec({X - 12, Y - 19, static_cast<float>(24 * f.hp / F.hp), 4}, Color{255, 90, 74, 255});
    }
}

void drawBoss(const Boss& b, const Drill& d, double time) {
    const BossDef& B = BOSSES[static_cast<std::size_t>(b.L)];
    const float R = static_cast<float>(BOSS_R * TS);
    float X = static_cast<float>(b.x * TS), Y = static_cast<float>(b.y * TS);
    if (b.state == BossState::Wind) {  // tremble avant de charger
        X += GetRandomValue(-25, 25) / 10.f;
        Y += GetRandomValue(-25, 25) / 10.f;
    }
    const Color col = b.flash > 0 ? WHITE : hex(B.color);
    const float a = static_cast<float>(std::atan2(d.y + PH / 2 - b.y, d.x + PW / 2 - b.x));
    const float tm = static_cast<float>(time);
    DrawCircleGradient({X, Y}, R * 1.9f, hex(B.color, 102), hex(B.color, 0));
    rlDisableBackfaceCulling();
    rlPushMatrix();
    rlTranslatef(X, Y, 0);
    rlPushMatrix();
    rlTranslatef(0, R, 0);
    ellipse(R * .9f, 5, Color{0, 0, 0, 89});  // ombre au sol
    rlPopMatrix();
    rlRotatef(a * RAD2DEG, 0, 0, 1);  // tourné vers la foreuse
    if (b.L == 0) {  // Mâchoire-Racine : anneaux, gueule et dents qui tournent
        for (int i = 4; i >= 1; --i) circle(-i * R * .55f, std::sin(tm * 5 + i) * 4, R * (.78f - i * .08f), i % 2 ? Color{168, 138, 82, 255} : col);
        circle(0, 0, R, col);
        circle(R * .15f, 0, R * .55f, Color{42, 18, 8, 255});
        for (int i = 0; i < 10; ++i) {
            const float t = i / 10.f * 6.283f + tm * .8f;
            tri({R * .15f + std::cos(t) * R * .55f, std::sin(t) * R * .55f}, {R * .15f + std::cos(t + .25f) * R * .55f, std::sin(t + .25f) * R * .55f},
                {R * .15f + std::cos(t) * R * .28f, std::sin(t) * R * .28f}, Color{246, 236, 208, 255});
        }
    } else if (b.L == 1) {  // Reine Scorpide : dard, pinces, carapace
        DrawSplineSegmentBezierQuadratic({-R * .7f, 0}, {-R * 1.7f, -R * .3f}, {-R * 1.2f, -R * 1.1f}, 7, col);
        circle(-R * 1.2f, -R * 1.1f, 7, Color{255, 211, 74, 255});
        for (float s : {-1.f, 1.f}) {
            rlPushMatrix();
            rlTranslatef(R * .85f, s * R * .7f, 0);
            rlRotatef(s * std::sin(tm * 6) * .3f * RAD2DEG, 0, 0, 1);
            circle(0, 0, R * .38f, col);
            circle(R * .25f, 0, R * .18f, Color{58, 18, 10, 255});
            rlPopMatrix();
        }
        ellipse(R, R * .72f, col);
        eyes(R * .55f, R * .25f);
    } else if (b.L == 2) {  // Colosse de Basalte : roc fissuré, lueur bleue
        Vector2 p[10];
        for (int i = 0; i < 9; ++i) {
            const float t = i / 9.f * 6.283f, q = R * (.85f + ((i * 37) % 10) / 40.f);
            p[i] = {std::cos(t) * q, std::sin(t) * q};
        }
        for (int i = 0; i < 9; ++i) tri({0, 0}, p[i], p[(i + 1) % 9], col);
        const Color glow{127, 230, 255, static_cast<unsigned char>(255 * (.5 + .3 * std::sin(time * 4)))};
        DrawLineEx({-R * .6f, -R * .3f}, {0, 0}, 3, glow);
        DrawLineEx({0, 0}, {-R * .3f, R * .6f}, 3, glow);
        DrawLineEx({0, 0}, {R * .2f, -R * .6f}, 3, glow);
        circle(R * .45f, -R * .25f, 5, Color{127, 230, 255, 255});
        circle(R * .45f, R * .25f, 5, Color{127, 230, 255, 255});
    } else {  // Cœur de Magma : couronne de flammes qui bat
        const float p = 1 + .08f * std::sin(tm * 8);
        for (int i = 0; i < 12; ++i) {
            const float t = i / 12.f * 6.283f + tm;
            tri({std::cos(t - .15f) * R * .9f, std::sin(t - .15f) * R * .9f}, {std::cos(t) * R * 1.35f * p, std::sin(t) * R * 1.35f * p},
                {std::cos(t + .15f) * R * .9f, std::sin(t + .15f) * R * .9f}, Color{255, 176, 46, 255});
        }
        circle(0, 0, R * p, col);
        circle(0, 0, R * .55f * p, Color{255, 211, 106, 255});
        eyes(R * .35f, R * .22f);
    }
    rlPopMatrix();
    rlEnableBackfaceCulling();
}

void drawBullets(const Creatures& c) {
    for (const Bullet& u : c.bullets) {
        const Vector2 p{static_cast<float>(u.x * TS), static_cast<float>(u.y * TS)};
        DrawCircleV(p, 7, hex(BOSSES[static_cast<std::size_t>(u.L)].color, 85));
        DrawCircleV(p, 4, Color{255, 243, 208, 255});
    }
}

void drawBossBar(const Creatures& c) {
    for (const Boss& b : c.bosses) {
        if (!b.active) continue;
        const int w = std::min(420, GetScreenWidth() * 78 / 100), x = (GetScreenWidth() - w) / 2, y = GetScreenHeight() - 84;
        const char* name = BOSSES[static_cast<std::size_t>(b.L)].name;
        DrawText(name, (GetScreenWidth() - MeasureText(name, 16)) / 2, y - 22, 16, Color{243, 230, 216, 255});
        DrawRectangle(x, y, w, 12, Color{0, 0, 0, 153});
        DrawRectangle(x, y, static_cast<int>(w * std::max(0.0, b.hp / b.max)), 12, Color{192, 76, 255, 255});
        DrawRectangleLines(x, y, w, 12, Color{67, 51, 42, 255});
        return;
    }
}
