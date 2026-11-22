#include "drill_view.hpp"

#include <cmath>

#include "raylib.h"
#include "rlgl.h"

namespace {

void rrect(float x, float y, float w, float h, float r, Color c) {  // rectangle aux coins arrondis de rayon r
    DrawRectangleRounded(Rectangle{x, y, w, h}, 2 * r / std::fmin(w, h), 6, c);
}
void tri(Vector2 a, Vector2 b, Vector2 c, Color col) { DrawTriangle(a, b, c, col); }

}  // namespace

void drawDrill(const filon::Drill& d, double time) {
    using namespace filon;
    const float X = static_cast<float>((d.x + PW / 2) * TS), Y = static_cast<float>((d.y + PH + d.vis) * TS);
    rlDisableBackfaceCulling();  // le retournement (sens de marche) inverse l'ordre des sommets
    rlPushMatrix();
    rlTranslatef(X, Y - 12, 0);
    if (d.side < 0) rlScalef(-1, 1, 1);
    rlRotatef(static_cast<float>(-d.tilt * RAD2DEG), 0, 0, 1);  // caisse inclinée selon la pente
    rlTranslatef(0, 12, 0);

    if (d.thrust) {  // flamme du propulseur
        const float fl = 7 + 7 * static_cast<float>(std::fabs(std::sin(time * 37)));
        tri({-13.5f, -7}, {-9.5f, -7}, {-11.5f, -7 + fl}, Color{255, 120, 40, 217});
        tri({-12.5f, -7}, {-10.5f, -7}, {-11.5f, -7 + fl * .55f}, Color{255, 226, 138, 255});
    }
    // chenilles, maillons qui défilent, galets
    rrect(-13, -8, 26, 8, 4, Color{35, 29, 24, 255});
    for (int k = 0; k < 7; ++k) {
        const float o = std::fmod(std::fmod(k * 4 - static_cast<float>(d.odo) * 12, 26.f) + 26.f, 26.f);
        DrawRectangleRec({-13 + o, -8, 2, 1.6f}, Color{61, 51, 43, 255});
        DrawRectangleRec({-13 + o, -1.6f, 2, 1.6f}, Color{61, 51, 43, 255});
    }
    for (float wx : {-8.f, 0.f, 8.f}) {
        DrawCircleV({wx, -4}, 2.8f, Color{90, 76, 64, 255});
        DrawCircleV({wx, -4}, 1, Color{168, 150, 132, 255});
    }
    // caisse
    rrect(-11, -24, 20, 16, 3, d.hurtCd > .35 ? WHITE : Color{229, 154, 47, 255});
    DrawRectangleRec({-11, -12, 20, 4}, Color{0, 0, 0, 51});
    for (int k = 0; k < 4; ++k) DrawRectangleRec({-10.f + k * 5, -11, 3, 2}, Color{42, 31, 25, 255});
    rrect(-11, -22, 6, 6, 2, Color{134, 217, 227, 255});
    DrawRectangleRec({-10, -21, 2, 2}, Color{255, 255, 255, 153});
    DrawRectangleRec({-13.5f, -13, 4, 6}, Color{85, 96, 107, 255});  // tuyère du propulseur

    // tourelle : la tête pivote de -45° à +45° et se tourne vers l'avant ou l'arrière (vue en perspective pendant le demi-tour)
    rlTranslatef(0, -16, 0);
    DrawCircleV({0, 0}, 7.5f, Color{58, 63, 70, 255});
    DrawCircleV({0, 0}, 6, Color{79, 88, 97, 255});
    rlPushMatrix();
    const float yaw = std::fabs(d.yaw) < .08 ? (d.yaw < 0 ? -.08f : .08f) : static_cast<float>(d.yaw);
    rlScalef(yaw, 1, 1);
    rlRotatef(static_cast<float>(-d.th * RAD2DEG), 0, 0, 1);
    if (d.drilling) rlTranslatef(static_cast<float>(std::sin(time * 90)) * .7f, 0, 0);  // vibration du forage
    DrawRectangleRec({2, -3, 9, 6}, Color{107, 115, 124, 255});
    DrawRectangleRec({2, -3, 9, 2}, Color{255, 255, 255, 46});
    DrawRectangleRec({10, -4.5f, 3, 9}, Color{46, 50, 56, 255});
    tri({13, -6}, {24, 0}, {13, 6}, Color{213, 218, 224, 255});
    for (int i = 0; i < 3; ++i) {  // spirale du foret
        const float o = 13 + std::fmod(std::fmod(i * 4 + static_cast<float>(d.spin) * 4, 11.f) + 11.f, 11.f), hh = 6 * (1 - (o - 13) / 11);
        DrawLineEx({o, -hh}, {o + 2, hh}, 1.5f, Color{125, 133, 143, 255});
    }
    rlPopMatrix();
    DrawCircleV({0, 0}, 2.5f, Color{138, 147, 157, 255});
    rlPopMatrix();
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
}
