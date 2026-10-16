#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifndef DEFAULT_DATA_DIR
#define DEFAULT_DATA_DIR "data"
#endif

namespace fs = std::filesystem;

constexpr int MIN_HESLO = 15;
constexpr int MAX_HESLO = 25;

bool jePismeno(char c) {
    return c >= 'A' && c <= 'Z';
}

// Male, priblizne jazykove modely priamo v programe, bez externych dat.
struct Jazyk {
    std::string nazov;
    std::array<double, 26> logPravdepodobnost{};
    std::array<bool, 26 * 26> dvojice{};
    std::array<bool, 26 * 26 * 26> trojice{};

    Jazyk(std::string meno, const std::array<double, 26>& frekvencie,
          const std::string& bezneDvojice, const std::string& bezneTrojice)
        : nazov(std::move(meno)) {
        const double sucet = std::accumulate(frekvencie.begin(), frekvencie.end(), 0.0);
        for (int i = 0; i < 26; ++i) {
            logPravdepodobnost[i] = std::log(frekvencie[i] / sucet);
        }
        std::istringstream pary(bezneDvojice);
        std::istringstream trojky(bezneTrojice);
        std::string slovo;
        while (pary >> slovo) {
            dvojice[(slovo[0] - 'A') * 26 + slovo[1] - 'A'] = true;
        }
        while (trojky >> slovo) {
            trojice[((slovo[0] - 'A') * 26 + slovo[1] - 'A') * 26 + slovo[2] - 'A'] = true;
        }
    }
};

std::vector<Jazyk> vytvorJazyky() {
    return {
        Jazyk("slovencina",
              {11.16, 1.78, 2.46, 3.72, 9.92, 0.16, 0.17, 2.48, 7.85,
               2.19, 4.06, 4.38, 3.56, 5.95, 9.54, 3.03, 0.001, 4.71,
               6.12, 5.72, 3.31, 4.66, 0.001, 0.03, 2.67, 3.06},
              "NA PO PR RO OV NE EN ST TO AN IE NI NO RA ER VA RE OS KO HO OD ON "
              "VE JE SK CH TE IN LA AL LI AT IT TA OU SA AJ MA DO IC VI AK EC DE "
              "LE RI TI VO MI CE AC AV TR LO KA ZI",
              "PRE PRI PRO STA OST NOS STI NIE ENI ANI OVA OVE ICH SKA SKE CHO "
              "KTO POD ALE AKO NAP RED STR TOR NOV ODN EHO NIA STN OJE VNE VAN "
              "OLO OCN SPO ZIV"),
        Jazyk("anglictina",
              {8.167, 1.492, 2.782, 4.253, 12.702, 2.228, 2.015, 6.094,
               6.966, 0.153, 0.772, 4.025, 2.406, 6.749, 7.507, 1.929,
               0.095, 5.987, 6.327, 9.056, 2.758, 0.978, 2.360, 0.150,
               1.974, 0.074},
              "TH HE IN ER AN RE ON AT EN ND TI ES OR TE OF ED IS IT AL AR ST TO "
              "NT NG SE HA AS OU IO LE VE CO ME DE HI RI RO IC NE EA RA CE LI CH "
              "LL BE MA SI OM UR",
              "THE AND ING HER ERE ENT THA NTH WAS ETH FOR DTH HES VER HIS OFT "
              "STH OTH RES ONT ION TIO ALL HAT HEN OME NOT ARE YOU ITH TED EVE "
              "WIT THI EST ERS ATI HIC OUR OUL ULD")
    };
}

std::string nacitaj(const fs::path& cesta) {
    std::ifstream subor(cesta, std::ios::binary);
    if (!subor) {
        throw std::runtime_error("Neda sa otvorit subor: " + cesta.string());
    }
    std::string text{std::istreambuf_iterator<char>(subor), std::istreambuf_iterator<char>()};
    if (subor.bad()) {
        throw std::runtime_error("Chyba citania: " + cesta.string());
    }
    return text;
}

void uloz(const fs::path& cesta, const std::string& text) {
    std::ofstream subor(cesta, std::ios::binary);
    subor << text;
    subor.flush();
    if (!subor) {
        throw std::runtime_error("Neda sa zapisat subor: " + cesta.string());
    }
}

