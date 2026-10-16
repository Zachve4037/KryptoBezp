#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef DEFAULT_DATA_DIR
#define DEFAULT_DATA_DIR "data"
#endif

namespace fs = std::filesystem;
using Byte = unsigned char;
using Hodiny = std::chrono::steady_clock;
constexpr std::uint32_t PRVE_HESLO = 10000000;
constexpr std::uint32_t POSLEDNE_HESLO = 99999999;
constexpr std::size_t DLZKA_KONTROLY = 64;

// Rovnako ako getKey v povodnom stream.c: opakujeme ASCII znaky hesla.
std::array<Byte, 256> getKey(const std::string& heslo) {
    std::array<Byte, 256> kluc{};
    for (std::size_t i = 0; i < kluc.size(); ++i) {
        kluc[i] = static_cast<Byte>(heslo[i % heslo.size()]);
    }
    return kluc;
}

struct RC4 {
    std::array<Byte, 256> s{};
    Byte i = 0;
    Byte j = 0;

    explicit RC4(const std::string& heslo) {
        const auto kluc = getKey(heslo);
        for (int n = 0; n < 256; ++n) {
            s[n] = static_cast<Byte>(n);
        }
        Byte posun = 0;
        for (int n = 0; n < 256; ++n) {
            posun = static_cast<Byte>(posun + s[n] + kluc[n]);
            std::swap(s[n], s[posun]);
        }
    }

    Byte dalsi() {
        ++i;
        j = static_cast<Byte>(j + s[i]);
        std::swap(s[i], s[j]);
        return s[static_cast<Byte>(s[i] + s[j])];
    }
};

// Prisna kontrola UTF-8 vratanie zakazu prilis dlheho kodovania a surrogatov.
// Kontrolne znaky ani diakritiku nemenime; testujeme len platnost kodovania.
struct Utf8 {
    int zostava = 0;
    Byte minimum = 0x80;
    Byte maximum = 0xBF;

    bool pridaj(Byte c) {
        if (zostava > 0) {
            if (c < minimum || c > maximum) return false;
            --zostava;
            minimum = 0x80;
            maximum = 0xBF;
        } else if (c <= 0x7F) {
            return true;
        } else if (c >= 0xC2 && c <= 0xDF) {
            zostava = 1;
        } else if (c >= 0xE0 && c <= 0xEF) {
            zostava = 2;
            if (c == 0xE0) minimum = 0xA0;
            if (c == 0xED) maximum = 0x9F;
        } else if (c >= 0xF0 && c <= 0xF4) {
            zostava = 3;
            if (c == 0xF0) minimum = 0x90;
            if (c == 0xF4) maximum = 0x8F;
        } else {
            return false;
        }
        return true;
    }
};

bool jeUtf8(const std::string& text) {
    Utf8 kontrola;
    for (Byte c : text) {
        if (!kontrola.pridaj(c)) return false;
    }
    return kontrola.zostava == 0;
}

std::string nacitaj(const fs::path& cesta) {
    std::ifstream subor(cesta, std::ios::binary);
    if (!subor) throw std::runtime_error("Neda sa otvorit: " + cesta.string());
    std::string obsah{std::istreambuf_iterator<char>(subor), std::istreambuf_iterator<char>()};
    if (subor.bad()) throw std::runtime_error("Chyba citania: " + cesta.string());
    return obsah;
}

void uloz(const fs::path& cesta, const std::string& obsah) {
    std::ofstream subor(cesta, std::ios::binary);
    subor << obsah;
    subor.flush();
    if (!subor) throw std::runtime_error("Neda sa zapisat: " + cesta.string());
}

std::string desifruj(std::string text, const std::string& heslo) {
    RC4 generator(heslo);
    for (char& c : text) {
        c = static_cast<char>(static_cast<Byte>(c) ^ generator.dalsi());
    }
    return text;
}

bool platneHeslo(const std::string& heslo) {
    return heslo.size() == 8 && heslo.front() != '0'
        && std::all_of(heslo.begin(), heslo.end(), [](char c) { return c >= '0' && c <= '9'; });
}

