#include "filon/core/terrain.hpp"

#include <algorithm>
#include <cmath>

namespace filon {
namespace {
constexpr int DECK8 = static_cast<int>(DECK * 8);  // première rangée de pixels du pont (20)
bool isHatchCol(int sx) { return sx >= GX1 * 8 && sx < GX2 * 8; }
}  // namespace

int Terrain::fl8(double v) { return static_cast<int>(std::floor(v * 8)); }

Terrain::Terrain(World& world, const std::vector<std::uint8_t>& bits)
    : w_(world), m_(static_cast<std::size_t>(MW * MH), 0), r_(static_cast<std::size_t>(W * H), 0) {
    const bool packed = bits.size() == static_cast<std::size_t>(MW * MH / 8);
    for (int sy = SURF * 8; sy < MH; ++sy) {
        const int ty = sy >> 3;
        for (int sx = 0; sx < MW; ++sx) {
            const std::size_t ti = static_cast<std::size_t>(ty * W + (sx >> 3)), i = static_cast<std::size_t>(sy * MW + sx);
            if (packed ? (bits[i >> 3] >> (i & 7)) & 1 : solid(w_.tiles[ti])) {
                m_[i] = 1;
                ++r_[ti];
            }
        }
    }
    // Pont de la base, trappe fermée (indestructible : le forage ignore le ciel)
    for (int sy = DECK8; sy < DECK8 + 4; ++sy)
        for (int sx = static_cast<int>(DX1 * 8); sx < DX2 * 8; ++sx) m_[static_cast<std::size_t>(sy * MW + sx)] = 1;
    // Garde-corps aux deux bouts, une case de haut
    const int rails[4] = {static_cast<int>(DX1 * 8), static_cast<int>(DX1 * 8) + 1, static_cast<int>(DX2 * 8) - 2, static_cast<int>(DX2 * 8) - 1};
    for (int sy = DECK8 - 8; sy < DECK8; ++sy)
        for (int sx : rails) m_[static_cast<std::size_t>(sy * MW + sx)] = 1;
}

std::vector<std::uint8_t> Terrain::packMask() const {
    std::vector<std::uint8_t> b(m_.size() / 8, 0);
    for (std::size_t i = 0; i < m_.size(); ++i)
        if (m_[i]) b[i >> 3] = static_cast<std::uint8_t>(b[i >> 3] | 1 << (i & 7));
    return b;
}

bool Terrain::cellSolid(int sx, int sy) const {
    if (sx < 0 || sx >= MW || sy >= MH) return true;
    if (sy < 0) return false;
    return m_[static_cast<std::size_t>(sy * MW + sx)] == 1;
}

bool Terrain::boxHits(double x, double y, double w, double h) const {
    const int x0 = static_cast<int>(std::floor(x * 8 + 1e-3)), x1 = static_cast<int>(std::floor((x + w) * 8 - 1e-3));
    const int y0 = static_cast<int>(std::floor(y * 8 + 1e-3)), y1 = static_cast<int>(std::floor((y + h) * 8 - 1e-3));
    for (int sy = y0; sy <= y1; ++sy)
        for (int sx = x0; sx <= x1; ++sx)
            if (cellSolid(sx, sy)) return true;
    return false;
}

DigResult Terrain::dig(double cx, double cy, double rad, double budget) {
    struct Cell {
        double d2;
        int sx, sy, tx, ty;
        std::uint8_t t;
    };
    std::vector<Cell> cells;
    std::uint8_t blocked = 0;
    const int sy0 = std::max(SURF * 8, fl8(cy - rad)), sy1 = std::min(MH - 1, fl8(cy + rad));
    const int sx0 = std::max(0, fl8(cx - rad)), sx1 = std::min(MW - 1, fl8(cx + rad));
    for (int sy = sy0; sy <= sy1; ++sy)
        for (int sx = sx0; sx <= sx1; ++sx) {
            if (!m_[static_cast<std::size_t>(sy * MW + sx)]) continue;
            const double dx = (sx + .5) / 8 - cx, dy = (sy + .5) / 8 - cy, d2 = dx * dx + dy * dy;
            if (d2 > rad * rad) continue;
            const int tx = sx >> 3, ty = sy >> 3;
            const std::uint8_t t = w_.tiles[static_cast<std::size_t>(ty * W + tx)];
            if (t == BED || t == SEAL) {
                blocked = t;
                continue;
            }
            cells.push_back({d2, sx, sy, tx, ty, t});
        }
    std::stable_sort(cells.begin(), cells.end(), [](const Cell& a, const Cell& b) { return a.d2 < b.d2; });
    int removed = 0;
    for (const Cell& c : cells) {
        const double cost = tileHP(c.t, c.ty) / 64;
        if (budget < cost) break;
        budget -= cost;
        removeCell(c.sx, c.sy, c.tx, c.ty, c.t);  // type relevé avant le forage, comme la version JS
        ++removed;
    }
    DigResult res{budget, removed, static_cast<int>(cells.size()) - removed, blocked, 0, 0, 0};
    if (!cells.empty()) {
        res.t = cells[0].t;
        res.tx = cells[0].tx;
        res.ty = cells[0].ty;
    }
    return res;
}

void Terrain::removeCell(int sx, int sy, int tx, int ty, std::uint8_t t) {
    const int i = sy * MW + sx;
    m_[static_cast<std::size_t>(i)] = 0;
    changedCells.push_back(i);
    const std::size_t ti = static_cast<std::size_t>(ty * W + tx);
    --r_[ti];
    xpGained += (t == HARD ? 2.0 : 1.0) / 64;
    if (isOre(t) && 64 - r_[ti] >= 20) {  // assez de terre retirée autour du filon : la pépite se détache
        freed.push_back({t, (sx + .5) / 8, (sy + .5) / 8});
        xpGained += 2 + (t - ORE_FIRST) * 2;
        w_.tiles[ti] = ROCK;
        changedTiles.push_back(static_cast<int>(ti));
    }
}

void Terrain::setHatch(bool open) {
    if (open == hatch_) return;
    for (int sy = DECK8; sy < DECK8 + 4; ++sy)
        for (int sx = static_cast<int>(std::floor(GX1 * 8)); sx < GX2 * 8; ++sx)
            if (isHatchCol(sx)) m_[static_cast<std::size_t>(sy * MW + sx)] = open ? 0 : 1;
    hatch_ = open;
}

}  // namespace filon
