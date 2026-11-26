#include "filon/core/save.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "filon/core/json.hpp"
#include "filon/core/terrain.hpp"

namespace filon {
namespace {

const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string b64(const std::vector<std::uint8_t>& u) {  // btoa
    std::string out;
    out.reserve((u.size() + 2) / 3 * 4);
    for (std::size_t i = 0; i < u.size(); i += 3) {
        const unsigned n = static_cast<unsigned>(u[i]) << 16 | (i + 1 < u.size() ? u[i + 1] << 8 : 0) | (i + 2 < u.size() ? u[i + 2] : 0);
        out += B64[n >> 18];
        out += B64[(n >> 12) & 63];
        out += i + 1 < u.size() ? B64[(n >> 6) & 63] : '=';
        out += i + 2 < u.size() ? B64[n & 63] : '=';
    }
    return out;
}

std::optional<std::vector<std::uint8_t>> unb64(const std::string& s) {  // atob ; nullopt si invalide
    std::vector<std::uint8_t> out;
    out.reserve(s.size() / 4 * 3);
    unsigned acc = 0;
    int bits = 0;
    std::size_t pad = 0;
    for (const char c : s) {
        if (c == '=') {
            ++pad;
            continue;
        }
        if (pad) return std::nullopt;  // rien après le remplissage
        const char* p = c ? std::strchr(B64, c) : nullptr;
        if (!p) return std::nullopt;
        acc = acc << 6 | static_cast<unsigned>(p - B64);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::uint8_t>(acc >> bits));
            acc &= (1u << bits) - 1;
        }
    }
    if (pad > 2) return std::nullopt;
    return out;
}

using json::Value;

bool isNum(const Value* v) { return v && v->type == Value::Number && std::isfinite(v->num); }

void readNum(const Value* v, double& out) {
    if (isNum(v)) out = v->num;
}

void readInt(const Value* v, int& out, int lo, int hi) {
    if (isNum(v)) out = static_cast<int>(std::clamp(std::floor(v->num), static_cast<double>(lo), static_cast<double>(hi)));
}

// tableau d'octets en base64, gardé seulement s'il a la taille attendue (comme boot() en JS)
std::vector<std::uint8_t> readBytes(const Value* v, std::size_t size) {
    if (!v || v->type != Value::String) return {};
    auto u = unb64(v->str);
    return u && u->size() == size ? std::move(*u) : std::vector<std::uint8_t>{};
}

int oreIndex(double id) {  // indice 0..8 d'un minerai, -1 sinon
    const double i = id - ORE_FIRST;
    return i >= 0 && i < ORE_COUNT && i == std::floor(i) ? static_cast<int>(i) : -1;
}

void readOres(const Value* v, int (&out)[ORE_COUNT]) {
    if (!v || v->type != Value::Object) return;
    for (std::size_t k = 0; k < v->keys.size(); ++k) {
        const int i = oreIndex(std::atof(v->keys[k].c_str()));
        if (i >= 0) readInt(&v->items[k], out[i], 0, 1 << 30);
    }
}

std::string ores(const int (&a)[ORE_COUNT]) {  // {"10":3,...} : clés entières, donc croissantes comme en JS
    std::string out = "{";
    for (int i = 0; i < ORE_COUNT; ++i)
        if (a[i]) out += (out.size() > 1 ? ",\"" : "\"") + std::to_string(ORE_FIRST + i) + "\":" + std::to_string(a[i]);
    return out + "}";
}

}  // namespace

double toFixed2(double v) {
    const double e = v * 8;
    // seules les valeurs impaires / 8 (x,125 x,375 x,625 x,875) tombent pile entre deux centièmes ; JS arrondit alors en s'éloignant de zéro
    if (std::fabs(v) < 1e15 && e == std::floor(e) && std::fmod(std::fabs(e), 2) == 1) return std::round(v * 100) / 100;
    char b[64];
    std::snprintf(b, sizeof b, "%.2f", v);  // arrondi exact de la valeur binaire, comme toFixed
    return std::strtod(b, nullptr);
}

