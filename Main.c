
#pragma warning(push, 3)

#include<stdio.h>

#include<windows.h>

#include<psapi.h>

#include<emmintrin.h>

#include <xaudio2.h>

#pragma warning(pop)

#include <Xinput.h>

#include<stdint.h>

#include "Main.h"

#include "Menus.h"

#include "StateManager.h"

#include "Animation.h"

#pragma comment(lib, "Winmm.lib")

#pragma comment(lib, "XAudio2.lib")

#pragma comment(lib, "XInput.lib")



HWND gGameWindow;
BOOL gGameIsRunning; //Global vars are initialized to zero by default
GAMEBITMAP gBackBuffer;
GAMEBITMAP g6x7Font;

int gFontCharacterPixelOffset[] = {
    //  .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. ..
        93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,
        //     !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /  0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ?
            94,64,87,66,67,68,70,85,72,73,71,77,88,74,91,92,52,53,54,55,56,57,58,59,60,61,86,84,89,75,90,93,
            //  @  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _
                65,0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,80,78,81,69,76,
                //  `  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z  {  |  }  ~  ..
                    62,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,82,79,83,63,93,
                    //  .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. ..
                        93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,
                        //  .. .. .. .. .. .. .. .. .. .. .. «  .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. »  .. .. .. ..
                            93,93,93,93,93,93,93,93,93,93,93,96,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,95,93,93,93,93,
                            //  .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. ..
                                93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,
                                //  .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. .. F2 .. .. .. .. .. .. .. .. .. .. .. .. ..
                                    93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,97,93,93,93,93,93,93,93,93,93,93,93,93,93
};

GAMEBITMAP gPlayerSpriteSheet;
GAMEPERFDATA gPerformanceData;
HERO gPlayer;
BOOL gWindowHasFocus;
REGISTRYPARAPMS gRegistryParams;
XINPUT_STATE gGamepadState;
int8_t gGamepadID = -1;
uint16_t gDeltaTime;
GAMEINPUT gGameInput;


IXAudio2* gXAudio;
IXAudio2MasteringVoice* gXAudioMasteringVoice;
IXAudio2SourceVoice* gXAudioSFXSourceVoice[NUMBER_OF_SFX_SOURCE_VOICES];
IXAudio2SourceVoice* gXAudioMusicSourceVoice;
uint8_t gSFXSourceVoiceSelector;
float gSFXVolume = 0.5f;
float gMusicVolume = 0.5f;

GAMESOUND gMenuNavigate;
GAMESOUND gMenuChoose;

//GAME STATES
STATEMANAGER gGameStateManager;

STATE* GAMESTATE_PreviousGameState;
STATE* GAMESTATE_DesiredGameState;

STATE GAMESTATE_OpeningSplashScreen;
STATE GAMESTATE_TitleScreen;
STATE GAMESTATE_Overworld;
STATE GAMESTATE_Battle;
STATE GAMESTATE_OptionsScreen;
STATE GAMESTATE_ExitYesNoScreen;
//GAME STATES


ANIMATION ANIMATION_PlayerActiveAnimation;
ANIMATION ANIMATION_PlayerIdle;
ANIMATION ANIMATION_PlayerWalk;

