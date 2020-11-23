#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct dict_entry_s {
    char* key;
    int value;
} dict_entry_s;

typedef struct dict_s {
    int len;
    int cap;
    dict_entry_s* entry;
} dict_s, * dict_t;


int DictFindIndex(dict_t dict, char* key);
    
int DictFind(dict_t dict, char* key);

void DictAdd(dict_t dict, char* key, int value);

dict_t DictNew(void);

void DictFree(dict_t dict);