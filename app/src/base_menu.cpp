#include "base_menu.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "raylib.h"

using namespace filon;

namespace {

const Color PANEL{30, 22, 19, 250}, PANEL2{42, 32, 27, 255}, LINE{67, 51, 42, 255}, FG{243, 230, 216, 255}, MUTED{179, 157, 138, 255},
    ACCENT{255, 177, 59, 255}, INK{42, 22, 6, 255};

std::string coins(double v) {  // 12 345 (espace fine pour les milliers)
    std::string s = std::to_string(static_cast<long long>(std::floor(v)));
    for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) s.insert(static_cast<std::size_t>(i), " ");
    return s;
}

bool button(Rectangle r, const std::string& label, bool enabled = true, bool big = false) {
    const bool hover = enabled && CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRounded(r, .25f, 4, enabled ? (hover ? Color{255, 196, 102, 255} : ACCENT) : LINE);
    const int fs = big ? 20 : 16, tw = MeasureText(label.c_str(), fs);
    DrawText(label.c_str(), static_cast<int>(r.x + (r.width - tw) / 2), static_cast<int>(r.y + (r.height - fs) / 2), fs, enabled ? INK : MUTED);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

Color oreColor(int i, bool glint) {
    const std::uint32_t c = glint ? ORES[static_cast<std::size_t>(i)].glint : ORES[static_cast<std::size_t>(i)].color;
    return Color{static_cast<unsigned char>(c >> 16), static_cast<unsigned char>(c >> 8), static_cast<unsigned char>(c), 255};
}

void row(float x, float y, float w, float h) { DrawRectangleRounded({x, y, w, h}, .15f, 4, PANEL2); }

void needBase(float x, float& y) {
    DrawText("Rejoins la base flottante pour l'utiliser (trappe, ou bras accroché dessous).", static_cast<int>(x), static_cast<int>(y), 14, MUTED);
    y += 26;
}

}  // namespace

