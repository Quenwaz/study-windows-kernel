
#include <windows.h>
#include <map>
#include <string>
#include <ctime>
#include <tchar.h>
#include <chrono>

#pragma data_seg("shared")
HHOOK hShellHook=NULL;
HWND hMainWindow = NULL; // 主程序的窗口句柄
#pragma data_seg()
#pragma comment(linker,"/SECTION:shared,RWS")


#ifdef VER1
std::map<std::string, long long> usageData; // 记录每个窗口的使用时间（毫秒）
std::string lastWindowTitle = "";
auto lastTimePoint = std::chrono::steady_clock::now();
#endif

HMODULE _hmod = NULL;

#ifdef VER1
std::string GetWindowTitle(HWND hwnd)
{
    char windowTitle[256];
    if (hwnd != NULL)
    {
        GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle)); // 获取窗口标题
        return std::string(windowTitle);
    }
    return "";
}
#endif



#define DBGMSG(fmt,...) \
{\
char __message__[1024] = {0};\
snprintf(__message__,sizeof __message__, fmt,__VA_ARGS__);\
OutputDebugStringA(__message__);\
}

LRESULT CALLBACK ShellProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    
    DBGMSG("code=%d, wparam=%d, lparam=%d\n", nCode, wParam, lParam);
    if (nCode == HSHELL_WINDOWACTIVATED )
    {
        const auto handstr = std::to_string(wParam);
        HWND hwnd = (HWND)wParam;
        COPYDATASTRUCT cds;
        cds.dwData = 1; // 可自定义标识
        cds.cbData = handstr.size() + 1;
        cds.lpData = (PVOID)handstr.c_str();

        SendMessage(hMainWindow, WM_COPYDATA, wParam, (LPARAM)&cds);
        const auto lasterr = GetLastError();
        DBGMSG("GetLastError=%d\n",lasterr);
#ifdef VER1
        std::string currentWindowTitle = GetWindowTitle(hwnd);
        if (!currentWindowTitle.empty())
        {
            if (currentWindowTitle != lastWindowTitle)
            {
                if (!lastWindowTitle.empty())
                {
                    auto now = std::chrono::steady_clock::now();
                    long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTimePoint).count();
                    usageData[lastWindowTitle] += duration;
                }
                lastWindowTitle = currentWindowTitle;
                lastTimePoint = std::chrono::steady_clock::now();
                
            }
        }
#endif
    }

    return CallNextHookEx(hShellHook, nCode, wParam, lParam);
}

extern "C" __declspec(dllexport) bool SetShellHook(HINSTANCE hInstance, HWND hWnd)
{
    hMainWindow = hWnd;
    hShellHook = SetWindowsHookEx(WH_SHELL, ShellProc, _hmod, 0);
    if (hShellHook == nullptr){
        fprintf(stderr, "occur error: %d\n", GetLastError());
    }
    return hShellHook != NULL;
}

extern "C" __declspec(dllexport) void RemoveShellHook()
{
    hMainWindow = NULL;
    if (hShellHook)
    {
        UnhookWindowsHookEx(hShellHook);
        hShellHook = NULL;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    OutputDebugStringA("hoook \n");
    _hmod = hModule;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
