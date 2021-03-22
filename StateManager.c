#include <stdlib.h>
#include "statemanager.h"



int STATEMANAGER_Scale(STATEMANAGER* statemanager) 
{
    statemanager->Capacity *= 2;
    statemanager->Stack = realloc(statemanager->Stack, (statemanager->Capacity * sizeof(STATE*)));
    return statemanager->Capacity;
}

int STATEMANAGER_Init(STATEMANAGER* statemanager) 
{
    statemanager->Capacity = 3;
    statemanager->Stack = malloc(statemanager->Capacity * sizeof(STATE*));
    statemanager->Top = -1;
    return 0;
}

int STATEMANAGER_Free(STATEMANAGER* statemanager)
{
    do
    {
        STATEMANAGER_Pop(statemanager);
    } while (statemanager->Top > -1);

    free(statemanager->Stack);
    return 0;
}

int STATEMANAGER_Push(STATEMANAGER* statemanager, STATE* state) 
{
    if (statemanager->Top + 1 == statemanager->Capacity) STATEMANAGER_Scale(statemanager);
    statemanager->Top++;
    statemanager->Stack[statemanager->Top] = state;
    if (state->Init) state->Init();
    return statemanager->Top;
}

int STATEMANAGER_Pop(STATEMANAGER* statemanager) 
{
    if (statemanager->Top == -1) return 0;
    STATE* top = STATEMANAGER_Top(statemanager);
    if (top->Destroy) top->Destroy();
    statemanager->Stack[statemanager->Top] = NULL;
    statemanager->Top--;
    return statemanager->Top;
}

STATE* STATEMANAGER_Top(STATEMANAGER* statemanager) 
{
    return statemanager->Stack[statemanager->Top];
}

int STATEMANAGER_Draw(STATEMANAGER* statemanager)
{
    STATE* state = STATEMANAGER_Top(statemanager);
    if (state->DRAW) return state->DRAW();
    return 1;
}

int STATEMANAGER_PPi(STATEMANAGER* statemanager)
{
    STATE* state = STATEMANAGER_Top(statemanager);
    if (state->PPi) return state->PPi();
    return 1;
}