#include "Dictionary.h"


int DictFindIndex(dict_t dict, char* key) 
{
    for (int i = 0; i < dict->len; i++) {
        if (key == dict->entry[i].key) {
            return i;
        }
    }
    return -1;
}

int DictFind(dict_t dict, char* key) 
{
    int idx = DictFindIndex(dict, key);
    return dict->entry[idx].value;
}

void DictAdd(dict_t dict, char* key, int value) 
{
    int idx = DictFindIndex(dict, key);
    if (idx != -1) {
        dict->entry[idx].value = value;
        return;
    }
    if (dict->len == dict->cap) {
        dict->cap *= 2;
        dict->entry = realloc(dict->entry, dict->cap * sizeof(dict_entry_s));
    }
    dict->entry[dict->len].key = key;
    dict->entry[dict->len].value = value;
    dict->len++;
}

dict_t DictNew(void) 
{
    dict_s proto = { 0, 10, malloc(10 * sizeof(dict_entry_s)) };
    dict_t d = malloc(sizeof(dict_s));
    *d = proto;
    return d;
}

void DictFree(dict_t dict) //Frees the memory from the created dictionary
{
    for (int i = 0; i < dict->len; i++) {
        free(dict->entry[i].key);
    }
    free(dict->entry);
    free(dict);
}