struct Subor {
    fs::path cesta;
    std::string sifra;
    std::string heslo;
    double sekundy = 0;
    std::uint32_t pokusy = 0;
};

fs::path cestaHesla(const Subor& subor, const fs::path& vystup) {
    return vystup / (subor.cesta.stem().string() + "_heslo.txt");
}

std::string zaznam(const Subor& subor) {
    std::ostringstream text;
    text << "Heslo: " << subor.heslo << '\n'
         << "Cas prelamovania (s): " << std::fixed << std::setprecision(6) << subor.sekundy << '\n'
         << "Skusanych hesiel: " << subor.pokusy << '\n'
         << "Subor: " << subor.cesta.filename().string() << '\n';
    return text.str();
}

// Ulozene heslo vzdy overime desifrovanim aktualneho vstupu. Cas zachovame.
bool nacitajHeslo(Subor& subor, const fs::path& vystup) {
    std::ifstream vstup(cestaHesla(subor, vystup));
    std::string riadok;
    if (!std::getline(vstup, riadok) || riadok.rfind("Heslo: ", 0) != 0) return false;
    const std::string heslo = riadok.substr(7);
    if (!platneHeslo(heslo)) return false;
    try {
        if (!std::getline(vstup, riadok) || riadok.rfind("Cas prelamovania (s): ", 0) != 0) return false;
        const double cas = std::stod(riadok.substr(21));
        if (!std::getline(vstup, riadok) || riadok.rfind("Skusanych hesiel: ", 0) != 0) return false;
        const auto pokusy = std::stoul(riadok.substr(17));
        if (!std::isfinite(cas) || cas < 0 || pokusy == 0 || pokusy > 90000000) return false;
        const std::string text = desifruj(subor.sifra, heslo);
        if (!jeUtf8(text)) return false;
        uloz(vystup / (subor.cesta.stem().string() + "_desifrovane.txt"), text);
        subor.heslo = heslo;
        subor.sekundy = cas;
        subor.pokusy = static_cast<std::uint32_t>(pokusy);
        return true;
    } catch (const std::invalid_argument&) {
        return false;
    } catch (const std::out_of_range&) {
        return false;
    }
}

void ulozPrehlad(const std::vector<Subor>& subory, const fs::path& cesta) {
    std::ostringstream text;
    text << "RC4 - hesla a casy prelamovania\n"
            "Rozsah: 10000000 az 99999999, heslo je retazec 8 ASCII cislic.\n"
            "Cas pri kazdom subore je cas od zaciatku spolocneho hladania po jeho najdenie.\n"
            "Pokusy pocitame spolocne pre vsetky subory. Ulozene casy sa pri dalsom spusteni nemenia.\n\n";
    for (const auto& subor : subory) {
        if (!subor.heslo.empty()) text << zaznam(subor) << '\n';
        else text << subor.cesta.filename().string() << ": heslo zatial nenajdene.\n\n";
    }
    uloz(cesta, text.str());
}

bool sediZaciatok(const std::string& sifra, const std::array<Byte, DLZKA_KONTROLY>& prud) {
    Utf8 kontrola;
    for (std::size_t i = 0; i < prud.size(); ++i) {
        if (!kontrola.pridaj(static_cast<Byte>(sifra[i]) ^ prud[i])) return false;
    }
    // Posledny znak moze pokracovat za hranicou prvych 64 bajtov.
    return true;
}

