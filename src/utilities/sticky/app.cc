#include <windows.h>
#include <windowsx.h>
#include <string>
#include <strsafe.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define IDC_BUTTON_TOPMOST 101
#define IDC_BUTTON_CANCEL_TOPMOST 102
#define IDC_EDIT 103

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SetTopMost(HWND hwnd, bool topmost);

HWND hButtonTopMost, hButtonCancelTopMost, hEdit, g_hMainWnd;
HWND currentWindow = NULL;
int action = IDC_BUTTON_TOPMOST;
HHOOK g_hMouseHook;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = TEXT("TopMost Window Class");

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    g_hMainWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        TEXT("窗口置顶工具"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 250, 200,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (g_hMainWnd == NULL) {
        return 0;
    }

    hButtonTopMost = CreateWindow(
        TEXT("BUTTON"),
        TEXT("置顶窗口"),
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        10, 10, 80, 30,
        g_hMainWnd,
        (HMENU)IDC_BUTTON_TOPMOST,
        hInstance,
        NULL
    );

    hButtonCancelTopMost = CreateWindow(
        TEXT("BUTTON"),
        TEXT("取消置顶"),
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        120, 10, 80, 30,
        g_hMainWnd,
        (HMENU)IDC_BUTTON_CANCEL_TOPMOST,
        hInstance,
        NULL
    );

    hEdit = CreateWindow(
        TEXT("EDIT"),
        TEXT(""),
        WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
        10, 50, 220, 110,
        g_hMainWnd,
        (HMENU)IDC_EDIT,
        hInstance,
        NULL
    );

    ShowWindow(g_hMainWnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}


LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if(nCode >= 0){
        if (wParam == WM_MOUSEMOVE)
        {
            LPMOUSEHOOKSTRUCT  mouseinfo = reinterpret_cast<LPMOUSEHOOKSTRUCT>(lParam);
            // POINT pt;
            // pt.x = GET_X_LPARAM(lParam);
            // pt.y = GET_Y_LPARAM(lParam);
            // ClientToScreen(hwnd, &pt);
            HWND windowUnderCursor = WindowFromPoint(mouseinfo->pt);
            static wchar_t title[256] = {0};
            if (windowUnderCursor != NULL && windowUnderCursor != g_hMainWnd && windowUnderCursor != currentWindow) {
                currentWindow = windowUnderCursor;
                ZeroMemory(title, sizeof(title));
                GetWindowText(currentWindow, title, sizeof(title));
            }

            TCHAR message[1024] = {0};
            swprintf_s(message, TEXT("[%d,%d] %s"), mouseinfo->pt.x, mouseinfo->pt.y,title);
            SetWindowText(hEdit, message);
        }else if (wParam == WM_LBUTTONDOWN)
        {
            if (currentWindow != NULL && currentWindow != g_hMainWnd) {
                SetTopMost(currentWindow, action == IDC_BUTTON_TOPMOST);
                // Unhook the mouse hook before exiting
                UnhookWindowsHookEx(g_hMouseHook);
            }
        }else if (WM_RBUTTONDOWN == wParam){
            // Unhook the mouse hook before exiting
            UnhookWindowsHookEx(g_hMouseHook);
        }
    }

    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {
    case WM_COMMAND:
        action = LOWORD(wParam);
        if (action == IDC_BUTTON_TOPMOST || action == IDC_BUTTON_CANCEL_TOPMOST) {
            // Set global mouse hook
            g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void SetTopMost(HWND targetHwnd, bool topmost) {
    HWND insertAfter = topmost ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(targetHwnd, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    
    wchar_t title[256];
    GetWindowText(targetHwnd, title, sizeof(title));
    std::wstring message = topmost ? TEXT("已置顶窗口：") : TEXT("已取消置顶窗口：");
    message += title;
    SetWindowText(hEdit, message.c_str());
}