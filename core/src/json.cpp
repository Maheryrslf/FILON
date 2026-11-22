#include "filon/core/json.hpp"

#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace filon::json {

const Value* Value::get(std::string_view key) const {
    if (type != Object) return nullptr;
    for (std::size_t i = 0; i < keys.size(); ++i)
        if (keys[i] == key) return &items[i];
    return nullptr;
}

namespace {

struct Parser {
    std::string_view s;
    std::size_t i = 0;
    int depth = 0;

    void ws() {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) ++i;
    }
    bool lit(std::string_view w) {
        if (s.substr(i, w.size()) != w) return false;
        i += w.size();
        return true;
    }
    static void utf8(std::string& out, unsigned c) {
        if (c < 0x80) {
            out += static_cast<char>(c);
        } else if (c < 0x800) {
            out += static_cast<char>(0xC0 | (c >> 6));
            out += static_cast<char>(0x80 | (c & 0x3F));
        } else if (c < 0x10000) {
            out += static_cast<char>(0xE0 | (c >> 12));
            out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (c & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (c >> 18));
            out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    bool hex4(unsigned& c) {
        if (i + 4 > s.size()) return false;
        c = 0;
        for (int k = 0; k < 4; ++k) {
            const char h = s[i++];
            c <<= 4;
            if (h >= '0' && h <= '9') c |= static_cast<unsigned>(h - '0');
            else if (h >= 'a' && h <= 'f') c |= static_cast<unsigned>(h - 'a' + 10);
            else if (h >= 'A' && h <= 'F') c |= static_cast<unsigned>(h - 'A' + 10);
            else return false;
        }
        return true;
    }
    bool str(std::string& out) {
        if (i >= s.size() || s[i] != '"') return false;
        ++i;
        while (i < s.size()) {
            const char c = s[i++];
            if (c == '"') return true;
            if (static_cast<unsigned char>(c) < 0x20) return false;
            if (c != '\\') {
                out += c;
                continue;
            }
            if (i >= s.size()) return false;
            switch (s[i++]) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    unsigned c1;
                    if (!hex4(c1)) return false;
                    if (c1 >= 0xD800 && c1 < 0xDC00 && lit("\\u")) {  // paire de substitution
                        unsigned c2;
                        if (!hex4(c2) || c2 < 0xDC00 || c2 >= 0xE000) return false;
                        c1 = 0x10000 + ((c1 - 0xD800) << 10) + (c2 - 0xDC00);
                    }
                    utf8(out, c1);
                    break;
                }
                default: return false;
            }
        }
        return false;
    }
    bool value(Value& v) {
        ws();
        if (i >= s.size() || ++depth > 64) return false;  // profondeur bornée : fichier venu du disque
        bool ok = true;
        const char c = s[i];
        if (c == '{') {
            v.type = Value::Object;
            ++i;
            ws();
            if (i < s.size() && s[i] == '}') {
                ++i;
            } else {
                for (;;) {
                    ws();
                    std::string k;
                    Value item;
                    if (!str(k)) return false;
                    ws();
                    if (!lit(":") || !value(item)) return false;
                    v.keys.push_back(std::move(k));
                    v.items.push_back(std::move(item));
                    ws();
                    if (lit(",")) continue;
                    ok = lit("}");
                    break;
                }
            }
        } else if (c == '[') {
            v.type = Value::Array;
            ++i;
            ws();
            if (i < s.size() && s[i] == ']') {
                ++i;
            } else {
                for (;;) {
                    Value item;
                    if (!value(item)) return false;
                    v.items.push_back(std::move(item));
                    ws();
                    if (lit(",")) continue;
                    ok = lit("]");
                    break;
                }
            }
        } else if (c == '"') {
            v.type = Value::String;
            ok = str(v.str);
        } else if (lit("true")) {
            v.type = Value::Bool;
            v.b = true;
        } else if (lit("false")) {
            v.type = Value::Bool;
        } else if (lit("null")) {
            v.type = Value::Null;
        } else {
            const std::size_t start = i;
            while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '-' || s[i] == '+' || s[i] == '.' || s[i] == 'e' || s[i] == 'E')) ++i;
            v.type = Value::Number;
            const auto r = std::from_chars(s.data() + start, s.data() + i, v.num);
            ok = i > start && r.ec == std::errc() && r.ptr == s.data() + i;
        }
        --depth;
        return ok;
    }
};

}  // namespace

std::optional<Value> parse(std::string_view text) {
    Parser p{text};
    Value v;
    if (!p.value(v)) return std::nullopt;
    p.ws();
    if (p.i != text.size()) return std::nullopt;
    return v;
}

std::string number(double v) {
    if (!std::isfinite(v)) return "null";
    if (v == 0) return "0";
    char buf[40];
    const auto r = std::to_chars(buf, buf + sizeof buf, std::fabs(v), std::chars_format::scientific);  // chiffres les plus courts
    const std::string sci(buf, r.ptr);
    const std::size_t e = sci.find('e');
    std::string dig = sci.substr(0, e);
    if (dig.size() > 1) dig.erase(1, 1);  // retire le point
    const int k = static_cast<int>(dig.size()), n = std::atoi(sci.c_str() + e + 1) + 1;
    std::string out = v < 0 ? "-" : "";
    // Number::toString (ECMA-262 §6.1.6.1.20)
    if (k <= n && n <= 21) {
        out += dig + std::string(static_cast<std::size_t>(n - k), '0');
    } else if (0 < n && n <= 21) {
        out += dig.substr(0, static_cast<std::size_t>(n)) + "." + dig.substr(static_cast<std::size_t>(n));
    } else if (-6 < n && n <= 0) {
        out += "0." + std::string(static_cast<std::size_t>(-n), '0') + dig;
    } else {
        out += dig.substr(0, 1);
        if (k > 1) out += "." + dig.substr(1);
        out += n - 1 >= 0 ? "e+" : "e-";
        out += std::to_string(std::abs(n - 1));
    }
    return out;
}

std::string quote(std::string_view s) {
    std::string out = "\"";
    for (const char c : s) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (static_cast<unsigned char>(c) < 0x20) {
            char b[8];
            std::snprintf(b, sizeof b, "\\u%04x", static_cast<unsigned>(c));
            out += b;
        } else {
            out += c;
        }
    }
    return out + "\"";
}

}  // namespace filon::json