std::vector<int> vyberPismena(const std::string& text) {
    std::vector<int> pismena;
    for (char c : text) {
        if (jePismeno(c)) {
            pismena.push_back(c - 'A');
        }
    }
    return pismena;
}

std::string desifruj(std::string text, const std::string& heslo) {
    std::size_t pozicia = 0;
    for (char& c : text) {
        if (jePismeno(c)) {
            c = static_cast<char>('A' + (c - heslo[pozicia % heslo.size()] + 26) % 26);
            ++pozicia; // Medzery, cisla, male pismena atd. heslo neposuvaju.
        }
    }
    return text;
}

// Pre kazdu poziciu hesla najdeme najpravdepodobnejsi Caesarov posun.
std::string odhadniHeslo(const std::vector<int>& text, int dlzka, const Jazyk& jazyk) {
    std::string heslo(dlzka, 'A');
    for (int pozicia = 0; pozicia < dlzka; ++pozicia) {
        std::array<int, 26> pocty{};
        for (std::size_t i = pozicia; i < text.size(); i += dlzka) {
            ++pocty[text[i]];
        }
        double najlepsie = -std::numeric_limits<double>::infinity();
        for (int posun = 0; posun < 26; ++posun) {
            double skore = 0;
            for (int c = 0; c < 26; ++c) {
                skore += pocty[c] * jazyk.logPravdepodobnost[(c - posun + 26) % 26];
            }
            if (skore > najlepsie) {
                najlepsie = skore;
                heslo[pozicia] = static_cast<char>('A' + posun);
            }
        }
    }
    return heslo;
}

// Vyssie skore znamena podobnost s jazykom, nejde o percento istoty.
double ohodnot(const std::vector<int>& text, const std::string& heslo, const Jazyk& jazyk) {
    double skore = 0;
    int predosle = 0;
    int predpredosle = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const int c = (text[i] - (heslo[i % heslo.size()] - 'A') + 26) % 26;
        skore += jazyk.logPravdepodobnost[c];
        if (i >= 1 && jazyk.dvojice[predosle * 26 + c]) {
            skore += 0.8;
        }
        if (i >= 2 && jazyk.trojice[(predpredosle * 26 + predosle) * 26 + c]) {
            skore += 1.5;
        }
        predpredosle = predosle;
        predosle = c;
    }
    return skore / static_cast<double>(text.size());
}

// Menime vzdy jeden znak hesla, kym sa skore desifrovaneho textu zlepsuje.
double doladHeslo(const std::vector<int>& text, std::string& heslo, const Jazyk& jazyk) {
    double najlepsie = ohodnot(text, heslo, jazyk);
    for (int kolo = 0; kolo < 10; ++kolo) {
        bool zmena = false;
        for (char& znak : heslo) {
            char najlepsiZnak = znak;
            for (char pokus = 'A'; pokus <= 'Z'; ++pokus) {
                znak = pokus;
                const double skore = ohodnot(text, heslo, jazyk);
                if (skore > najlepsie + 1e-9) {
                    najlepsie = skore;
                    najlepsiZnak = pokus;
                    zmena = true;
                }
            }
            znak = najlepsiZnak;
        }
        if (!zmena) {
            break;
        }
    }
    return najlepsie;
}

// Opakovane heslo (napr. ABCABC) ma rovnaky ucinok ako najkratsia perioda.
std::string skratHeslo(const std::string& heslo) {
    for (std::size_t dlzka = 1; dlzka <= heslo.size(); ++dlzka) {
        if (heslo.size() % dlzka != 0) {
            continue;
        }
        bool opakujeSa = true;
        for (std::size_t i = dlzka; i < heslo.size(); ++i) {
            if (heslo[i] != heslo[i % dlzka]) {
                opakujeSa = false;
                break;
            }
        }
        if (opakujeSa) {
            return heslo.substr(0, dlzka);
        }
    }
    return heslo;
}

struct Vysledok {
    std::string heslo;
    std::string jazyk;
    double skore = -std::numeric_limits<double>::infinity();
};

Vysledok najdiHeslo(const std::vector<int>& text, const std::vector<Jazyk>& jazyky) {
    if (text.size() < 2 * MIN_HESLO) {
        throw std::runtime_error("Prilis kratky text: treba aspon 30 pismen A-Z.");
    }
    Vysledok najlepsi;
    for (int dlzka = MIN_HESLO; dlzka <= MAX_HESLO; ++dlzka) {
        for (const Jazyk& jazyk : jazyky) {
            std::string heslo = odhadniHeslo(text, dlzka, jazyk);
            const double skore = doladHeslo(text, heslo, jazyk);
            if (skore > najlepsi.skore) {
                najlepsi = {skratHeslo(heslo), jazyk.nazov, skore};
            }
        }
    }
    return najlepsi;
}

