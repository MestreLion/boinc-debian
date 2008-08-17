#include <cstdio>

#include "md5_file.h"

int main(int, char** argv) {
    char out[33];
    double nbytes;

    md5_file(argv[1], out, nbytes);
    printf("%s\n%f bytes\n", out, nbytes);

    return 0;
}

const char *BOINC_RCSID_c6f4ef0a81 = "$Id: md5_test.C 4979 2005-01-02 18:29:53Z ballen $";
