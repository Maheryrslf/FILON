#include "terrain_view.hpp"

#include <algorithm>
#include <cmath>

using namespace filon;

TerrainView::TerrainView(const World& world, const Terrain& terrain, int texSeed)
    : world_(world), terrain_(terrain), texSeed_(texSeed), slots_(static_cast<std::size_t>(CHUNKS_X * CHUNKS_Y)) {}

TerrainView::~TerrainView() {
    for (Slot& s : slots_)
        if (s.built) UnloadTexture(s.tex);
}

void TerrainView::ensure(int cx, int cy) {
    Slot& s = slot(cx, cy);
    if (s.built) return;
    s.chunk = buildChunk(world_, terrain_, texSeed_, cx, cy);
    Image img{s.chunk.rgba.data(), CHUNK_W, CHUNK_H, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    s.tex = LoadTextureFromImage(img);  // filtre « point » par défaut : pixels nets
    s.built = true;
}

void TerrainView::sync(Terrain& terrain) {
    auto touch = [&](int sx, int sy) {
        if (sx < 0 || sy < 0 || sx >= MW || sy >= MH) return;
        Slot& s = slot(sx / CHUNK_W, sy / CHUNK_H);
        if (!s.built) return;  // sera construit à jour plus tard
        shadePixel(s.chunk, terrain, sx, sy);
        s.dirty = true;
    };
    for (int i : terrain.changedCells) {
        touch(i % MW, i / MW);
        touch(i % MW, i / MW + 1);  // le pixel du dessous change d'ombre
    }
    for (int ti : terrain.changedTiles) {
        const int tx = ti % W, ty = ti / W;
        Slot& s = slot(tx * 8 / CHUNK_W, ty * 8 / CHUNK_H);
        if (!s.built) continue;
        recolorTile(s.chunk, world_, terrain, texSeed_, tx, ty);
        s.dirty = true;
    }
    terrain.changedCells.clear();
    terrain.changedTiles.clear();
    for (Slot& s : slots_)
        if (s.dirty) {
            UpdateTexture(s.tex, s.chunk.rgba.data());
            s.dirty = false;
        }
}

void TerrainView::draw(float camX, float camY, float scale, int screenW, int screenH) {
    const float chunkPx = CW * TS;  // taille d'un bloc en pixels du monde (horizontalement)
    const int cx0 = std::max(0, static_cast<int>(std::floor(camX / chunkPx)));
    const int cx1 = std::min(CHUNKS_X - 1, static_cast<int>(std::floor((camX + screenW / scale) / chunkPx)));
    const int cy0 = std::max(0, static_cast<int>(std::floor(camY / (CH * TS))));
    const int cy1 = std::min(CHUNKS_Y - 1, static_cast<int>(std::floor((camY + screenH / scale) / (CH * TS))));
    for (int cy = cy0; cy <= cy1; ++cy)
        for (int cx = cx0; cx <= cx1; ++cx) {
            ensure(cx, cy);
            const Rectangle src{0, 0, CHUNK_W, CHUNK_H};
            const Rectangle dst{(cx * CW * TS - camX) * scale, (cy * CH * TS - camY) * scale, CW * TS * scale, CH * TS * scale};
            DrawTexturePro(slot(cx, cy).tex, src, dst, Vector2{0, 0}, 0, WHITE);
        }
}
