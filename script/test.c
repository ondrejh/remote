#include <stdio.h>

#include "webi_test.h"
//#include "webi.h" // can't use original because of PROGMEM pragma

const char *dir = "../webi";

int main(void) {
    printf("Webi header file test.\n");

    for (int i=0; i<NFILES; i++) {
        char fname[128];
        int p = sprintf(fname, dir);
        p += sprintf(&fname[p], fnames[i]);

        printf("%s %d ", fnames[i], flengths[i]);

        FILE *fp;
        fp = fopen(fname, "r");
        unsigned long pp = 0;
        do {
            int c = fgetc(fp);
            if (c == EOF)
                break;
            if (fdata[i][pp++] != (char) c) {
                printf("error %d\n", pp);
                break;
            }
        } while (!feof(fp));
        if (pp == flengths[i]) {
            printf("OK\n");
        }
        fclose(fp);
    }
    return 0;
}