void prelom(std::vector<Subor>& subory, const fs::path& vystup, const fs::path& prehlad) {
    auto chyba = std::count_if(subory.begin(), subory.end(), [](const Subor& s) { return s.heslo.empty(); });
    if (chyba == 0) return;
    const auto zaciatok = Hodiny::now();
    for (std::uint32_t cislo = PRVE_HESLO; cislo <= POSLEDNE_HESLO; ++cislo) {
        const std::string heslo = std::to_string(cislo);
        RC4 generator(heslo);
        std::array<Byte, DLZKA_KONTROLY> prud{};
        for (Byte& b : prud) b = generator.dalsi();
        for (auto& subor : subory) {
            if (!subor.heslo.empty() || !sediZaciatok(subor.sifra, prud)) continue;
            const std::string text = desifruj(subor.sifra, heslo);
            if (!jeUtf8(text)) continue;
            subor.heslo = heslo;
            subor.sekundy = std::chrono::duration<double>(Hodiny::now() - zaciatok).count();
            subor.pokusy = cislo - PRVE_HESLO + 1;
            uloz(vystup / (subor.cesta.stem().string() + "_desifrovane.txt"), text);
            uloz(cestaHesla(subor, vystup), zaznam(subor));
            ulozPrehlad(subory, prehlad);
            std::cout << "Najdene " << subor.cesta.filename().string() << ": " << heslo
                      << " (" << std::fixed << std::setprecision(2) << subor.sekundy << " s)" << std::endl;
            --chyba;
        }
        if (chyba == 0) break;
        const auto pokusy = cislo - PRVE_HESLO + 1;
        if (pokusy % 1000000 == 0) {
            const double sekundy = std::chrono::duration<double>(Hodiny::now() - zaciatok).count();
            std::cout << "Skusane: " << pokusy << " / 90000000 ("
                      << std::fixed << std::setprecision(1) << pokusy / 900000.0 << " %), "
                      << sekundy << " s" << std::endl;
        }
    }
    if (chyba != 0) throw std::runtime_error("V zadanom rozsahu sa nepodarilo najst vsetky hesla.");
}

int main(int argc, char* argv[]) {
    fs::path vstup = DEFAULT_DATA_DIR;
    bool znova = false;
    bool zadanaCesta = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--help") {
            std::cout << "Pouzitie: StreamCipher [subor.bin | priecinok] [--znova]\n"
                         "Bez argumentov spracuje vsetky .bin subory v projektovom data/.\n"
                         "Vysledky: podpriecinok vysledky/ pri vstupoch.\n"
                         "Ulozene hesla sa pouziju automaticky; --znova zopakuje hrubu silu.\n";
            return 0;
        } else if (argument == "--znova") {
            znova = true;
        } else if (!zadanaCesta && argument.rfind("--", 0) != 0) {
            vstup = argument;
            zadanaCesta = true;
        } else {
            std::cerr << "Neplatne argumenty. Pouzite --help.\n";
            return 1;
        }
    }
    try {
        vstup = fs::absolute(vstup);
        std::vector<fs::path> cesty;
        const bool priecinok = fs::is_directory(vstup);
        if (priecinok) {
            for (const auto& polozka : fs::directory_iterator(vstup)) {
                if (polozka.is_regular_file() && polozka.path().extension() == ".bin") cesty.push_back(polozka.path());
            }
        } else if (fs::is_regular_file(vstup)) {
            cesty.push_back(vstup);
        }
        if (cesty.empty()) throw std::runtime_error("Vstup neexistuje alebo neobsahuje subory .bin.");
        std::sort(cesty.begin(), cesty.end());
        const fs::path vystup = (priecinok ? vstup : vstup.parent_path()) / "vysledky";
        fs::create_directories(vystup);
        std::vector<Subor> subory;
        for (const auto& cesta : cesty) {
            Subor subor{cesta, nacitaj(cesta), {}, 0, 0};
            if (subor.sifra.size() < DLZKA_KONTROLY) throw std::runtime_error("Vstup musi mat aspon 64 bajtov: " + cesta.string());
            if (!znova && nacitajHeslo(subor, vystup)) {
                std::cout << "Ulozene heslo " << cesta.filename().string() << ": " << subor.heslo << std::endl;
            }
            subory.push_back(subor);
        }
        const fs::path prehlad = vystup / (priecinok ? "stream_hesla.txt" : cesty.front().stem().string() + "_prehlad.txt");
        ulozPrehlad(subory, prehlad);
        prelom(subory, vystup, prehlad);
        std::cout << "Hotovo. Prehlad: " << prehlad.string() << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Chyba: " << e.what() << '\n';
        return 1;
    }
}
