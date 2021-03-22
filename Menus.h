#pragma once

#include <stdint.h>

typedef struct MENUITEM
{
	char* Name;
	int16_t x;
	int16_t y;
	void(*Action)(void); //A function pointer

}MENUITEM;

typedef struct MENU
{
	char* Name;
	uint8_t SelectedItem;
	uint8_t ItemCount;
	MENUITEM** Items;

}MENU;


void MenuItem_TitleScreen_Resume(void);
void MenuItem_TitleScreen_StartNew(void);
void MenuItem_TitleScreen_Options(void);
void MenuItem_TitleScreen_Exit(void);
void MenuItem_ExitYesNo_Yes(void);
void MenuItem_ExitYesNo_No(void);


//Creating the TitleScreen Menu
MENUITEM gMI_ResumeGame = { "Resume", (GAME_RES_WIDTH / 2) - ((6 * 6) / 2), 100, MenuItem_TitleScreen_Resume };
MENUITEM gMI_StartGame = { "Start New Game", (GAME_RES_WIDTH / 2) - ((14 * 6) / 2), 120, MenuItem_TitleScreen_StartNew };
MENUITEM gMI_Options = { "Options", (GAME_RES_WIDTH / 2) - ((7 * 6) / 2), 140, MenuItem_TitleScreen_Options };
MENUITEM gMI_Exit = { "Exit", (GAME_RES_WIDTH / 2) - ((5 * 6) / 2), 160, MenuItem_TitleScreen_Exit };
MENUITEM* gMI_TitleScreenItems[] = { &gMI_ResumeGame, &gMI_StartGame, &gMI_Options, &gMI_Exit };
MENU gMenu_TitleScreen = { "Title Screen Menu", 0, _countof(gMI_TitleScreenItems), gMI_TitleScreenItems };


//Creating the ExitYesNo Menu
MENUITEM gMI_ExitYesNo_Yes = { "Yes", (GAME_RES_WIDTH / 2) - ((3 * 6) / 2), 100, MenuItem_ExitYesNo_Yes };
MENUITEM gMI_ExitYesNo_No = { "No", (GAME_RES_WIDTH / 2) - ((2 * 6) / 2), 115, MenuItem_ExitYesNo_No };
MENUITEM* gMI_ExitYesNoItems[] = { &gMI_ExitYesNo_Yes, &gMI_ExitYesNo_No };
MENU gMenu_ExitYesNo = { "Are you sure you want to exit?", 1, _countof(gMI_ExitYesNoItems), gMI_ExitYesNoItems };