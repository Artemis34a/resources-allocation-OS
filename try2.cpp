//
// Created by artemis on 07/11/2025.
//
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int une_globale;
const char* une_constante = "une-chaine";

void dump_maps(void) {
    FILE *f = fopen("/proc/self/maps", "r");
    assert(f != NULL);
    for(int c; ((c = fgetc(f)) != EOF); ) {
        putchar(c);
    }
    fclose(f);
    printf("Tapez RETURN >> ");
    getchar();
}

void print_local(int n) {
    int une_locale;
    printf("adresse de une_locale/%d = %012lx\n", n, (unsigned long) & une_locale);
    if (n < 4) print_local(n+1);
}

int main(int argc, char* argv[]) {
    static long une_statique = 1;

    char *alloc1 = static_cast<char *>(malloc(1024L * 1024L * 10L)); /* 10 Mo */
    printf("PID = %d\n", getpid());
    printf("adresse de une_globale  = %012lx\n", (unsigned long) & une_globale);
    printf("adresse de une_statique = %012lx\n", (unsigned long) & une_statique);
    print_local(0);
    printf("adresse de alloc1       = %012lx\n", (unsigned long) alloc1);
    printf("adresse de une_fonction = %012lx\n", (unsigned long) & main);
    printf("adresse d'une constante = %012lx\n", (unsigned long) une_constante);

    dump_maps();

    char *alloc2 = static_cast<char *>(malloc(1024L * 1024L * 100L)); /* 100 Mo */
    printf("adresse de alloc2       = %012lx\n", (unsigned long) alloc2);
    dump_maps();

    return (EXIT_SUCCESS);
}