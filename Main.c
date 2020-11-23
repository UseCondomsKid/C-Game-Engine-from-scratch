
#pragma warning(push, 3)

#include<stdio.h>

#include<windows.h>

#include<psapi.h>

#include<emmintrin.h>

#pragma warning(pop)

#include<stdint.h>

#include "Main.h"

#include "Dictionary.h"

#pragma comment(lib, "Winmm.lib")



HWND gGameWindow;
BOOL gGameIsRunning; //Global vars are initialized to zero by default
GAMEBITMAP gBackBuffer;
GAMEBITMAP g6x7Font;
dict_t gFontDict;
GAMEPERFDATA gPerformanceData;
HERO gPlayer;
BOOL gWindowHasFocus;
REGISTRYPARAPMS gRegistryParams;


int __stdcall WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, PSTR CommandLine, INT CmdShow)
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

    if ((NtDllModuleHandle = GetModuleHandleA("ntdll.dll")) == NULL)
    {
        MessageBoxA(NULL, "Couldn't load ntdll.dll!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if ((NtQueryTimerResolution = (_NtQueryTimerResolution)GetProcAddress(NtDllModuleHandle, "NtQueryTimerResolution")) == NULL)
    {
        MessageBoxA(NULL, "Couldn't find the NtQueryTimerResolution function in ntdll.dll!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }


    NtQueryTimerResolution(&gPerformanceData.MinimumTimerResolution, &gPerformanceData.MaximumTimerResolution, &gPerformanceData.CurrentTimerResolution);

    GetSystemInfo(&gPerformanceData.SystemInfo);

    GetSystemTimeAsFileTime(&gPerformanceData.PreviousSystemTime);

    if (GameIsAlreadyRunning() == TRUE)
    {
        MessageBoxA(NULL, "Another instance of this program is already running!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if (timeBeginPeriod(1) == TIMERR_NOCANDO) //Setting the Timer resolution to 1ms | timeBeginPeriod() returns a TIMERR_NOCANDO if it fails
    {
        MessageBoxA(NULL, "Failed to set global timer resolution!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if (SetPriorityClass(ProcessHandle, HIGH_PRIORITY_CLASS) == 0)
    {
        MessageBoxA(NULL, "Failed to set process priority!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST) == 0)
    {
        MessageBoxA(NULL, "Failed to set thread priority!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }

    if (CreateMainGameWindow() != ERROR_SUCCESS)
    {
        goto Exit;
    }

    if ((Load32BppBitmapFromFile(".\\Assets\\6x7Font.bmpx", &g6x7Font)) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    InitializeFont(&g6x7Font);
    
    QueryPerformanceFrequency((LARGE_INTEGER*)&gPerformanceData.PerfFrequency);

    gPerformanceData.DisplayDebugInfo = TRUE;
    gBackBuffer.BitMapInfo.bmiHeader.biSize = sizeof(gBackBuffer.BitMapInfo.bmiHeader);
    gBackBuffer.BitMapInfo.bmiHeader.biWidth = GAME_RES_WIDTH;
    gBackBuffer.BitMapInfo.bmiHeader.biHeight = GAME_RES_HEIGHT;
    gBackBuffer.BitMapInfo.bmiHeader.biBitCount = GAME_BPP;
    gBackBuffer.BitMapInfo.bmiHeader.biCompression = BI_RGB;
    gBackBuffer.BitMapInfo.bmiHeader.biPlanes = 1;
    gBackBuffer.Memory = VirtualAlloc(NULL, GAME_DRAWING_AREA_MEMORY_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (gBackBuffer.Memory == NULL)
    {
        MessageBoxA(NULL, "Failed to allocate memory for drawing surface!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    memset(gBackBuffer.Memory, 0x7F, GAME_DRAWING_AREA_MEMORY_SIZE);
    
    
    if (InitializeHero() != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Failed to initialize hero!", "Error!", MB_ICONEXCLAMATION | MB_OK);
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

        QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);

        ElapsedMicroseconds = FrameEnd - FrameStart;
        ElapsedMicroseconds *= 1000000;
        ElapsedMicroseconds /= gPerformanceData.PerfFrequency;

        gPerformanceData.TotalFramesRendered++;
        ElapsedMicrosecondsAccumulatorRaw += ElapsedMicroseconds;

        while (ElapsedMicroseconds < TARGET_MICROSECONDS_PER_FRAME)
        {
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

            GetProcessTimes(ProcessHandle,
                &ProcessCreationTime,
                &ProcessExitTime,
                (FILETIME*)&CurrentKernelCPUTime,
                (FILETIME*)&CurrentUserCPUTime);

            gPerformanceData.CPUPercent = (CurrentKernelCPUTime - PreviousKernelCPUTime) + (CurrentUserCPUTime - PreviousUserCPUTime);

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

        MessageBoxA(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        goto Exit;
    }



    gGameWindow = CreateWindowExA(0, WindowClass.lpszClassName, "Window Title", WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, GetModuleHandleA(NULL), NULL);
    //The '|' allows to mash styles together

    if (gGameWindow == NULL)
    {
        Result = GetLastError();

        MessageBoxA(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        
        goto Exit;
    }

    gPerformanceData.MonitorInfo.cbSize = sizeof(MONITORINFO);


    if (GetMonitorInfoA(MonitorFromWindow(gGameWindow, MONITOR_DEFAULTTOPRIMARY), &gPerformanceData.MonitorInfo) == 0)
    {
        Result = ERROR_MONITOR_NO_DESCRIPTOR;

        goto Exit;
    }

    gPerformanceData.MonitorWidth = gPerformanceData.MonitorInfo.rcMonitor.right - gPerformanceData.MonitorInfo.rcMonitor.left; //Long is 32 bits just like an int
    gPerformanceData.MonitorHeight = gPerformanceData.MonitorInfo.rcMonitor.bottom - gPerformanceData.MonitorInfo.rcMonitor.top;

    if (SetWindowLongPtrA(gGameWindow, GWL_STYLE, WS_VISIBLE) == 0)
    {
        Result = GetLastError();

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

    int16_t EscapeKeyIsDown = GetAsyncKeyState(VK_ESCAPE);
    int16_t DebugKeyIsDown = GetAsyncKeyState(VK_F1);
    int16_t LeftKeyIsDown = GetAsyncKeyState(VK_LEFT) | GetAsyncKeyState('A');
    int16_t RightKeyIsDown = GetAsyncKeyState(VK_RIGHT) | GetAsyncKeyState('D');
    int16_t UpKeyIsDown = GetAsyncKeyState(VK_UP) | GetAsyncKeyState('W');
    int16_t DownKeyIsDown = GetAsyncKeyState(VK_DOWN) | GetAsyncKeyState('S');

    static int16_t DebugKeyWasDown;
    static int16_t LeftKeyWasDown;
    static int16_t RightKeyWasDown;
    static int16_t UpKeyWasDown;
    static int16_t DownKeyWasDown;

    if (EscapeKeyIsDown)
    {
        SendMessageA(gGameWindow, WM_CLOSE, 0, 0);
    }

    if (DebugKeyIsDown && !DebugKeyWasDown)
    {
        gPerformanceData.DisplayDebugInfo = !gPerformanceData.DisplayDebugInfo;
    }

    if (!gPlayer.MovementRemaining)
    {
        if (DownKeyIsDown)
        {
            if (gPlayer.ScreenPosY < GAME_RES_HEIGHT - 16)
            {
                gPlayer.MovementRemaining = 16;
                gPlayer.Direction = DIRECTION_DOWN;
            }
        }
        else if (LeftKeyIsDown)
        {
            if (gPlayer.ScreenPosX > 0)
            {
                gPlayer.MovementRemaining = 16;
                gPlayer.Direction = DIRECTION_LEFT;
            }
        }
        else if (RightKeyIsDown)
        {
            if (gPlayer.ScreenPosX < GAME_RES_WIDTH - 16)
            {
                gPlayer.MovementRemaining = 16;
                gPlayer.Direction = DIRECTION_RIGHT;
            }
        }
        else if (UpKeyIsDown)
        {
            if (gPlayer.ScreenPosY > 0)
            {
                gPlayer.MovementRemaining = 16;
                gPlayer.Direction = DIRECTION_UP;
            }
        }
    }
    else
    {
        gPlayer.MovementRemaining--;

        if (gPlayer.Direction == DIRECTION_DOWN)
        {
            gPlayer.ScreenPosY++;
        }
        else if (gPlayer.Direction == DIRECTION_LEFT)
        {
            gPlayer.ScreenPosX--;
        }
        else if (gPlayer.Direction == DIRECTION_RIGHT)
        {
            gPlayer.ScreenPosX++;
        }
        else if (gPlayer.Direction == DIRECTION_UP)
        {
            gPlayer.ScreenPosY--;
        }

        switch (gPlayer.MovementRemaining)
        {
            case 16:
            {
                gPlayer.SpriteIndex = 0;
                break;
            }
            case 12:
            {
                gPlayer.SpriteIndex = 1;
                break;
            }
            case 8:
            {
                gPlayer.SpriteIndex = 0;
                break;
            }
            case 4:
            {
                gPlayer.SpriteIndex = 2;
                break;
            }
            case 0:
            {
                gPlayer.SpriteIndex = 0;
                break;
            }

            default:
            {

            }
        }
    }


    DebugKeyWasDown = DebugKeyIsDown;
    LeftKeyWasDown = LeftKeyIsDown;
    RightKeyWasDown = RightKeyIsDown;
    UpKeyWasDown = UpKeyIsDown;
    DownKeyWasDown = DownKeyIsDown;
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

    return(Error);
}

DWORD InitializeHero(void)
{
    DWORD Error = ERROR_SUCCESS;

    gPlayer.ScreenPosX = 64;
    gPlayer.ScreenPosY = 64;
    gPlayer.CurrentArmor = SUIT_0;
    gPlayer.Direction = DIRECTION_DOWN;

    if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Down_Standing.bmpx", &gPlayer.Sprite[SUIT_0][FACING_DOWN_0])) != ERROR_SUCCESS)
    {
        MessageBoxA(NULL, "Load32BppBitmapFromFile failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    if ((Error = Load32BppBitmapFromFile(".\\Assets\\Hero_Suit0_Down_Walk1.bmpx", &gPlayer.Sprite[SUIT_0][FACING_DOWN_1])) != ERROR_SUCCESS)
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
    }


Exit:
    return(Error);
}


void InitializeFont(_In_ GAMEBITMAP* FontSheet)
{
    uint16_t CharWidth = FontSheet->BitMapInfo.bmiHeader.biWidth / FONT_SHEET_CHARACTERS_PER_ROW;
    uint16_t CharHeight = FontSheet->BitMapInfo.bmiHeader.biHeight;

    uint16_t StartingFontSheetByte = (FontSheet->BitMapInfo.bmiHeader.biWidth * FontSheet->BitMapInfo.bmiHeader.biHeight) - FontSheet->BitMapInfo.bmiHeader.biWidth;

    gFontDict = DictNew();

    DictAdd(gFontDict, 'A', StartingFontSheetByte);
    DictAdd(gFontDict, 'B', StartingFontSheetByte + ((size_t)CharWidth * 1));
    DictAdd(gFontDict, 'C', StartingFontSheetByte + ((size_t)CharWidth * 2));
    DictAdd(gFontDict, 'D', StartingFontSheetByte + ((size_t)CharWidth * 3));
    DictAdd(gFontDict, 'E', StartingFontSheetByte + ((size_t)CharWidth * 4));
    DictAdd(gFontDict, 'F', StartingFontSheetByte + ((size_t)CharWidth * 5));
    DictAdd(gFontDict, 'G', StartingFontSheetByte + ((size_t)CharWidth * 6));
    DictAdd(gFontDict, 'H', StartingFontSheetByte + ((size_t)CharWidth * 7));
    DictAdd(gFontDict, 'I', StartingFontSheetByte + ((size_t)CharWidth * 8));
    DictAdd(gFontDict, 'J', StartingFontSheetByte + ((size_t)CharWidth * 9));
    DictAdd(gFontDict, 'K', StartingFontSheetByte + ((size_t)CharWidth * 10));
    DictAdd(gFontDict, 'L', StartingFontSheetByte + ((size_t)CharWidth * 11));
    DictAdd(gFontDict, 'M', StartingFontSheetByte + ((size_t)CharWidth * 12));
    DictAdd(gFontDict, 'N', StartingFontSheetByte + ((size_t)CharWidth * 13));
    DictAdd(gFontDict, 'O', StartingFontSheetByte + ((size_t)CharWidth * 14));
    DictAdd(gFontDict, 'P', StartingFontSheetByte + ((size_t)CharWidth * 15));
    DictAdd(gFontDict, 'Q', StartingFontSheetByte + ((size_t)CharWidth * 16));
    DictAdd(gFontDict, 'R', StartingFontSheetByte + ((size_t)CharWidth * 17));
    DictAdd(gFontDict, 'S', StartingFontSheetByte + ((size_t)CharWidth * 18));
    DictAdd(gFontDict, 'T', StartingFontSheetByte + ((size_t)CharWidth * 19));
    DictAdd(gFontDict, 'U', StartingFontSheetByte + ((size_t)CharWidth * 20));
    DictAdd(gFontDict, 'V', StartingFontSheetByte + ((size_t)CharWidth * 21));
    DictAdd(gFontDict, 'W', StartingFontSheetByte + ((size_t)CharWidth * 22));
    DictAdd(gFontDict, 'X', StartingFontSheetByte + ((size_t)CharWidth * 23));
    DictAdd(gFontDict, 'Y', StartingFontSheetByte + ((size_t)CharWidth * 24));
    DictAdd(gFontDict, 'Z', StartingFontSheetByte + ((size_t)CharWidth * 25));
    DictAdd(gFontDict, 'a', StartingFontSheetByte + ((size_t)CharWidth * 26));
    DictAdd(gFontDict, 'b', StartingFontSheetByte + ((size_t)CharWidth * 27));
    DictAdd(gFontDict, 'c', StartingFontSheetByte + ((size_t)CharWidth * 28));
    DictAdd(gFontDict, 'd', StartingFontSheetByte + ((size_t)CharWidth * 29));
    DictAdd(gFontDict, 'e', StartingFontSheetByte + ((size_t)CharWidth * 30));
    DictAdd(gFontDict, 'f', StartingFontSheetByte + ((size_t)CharWidth * 31));
    DictAdd(gFontDict, 'g', StartingFontSheetByte + ((size_t)CharWidth * 32));
    DictAdd(gFontDict, 'h', StartingFontSheetByte + ((size_t)CharWidth * 33));
    DictAdd(gFontDict, 'i', StartingFontSheetByte + ((size_t)CharWidth * 34));
    DictAdd(gFontDict, 'j', StartingFontSheetByte + ((size_t)CharWidth * 35));
    DictAdd(gFontDict, 'k', StartingFontSheetByte + ((size_t)CharWidth * 36));
    DictAdd(gFontDict, 'l', StartingFontSheetByte + ((size_t)CharWidth * 37));
    DictAdd(gFontDict, 'm', StartingFontSheetByte + ((size_t)CharWidth * 38));
    DictAdd(gFontDict, 'n', StartingFontSheetByte + ((size_t)CharWidth * 39));
    DictAdd(gFontDict, 'o', StartingFontSheetByte + ((size_t)CharWidth * 40));
    DictAdd(gFontDict, 'p', StartingFontSheetByte + ((size_t)CharWidth * 41));
    DictAdd(gFontDict, 'q', StartingFontSheetByte + ((size_t)CharWidth * 42));
    DictAdd(gFontDict, 'r', StartingFontSheetByte + ((size_t)CharWidth * 43));
    DictAdd(gFontDict, 's', StartingFontSheetByte + ((size_t)CharWidth * 44));
    DictAdd(gFontDict, 't', StartingFontSheetByte + ((size_t)CharWidth * 45));
    DictAdd(gFontDict, 'u', StartingFontSheetByte + ((size_t)CharWidth * 46));
    DictAdd(gFontDict, 'v', StartingFontSheetByte + ((size_t)CharWidth * 47));
    DictAdd(gFontDict, 'w', StartingFontSheetByte + ((size_t)CharWidth * 48));
    DictAdd(gFontDict, 'x', StartingFontSheetByte + ((size_t)CharWidth * 49));
    DictAdd(gFontDict, 'y', StartingFontSheetByte + ((size_t)CharWidth * 50));
    DictAdd(gFontDict, 'z', StartingFontSheetByte + ((size_t)CharWidth * 51));
    DictAdd(gFontDict, '0', StartingFontSheetByte + ((size_t)CharWidth * 52));
    DictAdd(gFontDict, '1', StartingFontSheetByte + ((size_t)CharWidth * 53));
    DictAdd(gFontDict, '2', StartingFontSheetByte + ((size_t)CharWidth * 54));
    DictAdd(gFontDict, '3', StartingFontSheetByte + ((size_t)CharWidth * 55));
    DictAdd(gFontDict, '4', StartingFontSheetByte + ((size_t)CharWidth * 56));
    DictAdd(gFontDict, '5', StartingFontSheetByte + ((size_t)CharWidth * 57));
    DictAdd(gFontDict, '6', StartingFontSheetByte + ((size_t)CharWidth * 58));
    DictAdd(gFontDict, '7', StartingFontSheetByte + ((size_t)CharWidth * 59));
    DictAdd(gFontDict, '8', StartingFontSheetByte + ((size_t)CharWidth * 60));
    DictAdd(gFontDict, '9', StartingFontSheetByte + ((size_t)CharWidth * 61));
    DictAdd(gFontDict, '`', StartingFontSheetByte + ((size_t)CharWidth * 62));
    DictAdd(gFontDict, '~', StartingFontSheetByte + ((size_t)CharWidth * 63));
    DictAdd(gFontDict, '!', StartingFontSheetByte + ((size_t)CharWidth * 64));
    DictAdd(gFontDict, '@', StartingFontSheetByte + ((size_t)CharWidth * 65));
    DictAdd(gFontDict, '#', StartingFontSheetByte + ((size_t)CharWidth * 66));
    DictAdd(gFontDict, '$', StartingFontSheetByte + ((size_t)CharWidth * 67));
    DictAdd(gFontDict, '%', StartingFontSheetByte + ((size_t)CharWidth * 68));
    DictAdd(gFontDict, '^', StartingFontSheetByte + ((size_t)CharWidth * 69));
    DictAdd(gFontDict, '&', StartingFontSheetByte + ((size_t)CharWidth * 70));
    DictAdd(gFontDict, '*', StartingFontSheetByte + ((size_t)CharWidth * 71));
    DictAdd(gFontDict, '(', StartingFontSheetByte + ((size_t)CharWidth * 72));
    DictAdd(gFontDict, ')', StartingFontSheetByte + ((size_t)CharWidth * 73));
    DictAdd(gFontDict, '-', StartingFontSheetByte + ((size_t)CharWidth * 74));
    DictAdd(gFontDict, '=', StartingFontSheetByte + ((size_t)CharWidth * 75));
    DictAdd(gFontDict, '_', StartingFontSheetByte + ((size_t)CharWidth * 76));
    DictAdd(gFontDict, '+', StartingFontSheetByte + ((size_t)CharWidth * 77));
    DictAdd(gFontDict, '\\', StartingFontSheetByte + ((size_t)CharWidth * 78));
    DictAdd(gFontDict, '|', StartingFontSheetByte + ((size_t)CharWidth * 79));
    DictAdd(gFontDict, '[', StartingFontSheetByte + ((size_t)CharWidth * 80));
    DictAdd(gFontDict, ']', StartingFontSheetByte + ((size_t)CharWidth * 81));
    DictAdd(gFontDict, '{', StartingFontSheetByte + ((size_t)CharWidth * 82));
    DictAdd(gFontDict, '}', StartingFontSheetByte + ((size_t)CharWidth * 83));
    DictAdd(gFontDict, ';', StartingFontSheetByte + ((size_t)CharWidth * 84));
    DictAdd(gFontDict, '\'', StartingFontSheetByte +((size_t)CharWidth * 85));
    DictAdd(gFontDict, ':', StartingFontSheetByte + ((size_t)CharWidth * 86));
    DictAdd(gFontDict, '"', StartingFontSheetByte + ((size_t)CharWidth * 87));
    DictAdd(gFontDict, ',', StartingFontSheetByte + ((size_t)CharWidth * 88));
    DictAdd(gFontDict, '<', StartingFontSheetByte + ((size_t)CharWidth * 89));
    DictAdd(gFontDict, '>', StartingFontSheetByte + ((size_t)CharWidth * 90));
    DictAdd(gFontDict, '.', StartingFontSheetByte + ((size_t)CharWidth * 91));
    DictAdd(gFontDict, '/', StartingFontSheetByte + ((size_t)CharWidth * 92));
    DictAdd(gFontDict, '?', StartingFontSheetByte + ((size_t)CharWidth * 93));
    DictAdd(gFontDict, ' ', StartingFontSheetByte + ((size_t)CharWidth * 94));
    DictAdd(gFontDict, '»', StartingFontSheetByte + ((size_t)CharWidth * 95));
    DictAdd(gFontDict, '«', StartingFontSheetByte + ((size_t)CharWidth * 96));
    DictAdd(gFontDict, '\xf2', StartingFontSheetByte + ((size_t)CharWidth * 97));

}


void BlitStringToBuffer(_In_ char* String, _In_ GAMEBITMAP* FontSheet, _In_ PIXEL32* Color, _In_ uint16_t x, _In_ uint16_t y)
{
    uint16_t CharWidth = (uint16_t)FontSheet->BitMapInfo.bmiHeader.biWidth / FONT_SHEET_CHARACTERS_PER_ROW;
    uint16_t CharHeight = (uint16_t)FontSheet->BitMapInfo.bmiHeader.biHeight;
    uint16_t BytesPerCharacter = (CharWidth * CharHeight * (FontSheet->BitMapInfo.bmiHeader.biBitCount / 8));
    uint16_t StringLength = strlen(String);
    GAMEBITMAP StringBitmap = { 0 };
    StringBitmap.BitMapInfo.bmiHeader.biBitCount = GAME_BPP;
    StringBitmap.BitMapInfo.bmiHeader.biHeight = CharHeight;
    StringBitmap.BitMapInfo.bmiHeader.biWidth = CharWidth * StringLength;
    StringBitmap.BitMapInfo.bmiHeader.biPlanes = 1;
    StringBitmap.BitMapInfo.bmiHeader.biCompression = BI_RGB;
    StringBitmap.Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ((size_t)BytesPerCharacter * (size_t)StringLength));
    
    for (int Character = 0; Character < StringLength; Character++)
    {
        int StartingFontSheetByte = DictFind(gFontDict, String[Character]);
        int FontSheetOffset = 0;
        int StringBitmapOffset = 0;
        PIXEL32 FontSheetPixel = { 0 };


        for (int YPixel = 0; YPixel < CharHeight; YPixel++)
        {
            for (int XPixel = 0; XPixel < CharWidth; XPixel++)
            {
                FontSheetOffset = StartingFontSheetByte + XPixel - (FontSheet->BitMapInfo.bmiHeader.biWidth * YPixel);

                StringBitmapOffset = (Character * CharWidth) + ((StringBitmap.BitMapInfo.bmiHeader.biWidth * StringBitmap.BitMapInfo.bmiHeader.biHeight) - \
                    StringBitmap.BitMapInfo.bmiHeader.biWidth) + XPixel - (StringBitmap.BitMapInfo.bmiHeader.biWidth) * YPixel;

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
    
    #ifdef SIMD
    for (int x = 0; x < GAME_RES_WIDTH * GAME_RES_HEIGHT; x += 4)
    {
        __m128i QuadPixel = { 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff };
        _mm_store_si128((PIXEL32*)gBackBuffer.Memory + x, QuadPixel);
    }
    #else
    for (int x = 0; x < GAME_RES_WIDTH * GAME_RES_HEIGHT; x ++)
    {
        PIXEL32 Pixel = { 0x7f, 0x00, 0x00, 0xff };
        memcpy((PIXEL32*)gBackBuffer.Memory + x, &Pixel, sizeof(PIXEL32));
    }
    #endif

    PIXEL32 FontColor = { 0x00, 0xFF, 0x00, 0xFF };

    BlitStringToBuffer("GAME OVER", &g6x7Font, &FontColor, 100, 100);

    Blit32BppBitmapToBuffer(&gPlayer.Sprite[gPlayer.CurrentArmor][gPlayer.Direction + gPlayer.SpriteIndex], gPlayer.ScreenPosX, gPlayer.ScreenPosY);


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

    if (gPerformanceData.DisplayDebugInfo == TRUE)
    {
        char DebugTextBuffer[64] = { 0 };

        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "FPS Raw:    %.01f", gPerformanceData.RawFPSAverage);
        TextOutA(DeviceContext, 0, 0, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "FPS Cooked: %.01f", gPerformanceData.CoockedFPSAverage);
        TextOutA(DeviceContext, 0, 13, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Min Timer Resolution: %.02f", gPerformanceData.MinimumTimerResolution / 10000.0f);
        TextOutA(DeviceContext, 0, 26, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Max Timer Resolution: %.02f", gPerformanceData.MaximumTimerResolution / 10000.0f);
        TextOutA(DeviceContext, 0, 39, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Cur Timer Resolution: %.02f", gPerformanceData.CurrentTimerResolution / 10000.0f);
        TextOutA(DeviceContext, 0, 52, DebugTextBuffer, (int)strlen(DebugTextBuffer));
        
        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Handles: %lu", gPerformanceData.HandleCount);
        TextOutA(DeviceContext, 0, 65, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Memory:  %lu KB", gPerformanceData.MemInfo.PrivateUsage / 1024);
        TextOutA(DeviceContext, 0, 78, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "CPU:     %.02f %%", gPerformanceData.CPUPercent);
        TextOutA(DeviceContext, 0, 91, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Total Frames: %llu", gPerformanceData.TotalFramesRendered);
        TextOutA(DeviceContext, 0, 104, DebugTextBuffer, (int)strlen(DebugTextBuffer));

        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "ScreenPos: (%d,%d)", gPlayer.ScreenPosX, gPlayer.ScreenPosY);
        TextOutA(DeviceContext, 0, 117, DebugTextBuffer, (int)strlen(DebugTextBuffer));
    }

    ReleaseDC(gGameWindow, DeviceContext);
}

void Blit32BppBitmapToBuffer(_In_ GAMEBITMAP* GameBitmap, _In_ uint16_t x, _In_ uint16_t y)
{
    int32_t StartingScreenPixel = ((GAME_RES_WIDTH * GAME_RES_HEIGHT) - GAME_RES_WIDTH) - (GAME_RES_WIDTH * y) + x;
    int32_t StartingBitmapPixel = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - \
        GameBitmap->BitMapInfo.bmiHeader.biWidth;
    int32_t MemoryOffset = 0;
    int32_t BitmapOffset = 0;
    PIXEL32 BitmapPixel = { 0 };
    //PIXEL32 BackgroundPixel = { 0 };

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
        LogMessageA(LOG_LEVEL_ERROR, "[%s] RegCreateKey failed with error code 0x%08lx!", __FUNCTION__, Result);
        //__FUNCTION__ logs the name of the function that we are currenty in

        goto Exit;
    }

    if (RegDisposition == REG_CREATED_NEW_KEY)
    {
        LogMessageA(LOG_LEVEL_INFO, "[%s] Registy key did not exist; created new key HKCU\\SOFTWARE\\%s.", __FUNCTION__, GAME_NAME);
    }
    else
    {
        LogMessageA(LOG_LEVEL_INFO, "[%s] Opened existing registry key HKCU\\SOFTWARE\\%s", __FUNCTION__, GAME_NAME);
    }

    Result = RegGetValueA(RegKey, NULL, "LogLevel", RRF_RT_DWORD, NULL, (BYTE*)&gRegistryParams.LogLevel, &RegBytesRead);

    if (Result != ERROR_SUCCESS)
    {
        if (Result == ERROR_FILE_NOT_FOUND)
        {
            Result = ERROR_SUCCESS;

            LogMessageA(LOG_LEVEL_INFO, "[%s] Registry value 'LogLevel' not found. Using default of 0. (LOG_LEVEL_NONE)", __FUNCTION__);
            gRegistryParams.LogLevel = LOG_LEVEL_NONE;
        }
        else
        {
            LogMessageA(LOG_LEVEL_ERROR, "[%s] Failed to read the 'LogLevel' registry value! Error: 0x%08lx!", __FUNCTION__, Result);
            goto Exit;
        }
    }


    LogMessageA(LOG_LEVEL_INFO, "[%s] LogLevel is %d.", __FUNCTION__, gRegistryParams.LogLevel);


Exit:

    if (RegKey)
    {
        RegCloseKey(RegKey);
    }

    return(Result);
}

void LogMessageA(_In_ DWORD LogLevel, _In_ char* Message, _In_ ...)
{
    size_t MessageLength = strlen(Message);
    SYSTEMTIME Time = { 0 };
    HANDLE LogFileHandle = INVALID_HANDLE_VALUE;
    DWORD EndOfFile = 0;
    DWORD NumberOfBytesWritten = 0;
    char DateTimeString[96] = { 0 };
    char SeverityString[8] = { 0 };
    char FormattedString[4096] = { 0 };
    int Error = 0;

    if (gRegistryParams.LogLevel < LogLevel)
    {
        return;
    }

    if (MessageLength < 1 || MessageLength > 4096)
    {
        return;
    }

    switch (LogLevel)
    {
        case LOG_LEVEL_NONE:
        {
            return;
        }
        case LOG_LEVEL_INFO:
        {
            strcpy_s(SeverityString, sizeof(SeverityString), "[INFO]"); //strcpy_s: String copy
            break;
        }
        case LOG_LEVEL_WARN:
        {
            strcpy_s(SeverityString, sizeof(SeverityString), "[WARN]");
            break;
        }
        case LOG_LEVEL_ERROR:
        {
            strcpy_s(SeverityString, sizeof(SeverityString), "[ERROR]");
            break;
        }
        case LOG_LEVEL_DEBUG:
        {
            strcpy_s(SeverityString, sizeof(SeverityString), "[DEBUG]");
            break;
        }
        default:
        {
            //Assert
        }
    }

    GetLocalTime(&Time);

    va_list ArgPointer = NULL;
    va_start(ArgPointer, Message);
    _vsnprintf_s(FormattedString, sizeof(FormattedString), _TRUNCATE, Message, ArgPointer);
    va_end(ArgPointer);

    Error = _snprintf_s(DateTimeString, sizeof(DateTimeString), _TRUNCATE, "\r\n[%02u/%02u/%u %02u:%02u:%02u.%03u]", Time.wMonth, Time.wDay, Time.wYear, Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds);

    if ((LogFileHandle = CreateFileA(LOG_FILE_NAME, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        return;
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