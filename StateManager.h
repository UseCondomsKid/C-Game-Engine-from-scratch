#pragma once

#ifndef ENGINE_STATEMANAGER_H
#define ENGINE_STATEMANAGER_H

typedef unsigned int (*fnPtr)();
typedef unsigned int (*fnPtrFl)();

typedef struct 
{
	fnPtr Init;
	fnPtrFl DRAW;
	fnPtrFl PPi;
	fnPtr Destroy;
} STATE;

typedef struct
{
	STATE** Stack;
	int Capacity;
	int Top;
} STATEMANAGER;

int STATEMANAGER_Init(STATEMANAGER* statemanager);
int STATEMANAGER_Free(STATEMANAGER* statemanager);
int STATEMANAGER_Push(STATEMANAGER* statemanager, STATE* state);
int STATEMANAGER_Pop(STATEMANAGER* statemanager);
STATE* STATEMANAGER_Top(STATEMANAGER* statemanager);

int STATEMANAGER_Draw(STATEMANAGER* statemanager);
int STATEMANAGER_PPi(STATEMANAGER* statemanager);


#endif