std::string BaseMenu::draw(Economy& e, Drill& d, bool atBase, Progress& prog) {
    action = MenuAction::None;
    if (!open) return {};
    std::string msg;
    const float sw = static_cast<float>(GetScreenWidth()), sh = static_cast<float>(GetScreenHeight());
    const float w = std::min(600.f, sw - 24), h = std::min(sh - 110, 600.f), x0 = (sw - w) / 2, y0 = sh - h;
    DrawRectangle(0, 0, static_cast<int>(sw), static_cast<int>(sh), Color{0, 0, 0, 90});
    DrawRectangleRec({x0, y0, w, h}, PANEL);
    DrawRectangleRec({x0, y0, w, 3}, ACCENT);

    // onglets
    static const char* tabs[] = {"COMPTOIR", "FONDERIE", "ATELIER", "TALENTS", "OPTIONS"};
    const float tw = (w - 90) / 5;
    for (int i = 0; i < 5; ++i) {
        const Rectangle r{x0 + i * tw, y0 + 3, tw, 40};
        const bool hover = CheckCollisionPointRec(GetMousePosition(), r);
        const int fw = MeasureText(tabs[i], 16);
        DrawText(tabs[i], static_cast<int>(r.x + (tw - fw) / 2), static_cast<int>(r.y + 13), 16, i == tab ? ACCENT : (hover ? FG : MUTED));
        if (i == tab) DrawRectangleRec({r.x + 8, r.y + 37, tw - 16, 3}, ACCENT);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) tab = i;
    }
    {
        const Rectangle r{x0 + w - 86, y0 + 9, 76, 28};
        const bool hover = CheckCollisionPointRec(GetMousePosition(), r);
        DrawText("FERMER", static_cast<int>(r.x + 8), static_cast<int>(r.y + 7), 16, hover ? ACCENT : FG);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) open = false;
    }
    DrawRectangleRec({x0, y0 + 44, w, 1}, LINE);

    const float x = x0 + 16, cw = w - 32;
    float y = y0 + 58;
    char buf[160];

    if (tab == 0) {  // Comptoir
        if (!atBase) needBase(x, y);
        const int total = static_cast<int>(std::round(cargoValue(e) + barsValue(e)));
        std::snprintf(buf, sizeof buf, "À vendre : %s pièces", coins(total).c_str());
        DrawText(buf, static_cast<int>(x), static_cast<int>(y), 18, ACCENT);
        y += 30;
        bool any = false;
        for (int pass = 0; pass < 2; ++pass)
            for (int i = 0; i < ORE_COUNT; ++i) {
                const int n = pass ? e.bars[i] : e.cargo[i];
                if (!n) continue;
                any = true;
                const double v = oreValue(e, i) * (pass ? BAR_FACTOR : 1);
                row(x, y, cw, 34);
                DrawRectangle(static_cast<int>(x + 10), static_cast<int>(y + 11), 12, 12, oreColor(i, false));
                DrawRectangle(static_cast<int>(x + 10), static_cast<int>(y + 11), 6, 6, oreColor(i, true));
                std::snprintf(buf, sizeof buf, "%s%s   %d × %s", ORES[static_cast<std::size_t>(i)].name, pass ? " (lingot)" : "", n, coins(std::round(v)).c_str());
                DrawText(buf, static_cast<int>(x + 32), static_cast<int>(y + 9), 16, FG);
                const std::string t = coins(std::round(n * v));
                DrawText(t.c_str(), static_cast<int>(x + cw - 12 - MeasureText(t.c_str(), 16)), static_cast<int>(y + 9), 16, ACCENT);
                y += 38;
            }
        if (!any) {
            DrawText("Rien à vendre. Descends chercher du minerai.", static_cast<int>(x), static_cast<int>(y), 16, MUTED);
            y += 28;
        }
        if (button({x, y + 6, cw, 44}, "TOUT VENDRE", atBase && total > 0, true)) {
            const int v = sellAll(e);
            msg = "Vendu : +" + coins(v) + " pièces";
            action = MenuAction::Sell;
        }
        DrawText("Le minerai fondu en lingots vaut 2,5 fois plus.", static_cast<int>(x), static_cast<int>(y + 60), 14, MUTED);
    } else if (tab == 1) {  // Fonderie
        int queued = 0;
        for (const auto& it : e.furnace.queue) queued += it.second;
        const double per = effects(e).smelt;
        row(x, y, cw, 50);
        if (queued) {
            std::snprintf(buf, sizeof buf, "Fonte : %s", ORES[e.furnace.queue.front().first - ORE_FIRST].name);
            DrawText(buf, static_cast<int>(x + 12), static_cast<int>(y + 8), 18, FG);
            std::snprintf(buf, sizeof buf, "%d minerai%s en file, fini dans %d s", queued, queued > 1 ? "s" : "", static_cast<int>(std::ceil(furnaceRemaining(e))));
            DrawText(buf, static_cast<int>(x + 12), static_cast<int>(y + 30), 14, MUTED);
        } else {
            DrawText("Four à l'arrêt", static_cast<int>(x + 12), static_cast<int>(y + 8), 18, FG);
            DrawText("Verse ta soute dans le four pour lancer la fonte.", static_cast<int>(x + 12), static_cast<int>(y + 30), 14, MUTED);
        }
        y += 58;
        if (queued) {
            DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(cw), 12, Color{0, 0, 0, 150});
            DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(cw * e.furnace.progress / per), 12, ACCENT);
            y += 20;
        }
        if (!atBase) needBase(x, y);
        std::snprintf(buf, sizeof buf, "FONDRE LA SOUTE (%d)", cargoCount(e));
        if (button({x, y + 4, cw, 44}, buf, atBase && cargoCount(e) > 0, true)) {
            smeltAll(e);
            msg = "Minerai versé dans le four.";
            action = MenuAction::Smelt;
        }
        y += 58;
        std::snprintf(buf, sizeof buf, "%.1f s par minerai. Le four tourne aussi jeu fermé, jusqu'à 8 h. Les lingots se vendent au comptoir.", per);
        DrawText(buf, static_cast<int>(x), static_cast<int>(y), 14, MUTED);
    } else if (tab == 2) {  // Atelier
        std::snprintf(buf, sizeof buf, "Tes pièces : %s", coins(e.coins).c_str());
        DrawText(buf, static_cast<int>(x), static_cast<int>(y), 18, ACCENT);
        y += 26;
        if (!atBase) needBase(x, y);
        const float rh = std::min(44.f, (y0 + h - y - 8) / static_cast<float>(UPGRADE_COUNT));
        for (int i = 0; i < UPGRADE_COUNT; ++i) {
            const auto u = static_cast<Upgrade>(i);
            const UpgradeDef& def = UPGRADES[static_cast<std::size_t>(i)];
            const int l = e.up[i], c = cost(e, u);
            const bool max = l >= def.max;
            row(x, y, cw, rh - 4);
            std::snprintf(buf, sizeof buf, "%s  niv. %d/%d", def.name, l, def.max);
            DrawText(buf, static_cast<int>(x + 10), static_cast<int>(y + 4), 16, FG);
            const std::string fx = def.desc + std::string(" : ") + effectText(u, l) + (max ? "" : " -> " + effectText(u, l + 1));
            DrawText(fx.c_str(), static_cast<int>(x + 10), static_cast<int>(y + 22), 12, MUTED);
            if (button({x + cw - 120, y + 5, 110, rh - 14}, max ? "MAX" : coins(c), !max && atBase && e.coins >= c) && buy(e, u)) {
                if (u == TANK) d.fuel = drillStats(e).fuelMax;
                if (u == HULL) d.hull = drillStats(e).hullMax;
                msg = std::string(def.name) + " niveau " + std::to_string(e.up[i]);
                action = MenuAction::Buy;
            }
            y += rh;
        }
    } else if (tab == 3) {  // Talents
        const int pts = freePoints(e);
        std::snprintf(buf, sizeof buf, "Niveau %d   %d/%d XP   %d point%s à placer", e.lvl, static_cast<int>(e.xp), xpNeed(e.lvl), pts, pts > 1 ? "s" : "");
        DrawText(buf, static_cast<int>(x), static_cast<int>(y), 18, ACCENT);
        y += 32;
        for (int i = 0; i < TALENT_COUNT; ++i) {
            const TalentDef& t = TALENTS[static_cast<std::size_t>(i)];
            row(x, y, cw, 48);
            std::snprintf(buf, sizeof buf, "%s  %d/%d", t.name, e.tal[i], TALENT_MAX);
            DrawText(buf, static_cast<int>(x + 10), static_cast<int>(y + 6), 16, FG);
            DrawText(t.desc, static_cast<int>(x + 10), static_cast<int>(y + 27), 14, MUTED);
            if (button({x + cw - 70, y + 8, 60, 32}, "+1", pts > 0 && e.tal[i] < TALENT_MAX) && addTalent(e, static_cast<Talent>(i))) {
                msg = std::string(t.name) + " niveau " + std::to_string(e.tal[i]);
                action = MenuAction::Buy;
            }
            y += 54;
        }
        DrawText("Chaque niveau donne 1 point. L'XP vient du forage, des créatures et des gardiens.", static_cast<int>(x), static_cast<int>(y + 4), 14, MUTED);
    } else {  // Options : volumes (pas de 5 %, comme les curseurs de la version web), statistiques
        auto slider = [&](const char* name, double& v) {
            row(x, y, cw, 44);
            DrawText(name, static_cast<int>(x + 12), static_cast<int>(y + 13), 18, FG);
            const Rectangle bar{x + cw * .35f, y + 17, cw * .5f, 10};
            const Vector2 m = GetMousePosition();
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, {bar.x - 12, y, bar.width + 24, 44}))
                v = std::round(std::clamp((m.x - bar.x) / bar.width, 0.f, 1.f) * 20) / 20;
            DrawRectangleRounded(bar, .5f, 4, Color{0, 0, 0, 150});
            DrawRectangleRounded({bar.x, bar.y, static_cast<float>(bar.width * v), bar.height}, .5f, 4, ACCENT);
            DrawCircleV({bar.x + static_cast<float>(bar.width * v), bar.y + 5}, 9, FG);
            const std::string pct = std::to_string(static_cast<int>(std::lround(v * 100))) + " %";
            DrawText(pct.c_str(), static_cast<int>(x + cw - 12 - MeasureText(pct.c_str(), 16)), static_cast<int>(y + 14), 16, MUTED);
            y += 50;
        };
        slider("Musique", prog.set.music);
        slider("Effets", prog.set.sfx);
        {  // case à cocher : secousses de l'écran (à couper si elles gênent)
            const Rectangle line{x, y, cw, 44}, box{x + cw - 38, y + 11, 22, 22};
            const bool hover = CheckCollisionPointRec(GetMousePosition(), line);
            row(x, y, cw, 44);
            DrawText("Secousses de l'écran", static_cast<int>(x + 12), static_cast<int>(y + 13), 18, FG);
            DrawRectangleRounded(box, .25f, 4, prog.set.shake ? ACCENT : Color{0, 0, 0, 150});
            DrawRectangleRoundedLinesEx(box, .25f, 4, 2, hover ? FG : LINE);
            if (prog.set.shake) {  // coche
                DrawLineEx({box.x + 5, box.y + 11}, {box.x + 9, box.y + 16}, 3, INK);
                DrawLineEx({box.x + 9, box.y + 16}, {box.x + 17, box.y + 6}, 3, INK);
            }
            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) prog.set.shake = !prog.set.shake;
            y += 50;
        }
        int beaten = 0;
        for (int b : prog.boss) beaten += b != 0;
        const int min = static_cast<int>(prog.play / 60);
        std::snprintf(buf, sizeof buf, "Record : %d m   Gardiens vaincus : %d/4   Temps de jeu : %d h %02d min", static_cast<int>(prog.maxDepth) * 2, beaten, min / 60,
                      min % 60);
        DrawText(buf, static_cast<int>(x), static_cast<int>(y + 6), 16, MUTED);
        DrawText("Aucune pub, aucun traqueur, aucune connexion :", static_cast<int>(x), static_cast<int>(y + 32), 14, MUTED);
        DrawText("la partie est enregistrée sur cet appareil seulement.", static_cast<int>(x), static_cast<int>(y + 50), 14, MUTED);
    }
    return msg;
}
