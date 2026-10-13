#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

const char *heslo = "JOHA";

void kassisk(const std::string &file_path);

int main() {
    FILE *input = stdin;
    FILE *output = stdout;

    int c;

    input = fopen("../data/viegen_input", "rt");
    if (input == nullptr) {
        std::cout << "unable to open input file!" << std::endl;
        return 1;
    }

    output = fopen("../data/viegen_output", "wt");
    if (output == nullptr) {
        std::cout << "unable to open output file!" << std::endl;
        fclose(input);
        return 2;
    }

    const char *p = heslo;

    while ((c = fgetc(input)) != EOF) {
        c = std::toupper(c);

        if (c >= 'A' && c <= 'Z') {
            int i = c - 'A';
            int j = *p - 'A';

            i = (i + j) % 26;

            fputc(i + 'A', output);

            p++;
            if (!*p) {
                p = heslo;
            }
        } else {
            fputc(c, output);
        }
    }

    fclose(input);
    fclose(output);

    kassisk("../data/viegen_output");

    return 0;
}


void kassisk(const std::string &file_path) {
    std::ifstream file(file_path);

    if (!file) {
        std::cout << "unable to open file!!!" << std::endl;
        return;
    }

    std::string text;
    char c;

    while (file.get(c)) {
        c = std::toupper(static_cast<unsigned char>(c));

        if (c >= 'A' && c <= 'Z') {
            text += c;
        }
    }

    file.close();

    std::cout << "\nText pre Kasiski analyzu:\n";
    std::cout << text << "\n\n";

    for (int i = 0; i < static_cast<int>(text.size()) - 2; ++i) {
        std::string triple = text.substr(i, 3);

        for (int j = i + 1; j < static_cast<int>(text.size()) - 2; ++j) {
            if (triple == text.substr(j, 3)) {
                std::cout
                        << j - i
                        << " (" << triple << "), ";
            }
        }
    }

    std::cout << std::endl;
}
