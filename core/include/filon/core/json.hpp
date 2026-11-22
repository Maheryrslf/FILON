#pragma once
// JSON minimal pour la sauvegarde : lecture complète, et nombres écrits exactement comme JSON.stringify en JS.
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace filon::json {

struct Value {
    enum Type { Null, Bool, Number, String, Array, Object } type = Null;
    bool b = false;
    double num = 0;
    std::string str;
    std::vector<Value> items;      // tableau, ou valeurs de l'objet
    std::vector<std::string> keys;  // clés de l'objet, dans l'ordre du fichier

    const Value* get(std::string_view key) const;  // nullptr si absente ou si ce n'est pas un objet
};

std::optional<Value> parse(std::string_view text);  // nullopt si le texte n'est pas du JSON valide
std::string number(double v);                       // comme String(v) en JS ; null si non fini
std::string quote(std::string_view s);              // chaîne entre guillemets, échappée

}  // namespace filon::json
