#pragma once

#include <xaudio2.h> //This should not be here find a way to fix this

#ifdef _DEBUG
	#define ASSERT(Expression, Message) if (!(Expression)) { *(int*)0 = 0; }
#else
	#define ASSERT(Expression, Message) ((void)0);
#endif

#define GAME_NAME "GAME_A"

#define GAME_RES_WIDTH  384
#define GAME_RES_HEIGHT 240
#define GAME_BPP		32
#define GAME_DRAWING_AREA_MEMORY_SIZE (GAME_RES_WIDTH * GAME_RES_HEIGHT * (GAME_BPP / 8))
#define CALCULATE_AVG_FPS_EVERY_X_FRAMES	120
#define TARGET_MICROSECONDS_PER_FRAME 16667ULL
#define NUMBER_OF_SFX_SOURCE_VOICES 4
#define SIMD
//#define GRID_MOVEMENT

#define SUIT_0 0
#define SUIT_1 1
#define SUIT_2 2

#define	FACING_DOWN_0 0
#define FACING_DOWN_1 1
#define FACING_DOWN_2 2

#define FACING_LEFT_0 3
#define FACING_LEFT_1 4
#define FACING_LEFT_2 5

#define FACING_RIGHT_0 6
#define FACING_RIGHT_1 7
#define FACING_RIGHT_2 8

#define FACING_UP_0 9
#define FACING_UP_1 10
#define FACING_UP_2 11

#define DIRECTION_DOWN 0
#define DIRECTION_LEFT 3
#define DIRECTION_RIGHT 6
#define DIRECTION_UP 9

#define FONT_SHEET_CHARACTERS_PER_ROW 98

typedef enum LOGLEVEL
{
	LL_NONE = 0,
	LL_ERROR = 1,
	LL_WARNING = 2,
	LL_INFO = 3,
	LL_DEBUG = 4

}LOGLEVEL;

#define LOG_FILE_NAME GAME_NAME".log"


#pragma warning(disable: 4820) //Disable warning about structure padding
#pragma warning(disable: 5045) //Disable warning about Specture/Meltdown CPU volnerability
//#pragma warning(disable: 26451)


typedef LONG(NTAPI* _NtQueryTimerResolution) (OUT PULONG MinimumResolution, OUT PULONG MaximumResolution, OUT PULONG CurrentResolution);

_NtQueryTimerResolution NtQueryTimerResolution;


typedef struct GAMEBITMAP 
{
	BITMAPINFO BitMapInfo;	//44 bytes
	void* Memory;			//8 bytes

} GAMEBITMAP;

typedef struct GAMESOUND
{
	WAVEFORMATEX WaveFormat;
	XAUDIO2_BUFFER Buffer;

} GAMESOUND;

typedef struct PIXEL32
{
	uint8_t Blue;
	uint8_t Green;
	uint8_t Red;
	uint8_t Alpha;

} PIXEL32;


typedef struct GAMEPERFDATA
{
	uint64_t TotalFramesRendered;
	float RawFPSAverage;
	float CoockedFPSAverage;
	int64_t PerfFrequency;
	MONITORINFO MonitorInfo;
	int32_t MonitorWidth;
	int32_t MonitorHeight;
	BOOL DisplayDebugInfo;
	ULONG MinimumTimerResolution;
	ULONG MaximumTimerResolution;
	ULONG CurrentTimerResolution;
	DWORD HandleCount;
	PROCESS_MEMORY_COUNTERS_EX MemInfo;
	SYSTEM_INFO SystemInfo;
	int64_t CurrentSystemTime;
	int64_t PreviousSystemTime;
	double CPUPercent;

} GAMEPERFDATA;

typedef struct _ENTITY
{
	char Name[12];
	GAMEBITMAP Sprite;
	int16_t ScreenPosX;
	int16_t ScreenPosY;
	int NumberOfAnimations;

} _ENTITY;


typedef struct HERO
{
	_ENTITY Entity;
	uint8_t MovementRemaining;
	uint8_t Direction;
	uint8_t CurrentArmor;
	int32_t HP;
	int32_t Strength;
	int32_t MP;

}HERO;


typedef struct REGISTRYPARAPMS
{
	DWORD LogLevel;

} REGISTRYPARAPMS;


typedef struct GAMEINPUT
{
	int16_t EscapeKeyIsDown;
	int16_t DebugKeyIsDown;
	int16_t LeftKeyIsDown;
	int16_t RightKeyIsDown;
	int16_t UpKeyIsDown;
	int16_t DownKeyIsDown;
	int16_t ChooseKeyIsDown;

	int16_t DebugKeyWasDown;
	int16_t LeftKeyWasDown;
	int16_t RightKeyWasDown;
	int16_t UpKeyWasDown;
	int16_t DownKeyWasDown;
	int16_t ChooseKeyWasDown;

} GAMEINPUT;


LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam);

DWORD CreateMainGameWindow(void);

BOOL GameIsAlreadyRunning(void);

void ProcessPlayerInput(void);

DWORD Load32BppBitmapFromFile(_In_ char* FileName, _Inout_ GAMEBITMAP* GameBitmap);

DWORD InitializeHero(void);

void Blit32BppBitmapToBuffer(_In_ GAMEBITMAP* GameBitmap, _In_ uint16_t x, _In_ uint16_t y);

void BlitStringToBuffer(_In_ char* String, _In_ GAMEBITMAP* FontSheet, _In_ PIXEL32* Color, _In_ uint16_t x, _In_ uint16_t y); 

void RenderFrameGraphics(void);

void ClearScreen(_In_ __m128i* Color);

DWORD LoadRegistryParameters(void);

void LogMessageA(_In_ LOGLEVEL LogLevel, _In_ char* Message, _In_ ...);

void DrawDebugInfo(void);

void FindFirstConnectedGamepad(void);

HRESULT InitializeSoundEngine(void);

DWORD LoadWavFromFile(_In_ char* FileName, _Inout_ GAMESOUND* GameSound);

void PlayGameSound(_In_ GAMESOUND* GameSound);


//Menu Items Functions
void MenuItem_TitleScreen_Resume(void);

void MenuItem_TitleScreen_StartNew(void);

void MenuItem_TitleScreen_Options(void);

void MenuItem_TitleScreen_Exit(void);


//Draw Functions
void DRAW_Overworld(void);

void DRAW_TitleScreen(void);

void DRAW_ExitYesNoScreen(void);


//Process Player Inputs Functions
void PPI_Overworld(void);

void PPI_TitleScreen(void);

void PPI_ExitYesNoScreen(void);
