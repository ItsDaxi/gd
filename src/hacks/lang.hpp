#pragma once
#include <string>
#include <vector>

namespace mh::lang {

// Carica le lingue incorporate + quelle in megahack/langs/*.json
void init();
// Imposta la lingua corrente (es. "it-IT"); fallback su en-GB
void set(const std::string& code);
const std::string& current();
std::vector<std::string> available();

// Traduzione: chiave Mega Hack -> testo. Se manca restituisce un nome leggibile.
const std::string& tr(const std::string& key);
// Esiste la chiave nella lingua corrente o in en-GB?
bool has(const std::string& key);
// Tip della chiave (key + "/TIP") o stringa vuota
const std::string& tip(const std::string& key);

} // namespace mh::lang

inline const char* T(const std::string& key) { return mh::lang::tr(key).c_str(); }
