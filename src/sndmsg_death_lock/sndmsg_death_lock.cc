#include <Windows.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HWND hWnd;
    MSG msg;
    WNDCLASS wc;

    ZeroMemory(&wc, sizeof wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = TEXT("Deadlock");

    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, TEXT("Window registration failed!"), TEXT("Error"), MB_ICONERROR);
        return 0;
    }

    hWnd = CreateWindow(TEXT("Deadlock"), TEXT("Deadlock Program"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 320, 240, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        MessageBox(NULL, TEXT("Window creation failed!"), TEXT("Error"), MB_ICONERROR);
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)TEXT("Hello, World!"));
        SendMessage(hWnd, WM_PAINT, 0, 0);
        break;

    case WM_PAINT:
        PAINTSTRUCT ps;
        HDC hdc;

        hdc = BeginPaint(hWnd, &ps);

        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}