std::vector<fs::path> najdiSubory(const fs::path& vstup) {
    if (fs::is_regular_file(vstup)) {
        return {vstup};
    }
    if (!fs::is_directory(vstup)) {
        throw std::runtime_error("Subor alebo priecinok neexistuje: " + vstup.string());
    }
    std::vector<fs::path> subory;
    for (const auto& polozka : fs::directory_iterator(vstup)) {
        if (polozka.is_regular_file() && polozka.path().extension() == ".txt") {
            subory.push_back(polozka.path());
        }
    }
    std::sort(subory.begin(), subory.end());
    if (subory.empty()) {
        throw std::runtime_error("Priecinok neobsahuje ziadne vstupne subory .txt.");
    }
    return subory;
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "Pouzitie: KryptoBezp [subor | priecinok]\n"
                     "Bez argumentov spracuje vsetky .txt subory v projektovom data/.\n"
                     "Hlada hesla dlzky 15-25 pre slovencinu aj anglictinu.\n"
                     "Vysledky ulozi do podpriecinka vysledky/ pri vstupoch.\n";
        return 0;
    }
    if (argc > 2) {
        std::cerr << "Pouzitie: KryptoBezp [subor | priecinok]\n";
        return 1;
    }
    try {
        const fs::path vstup = fs::absolute(argc == 2 ? fs::path(argv[1]) : fs::path(DEFAULT_DATA_DIR));
        const auto subory = najdiSubory(vstup);
        const fs::path vystup = (fs::is_directory(vstup) ? vstup : vstup.parent_path()) / "vysledky";
        fs::create_directories(vystup);
        const auto jazyky = vytvorJazyky();
        std::ostringstream prehlad;
        prehlad << "Vigenere: hladanie hesiel dlzky 15-25 (A-Z, mod 26)\n"
                   "Statisticky odhad; pri kratkych textoch skontrolujte citatelnost.\n\n";
        int chyby = 0;
        for (const auto& subor : subory) {
            try {
                const std::string text = nacitaj(subor);
                const auto pismena = vyberPismena(text);
                std::cout << "Analyzujem " << subor.filename().string() << "..." << std::endl;
                const Vysledok vysledok = najdiHeslo(pismena, jazyky);
                const std::string priamyText = desifruj(text, vysledok.heslo);
                const fs::path ciel = vystup / (subor.stem().string() + "_desifrovane.txt");
                uloz(ciel, priamyText);
                std::ostringstream zaznam;
                zaznam << subor.filename().string() << '\n'
                       << "Heslo: " << vysledok.heslo << '\n'
                       << "Dlzka hesla: " << vysledok.heslo.size() << '\n'
                       << "Jazyk: " << vysledok.jazyk << '\n'
                       << "Pocet pismen: " << pismena.size() << '\n'
                       << "Skore: " << std::fixed << std::setprecision(4) << vysledok.skore << '\n'
                       << "Vystup: " << ciel.filename().string() << '\n';
                if (vysledok.heslo.size() < MIN_HESLO) {
                    zaznam << "Poznamka: najdena najkratsia perioda opakovaneho hesla je kratsia nez 15.\n";
                }
                prehlad << zaznam.str() << '\n';
                std::cout << zaznam.str() << '\n' << priamyText << "\n\n";
            } catch (const std::exception& e) {
                ++chyby;
                prehlad << subor.filename().string() << " - CHYBA: " << e.what() << "\n\n";
                std::cerr << subor.filename().string() << ": " << e.what() << '\n';
            }
        }
        // Jednotlive spustenie neprepise pripadny prehlad celej davky.
        const fs::path prehladCesta = vystup / (fs::is_directory(vstup)
            ? "hesla.txt" : subory.front().stem().string() + "_heslo.txt");
        uloz(prehladCesta, prehlad.str());
        std::cout << "Prehlad hesiel: " << prehladCesta.string() << '\n';
        return chyby == 0 ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Chyba: " << e.what() << '\n';
        return 1;
    }
}