std::string toJson(const SaveData& s) {
    using json::number;
    const Economy& e = s.eco;
    const Progress& p = s.prog;
    std::string o = "{\"v\":1,\"seed\":" + std::to_string(s.seed) + ",\"coins\":" + number(e.coins);
    o += ",\"cargo\":" + ores(e.cargo) + ",\"bars\":" + ores(e.bars) + ",\"furnace\":{\"q\":[";
    for (std::size_t i = 0; i < e.furnace.queue.size(); ++i)
        o += (i ? ",[" : "[") + std::to_string(e.furnace.queue[i].first) + "," + std::to_string(e.furnace.queue[i].second) + "]";
    o += "],\"p\":" + number(e.furnace.progress) + ",\"last\":" + number(static_cast<double>(e.furnace.last)) + "},\"up\":{";
    for (int i = 0; i < UPGRADE_COUNT; ++i) o += (i ? ",\"" : "\"") + std::string(UPGRADES[static_cast<std::size_t>(i)].key) + "\":" + std::to_string(e.up[i]);
    o += "},\"tal\":{";
    for (int i = 0; i < TALENT_COUNT; ++i) o += (i ? ",\"" : "\"") + std::string(TALENTS[static_cast<std::size_t>(i)].key) + "\":" + std::to_string(e.tal[i]);
    o += "},\"xp\":" + number(e.xp) + ",\"lvl\":" + std::to_string(e.lvl) + ",\"boss\":[";
    for (int i = 0; i < 4; ++i) o += (i ? "," : "") + std::to_string(p.boss[static_cast<std::size_t>(i)]);
    o += "],\"dead\":[";
    for (std::size_t i = 0; i < p.dead.size(); ++i) o += (i ? "," : "") + std::to_string(p.dead[i]);
    o += "],\"maxDepth\":" + number(p.maxDepth) + ",\"play\":" + number(p.play) + ",\"seenL\":" + std::to_string(p.seenL) +
         ",\"fallTip\":" + std::to_string(p.fallTip);
    o += ",\"set\":{\"music\":" + number(p.set.music) + ",\"sfx\":" + number(p.set.sfx) + ",\"vib\":" + (p.set.vib ? "true" : "false") +
         ",\"stick\":" + json::quote(p.set.stick) + (p.set.shake ? "" : ",\"shake\":false") + "}";
    o += ",\"pos\":" + (s.pos ? "[" + number((*s.pos)[0]) + "," + number((*s.pos)[1]) + "]" : std::string("null"));
    o += ",\"fuel\":" + (s.fuel ? number(*s.fuel) : "null") + ",\"hull\":" + (s.hull ? number(*s.hull) : "null");
    o += ",\"tiles\":" + (s.tiles.empty() ? "null" : "\"" + b64(s.tiles) + "\"");
    o += ",\"exp\":" + (s.explored.empty() ? "null" : "\"" + b64(s.explored) + "\"");
    o += ",\"nug\":[";
    for (std::size_t i = 0; i < s.nuggets.size(); ++i)
        o += (i ? ",[" : "[") + std::to_string(s.nuggets[i].id) + "," + number(toFixed2(s.nuggets[i].x)) + "," + number(toFixed2(s.nuggets[i].y)) + "]";
    o += "],\"mask\":" + (s.mask.empty() ? "null" : "\"" + b64(s.mask) + "\"");
    return o + "}";
}

