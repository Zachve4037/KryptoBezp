#include <cctype>
#include <stdio.h>
#include <stdlib.h>

#define DECIPHER


/* generator pseudonahodnych cisel */

long my_randx;

void my_seed(long s)
/* pociatocna inicializacia generatora */
{
    my_randx = s;
}

float my_rand(void)
/* generuje nahodne cislo v intervale <0,1) */
{
    /* prechod do dalsieho stavu generatora */
    my_randx = (8121 * my_randx + 28411) % 134456;
    /* vypocet navratovej hodnoty */
    return (float) my_randx / 134456.0f;
}


/* hlavny program */

int main(int argc, char * argv[])
{
    FILE * input = stdin;
    FILE * output = stdout;

    int i, c, r;

    my_seed(0); /* nastav random seed - toto je hodnota kluca */

    if (argc >= 2) {
        input = fopen(argv[1], "rt");
        if (input == NULL) return 1;
    }

    if (argc >= 3) {
        output = fopen(argv[2], "wt");
        if (output == NULL) return 2;
    }

    while ((c = fgetc(input)) != EOF) {

        c = std::toupper(c);

        if (c >= 'A' && c <= 'Z') {
            i = c - 'A';
            /* generuje nahodne cislo 0 az 25 s rovnomernym
            rozdelenim pravdepodobnosti */
            r = (int) (26 * my_rand());
#ifndef DECIPHER
            i = (i + r) % 26;
#else
            i = (i + 26 - r) % 26;
#endif
            fputc(i + 'A', output);
        } else fputc(c, output);
    }

    if (input != stdin) fclose(input);
    if (output != stdout) fclose(output);

    return 0;
}