int __stdcall WinMain(_In_ HINSTANCE Instance, _In_opt_ HINSTANCE PreviousInstance, _In_ PSTR CommandLine, _In_ INT CmdShow)
{
    UNREFERENCED_PARAMETER(Instance);

    UNREFERENCED_PARAMETER(PreviousInstance);

    UNREFERENCED_PARAMETER(CommandLine);

    UNREFERENCED_PARAMETER(CmdShow);


    MSG Message = { 0 };
    int64_t FrameStart = 0;
    int64_t FrameEnd = 0;
    int64_t ElapsedMicroseconds = 0;
    int64_t ElapsedMicrosecondsAccumulatorRaw = 0;
    int64_t ElapsedMicrosecondsAccumulatorCooked = 0;
    HMODULE NtDllModuleHandle = NULL;
    FILETIME ProcessCreationTime = { 0 };
    FILETIME ProcessExitTime = { 0 };
    int64_t CurrentUserCPUTime = 0;
    int64_t CurrentKernelCPUTime = 0;
    int64_t PreviousUserCPUTime = 0;
    int64_t PreviousKernelCPUTime = 0;
    HANDLE ProcessHandle = GetCurrentProcess();


    if (LoadRegistryParameters() != ERROR_SUCCESS)
    {
        goto Exit;
    }

    LogMessageA(LL_INFO, "[%s] %s is starting.", __FUNCTION__, GAME_NAME);

    if ((NtDllModuleHandle = GetModuleHandleA("ntdll.dll")) == NULL)
    {
        LogMessageA(LL_ERROR, "[%s] Couldn't load ntdll.dll! Error 0x%08lx!", __FUNCTION__, GetLastError());
        MessageBoxA(NULL, "Couldn't load ntdll.dll!", "Error!", MB_ICONERROR | MB_OK);

        goto Exit;
    }

    if ((NtQueryTimerResolution = (_NtQueryTimerResolution)GetProcAddress(NtDllModuleHandle, "NtQueryTimerResolution")) == NULL)
    {
        LogMessageA(LL_ERROR, "[%s] Couldn't find the NtQueryTimerResolution function in ntdll.dll! Error 0x%08lx!", __FUNCTION__, GetLastError());
        MessageBoxA(NULL, "Couldn't find the NtQueryTimerResolution function in ntdll.dll!", "Error!", MB_ICONERROR | MB_OK);

        goto Exit;
    }


    NtQueryTimerResolution(&gPerformanceData.MinimumTimerResolution, &gPerformanceData.MaximumTimerResolution, &gPerformanceData.CurrentTimerResolution);

    GetSystemInfo(&gPerformanceData.SystemInfo);

    LogMessageA(LL_INFO, "[%s] Number of CPUs: %d", __FUNCTION__, gPerformanceData.SystemInfo.dwNumberOfProcessors);

    switch (gPerformanceData.SystemInfo.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_INTEL:
        {
            LogMessageA(LL_INFO, "[%s] CPU Architecture: x86", __FUNCTION__);
            break;
        }
        case PROCESSOR_ARCHITECTURE_IA64:
        {
            LogMessageA(LL_INFO, "[%s] CPU Architecture: Itanium", __FUNCTION__);

            break;
        }
        case PROCESSOR_ARCHITECTURE_ARM64:
        {
            LogMessageA(LL_INFO, "[%s] CPU Architecture: ARM64", __FUNCTION__);
            break;
        }
        case PROCESSOR_ARCHITECTURE_ARM:
        {
            LogMessageA(LL_INFO, "[%s] CPU Architecture: ARM", __FUNCTION__);
            break;
        }
        case PROCESSOR_ARCHITECTURE_AMD64:
        {
            LogMessageA(LL_INFO, "[%s] CPU Architecture: x64", __FUNCTION__);
            break;
        }
        default:
        {
            LogMessageA(LL_INFO, "[%s] CPU Architecture: Unknown", __FUNCTION__);
        }
    }


    GetSystemTimeAsFileTime(&gPerformanceData.PreviousSystemTime);

    if (GameIsAlreadyRunning() == TRUE)
    {
        LogMessageA(LL_WARNING, "[%s] Another instance of this program is already running!", __FUNCTION__);
        MessageBoxA(NULL, "Another instance of this program is already running!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if (timeBeginPeriod(1) == TIMERR_NOCANDO) //Setting the Timer resolution to 1ms | timeBeginPeriod() returns a TIMERR_NOCANDO if it fails
    {
        LogMessageA(LL_ERROR, "[%s] Failed to set global timer resolution!", __FUNCTION__);
        MessageBoxA(NULL, "Failed to set global timer resolution!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if (SetPriorityClass(ProcessHandle, HIGH_PRIORITY_CLASS) == 0)
    {
        LogMessageA(LL_ERROR, "[%s] Failed to set process priority!", __FUNCTION__, GetLastError());
        MessageBoxA(NULL, "Failed to set process priority!", "Error!", MB_ICONERROR | MB_OK);

        goto Exit;
    }

    if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST) == 0)
    {
        LogMessageA(LL_ERROR, "[%s] Failed to set thread priority! Error 0x%08lx!", __FUNCTION__, GetLastError());
        MessageBoxA(NULL, "Failed to set thread priority!", "Error!", MB_ICONERROR | MB_OK);

        goto Exit;
    }

    if (CreateMainGameWindow() != ERROR_SUCCESS)
    {
        LogMessageA(LL_ERROR, "[%s] CreateMainGameWindow failed!", __FUNCTION__);

        goto Exit;
    }

    if ((Load32BppBitmapFromFile(".\\Assets\\6x7Font.bmpx", &g6x7Font)) != ERROR_SUCCESS)
    {
        LogMessageA(LL_ERROR, "[%s] Loading 6x7Font.bmpx failed!", __FUNCTION__);
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONERROR | MB_OK);

        goto Exit;
    }

    if (InitializeSoundEngine() != S_OK)
    {
        LogMessageA(LL_ERROR, "[%s] Initializing sound engine failed!", __FUNCTION__);
        MessageBoxA(NULL, "InitializeSoundEngine failed!", "Error!", MB_ICONERROR | MB_OK);
        goto Exit;
    }

    if (LoadWavFromFile(".\\Assets\\MenuNavigate.wav", &gMenuNavigate) != ERROR_SUCCESS)
    {
        LogMessageA(LL_ERROR, "[%s] Loading MenuNavigate.wav failed!", __FUNCTION__);
        MessageBoxA(NULL, "LoadWaveFromFile failed!", "Error!", MB_ICONERROR | MB_OK);
        goto Exit;
    }

    if (LoadWavFromFile(".\\Assets\\MenuChoose.wav", &gMenuChoose) != ERROR_SUCCESS)
    {
        LogMessageA(LL_ERROR, "[%s] Loading MenuNavigate.wav failed!", __FUNCTION__);
        MessageBoxA(NULL, "LoadWaveFromFile failed!", "Error!", MB_ICONERROR | MB_OK);
        goto Exit;
    }
    
    QueryPerformanceFrequency((LARGE_INTEGER*)&gPerformanceData.PerfFrequency);


    STATEMANAGER_Init(&gGameStateManager);
    STATEMANAGER_Push(&gGameStateManager, &GAMESTATE_TitleScreen);
    GAMESTATE_TitleScreen.DRAW = DRAW_TitleScreen;
    GAMESTATE_TitleScreen.PPi = PPI_TitleScreen;
    GAMESTATE_Overworld.DRAW = DRAW_Overworld;
    GAMESTATE_Overworld.PPi = PPI_Overworld;
    GAMESTATE_ExitYesNoScreen.DRAW = DRAW_ExitYesNoScreen;
    GAMESTATE_ExitYesNoScreen.PPi = PPI_ExitYesNoScreen;
    //push other game states


    gPerformanceData.DisplayDebugInfo = FALSE;
    gBackBuffer.BitMapInfo.bmiHeader.biSize = sizeof(gBackBuffer.BitMapInfo.bmiHeader);
    gBackBuffer.BitMapInfo.bmiHeader.biWidth = GAME_RES_WIDTH;
    gBackBuffer.BitMapInfo.bmiHeader.biHeight = GAME_RES_HEIGHT;
    gBackBuffer.BitMapInfo.bmiHeader.biBitCount = GAME_BPP;
    gBackBuffer.BitMapInfo.bmiHeader.biCompression = BI_RGB;
    gBackBuffer.BitMapInfo.bmiHeader.biPlanes = 1;
    gBackBuffer.Memory = VirtualAlloc(NULL, GAME_DRAWING_AREA_MEMORY_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (gBackBuffer.Memory == NULL)
    {
        LogMessageA(LL_ERROR, "[%s] Failed to allocate memory for drawing surface! Error 0x%08lx!", __FUNCTION__, GetLastError());
        MessageBoxA(NULL, "Failed to allocate memory for drawing surface!", "Error!", MB_ICONERROR | MB_OK);

        goto Exit;
    }

    memset(gBackBuffer.Memory, 0x7F, GAME_DRAWING_AREA_MEMORY_SIZE);
    
    
    if (InitializeHero() != ERROR_SUCCESS)
    {
        LogMessageA(LL_ERROR, "[%s] Failed to initialize hero!", __FUNCTION__);
        MessageBoxA(NULL, "Failed to initialize hero!", "Error!", MB_ICONERROR | MB_OK);

        goto Exit;
    }

    gGameIsRunning = TRUE;


    while (gGameIsRunning == TRUE) 
    {
        QueryPerformanceCounter((LARGE_INTEGER*)&FrameStart);
        


        while (PeekMessageA(&Message, gGameWindow, 0, 0, PM_REMOVE))
        {
            DispatchMessageA(&Message);
        }

        ProcessPlayerInput();

        RenderFrameGraphics();

        //UpdateHealth(Player);

        QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);

        ElapsedMicroseconds = FrameEnd - FrameStart; //This is the delta
        ElapsedMicroseconds *= 1000000;
        ElapsedMicroseconds /= gPerformanceData.PerfFrequency;

        gPerformanceData.TotalFramesRendered++;
        ElapsedMicrosecondsAccumulatorRaw += ElapsedMicroseconds;

        while (ElapsedMicroseconds < TARGET_MICROSECONDS_PER_FRAME)
        {
            gDeltaTime = (FrameEnd - FrameStart) / 1000;

            ElapsedMicroseconds = FrameEnd - FrameStart;
            ElapsedMicroseconds *= 1000000;
            ElapsedMicroseconds /= gPerformanceData.PerfFrequency;

            QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);


            if (ElapsedMicroseconds < (TARGET_MICROSECONDS_PER_FRAME * 0.75f))
            {
                Sleep(1); //Could be anywhere from 1ms to a full system timer tick (global windows timer resolution)
            }
        }

        ElapsedMicrosecondsAccumulatorCooked += ElapsedMicroseconds;


        if (gPerformanceData.TotalFramesRendered % CALCULATE_AVG_FPS_EVERY_X_FRAMES == 0)
        {
            GetSystemTimeAsFileTime((FILETIME*)&gPerformanceData.CurrentSystemTime);

            FindFirstConnectedGamepad();

            GetProcessTimes(ProcessHandle,
                &ProcessCreationTime,
                &ProcessExitTime,
                (FILETIME*)&CurrentKernelCPUTime,
                (FILETIME*)&CurrentUserCPUTime);

            gPerformanceData.CPUPercent = (double)(CurrentKernelCPUTime - PreviousKernelCPUTime) + (CurrentUserCPUTime - PreviousUserCPUTime);

            gPerformanceData.CPUPercent /= (gPerformanceData.CurrentSystemTime - gPerformanceData.PreviousSystemTime);

            gPerformanceData.CPUPercent /= gPerformanceData.SystemInfo.dwNumberOfProcessors;

            gPerformanceData.CPUPercent *= 100;
            
            GetProcessHandleCount(ProcessHandle, &gPerformanceData.HandleCount);

            K32GetProcessMemoryInfo(ProcessHandle, (PROCESS_MEMORY_COUNTERS*)&gPerformanceData.MemInfo, sizeof(gPerformanceData.MemInfo));

            gPerformanceData.RawFPSAverage = 1.0f / ((ElapsedMicrosecondsAccumulatorRaw / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001f);
            gPerformanceData.CoockedFPSAverage = 1.0f / ((ElapsedMicrosecondsAccumulatorCooked / CALCULATE_AVG_FPS_EVERY_X_FRAMES) * 0.000001f);

            ElapsedMicrosecondsAccumulatorRaw = 0;
            ElapsedMicrosecondsAccumulatorCooked = 0;
            PreviousKernelCPUTime = CurrentKernelCPUTime;
            PreviousUserCPUTime = CurrentUserCPUTime;
            gPerformanceData.PreviousSystemTime = gPerformanceData.CurrentSystemTime;
        }
    }

Exit:

    return (0);
}


LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {

        case WM_CLOSE:
        {
            gGameIsRunning = FALSE;

            PostQuitMessage(0);
            break;
        }
        case WM_ACTIVATE:
        {
            if (WParam == 0)
            {
                gWindowHasFocus = FALSE;
            }
            else
            {
                gWindowHasFocus = TRUE;
                ShowCursor(FALSE);
            }

            break;
        }

        default:
        {
            Result = DefWindowProc(WindowHandle, Message, WParam, LParam); //returns if the message was not used
        }
    }
    return (Result);
}