std::optional<SaveData> fromJson(std::string_view text) {
    const auto root = json::parse(text);
    if (!root || root->type != Value::Object) return std::nullopt;
    const Value* v = root->get("v");
    if (!isNum(v) || v->num != 1) return std::nullopt;
    SaveData s;
    Economy& e = s.eco;
    Progress& p = s.prog;
    if (const Value* x = root->get("seed"); isNum(x) && x->num >= 0 && x->num < 4294967296.0) s.seed = static_cast<std::uint32_t>(x->num);
    readNum(root->get("coins"), e.coins);
    readOres(root->get("cargo"), e.cargo);
    readOres(root->get("bars"), e.bars);
    if (const Value* f = root->get("furnace"); f && f->type == Value::Object) {
        if (const Value* q = f->get("q"); q && q->type == Value::Array)
            for (const Value& it : q->items)
                if (it.type == Value::Array && it.items.size() == 2 && isNum(&it.items[0]) && isNum(&it.items[1]) && oreIndex(it.items[0].num) >= 0 && it.items[1].num >= 1)
                    e.furnace.queue.emplace_back(static_cast<std::uint8_t>(it.items[0].num), static_cast<int>(std::min(it.items[1].num, 1e9)));
        readNum(f->get("p"), e.furnace.progress);
        if (const Value* l = f->get("last"); isNum(l)) e.furnace.last = static_cast<std::int64_t>(l->num);
    }
    if (const Value* u = root->get("up"))
        for (int i = 0; i < UPGRADE_COUNT; ++i) readInt(u->get(UPGRADES[static_cast<std::size_t>(i)].key), e.up[i], 0, UPGRADES[static_cast<std::size_t>(i)].max);
    if (const Value* t = root->get("tal"))
        for (int i = 0; i < TALENT_COUNT; ++i) readInt(t->get(TALENTS[static_cast<std::size_t>(i)].key), e.tal[i], 0, TALENT_MAX);
    readNum(root->get("xp"), e.xp);
    readInt(root->get("lvl"), e.lvl, 1, 1000);
    if (const Value* b = root->get("boss"); b && b->type == Value::Array)
        for (std::size_t i = 0; i < 4 && i < b->items.size(); ++i) p.boss[i] = (isNum(&b->items[i]) && b->items[i].num) || (b->items[i].type == Value::Bool && b->items[i].b);
    if (const Value* d = root->get("dead"); d && d->type == Value::Array)
        for (const Value& it : d->items)
            if (isNum(&it)) p.dead.push_back(static_cast<int>(std::clamp(it.num, -1e9, 1e9)));
    readNum(root->get("maxDepth"), p.maxDepth);
    readNum(root->get("play"), p.play);
    readInt(root->get("seenL"), p.seenL, 0, 1000);
    if (const Value* f = root->get("fallTip"); f && (isNum(f) ? f->num != 0 : f->type == Value::Bool && f->b)) p.fallTip = 1;
    if (const Value* st = root->get("set")) {
        readNum(st->get("music"), p.set.music);
        readNum(st->get("sfx"), p.set.sfx);
        if (const Value* x = st->get("vib"); x && x->type == Value::Bool) p.set.vib = x->b;
        if (const Value* x = st->get("stick"); x && x->type == Value::String) p.set.stick = x->str;
        if (const Value* x = st->get("shake"); x && x->type == Value::Bool) p.set.shake = x->b;
    }
    if (const Value* x = root->get("pos"); x && x->type == Value::Array && x->items.size() == 2 && isNum(&x->items[0]) && isNum(&x->items[1]))
        s.pos = std::array<double, 2>{x->items[0].num, x->items[1].num};
    if (const Value* x = root->get("fuel"); isNum(x)) s.fuel = x->num;
    if (const Value* x = root->get("hull"); isNum(x)) s.hull = x->num;
    s.tiles = readBytes(root->get("tiles"), static_cast<std::size_t>(W * H));
    s.explored = readBytes(root->get("exp"), static_cast<std::size_t>(W * H));
    s.mask = readBytes(root->get("mask"), static_cast<std::size_t>(MW * MH / 8));
    if (const Value* n = root->get("nug"); n && n->type == Value::Array)
        for (const Value& it : n->items)
            if (it.type == Value::Array && it.items.size() == 3 && isNum(&it.items[0]) && isNum(&it.items[1]) && isNum(&it.items[2]) && oreIndex(it.items[0].num) >= 0)
                s.nuggets.push_back({static_cast<std::uint8_t>(it.items[0].num), it.items[1].num, it.items[2].num, 0, 0});
    return s;
}

bool writeFileAtomic(const std::string& path, const std::string& text) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path target(path);
    if (target.has_parent_path()) fs::create_directories(target.parent_path(), ec);
    const std::string tmp = path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        f << text;
        f.close();
        if (!f) return false;
    }
    fs::rename(tmp, target, ec);
    return !ec;
}

std::optional<std::string> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::nullopt;
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

}  // namespace filon
