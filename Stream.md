# Prúdová šifra RC4 – postup riešenia

Cieľom bolo nájsť všetky štyri heslá v rozsahu `"10000000"` až `"99999999"` a dešifrovať súbory do pôvodného UTF-8 textu. Riešenie je v [src/StreamCipher.cpp](src/StreamCipher.cpp). Vychádza z [programu stream.c zo zadania](https://mayor.fri.uniza.sk/krypto/2026/u3/stream.c).

## 1. Kontrola vstupov

V projekte sú vstupy pomenované `stream1.bin` až `stream4.bin`. Súbory `viegen1.txt` až `viegen4.txt` patria k predchádzajúcej úlohe.

Všetky štyri binárne súbory som skontroloval pomocou MD5. Odtlačky súhlasia so zadaním:

| Súbor v projekte | Názov v zadaní | MD5 |
|---|---|---|
| `data/stream1.bin` | `text1_enc.bin` | `840605b4c0b5b2995726dd4566ae5af7` |
| `data/stream2.bin` | `text2_enc.bin` | `d8d5d3f3b09ccff4c5be1821b7a76a6c` |
| `data/stream3.bin` | `text3_enc.bin` | `7a6974d86bab45c20d202b3883554ed4` |
| `data/stream4.bin` | `text4_enc.bin` | `d56cbc434909ff7fe1bb5001b629673b` |

Kontrolu možno zopakovať v PowerShelli:

```powershell
Get-FileHash -Algorithm MD5 data\stream*.bin
```

Vstupy aj výstupy sa otvárajú v binárnom režime. Nemenia sa konce riadkov, diakritika ani žiadne iné bajty.

## 2. Vytvorenie kľúča: getKey

Heslo je **reťazec ôsmich číslic**, nie celé číslo použité priamo ako násada generátora. Funkcia `getKey` opakuje znaky hesla do poľa 256 bajtov:

```text
heslo:       "12345678"
256 B kľúč:  "123456781234567812345678..."
key[i] = heslo[i % 8]
```

Číslica `'1'` má ASCII hodnotu 49, nie 1. Číslica `'8'` má hodnotu 56. Keďže 256 je násobkom 8, heslo sa zopakuje presne 32-krát.

## 3. Inicializácia a použitie RC4

Najprv sa pole `S` naplní hodnotami 0 až 255. Potom sa premieša podľa kľúča:

```text
j = 0
pre i od 0 po 255:
    j = (j + S[i] + key[i]) mod 256
    vymeň S[i] a S[j]
```

Po inicializácii sa oba indexy nastavia na nulu. Každý bajt prúdu sa vypočíta takto:

```text
i = (i + 1) mod 256
j = (j + S[i]) mod 256
vymeň S[i] a S[j]
r = S[(S[i] + S[j]) mod 256]
```

Dešifrovanie je XOR bajtu šifrového textu s bajtom prúdu: `P = C XOR r`. Rovnaká operácia slúži aj na šifrovanie. Pri každom novom hesle sa stav RC4 inicializuje od začiatku. Nepreskakujú sa počiatočné bajty prúdu.

## 4. Útok hrubou silou

Program postupne skúša všetkých **90 000 000** možných hesiel od `10000000` po `99999999` vrátane. Každé číslo najprv prevedie na reťazec. Ak už nájde heslá všetkých vstupov, skončí skôr.

Pre kandidáta vytvorí stav RC4 a vygeneruje prvých 64 bajtov prúdu. Pomocou nich skontroluje začiatky všetkých ešte nevyriešených súborov. Zadanie hovorí, že pôvodné texty sú v UTF-8, preto kandidát vypadne hneď, ako po XOR vznikne neplatná UTF-8 postupnosť.

Kontrola podporuje aj diakritiku a viacbajtové znaky. Ak 64. bajt patrí k rozpracovanému znaku, kandidát sa ešte nezamietne. Pri úspešnej kontrole začiatku sa dešifruje celý súbor a skontroluje sa úplnosť UTF-8 až do konca.

Rovnaké heslo vytvára rovnaký počiatočný prúd pre každý vstup. Preto možno jedno skúšanie hesla použiť na všetky štyri súbory. Každý súbor sa napriek tomu posudzuje samostatne a môže mať iné heslo. Používa sa jedno vlákno, bez externých knižníc, slovníkov a frekvenčných tabuliek.

Platné UTF-8 je filter kandidátov, nie matematický dôkaz správneho hesla. Pri nájdených výsledkoch treba skontrolovať aj zmysluplnosť textov.

## 5. Uloženie výsledkov a meranie času

Čas sa meria pomocou `std::chrono::steady_clock`. Pri každom súbore ide o čas od začiatku **spoločného prelamovania** po nájdenie jeho hesla. Nie sú to časy štyroch samostatných útokov a nesčítavajú sa. Počet pokusov je počet dovtedy overených hesiel v spoločnom prechode.

Ihneď po nájdení hesla sa uložia:

- `data/vysledky/streamN_desifrovane.txt` – úplný pôvodný text v UTF-8.
- `data/vysledky/streamN_heslo.txt` – heslo, nameraný čas a počet pokusov.
- `data/vysledky/stream_hesla.txt` – spoločný prehľad výsledkov.

Pri ďalšom spustení sa heslá načítajú z uložených súborov a overia sa dešifrovaním aktuálnych vstupov. Pôvodné namerané časy sa zachovajú. Pri obhajobe tak program nemusí opakovať prelamovanie. Heslá nie sú zapísané v zdrojovom kóde.

## 6. Spustenie

V CLione obnov CMake, zvoľ zostavenie **Release** a spusti cieľ **StreamCipher**. Bez argumentov sa spracujú všetky súbory `.bin` priamo v projektovom priečinku `data`.

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target StreamCipher
.\build\StreamCipher.exe
```

Pri generátore Visual Studio je program v `build\Release\StreamCipher.exe`. V prostredí MinGW musí byť jeho priečinok `bin` dostupný v `PATH`, napríklad cez terminál CLionu.

Možnosti spustenia:

```powershell
.\build\StreamCipher.exe                         # všetky .bin v data; použije uložené heslá
.\build\StreamCipher.exe data\stream1.bin        # iba jeden súbor
.\build\StreamCipher.exe data                    # vlastný priečinok so vstupmi
.\build\StreamCipher.exe --znova                 # zopakuje útok a nameria nové časy
.\build\StreamCipher.exe --help
```

Pri zadaní inej cesty sa výsledky ukladajú do jej podpriečinka `vysledky`. Pri jednom súbore vznikne samostatný prehľad `nazov_prehlad.txt`, aby sa neprepísal prehľad celej dávky. Vstup musí mať aspoň 64 bajtov. Priebeh útoku sa vypisuje po každom milióne pokusov.

## 7. Výsledky a overenie

| Súbor | Nájdené heslo | Čas od začiatku spoločného útoku | Počet skúšaných hesiel | Obsah |
|---|---|---:|---:|---|
| `stream1.bin` | `12345678` | 2,506564 s | 2 345 679 | Orange a solárne napájanie vysielačov |
| `stream2.bin` | `55555555` | 59,828266 s | 45 555 556 | Robot Flippy II a príprava hranolčekov |
| `stream3.bin` | `89898989` | 105,080597 s | 79 898 990 | SpaceX, NASA a Hubbleov teleskop |
| `stream4.bin` | `73081638` | 83,001187 s | 63 081 639 | Bitcoin: A Peer-to-Peer Electronic Cash System |

Všetky heslá boli nájdené za približne **105,08 sekundy** v jednom spoločnom prechode. Časy pochádzajú zo skutočného behu zostaveného pomocou GCC 15.2 s optimalizáciou `-O3`, nie z odhadu. Na inom počítači alebo pri zostavení Debug budú iné.

Úplné výsledky:

- [Prehľad hesiel a časov](data/vysledky/stream_hesla.txt)
- [Text 1](data/vysledky/stream1_desifrovane.txt)
- [Text 2](data/vysledky/stream2_desifrovane.txt)
- [Text 3](data/vysledky/stream3_desifrovane.txt)
- [Text 4](data/vysledky/stream4_desifrovane.txt)

Pri kontrole som overil zhodu so šifrovaním pôvodným `stream.c` na skúšobných textoch s diakritikou a formátovaním. Nájdené texty sú zmysluplné a platné UTF-8. Výsledky všetkých štyroch súborov boli tiež porovnané bajt po bajte s dešifrovaním pôvodným programom. Opätovné spustenie používa uložené heslá a nemení pôvodné časy.
