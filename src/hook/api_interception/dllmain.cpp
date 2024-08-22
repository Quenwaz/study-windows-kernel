#include <Windows.h>
#include <cstdio>

HWND GetConsoleHwnd(void)
{
    #define MY_BUFSIZE 1024 // Buffer size for console window titles.
    HWND hwndFound;         // This is what is returned to the caller.
    char pszNewWindowTitle[MY_BUFSIZE]; // Contains fabricated
                                        // WindowTitle.
    char pszOldWindowTitle[MY_BUFSIZE]; // Contains original
                                        // WindowTitle.

    // Fetch current window title.
    GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);

    wsprintf(pszNewWindowTitle,"%d/%d",
                GetTickCount(),
                GetCurrentProcessId());
    // Change current window title.
    SetConsoleTitle(pszNewWindowTitle);
    // Ensure window title has been updated.
    Sleep(40);
    // Look for NewWindowTitle.
    hwndFound=FindWindow(NULL, pszNewWindowTitle);
    // Restore original window title.
    SetConsoleTitle(pszOldWindowTitle);
    return(hwndFound);
}



BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, PVOID fImpLoad) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        fprintf(stderr, "API HOOK DLL_PROCESS_ATTACH");
        // HWND currentCuiWnd = GetConsoleHwnd();
        // if (currentCuiWnd == NULL){
        //     AllocConsole();
        //     freopen("CONOUT$", "w", stdout);
        // }
    }

    return(TRUE);
}
