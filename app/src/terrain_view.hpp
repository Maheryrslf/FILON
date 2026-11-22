#pragma once
// Affichage du terrain : un bloc de texture (96 x 64 pixels de terre) par texture GPU, créé à la demande
// et mis à jour après chaque forage.
#include <vector>

#include "filon/core/texture.hpp"
#include "raylib.h"

class TerrainView {
public:
    TerrainView(const filon::World& world, const filon::Terrain& terrain, int texSeed);
    ~TerrainView();
    TerrainView(const TerrainView&) = delete;
    TerrainView& operator=(const TerrainView&) = delete;

    // Applique les changements du forage (pixels retirés, filons extraits) ; à appeler après chaque dig().
    void sync(filon::Terrain& terrain);
    // Dessine les blocs visibles ; camX/camY en pixels du monde, scale = pixels écran par pixel du monde.
    void draw(float camX, float camY, float scale, int screenW, int screenH);

private:
    struct Slot {
        bool built = false, dirty = false;
        filon::Chunk chunk;
        Texture2D tex{};
    };
    Slot& slot(int cx, int cy) { return slots_[static_cast<std::size_t>(cy * filon::CHUNKS_X + cx)]; }
    void ensure(int cx, int cy);

    const filon::World& world_;
    const filon::Terrain& terrain_;
    int texSeed_;
    std::vector<Slot> slots_;
};
