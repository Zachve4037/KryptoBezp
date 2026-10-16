# Vigenèrova šifra

Program automaticky hľadá heslá dĺžky **15 až 25 znakov** pre slovenské aj anglické texty. Stačí spustiť cieľ **KryptoBezp** v CLione. Bez argumentov spracuje všetky súbory `.txt` priamo v priečinku `data`.

## Použitie

Každý vstupný súbor obsahuje jeden text zašifrovaný jedným heslom. Aktuálne zadanie je v `data/viegen1.txt` až `data/viegen4.txt`.

Výsledky sa zobrazia v konzole a uložia do `data/vysledky/`:

- `hesla.txt` – heslá, ich dĺžky a rozpoznané jazyky pre celú dávku.
- `viegen1_desifrovane.txt` až `viegen4_desifrovane.txt` – úplné dešifrované texty so zachovaným formátovaním.

Možno zadať aj iný súbor alebo priečinok:

```powershell
.\build\KryptoBezp.exe
.\build\KryptoBezp.exe data\viegen3.txt
.\build\KryptoBezp.exe data
.\build\KryptoBezp.exe --help
```

Pri vlastnom vstupe vznikne podpriečinok `vysledky` vedľa vstupného súboru alebo v zadanom priečinku. Spracovanie jedného súboru vytvorí samostatný prehľad `nazov_heslo.txt`, takže neprepíše prehľad celej dávky. Ďalšie spustenie aktualizuje príslušné výsledky.

## Zostavenie

Stačí kompilátor C++20 a CMake 3.16 alebo novší, bez ďalších knižníc a bez internetu:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Pri generátore Visual Studio je program v `build\Release\KryptoBezp.exe`. Pri MinGW/Ninja je v `build\KryptoBezp.exe`. V CLione stačí obnoviť CMake a spustiť **KryptoBezp**; program nájde projektový priečinok `data` aj pri spustení z adresára zostavenia. Pôvodná prúdová šifra má samostatný cieľ **StreamCipher**, pretože obsahuje vlastnú funkciu `main`.

## Ako hľadanie funguje

1. Vyberú sa iba veľké písmená `A`–`Z`. Všetky ostatné znaky zostanú vo výstupe na pôvodnom mieste a neposúvajú heslo. Malé písmená sa nemenia na veľké.
2. Pre každú dĺžku hesla od 15 do 25 sa písmená rozdelia podľa pozície v hesle. Každá skupina je samostatná Caesarova šifra.
3. Pre každú skupinu sa vyskúša všetkých 26 posunov a vyberie sa najpravdepodobnejší podľa približného zastúpenia písmen v slovenčine alebo angličtine.
4. Heslo sa doladí zmenami jednotlivých znakov podľa výskytu bežných dvojíc a trojíc písmen v celom texte. Vyberie sa kombinácia hesla a jazyka s najvyšším skóre.
5. Dešifrovanie používa `P = (C − K + 26) mod 26`, kde `A = 0` až `Z = 25`.

Približné zastúpenie 26 písmen a krátke zoznamy bežných dvojíc a trojíc sú priamo v zdrojovom kóde. Program nesťahuje frekvenčné tabuľky ani nepoužíva externý slovník. Heslá zo zadania nie sú v programe uložené.

Ide o štatistické hľadanie, takže veľmi krátky alebo nezvyčajný text môže viesť k nesprávnemu odhadu. Skóre slúži na porovnanie kandidátov, nie ako percento istoty. Vstup musí obsahovať aspoň 30 písmen `A`–`Z`.

Staršie súbory `viegen_input` a `viegen_output` nemajú príponu `.txt`, preto sa do dávky nezaradia. Dajú sa spracovať zadaním konkrétnej cesty. Obsahujú kratšie opakované heslá; program pri takejto zhode vypíše najkratšiu periódu aj poznámku o kratšom hesle.
