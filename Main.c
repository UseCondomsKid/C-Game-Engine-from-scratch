
#pragma warning(push, 3)

#include<stdio.h>

#include<windows.h>

#include<psapi.h>

#include<emmintrin.h>

#pragma warning(pop)

#include<stdint.h>

#include "Main.h"

#pragma comment(lib, "Winmm.lib")



HWND gGameWindow;
BOOL gGameIsRunning; //Global vars are initialized to zero by default
GAMEBITMAP gBackBuffer;
GAMEBITMAP g6x7Font;
GAMEPERFDATA gPerformanceData;
HERO gPlayer;
BOOL gWindowHasFocus;


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


void BlitStringToBuffer(_In_ char* String, _In_ GAMEBITMAP* GameBitmap, _In_ uint16_t x, _In_ uint16_t y)
{
    int CharWidth = GameBitmap->BitMapInfo.bmiHeader.biWidth / FONT_SHEET_CHARACTERS_PER_ROW;
    int CharHeight = GameBitmap->BitMapInfo.bmiHeader.biHeight;
    int BytesPerCharacter = (CharWidth * CharHeight * (GameBitmap->BitMapInfo.bmiHeader.biBitCount / 8));
    int StringLength = strlen(String);
    GAMEBITMAP StringBitmap = { 0 };
    StringBitmap.BitMapInfo.bmiHeader.biBitCount = GAME_BPP;
    StringBitmap.BitMapInfo.bmiHeader.biHeight = CharHeight;
    StringBitmap.BitMapInfo.bmiHeader.biWidth = CharWidth * StringLength;
    StringBitmap.BitMapInfo.bmiHeader.biPlanes = 1;
    StringBitmap.BitMapInfo.bmiHeader.biCompression = BI_RGB;
    StringBitmap.Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BytesPerCharacter * StringLength);

    for (int Character = 0; Character < StringLength; Character++)
    {
        int StartingFontSheetByte = 0;
        int FontSheetOffset = 0;
        int StringBitmapOffset = 0;
        PIXEL32 FontSheetPixel = { 0 };


        switch (String[Character])
        {
        case 'A':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth;

            break;
        }
        case 'B':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + CharWidth;

            break;
        }
        case 'C':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 2);

            break;
        }
        case 'D':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 3);

            break;
        }
        case 'E':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 4);

            break;
        }
        case 'F':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 5);

            break;
        }
        case 'G':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 6);

            break;
        }
        case 'H':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 7);

            break;
        }
        case 'I':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 8);

            break;
        }
        case 'J':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 9);

            break;
        }
        case 'K':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 10);

            break;
        }
        case 'L':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 11);

            break;
        }
        case 'M':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 12);

            break;
        }
        case 'N':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 13);

            break;
        }
        case 'O':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 14);

            break;
        }
        case 'P':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 15);

            break;
        }
        case 'Q':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 16);

            break;
        }
        case 'R':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 17);

            break;
        }
        case 'S':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 18);

            break;
        }
        case 'T':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 19);

            break;
        }
        case 'U':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 20);

            break;
        }
        case 'V':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 21);

            break;
        }
        case 'W':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 22);

            break;
        }
        case 'X':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 23);

            break;
        }
        case 'Y':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 24);

            break;
        }
        case 'Z':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 25);

            break;
        }
        case 'a':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 26);

            break;
        }
        case 'b':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 27);

            break;
        }
        case 'c':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 28);

            break;
        }
        case 'd':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 29);

            break;
        }
        case 'e':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 30);

            break;
        }
        case 'f':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 31);

            break;
        }
        case 'g':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 32);

            break;
        }
        case 'h':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 33);

            break;
        }
        case 'i':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 34);

            break;
        }
        case 'j':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 35);

            break;
        }
        case 'k':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 36);

            break;
        }
        case 'l':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 37);

            break;
        }
        case 'm':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 38);

            break;
        }
        case 'n':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 39);

            break;
        }
        case 'o':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 40);

            break;
        }
        case 'p':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 41);

            break;
        }
        case 'q':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 42);

            break;
        }
        case 'r':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 43);

            break;
        }
        case 's':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 44);

            break;
        }
        case 't':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 45);

            break;
        }
        case 'u':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 46);

            break;
        }
        case 'v':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 47);

            break;
        }
        case 'w':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 48);

            break;
        }
        case 'x':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 49);

            break;
        }
        case 'y':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 50);

            break;
        }
        case 'z':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 51);

            break;
        }
        case '0':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 52);

            break;
        }
        case '1':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 53);

            break;
        }
        case '2':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 54);

            break;
        }
        case '3':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 55);

            break;
        }
        case '4':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 56);

            break;
        }
        case '5':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 57);

            break;
        }
        case '6':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 58);

            break;
        }
        case '7':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 59);

            break;
        }
        case '8':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 60);

            break;
        }
        case '9':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 61);

            break;
        }
        case '`':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 62);

            break;
        }
        case '~':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 63);

            break;
        }
        case '!':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 64);

            break;
        }
        case '@':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 65);

            break;
        }
        case '#':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 66);

            break;
        }
        case '$':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 67);

            break;
        }
        case '%':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 68);

            break;
        }
        case '^':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 69);

            break;
        }
        case '&':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 70);

            break;
        }
        case '*':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 71);

            break;
        }
        case '(':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 72);

            break;
        }
        case ')':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 73);

            break;
        }
        case '-':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 74);

            break;
        }
        case '=':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 75);

            break;
        }
        case '_':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 76);

            break;
        }
        case '+':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 77);

            break;
        }
        case '\\':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 78);

            break;
        }
        case '|':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 79);

            break;
        }
        case '[':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 80);

            break;
        }
        case ']':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 81);

            break;
        }
        case '{':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 82);

            break;
        }
        case '}':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 83);

            break;
        }
        case ';':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 84);

            break;
        }
        case '\'':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 85);

            break;
        }
        case ':':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 86);

            break;
        }
        case '"':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 87);

            break;
        }
        case ',':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 88);

            break;
        }
        case '<':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 89);

            break;
        }
        case '>':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 90);

            break;
        }
        case '.':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 91);

            break;
        }
        case '/':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 92);

            break;
        }
        case '?':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 93);

            break;
        }
        case ' ':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 94);

            break;
        }
        case '»':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 95);

            break;
        }
        case '«':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 96);

            break;
        }
        case '\xf2':
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 97);

            break;
        }
        default:
        {
            StartingFontSheetByte = (GameBitmap->BitMapInfo.bmiHeader.biWidth * GameBitmap->BitMapInfo.bmiHeader.biHeight) - GameBitmap->BitMapInfo.bmiHeader.biWidth + (CharWidth * 93);
        }
        }


        for (int YPixel = 0; YPixel < CharHeight; YPixel++)
        {
            for (int XPixel = 0; XPixel < CharWidth; XPixel++)
            {
                FontSheetOffset = StartingFontSheetByte + XPixel - (GameBitmap->BitMapInfo.bmiHeader.biWidth * YPixel);

                StringBitmapOffset = (Character * CharWidth) + ((StringBitmap.BitMapInfo.bmiHeader.biWidth * StringBitmap.BitMapInfo.bmiHeader.biHeight) - \
                    StringBitmap.BitMapInfo.bmiHeader.biWidth) + XPixel - (StringBitmap.BitMapInfo.bmiHeader.biWidth) * YPixel;

                memcpy_s(&FontSheetPixel, sizeof(PIXEL32), (PIXEL32*)GameBitmap->Memory + FontSheetOffset, sizeof(PIXEL32));

                FontSheetPixel.Red = 0xff;

                FontSheetPixel.Green = 0x00;

                FontSheetPixel.Blue = 0x00;

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

    BlitStringToBuffer("GAME OVER", &g6x7Font, 100, 100);

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