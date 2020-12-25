#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct dict_entry_s {
    const char* key;
    int value;
} dict_entry_s;

typedef struct dict_s {
    int len;
    int cap;
    dict_entry_s* entry;
} dict_s, * dict_t;


int DictFindIndex(_In_ dict_t dict, _In_ const char* key);
    
int DictFind(_In_ dict_t dict, _In_ const char* key);

void DictAdd(_In_ dict_t dict, _In_ const char* key, _In_ int value);

dict_t DictNew(void);

void DictFree(_In_ dict_t dict);