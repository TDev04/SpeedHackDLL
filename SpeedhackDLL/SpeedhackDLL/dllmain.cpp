#include <windows.h>
#include <detours.h>
#include <iostream>
#include <thread>
#include <atomic>

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

// typedefs for target functions
typedef BOOL(WINAPI* tQueryPerformanceCounter)(LARGE_INTEGER*);
typedef DWORD(WINAPI* tGetTickCount)(VOID);
typedef ULONGLONG(WINAPI* tGetTickCount64)(VOID);

// original function pointers
static tQueryPerformanceCounter True_QPC = nullptr;
static tGetTickCount True_GTC = nullptr;
static tGetTickCount64 True_GTC64 = nullptr;

// speed and base/offset times
static std::atomic<double> g_speed(1.0);

static LONGLONG QPC_Base = 0, QPC_Offset = 0;
static DWORD GTC_Base = 0, GTC_Offset = 0;
static ULONGLONG GTC64_Base = 0, GTC64_Offset = 0;

// hooking QueryPerformanceCounter
BOOL WINAPI Hooked_QPC(LARGE_INTEGER* lpPerformanceCount)
{
    LARGE_INTEGER real;
    True_QPC(&real);

    if (QPC_Base == 0) {
        QPC_Base = real.QuadPart;
        QPC_Offset = QPC_Base;
    }

    LONGLONG elapsed = real.QuadPart - QPC_Base;
    LONGLONG scaled = (LONGLONG)(elapsed * g_speed.load());
    lpPerformanceCount->QuadPart = QPC_Offset + scaled;

    return TRUE;
}

// hooking  GetTickCount
DWORD WINAPI Hooked_GTC()
{
    DWORD real = True_GTC();

    if (GTC_Base == 0) {
        GTC_Base = real;
        GTC_Offset = real;
    }

    DWORD elapsed = real - GTC_Base;
    DWORD scaled = (DWORD)(elapsed * g_speed.load());
    return GTC_Offset + scaled;
}

// hooking GetTickCount64
ULONGLONG WINAPI Hooked_GTC64()
{
    ULONGLONG real = True_GTC64();

    if (GTC64_Base == 0) {
        GTC64_Base = real;
        GTC64_Offset = real;
    }

    ULONGLONG elapsed = real - GTC64_Base;
    ULONGLONG scaled = (ULONGLONG)(elapsed * g_speed.load());
    return GTC64_Offset + scaled;
}

DWORD WINAPI HotkeyThread(LPVOID)
{
    RegisterHotKey(NULL, 1, MOD_CONTROL, VK_UP);   // Ctrl+Up
    RegisterHotKey(NULL, 2, MOD_CONTROL, VK_DOWN); // Ctrl+Down

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            if (msg.wParam == 1) {
                g_speed.store(g_speed.load() + 0.1);
                std::cout << "Speed: " << g_speed.load() << "\n";
            }
            else if (msg.wParam == 2) {
                g_speed.store(MAX(0.1, g_speed.load() - 0.1));
                std::cout << "Speed: " << g_speed.load() << "\n";
            }
        }
    }
    return 0;
}

// console thread for input
DWORD WINAPI ConsoleThread(LPVOID)
{
    if (AllocConsole()) {
        FILE* f;
        freopen_s(&f, "CONIN$", "r", stdin);
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);

        std::cout << "Type new speed (0 - 500):\n";

        while (true) {
            double newSpeed;
            if (std::cin >> newSpeed) {
                if (newSpeed >= 0 && newSpeed <= 100.0) {
                    // update offsets so time stays continuous
                    GTC_Offset = Hooked_GTC();
                    GTC_Base = True_GTC();

                    GTC64_Offset = Hooked_GTC64();
                    GTC64_Base = True_GTC64();

                    LARGE_INTEGER tmp;
                    Hooked_QPC(&tmp);
                    QPC_Offset = tmp.QuadPart;
                    LARGE_INTEGER base;
                    True_QPC(&base);
                    QPC_Base = base.QuadPart;

                    g_speed.store(newSpeed);
                    std::cout << "Speed set to " << newSpeed << "\n";
                }
                else {
                    std::cout << "Speed must be between 0.001 and 100.0\n";
                }
            }
            else {
                std::cin.clear();
                std::cin.ignore(1000, '\n');
            }
        }
    }
    else {
        MessageBoxA(NULL, "The console could not be allocated, perhaps another console is hooked to it?\nUse Ctrl+Up and Ctrl+Dwn to manipulate the speed.", "Warning", MB_ICONEXCLAMATION | MB_OK);
        CreateThread(nullptr, 0, HotkeyThread, nullptr, 0, nullptr);
    }
    return 0;
}

// main dll thread
DWORD WINAPI MainThread(LPVOID)
{
    // resolve the functions
    True_QPC = (tQueryPerformanceCounter)GetProcAddress(GetModuleHandleA("kernel32.dll"), "QueryPerformanceCounter");
    True_GTC = (tGetTickCount)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetTickCount");
    True_GTC64 = (tGetTickCount64)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetTickCount64");

    if (True_QPC && True_GTC && True_GTC64) {
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourAttach(&(PVOID&)True_QPC, Hooked_QPC);
        DetourAttach(&(PVOID&)True_GTC, Hooked_GTC);
        DetourAttach(&(PVOID&)True_GTC64, Hooked_GTC64);

        LONG error = DetourTransactionCommit();
        if (error == NO_ERROR) {
            CreateThread(nullptr, 0, ConsoleThread, nullptr, 0, nullptr);
        }
        else {
            char msg[256];
            sprintf_s(msg, "Failed to install hook: %ld", error);
            MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
        }
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
    }
    return TRUE;
}