DWORD CreateMainGameWindow(void)
{
    DWORD Result = ERROR_SUCCESS;


    WNDCLASSEXA WindowClass = { 0 }; //Initialize this data structure to zero


    WindowClass.cbSize = sizeof(WNDCLASSEXA); //size in bytes
    WindowClass.style = 0;
    WindowClass.lpfnWndProc = MainWindowProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = GetModuleHandleA(NULL);
    WindowClass.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    WindowClass.hIconSm = LoadIconA(NULL, IDI_APPLICATION);
    WindowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
    WindowClass.hbrBackground = CreateSolidBrush(RGB(255, 0, 255));
    WindowClass.lpszMenuName = NULL;
    WindowClass.lpszClassName = GAME_NAME "_WINDOWCLASS";

    //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);



    if (RegisterClassExA(&WindowClass) == 0)
    {
        Result = GetLastError();

        LogMessageA(LL_ERROR, "[%s] RegisterClassExA failed! Error 0x%08lx!", __FUNCTION__, Result);
        MessageBoxA(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }



    gGameWindow = CreateWindowExA(0, WindowClass.lpszClassName, "Window Title", WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, GetModuleHandleA(NULL), NULL);
    //The '|' allows to mash styles together

    if (gGameWindow == NULL)
    {
        Result = GetLastError();

        LogMessageA(LL_ERROR, "[%s] CreateWindowExA failed! Error 0x%08lx!", __FUNCTION__, Result);
        MessageBoxA(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        
        goto Exit;
    }

    gPerformanceData.MonitorInfo.cbSize = sizeof(MONITORINFO);


    if (GetMonitorInfoA(MonitorFromWindow(gGameWindow, MONITOR_DEFAULTTOPRIMARY), &gPerformanceData.MonitorInfo) == 0)
    {
        Result = ERROR_MONITOR_NO_DESCRIPTOR;

        LogMessageA(LL_ERROR, "[%s] GetMonitorInfoA(MonitorFromWindow()) failed! Error 0x%08lx!", __FUNCTION__, Result);

        goto Exit;
    }

    gPerformanceData.MonitorWidth = gPerformanceData.MonitorInfo.rcMonitor.right - gPerformanceData.MonitorInfo.rcMonitor.left; //Long is 32 bits just like an int
    gPerformanceData.MonitorHeight = gPerformanceData.MonitorInfo.rcMonitor.bottom - gPerformanceData.MonitorInfo.rcMonitor.top;

    if (SetWindowLongPtrA(gGameWindow, GWL_STYLE, WS_VISIBLE) == 0)
    {
        Result = GetLastError();

        LogMessageA(LL_ERROR, "[%s] SetWindowLongPtrA failed! Error 0x%08lx!", __FUNCTION__, Result);

        goto Exit;
    }

    if (SetWindowPos(gGameWindow, 
        HWND_TOP, 
        gPerformanceData.MonitorInfo.rcMonitor.left, 
        gPerformanceData.MonitorInfo.rcMonitor.top, 
        gPerformanceData.MonitorWidth, 
        gPerformanceData.MonitorHeight, 
        SWP_NOOWNERZORDER | SWP_FRAMECHANGED) == 0)
    {
        Result = GetLastError();

        LogMessageA(LL_ERROR, "[%s] SetWindowPos failed! Error 0x%08lx!", __FUNCTION__, Result);

        goto Exit;
    }


Exit:

    return(Result);
}

BOOL GameIsAlreadyRunning(void) 
{
    HANDLE Mutex = NULL;

    Mutex = CreateMutexA(NULL, FALSE, GAME_NAME "_GameMutex"); 
    /*Creates a global Mutex whenever the program runs, whatever instance has the Mutex is the main instance which prevents other
    instances of the program from running while the main one is*/

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

void ProcessPlayerInput(void)
{
    if (gWindowHasFocus == FALSE)
    {
        return;
    }
    
    gGameInput.EscapeKeyIsDown = GetAsyncKeyState(VK_ESCAPE);
    gGameInput.DebugKeyIsDown = GetAsyncKeyState(VK_F1);
    gGameInput.LeftKeyIsDown = GetAsyncKeyState(VK_LEFT) | GetAsyncKeyState('A');
    gGameInput.RightKeyIsDown = GetAsyncKeyState(VK_RIGHT) | GetAsyncKeyState('D');
    gGameInput.UpKeyIsDown = GetAsyncKeyState(VK_UP) | GetAsyncKeyState('W');
    gGameInput.DownKeyIsDown = GetAsyncKeyState(VK_DOWN) | GetAsyncKeyState('S');
    gGameInput.ChooseKeyIsDown = GetAsyncKeyState(VK_RETURN);

    if (gGamepadID > -1)
    {
        if (XInputGetState(gGamepadID, &gGamepadState) == ERROR_SUCCESS)
        {
            gGameInput.EscapeKeyIsDown |= gGamepadState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK;

            gGameInput.LeftKeyIsDown |= gGamepadState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;

            gGameInput.RightKeyIsDown |= gGamepadState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

            gGameInput.UpKeyIsDown |= gGamepadState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP;

            gGameInput.DownKeyIsDown |= gGamepadState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;

            gGameInput.ChooseKeyIsDown |= gGamepadState.Gamepad.wButtons & XINPUT_GAMEPAD_A;
        }
    }
    
    STATEMANAGER_PPi(&gGameStateManager);
    //What does this even do ?
    //If i remove it the PPI functions for all the states won't work anymore

    gGameInput.DebugKeyWasDown = gGameInput.DebugKeyIsDown;
    gGameInput.LeftKeyWasDown = gGameInput.LeftKeyIsDown;
    gGameInput.RightKeyWasDown = gGameInput.RightKeyIsDown;
    gGameInput.UpKeyWasDown = gGameInput.UpKeyIsDown;
    gGameInput.DownKeyWasDown = gGameInput.DownKeyIsDown;
    gGameInput.ChooseKeyWasDown = gGameInput.ChooseKeyIsDown;
}

DWORD Load32BppBitmapFromFile(_In_ char* FileName, _Inout_ GAMEBITMAP* GameBitmap)
{
    DWORD Error = ERROR_SUCCESS;

    HANDLE FileHandle = INVALID_HANDLE_VALUE;

    WORD BitmapHeader = 0;

    DWORD PixelDataOffset = 0;

    DWORD NumberOfBytesRead = 2;

    if ((FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        Error = GetLastError();
        goto Exit;
    }

    if (ReadFile(FileHandle, &BitmapHeader, 2, &NumberOfBytesRead, NULL, NULL) == 0)
    {
        Error = GetLastError();
        goto Exit;
    }

    if (BitmapHeader != 0x4d42) //"BM" backwards
    {
        Error = ERROR_FILE_INVALID;
        goto Exit;
    }

    if (SetFilePointer(FileHandle, 0xA, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        Error = GetLastError();
        goto Exit;
    }

    if (ReadFile(FileHandle, &PixelDataOffset, sizeof(DWORD), &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();
        goto Exit;
    }

    if (SetFilePointer(FileHandle, 0xE, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        Error = GetLastError();
        goto Exit;
    }

    if (ReadFile(FileHandle, &GameBitmap->BitMapInfo.bmiHeader, sizeof(BITMAPINFOHEADER), &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();
        goto Exit;
    }

    if ((GameBitmap->Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, GameBitmap->BitMapInfo.bmiHeader.biSizeImage)) == NULL)
    {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    if (SetFilePointer(FileHandle, PixelDataOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        Error = GetLastError();
        goto Exit;
    }

    if (ReadFile(FileHandle, GameBitmap->Memory, GameBitmap->BitMapInfo.bmiHeader.biSizeImage, &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();
        goto Exit;
    }



Exit:

    if (FileHandle && (FileHandle != INVALID_HANDLE_VALUE))
    {
        CloseHandle(FileHandle);
    }

    if (Error == ERROR_SUCCESS)
    {
        LogMessageA(LL_INFO, "[%s] Loading succesful: %s", __FUNCTION__, FileName, Error);
    }
    else
    {
        LogMessageA(LL_ERROR, "[%s] Loading failed: %s! Error 0x%08lx!", __FUNCTION__, FileName, Error);
    }

    return(Error);
}

DWORD InitializeHero(void)
{
    DWORD Error = ERROR_SUCCESS;

    gPlayer.Entity.ScreenPosX = 64;
    gPlayer.Entity.ScreenPosY = 64;
    gPlayer.CurrentArmor = SUIT_0;
    gPlayer.Direction = DIRECTION_DOWN;

    if ((Error = Load32BppBitmapFromFile(".\\Assets\\TestElfIdle.bmpx", &gPlayerSpriteSheet)) != ERROR_SUCCESS)
    {

        LogMessageA(Error, "[%s] Loading player sprite sheet failed!", __FUNCTION__);
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    gPlayer.Entity.NumberOfAnimations = 2;

    ANIMATION_PlayerIdle = CreateAnimation("Idle", 16, 16, 0, 4, .05f);
    ANIMATION_PlayerWalk = CreateAnimation("Walk", 16, 16, 1, 4, .1f);
    ANIMATION_PlayerActiveAnimation = ANIMATION_PlayerIdle;

    //Player = CreateEntity();
    //AddComponent(Player, COMPONENT_HEALTH);


    //Check out a video by i forgot who, but he speaks about why global variables suck and why you shouldn't use them
    //Find a solution to gAnimation, I don't want it to be a global variable
    //MAYBE you can define the animations in an external file and then read from that file --> Check this out


    /*if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Down_Walk1.bmpx", &gPlayer.Sprite[SUIT_0][FACING_DOWN_1])) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Down_Walk2.bmpx", &gPlayer.Sprite[SUIT_0][FACING_DOWN_2])) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Left_Standing.bmpx", &gPlayer.Sprite[SUIT_0][FACING_LEFT_0])) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Left_Walk1.bmpx", &gPlayer.Sprite[SUIT_0][FACING_LEFT_1])) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Left_Walk2.bmpx", &gPlayer.Sprite[SUIT_0][FACING_LEFT_2])) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Right_Standing.bmpx", &gPlayer.Sprite[SUIT_0][FACING_RIGHT_0])) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Right_Walk1.bmpx", &gPlayer.Sprite[SUIT_0][FACING_RIGHT_1])) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Right_Walk2.bmpx", &gPlayer.Sprite[SUIT_0][FACING_RIGHT_2])) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Up_Standing.bmpx", &gPlayer.Sprite[SUIT_0][FACING_UP_0])) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Up_Walk1.bmpx", &gPlayer.Sprite[SUIT_0][FACING_UP_1])) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Up_Walk2.bmpx", &gPlayer.Sprite[SUIT_0][FACING_UP_2])) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }*/


Exit:
    return(Error);
}



void BlitStringToBuffer(_In_ char* String, _In_ GAMEBITMAP* FontSheet, _In_ PIXEL32* Color, _In_ uint16_t x, _In_ uint16_t y)
{

    uint16_t CharWidth = (uint16_t)FontSheet->BitMapInfo.bmiHeader.biWidth / FONT_SHEET_CHARACTERS_PER_ROW;
    uint16_t CharHeight = (uint16_t)FontSheet->BitMapInfo.bmiHeader.biHeight;
    uint16_t BytesPerCharacter = (CharWidth * CharHeight * (FontSheet->BitMapInfo.bmiHeader.biBitCount / 8));
    uint16_t StringLength = (uint16_t)strlen(String);
    GAMEBITMAP StringBitmap = { 0 };
    StringBitmap.BitMapInfo.bmiHeader.biBitCount = GAME_BPP;
    StringBitmap.BitMapInfo.bmiHeader.biHeight = CharHeight;
    StringBitmap.BitMapInfo.bmiHeader.biWidth = CharWidth * StringLength;
    StringBitmap.BitMapInfo.bmiHeader.biPlanes = 1;
    StringBitmap.BitMapInfo.bmiHeader.biCompression = BI_RGB;
    StringBitmap.Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ((size_t)BytesPerCharacter * (size_t)StringLength));


    for (int Character = 0; Character < StringLength; Character++)
    {
        int StartingFontSheetPixel = 0;
        int FontSheetOffset = 0;
        int StringBitmapOffset = 0;

        PIXEL32 FontSheetPixel = { 0 };
        StartingFontSheetPixel = (FontSheet->BitMapInfo.bmiHeader.biWidth * FontSheet->BitMapInfo.bmiHeader.biHeight) - FontSheet->BitMapInfo.bmiHeader.biWidth + (CharWidth * gFontCharacterPixelOffset[(unsigned char)String[Character]]);

        for (int YPixel = 0; YPixel < CharHeight; YPixel++)
        {
            for (int XPixel = 0; XPixel < CharWidth; XPixel++)
            {
                FontSheetOffset = StartingFontSheetPixel + XPixel - (FontSheet->BitMapInfo.bmiHeader.biWidth * YPixel);
                StringBitmapOffset = (Character * CharWidth) + ((StringBitmap.BitMapInfo.bmiHeader.biWidth * StringBitmap.BitMapInfo.bmiHeader.biHeight) - \
                    StringBitmap.BitMapInfo.bmiHeader.biWidth) + XPixel - (StringBitmap.BitMapInfo.bmiHeader.biWidth) * YPixel;

                memcpy_s(&FontSheetPixel, sizeof(PIXEL32), (PIXEL32*)FontSheet->Memory + FontSheetOffset, sizeof(PIXEL32));

                memcpy_s(&FontSheetPixel, sizeof(PIXEL32), (PIXEL32*)FontSheet->Memory + FontSheetOffset, sizeof(PIXEL32));

                FontSheetPixel.Red = Color->Red;

                FontSheetPixel.Green = Color->Green;

                FontSheetPixel.Blue = Color->Blue;

                memcpy_s((PIXEL32*)StringBitmap.Memory + StringBitmapOffset, sizeof(PIXEL32), &FontSheetPixel, sizeof(PIXEL32));
            }
        }
    }


    Blit32BppBitmapToBuffer(&StringBitmap, x, y);

    if (StringBitmap.Memory)
    {
        HeapFree(GetProcessHeap(), 0, StringBitmap.Memory);
    }

}


void RenderFrameGraphics(void)
{
    STATEMANAGER_Draw(&gGameStateManager);


    if (gPerformanceData.DisplayDebugInfo == TRUE)
    {
        DrawDebugInfo();
    }

    HDC DeviceContext = GetDC(gGameWindow);

    StretchDIBits(DeviceContext, 
        0, 
        0, 
        gPerformanceData.MonitorWidth,
        gPerformanceData.MonitorHeight,
        0, 
        0, 
        GAME_RES_WIDTH, 
        GAME_RES_HEIGHT, 
        gBackBuffer.Memory, 
        &gBackBuffer.BitMapInfo, 
        DIB_RGB_COLORS, 
        SRCCOPY);

    SelectObject(DeviceContext, (HFONT)GetStockObject(ANSI_FIXED_FONT));

    ReleaseDC(gGameWindow, DeviceContext);
}

void ClearScreen(_In_ __m128i* Color)
{
    for (int index = 0; index < (GAME_RES_WIDTH * GAME_RES_HEIGHT) / 4; index++)
    {
        _mm_store_si128((__m128i*)gBackBuffer.Memory + index, *Color);
    }
}

void Blit32BppBitmapToBuffer(_In_ GAMEBITMAP* GameBitmap, _In_ uint16_t x, _In_ uint16_t y)
{
    int32_t StartingScreenPixel = ((GAME_RES_WIDTH * GAME_RES_HEIGHT) - GAME_RES_WIDTH) - (GAME_RES_WIDTH * y) + x;

    int32_t StartingBitmapPixel = ((GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - \
        GameBitmap->BitMapInfo.bmiHeader.biWidth);

    int32_t MemoryOffset = 0;

    int32_t BitmapOffset = 0;

    PIXEL32 BitmapPixel = { 0 };

    PIXEL32 BackgroundPixel = { 0 };

    for (int16_t YPixel = 0; YPixel < GameBitmap->BitMapInfo.bmiHeader.biHeight; YPixel++)
    {
        for (int16_t XPixel = 0; XPixel < GameBitmap->BitMapInfo.bmiHeader.biWidth; XPixel++)
        {
            MemoryOffset = StartingScreenPixel + XPixel - (GAME_RES_WIDTH * YPixel);

            BitmapOffset = StartingBitmapPixel + XPixel - (GameBitmap->BitMapInfo.bmiHeader.biWidth * YPixel);

            memcpy_s(&BitmapPixel, sizeof(PIXEL32), (PIXEL32*)GameBitmap->Memory + BitmapOffset, sizeof(PIXEL32));

            if (BitmapPixel.Alpha == 255)
            {
                memcpy_s((PIXEL32*)gBackBuffer.Memory + MemoryOffset, sizeof(PIXEL32), &BitmapPixel, sizeof(PIXEL32));
            }

        }
    }


}

DWORD LoadRegistryParameters(void)
{
    DWORD Result = ERROR_SUCCESS;
    HKEY RegKey = NULL;
    DWORD RegDisposition = 0;
    DWORD RegBytesRead = sizeof(DWORD);

    Result = RegCreateKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\" GAME_NAME, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &RegKey, &RegDisposition);

    if (Result != ERROR_SUCCESS)
    {
        LogMessageA(LL_ERROR, "[%s] RegCreateKey failed with error code 0x%08lx!", __FUNCTION__, Result);
        //__FUNCTION__ logs the name of the function that we are currenty in

        goto Exit;
    }

    if (RegDisposition == REG_CREATED_NEW_KEY)
    {
        LogMessageA(LL_INFO, "[%s] Registy key did not exist; created new key HKCU\\SOFTWARE\\%s.", __FUNCTION__, GAME_NAME);
    }
    else
    {
        LogMessageA(LL_INFO, "[%s] Opened existing registry key HKCU\\SOFTWARE\\%s", __FUNCTION__, GAME_NAME);
    }

    Result = RegGetValueA(RegKey, NULL, "LogLevel", RRF_RT_DWORD, NULL, (BYTE*)&gRegistryParams.LogLevel, &RegBytesRead);

    if (Result != ERROR_SUCCESS)
    {
        if (Result == ERROR_FILE_NOT_FOUND)
        {
            Result = ERROR_SUCCESS;

            LogMessageA(LL_INFO, "[%s] Registry value 'LogLevel' not found. Using default of 0. (LOG_LEVEL_NONE)", __FUNCTION__);
            gRegistryParams.LogLevel = LL_NONE;
        }
        else
        {
            LogMessageA(LL_ERROR, "[%s] Failed to read the 'LogLevel' registry value! Error: 0x%08lx!", __FUNCTION__, Result);
            goto Exit;
        }
    }


    LogMessageA(LL_INFO, "[%s] LogLevel is %d.", __FUNCTION__, gRegistryParams.LogLevel);


Exit:

    if (RegKey)
    {
        RegCloseKey(RegKey);
    }

    return(Result);
}

void LogMessageA(_In_ LOGLEVEL LogLevel, _In_ char* Message, _In_ ...)
{
    size_t MessageLength = strlen(Message);
    SYSTEMTIME Time = { 0 };
    HANDLE LogFileHandle = INVALID_HANDLE_VALUE;
    DWORD EndOfFile = 0;
    DWORD NumberOfBytesWritten = 0;
    char DateTimeString[96] = { 0 };
    char SeverityString[8] = { 0 };
    char FormattedString[4096] = { 0 };


    if (gRegistryParams.LogLevel < LogLevel)
    {
        return;
    }

    if (MessageLength < 1 || MessageLength > 4096)
    {
        ASSERT(FALSE, "Message was either too short or too long!");
    }

    switch (LogLevel)
    {
        case LL_NONE:
        {
            return;
        }
        case LL_INFO:
        {
            strcpy_s(SeverityString, sizeof(SeverityString), "[INFO]"); //strcpy_s: String copy
            break;
        }
        case LL_WARNING:
        {
            strcpy_s(SeverityString, sizeof(SeverityString), "[WARN]");
            break;
        }
        case LL_ERROR:
        {
            strcpy_s(SeverityString, sizeof(SeverityString), "[ERROR]");
            break;
        }
        case LL_DEBUG:
        {
            strcpy_s(SeverityString, sizeof(SeverityString), "[DEBUG]");
            break;
        }
        default:
        {
            ASSERT(FALSE, "Unrecognized long level!");
        }
    }

    GetLocalTime(&Time);

    va_list ArgPointer = NULL;
    va_start(ArgPointer, Message);
    _vsnprintf_s(FormattedString, sizeof(FormattedString), _TRUNCATE, Message, ArgPointer);
    va_end(ArgPointer);

    _snprintf_s(DateTimeString, sizeof(DateTimeString), _TRUNCATE, "\r\n[%02u/%02u/%u %02u:%02u:%02u.%03u]", Time.wMonth, Time.wDay, Time.wYear, Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds);

    if ((LogFileHandle = CreateFileA(LOG_FILE_NAME, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        ASSERT(FALSE, "Failed to access log file!");
    }

    EndOfFile = SetFilePointer(LogFileHandle, 0, NULL, FILE_END);

    WriteFile(LogFileHandle, DateTimeString, (DWORD)strlen(DateTimeString), &NumberOfBytesWritten, NULL);

    WriteFile(LogFileHandle, SeverityString, (DWORD)strlen(SeverityString), &NumberOfBytesWritten, NULL);

    WriteFile(LogFileHandle, FormattedString, (DWORD)strlen(FormattedString), &NumberOfBytesWritten, NULL);

    if (LogFileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(LogFileHandle);
    }

}

__forceinline void DrawDebugInfo(void)
{
    char DebugTextBuffer[64] = { 0 };

    PIXEL32 White = { 0xFF, 0xFF, 0xFF, 0xFF };

    sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "FPS Raw:   %.01f", gPerformanceData.RawFPSAverage);
    BlitStringToBuffer(DebugTextBuffer, &g6x7Font, &White, 0, 0);

    sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "FPS Cookd: %.01f", gPerformanceData.CoockedFPSAverage);
    BlitStringToBuffer(DebugTextBuffer, &g6x7Font, &White, 0, 8);

    sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Min Timer: %.02f", gPerformanceData.MinimumTimerResolution / 10000.0f);
    BlitStringToBuffer(DebugTextBuffer, &g6x7Font, &White, 0, 16);

    sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Max Timer: %.02f", gPerformanceData.MaximumTimerResolution / 10000.0f);
    BlitStringToBuffer(DebugTextBuffer, &g6x7Font, &White, 0, 24);

    sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Cur Timer: %.02f", gPerformanceData.CurrentTimerResolution / 10000.0f);
    BlitStringToBuffer(DebugTextBuffer, &g6x7Font, &White, 0, 32);

    sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Handles:   %lu", gPerformanceData.HandleCount);
    BlitStringToBuffer(DebugTextBuffer, &g6x7Font, &White, 0, 40);

    sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Memory:    %lu KB", gPerformanceData.MemInfo.PrivateUsage / 1024);
    BlitStringToBuffer(DebugTextBuffer, &g6x7Font, &White, 0, 48);

    sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "CPU:       %.02f %%", gPerformanceData.CPUPercent);
    BlitStringToBuffer(DebugTextBuffer, &g6x7Font, &White, 0, 56);

    sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "FramesT:   %llu", gPerformanceData.TotalFramesRendered);
    BlitStringToBuffer(DebugTextBuffer, &g6x7Font, &White, 0, 64);

    sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "ScreenXY:  (%d,%d)", gPlayer.Entity.ScreenPosX, gPlayer.Entity.ScreenPosY);
    BlitStringToBuffer(DebugTextBuffer, &g6x7Font, &White, 0, 72);

    sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "DeltaTime: %lu", gDeltaTime);
    BlitStringToBuffer(DebugTextBuffer, &g6x7Font, &White, 0, 80);
}

void FindFirstConnectedGamepad(void)
{
    gGamepadID = -1;

    for (int8_t GamepadIndex = 0; GamepadIndex < XUSER_MAX_COUNT && gGamepadID == -1; GamepadIndex++)
    {
        XINPUT_STATE State = { 0 };

        if (XInputGetState(GamepadIndex, &State) == ERROR_SUCCESS)
        {
            gGamepadID = GamepadIndex;
        }
    }
}

HRESULT InitializeSoundEngine(void)
{
    HRESULT Result = S_OK;
    WAVEFORMATEX SfxWaveFormat = { 0 };
    WAVEFORMATEX MusicWaveFormat = { 0 };
    Result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (Result != S_OK)
    {
        LogMessageA(LL_ERROR, "[%s] CoInitializeEx failed with 0x%08lx!", __FUNCTION__, Result);
        goto Exit;
    }
    Result = XAudio2Create(&gXAudio, 0, XAUDIO2_ANY_PROCESSOR);
    if (FAILED(Result))
    {
        LogMessageA(LL_ERROR, "[%s] XAudio2Create failed with 0x%08lx!", __FUNCTION__, Result);
        goto Exit;
    }
    Result = gXAudio->lpVtbl->CreateMasteringVoice(gXAudio, &gXAudioMasteringVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, 0, NULL, 0);
    if (FAILED(Result))
    {
        LogMessageA(LL_ERROR, "[%s] CreateMasteringVoice failed with 0x%08lx!", __FUNCTION__, Result);
        goto Exit;
    }
    SfxWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    SfxWaveFormat.nChannels = 1; // Mono
    SfxWaveFormat.nSamplesPerSec = 44100;
    SfxWaveFormat.nAvgBytesPerSec = SfxWaveFormat.nSamplesPerSec * SfxWaveFormat.nChannels * 2;
    SfxWaveFormat.nBlockAlign = SfxWaveFormat.nChannels * 2;
    SfxWaveFormat.wBitsPerSample = 16;
    SfxWaveFormat.cbSize = 0x6164;
    for (uint8_t Counter = 0; Counter < NUMBER_OF_SFX_SOURCE_VOICES; Counter++)
    {
        Result = gXAudio->lpVtbl->CreateSourceVoice(gXAudio, &gXAudioSFXSourceVoice[Counter], &SfxWaveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, NULL, NULL, NULL);
        if (Result != S_OK)
        {
            LogMessageA(LL_ERROR, "[%s] CreateSourceVoice failed with 0x%08lx!", __FUNCTION__, Result);
            goto Exit;
        }
        gXAudioSFXSourceVoice[Counter]->lpVtbl->SetVolume(gXAudioSFXSourceVoice[Counter], gSFXVolume, XAUDIO2_COMMIT_NOW);
    }
    MusicWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    MusicWaveFormat.nChannels = 2; // Stereo
    MusicWaveFormat.nSamplesPerSec = 44100;
    MusicWaveFormat.nAvgBytesPerSec = MusicWaveFormat.nSamplesPerSec * MusicWaveFormat.nChannels * 2;
    MusicWaveFormat.nBlockAlign = MusicWaveFormat.nChannels * 2;
    MusicWaveFormat.wBitsPerSample = 16;
    MusicWaveFormat.cbSize = 0;
    Result = gXAudio->lpVtbl->CreateSourceVoice(gXAudio, &gXAudioMusicSourceVoice, &MusicWaveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, NULL, NULL, NULL);
    if (Result != S_OK)
    {
        LogMessageA(LL_ERROR, "[%s] CreateSourceVoice failed with 0x%08lx!", __FUNCTION__, Result);
        goto Exit;
    }
    gXAudioMusicSourceVoice->lpVtbl->SetVolume(gXAudioMusicSourceVoice, gMusicVolume, XAUDIO2_COMMIT_NOW);

Exit:
    return(Result);
}


DWORD LoadWavFromFile(_In_ char* FileName, _Inout_ GAMESOUND* GameSound)
{
    DWORD Error = ERROR_SUCCESS;

    DWORD NumberOfBytesRead = 0;

    DWORD RIFF = 0;

    uint16_t DataChunkOffset = 0;

    DWORD DataChunkSearcher = 0;

    BOOL DataChunkFound = FALSE;

    DWORD DataChunkSize = 0;

    HANDLE FileHandle = INVALID_HANDLE_VALUE;

    void* AudioData = NULL;


    if ((FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        Error = GetLastError();

        LogMessageA(LL_ERROR, "[%s] CreateFileA failed with 0x%08lx!", __FUNCTION__, Error);

        goto Exit;
    }

    if (ReadFile(FileHandle, &RIFF, sizeof(DWORD), &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();

        LogMessageA(LL_ERROR, "[%s] ReadFile failed with 0x%08lx!", __FUNCTION__, Error);

        goto Exit;
    }

    if (RIFF != 0x46464952) // "RIFF" backwards
    {
        Error = ERROR_FILE_INVALID;

        LogMessageA(LL_ERROR, "[%s] First four bytes of this file are not 'RIFF'!", __FUNCTION__, Error);

        goto Exit;
    }

    if (SetFilePointer(FileHandle, 20, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        Error = GetLastError();

        LogMessageA(LL_ERROR, "[%s] SetFilePointer failed with 0x%08lx!", __FUNCTION__, Error);

        goto Exit;
    }

    if (ReadFile(FileHandle, &GameSound->WaveFormat, sizeof(WAVEFORMATEX), &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();

        LogMessageA(LL_ERROR, "[%s] ReadFile failed with 0x%08lx!", __FUNCTION__, Error);

        goto Exit;
    }

    if (GameSound->WaveFormat.nBlockAlign != ((GameSound->WaveFormat.nChannels * GameSound->WaveFormat.wBitsPerSample) / 8) ||
        (GameSound->WaveFormat.wFormatTag != WAVE_FORMAT_PCM) ||
        (GameSound->WaveFormat.wBitsPerSample != 16))
    {
        Error = ERROR_DATATYPE_MISMATCH;

        LogMessageA(LL_ERROR, "[%s] This wav file did not meet the format requirements! Only PCM format, 44.1KHz, 16 bits per sample wav files are supported. 0x%08lx!", __FUNCTION__, Error);

        goto Exit;
    }

    while (DataChunkFound == FALSE)
    {
        if (SetFilePointer(FileHandle, DataChunkOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            Error = GetLastError();

            LogMessageA(LL_ERROR, "[%s] SetFilePointer failed with 0x%08lx!", __FUNCTION__, Error);

            goto Exit;
        }

        if (ReadFile(FileHandle, &DataChunkSearcher, sizeof(DWORD), &NumberOfBytesRead, NULL) == 0)
        {
            Error = GetLastError();

            LogMessageA(LL_ERROR, "[%s] ReadFile failed with 0x%08lx!", __FUNCTION__, Error);

            goto Exit;
        }

        if (DataChunkSearcher == 0x61746164) // 'data', backwards
        {
            DataChunkFound = TRUE;

            break;
        }
        else
        {
            DataChunkOffset += 4;
        }

        if (DataChunkOffset > 256)
        {
            Error = ERROR_DATATYPE_MISMATCH;

            LogMessageA(LL_ERROR, "[%s] Data chunk not found within first 256 bytes of this file! 0x%08lx!", __FUNCTION__, Error);

            goto Exit;
        }
    }

    if (SetFilePointer(FileHandle, DataChunkOffset + 4, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        Error = GetLastError();

        LogMessageA(LL_ERROR, "[%s] SetFilePointer failed with 0x%08lx!", __FUNCTION__, Error);

        goto Exit;
    }

    if (ReadFile(FileHandle, &DataChunkSize, sizeof(DWORD), &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();

        LogMessageA(LL_ERROR, "[%s] ReadFile failed with 0x%08lx!", __FUNCTION__, Error);

        goto Exit;
    }

    AudioData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, DataChunkSize);

    if (AudioData == NULL)
    {
        Error = ERROR_NOT_ENOUGH_MEMORY;

        LogMessageA(LL_ERROR, "[%s] HeapAlloc failed with 0x%08lx!", __FUNCTION__, Error);

        goto Exit;
    }

    GameSound->Buffer.Flags = XAUDIO2_END_OF_STREAM;

    GameSound->Buffer.AudioBytes = DataChunkSize;

    if (SetFilePointer(FileHandle, DataChunkOffset + 8, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        Error = GetLastError();

        LogMessageA(LL_ERROR, "[%s] SetFilePointer failed with 0x%08lx!", __FUNCTION__, Error);

        goto Exit;
    }

    if (ReadFile(FileHandle, AudioData, DataChunkSize, &NumberOfBytesRead, NULL) == 0)
    {
        Error = GetLastError();

        LogMessageA(LL_ERROR, "[%s] ReadFile failed with 0x%08lx!", __FUNCTION__, Error);

        goto Exit;
    }

    GameSound->Buffer.pAudioData = AudioData;

Exit:

    if (Error == ERROR_SUCCESS)
    {
        LogMessageA(LL_INFO, "[%s] Successfully loaded %s.", __FUNCTION__, FileName);
    }
    else
    {
        LogMessageA(LL_ERROR, "[%s] Failed to load %s! Error: 0x%08lx!", __FUNCTION__, FileName, Error);
    }

    if (FileHandle && (FileHandle != INVALID_HANDLE_VALUE))
    {
        CloseHandle(FileHandle);
    }

    return(Error);
}

void PlayGameSound(_In_ GAMESOUND* GameSound)
{
    gXAudioSFXSourceVoice[gSFXSourceVoiceSelector]->lpVtbl->SubmitSourceBuffer(gXAudioSFXSourceVoice[gSFXSourceVoiceSelector], &GameSound->Buffer, NULL);

    gXAudioSFXSourceVoice[gSFXSourceVoiceSelector]->lpVtbl->Start(gXAudioSFXSourceVoice[gSFXSourceVoiceSelector], 0, XAUDIO2_COMMIT_NOW);

    gSFXSourceVoiceSelector++;

    if (gSFXSourceVoiceSelector > (NUMBER_OF_SFX_SOURCE_VOICES - 1))
    {
        gSFXSourceVoiceSelector = 0;
    }
}




void MenuItem_TitleScreen_Resume(void)
{
    GAMESTATE_PreviousGameState = STATEMANAGER_Top(&gGameStateManager);
    STATEMANAGER_Pop(&gGameStateManager);
    STATEMANAGER_Push(&gGameStateManager, &GAMESTATE_Overworld);
}

void MenuItem_TitleScreen_StartNew(void)
{

}

void MenuItem_TitleScreen_Options(void)
{

}

void MenuItem_ExitYesNo_Yes(void)
{
    SendMessageA(gGameWindow, WM_CLOSE, 0, 0);
}

void MenuItem_ExitYesNo_No(void)
{
    GAMESTATE_PreviousGameState = STATEMANAGER_Top(&gGameStateManager);

    STATEMANAGER_Pop(&gGameStateManager);

    STATEMANAGER_Push(&gGameStateManager, GAMESTATE_PreviousGameState);
}

void MenuItem_TitleScreen_Exit(void)
{
    GAMESTATE_PreviousGameState = STATEMANAGER_Top(&gGameStateManager);

    STATEMANAGER_Pop(&gGameStateManager);

    STATEMANAGER_Push(&gGameStateManager, &GAMESTATE_ExitYesNoScreen);
}





void DRAW_Overworld(void)
{

    #ifdef SIMD
    __m128i QuadPixel = { 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff };

    ClearScreen(&QuadPixel);

    #else
    __m256i OctoPixel = { 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff,
                          0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff };

    ClearScreen(&OctoPixel);

    #endif


    PIXEL32 FontColor = { 0x00, 0xFF, 0xFF, 0xFF };

    BlitStringToBuffer("GAME OVER", &g6x7Font, &FontColor, 100, 100);


    //Blit32BppBitmapToBuffer(&gPlayer.Entity.Sprite, gPlayer.Entity.ScreenPosX, gPlayer.Entity.ScreenPosY);
    AnimateSprite(ANIMATION_PlayerActiveAnimation, gPlayer.Entity, &gPlayerSpriteSheet);
}

void DRAW_TitleScreen(void)
{
    PIXEL32 White = { 0xFF, 0xFF, 0xFF, 0xFF };

    static uint64_t LocalFrameCounter;
    static uint64_t LastFrameSeen;

    memset(gBackBuffer.Memory, 0, GAME_DRAWING_AREA_MEMORY_SIZE);
    
    BlitStringToBuffer(GAME_NAME, &g6x7Font, &White, (GAME_RES_WIDTH / 2) - ((strlen(GAME_NAME) * 6) / 2), 60);

    for (uint8_t MenuItem = 0; MenuItem < gMenu_TitleScreen.ItemCount; MenuItem++)
    {
        BlitStringToBuffer(gMenu_TitleScreen.Items[MenuItem]->Name,
            &g6x7Font,
            &White, gMenu_TitleScreen.Items[MenuItem]->x,
            gMenu_TitleScreen.Items[MenuItem]->y);
    }

    BlitStringToBuffer("»",
        &g6x7Font,
        &White,
        gMenu_TitleScreen.Items[gMenu_TitleScreen.SelectedItem]->x - 6,
        gMenu_TitleScreen.Items[gMenu_TitleScreen.SelectedItem]->y);
}

void DRAW_ExitYesNoScreen(void)
{
    PIXEL32 White = { 0xFF, 0xFF, 0xFF, 0xFF };

    static uint64_t LocalFrameCounter;
    static uint64_t LastFrameSeen;

    memset(gBackBuffer.Memory, 0, GAME_DRAWING_AREA_MEMORY_SIZE);

    //Drawing the menu's name
    BlitStringToBuffer(gMenu_ExitYesNo.Name, &g6x7Font, &White, (GAME_RES_WIDTH / 2) - ((strlen(gMenu_ExitYesNo.Name) * 6) / 2), 60);

    //Drawing Yes and No Items
    BlitStringToBuffer(gMenu_ExitYesNo.Items[0]->Name, &g6x7Font, &White, gMenu_ExitYesNo.Items[0]->x, gMenu_ExitYesNo.Items[0]->y);
    BlitStringToBuffer(gMenu_ExitYesNo.Items[1]->Name, &g6x7Font, &White, gMenu_ExitYesNo.Items[1]->x, gMenu_ExitYesNo.Items[1]->y);

    //Drawing Cursor
    BlitStringToBuffer("»",
        &g6x7Font,
        &White,
        gMenu_ExitYesNo.Items[gMenu_ExitYesNo.SelectedItem]->x - 6,
        gMenu_ExitYesNo.Items[gMenu_ExitYesNo.SelectedItem]->y);
}

void PPI_Overworld(void)
{

    if (gGameInput.EscapeKeyIsDown)
    {
        GAMESTATE_PreviousGameState = STATEMANAGER_Top(&gGameStateManager);

        STATEMANAGER_Pop(&gGameStateManager);

        STATEMANAGER_Push(&gGameStateManager, &GAMESTATE_TitleScreen);
    }

    if (gGameInput.DebugKeyIsDown && !gGameInput.DebugKeyWasDown)
    {
        gPerformanceData.DisplayDebugInfo = !gPerformanceData.DisplayDebugInfo;
    }


#ifdef GRID_MOVEMENT

    if (!gPlayer.MovementRemaining)
    {
        if (LeftKeyIsDown)
        {
            if (gPlayer.Entity.ScreenPosX > 0)
            {
                gPlayer.MovementRemaining = 16;
                gPlayer.Direction = DIRECTION_LEFT;
            }
        }
        else if (RightKeyIsDown)
        {
            if (gPlayer.Entity.ScreenPosX < GAME_RES_WIDTH - 16)
            {
                gPlayer.MovementRemaining = 16;
                gPlayer.Direction = DIRECTION_RIGHT;
            }
        }
        else if (UpKeyIsDown)
        {
            if (gPlayer.Entity.ScreenPosY > 0)
            {
                gPlayer.MovementRemaining = 16;
                gPlayer.Direction = DIRECTION_UP;
            }
        }
        else if (DownKeyIsDown)
        {
            if (gPlayer.Entity.ScreenPosY < GAME_RES_HEIGHT - 16)
            {
                gPlayer.MovementRemaining = 16;
                gPlayer.Direction = DIRECTION_DOWN;
            }
        }
    }
    else
    {
        gPlayer.MovementRemaining--;

        if (gPlayer.Direction == DIRECTION_DOWN)
        {
            gPlayer.Entity.ScreenPosY++;
        }
        else if (gPlayer.Direction == DIRECTION_LEFT)
        {
            gPlayer.Entity.ScreenPosX--;
        }
        else if (gPlayer.Direction == DIRECTION_RIGHT)
        {
            gPlayer.Entity.ScreenPosX++;
        }
        else if (gPlayer.Direction == DIRECTION_UP)
        {
            gPlayer.Entity.ScreenPosY--;
        }
    }

#else

    if (gGameInput.LeftKeyIsDown)
    {
        ANIMATION_PlayerActiveAnimation = ANIMATION_PlayerWalk;
        if (gPlayer.Entity.ScreenPosX > 0)
        {
            gPlayer.Entity.ScreenPosX--;
        }
    }
    else if (gGameInput.RightKeyIsDown)
    {
        ANIMATION_PlayerActiveAnimation = ANIMATION_PlayerWalk;
        if (gPlayer.Entity.ScreenPosX < GAME_RES_WIDTH - 16)
        {
            gPlayer.Entity.ScreenPosX++;
        }
    }
    else if (gGameInput.UpKeyIsDown)
    {
        ANIMATION_PlayerActiveAnimation = ANIMATION_PlayerWalk;
        if (gPlayer.Entity.ScreenPosY > 0)
        {
            gPlayer.Entity.ScreenPosY--;
        }
    }
    else if (gGameInput.DownKeyIsDown)
    {
        ANIMATION_PlayerActiveAnimation = ANIMATION_PlayerWalk;
        if (gPlayer.Entity.ScreenPosY < GAME_RES_HEIGHT - 16)
        {
            gPlayer.Entity.ScreenPosY++;
        }
    }
    else
    {
        ANIMATION_PlayerActiveAnimation = ANIMATION_PlayerIdle;
    }

#endif
}

void PPI_TitleScreen(void)
{
    if (gGameInput.DebugKeyIsDown && !gGameInput.DebugKeyWasDown)
    {
        gPerformanceData.DisplayDebugInfo = !gPerformanceData.DisplayDebugInfo;
    }

    if (gGameInput.DownKeyIsDown && !gGameInput.DownKeyWasDown)
    {
        if (gMenu_TitleScreen.SelectedItem < gMenu_TitleScreen.ItemCount - 1)
        {
            gMenu_TitleScreen.SelectedItem++;

            PlayGameSound(&gMenuNavigate);
        }
    }

    if (gGameInput.UpKeyIsDown && !gGameInput.UpKeyWasDown)
    {
        if (gMenu_TitleScreen.SelectedItem > 0)
        {
            gMenu_TitleScreen.SelectedItem--;

            PlayGameSound(&gMenuNavigate);
        }
    }

    if (gGameInput.ChooseKeyIsDown && !gGameInput.ChooseKeyWasDown)
    {
        gMenu_TitleScreen.Items[gMenu_TitleScreen.SelectedItem]->Action();

        PlayGameSound(&gMenuChoose);
    }
}

void PPI_ExitYesNoScreen(void)
{
    if (gGameInput.DebugKeyIsDown && !gGameInput.DebugKeyWasDown)
    {
        gPerformanceData.DisplayDebugInfo = !gPerformanceData.DisplayDebugInfo;
    }

    if (gGameInput.DownKeyIsDown && !gGameInput.DownKeyWasDown)
    {
        if (gMenu_ExitYesNo.SelectedItem < gMenu_ExitYesNo.ItemCount - 1)
        {
            gMenu_ExitYesNo.SelectedItem++;

            PlayGameSound(&gMenuNavigate);
        }
    }

    if (gGameInput.UpKeyIsDown && !gGameInput.UpKeyWasDown)
    {
        if (gMenu_ExitYesNo.SelectedItem > 0)
        {
            gMenu_ExitYesNo.SelectedItem--;

            PlayGameSound(&gMenuNavigate);
        }
    }

    if (gGameInput.ChooseKeyIsDown && !gGameInput.ChooseKeyWasDown)
    {
        gMenu_ExitYesNo.Items[gMenu_ExitYesNo.SelectedItem]->Action();

        PlayGameSound(&gMenuChoose);
    }
}