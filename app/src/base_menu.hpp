#pragma once
// Menu de la base : Comptoir, Fonderie, Atelier, Talents, Options. Interface immédiate dessinée avec raylib.
#include <string>

#include "filon/core/economy.hpp"
#include "filon/core/save.hpp"

enum class MenuAction { None, Sell, Smelt, Buy };  // pour le bruitage

struct BaseMenu {
    bool open = false;
    int tab = 0;                            // 0 Comptoir, 1 Fonderie, 2 Atelier, 3 Talents, 4 Options
    MenuAction action = MenuAction::None;   // ce que le dernier draw() a fait

    // Dessine le menu et traite les clics. Renvoie un message à afficher (vide sinon).
    // atBase : vente, fonte et achats ne sont possibles qu'à la base. prog : réglages (volumes) et statistiques.
    std::string draw(filon::Economy& e, filon::Drill& d, bool atBase, filon::Progress& prog);
};
