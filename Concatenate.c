#include "Concatenate.h"

#include <stdlib.h>
#include <stdarg.h>

char* strconcat(_In_ int num_args,_In_ ...) {
    int strsize = 0;
    va_list ap;
    va_start(ap, num_args);
    for (int i = 0; i < num_args; i++)
        strsize += strlen(va_arg(ap, char*));

    char* res = malloc(strsize + 1);
    strsize = 0;
    va_start(ap, num_args);
    for (int i = 0; i < num_args; i++) {
        char* s = va_arg(ap, char*);
        strcpy(res + strsize, s);
        strsize += strlen(s);
    }
    va_end(ap);
    res[strsize] = '\0';

    return res;
}