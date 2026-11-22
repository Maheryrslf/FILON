#pragma once
// Menu de la base : Comptoir, Fonderie, Atelier, Talents. Interface immédiate dessinée avec raylib.
#include <string>

#include "filon/core/economy.hpp"

struct BaseMenu {
    bool open = false;
    int tab = 0;  // 0 Comptoir, 1 Fonderie, 2 Atelier, 3 Talents

    // Dessine le menu et traite les clics. Renvoie un message à afficher (vide sinon).
    // atBase : vente, fonte et achats ne sont possibles qu'à la base.
    std::string draw(filon::Economy& e, filon::Drill& d, bool atBase);
};
