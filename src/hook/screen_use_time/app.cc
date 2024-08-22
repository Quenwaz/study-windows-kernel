#include <windows.h>
#include <commctrl.h>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iostream>

#pragma comment(lib, "comctl32.lib")


#define DBGMSG(fmt,...) \
{\
char __message__[1024] = {0};\
snprintf(__message__,sizeof __message__, fmt,__VA_ARGS__);\
OutputDebugStringA(__message__);\
}

// 全局变量
HINSTANCE hInst;
HWND hListView;

// 函数声明
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void AddDataToListView(HWND hListView, std::map<std::string, long long>& data);
std::string FormatTime(long long milliseconds);

HHOOK hShellHook;
std::map<std::string, long long> usageData; // 记录每个窗口的使用时间（毫秒）
std::string lastWindowTitle = "";
auto lastTimePoint = std::chrono::steady_clock::now();

typedef bool (*SetHookFunc)(HINSTANCE, HWND);
typedef void (*RemoveHookFunc)();

std::string GetWindowTitle(HWND hwnd)
{
    char windowTitle[256]  = {0};
    if (hwnd != NULL)
    {
        GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle)); // 获取窗口标题
        return std::string(windowTitle);
    }
    return "";
}



// 程序入口
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{


    ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
    HMODULE hMod = LoadLibrary(TEXT("libscreenusetime.dll"));
    

    hInst = hInstance;
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("ScreenTimeMonitor");

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, TEXT("Screen Usage Time Monitor"),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, hInstance, NULL);


    SetHookFunc SetShellHook = (SetHookFunc)GetProcAddress(hMod, "SetShellHook");
    RemoveHookFunc RemoveShellHook = (RemoveHookFunc)GetProcAddress(hMod, "RemoveShellHook");

    
    if (SetShellHook)
    {
        if (!SetShellHook(hInst, hwnd)){
            MessageBox(hwnd, TEXT("hook failed"), TEXT("error"), MB_OK);
        }
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    RemoveShellHook();

    return (int)msg.wParam;
}



// 窗口过程
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        InitCommonControls(); // 初始化控件

        // 创建ListView控件
        hListView = CreateWindow(WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT,
            0, 0, 400, 300, hwnd, NULL, hInst, NULL);

        // 添加列
        LVCOLUMN lvCol = { 0 };
        lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvCol.pszText = TEXT("Window Title");
        lvCol.cx = 250;
        ListView_InsertColumn(hListView, 0, &lvCol);

        lvCol.pszText = TEXT("Usage Time (ms)");
        lvCol.cx = 120;
        ListView_InsertColumn(hListView, 1, &lvCol);

        // // 初始化一些假数据 (替换为实际数据)
        // usageData["Notepad"] = 123456;
        // usageData["Explorer"] = 7890;
        // usageData["Chrome"] = 654321;

        // // 添加数据到ListView
        // AddDataToListView(hListView, usageData);
        break;
    }
    case WM_SIZE:
        // 调整ListView大小
        MoveWindow(hListView, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;

    case WM_COPYDATA:
    {
        COPYDATASTRUCT* pCDS = (COPYDATASTRUCT*)lParam;
        if (pCDS->dwData == 1) // 检查标识
        {
            HWND activeWnd =  (HWND)wParam; //(HWND)std::atoi((const char*)pCDS->lpData); // 
            if (activeWnd != nullptr){
                std::string currentWindowTitle = GetWindowTitle(activeWnd);
                DBGMSG("activeWnd:%d, title:%s\n",activeWnd,currentWindowTitle.c_str());

                if (!currentWindowTitle.empty())
                {
                    if (currentWindowTitle != lastWindowTitle)
                    {
                        if (!lastWindowTitle.empty() && activeWnd != hwnd)
                        {
                            auto now = std::chrono::steady_clock::now();
                            long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTimePoint).count();
                            usageData[lastWindowTitle] += duration;
                            AddDataToListView(hListView, usageData);

                        }
                        lastWindowTitle = currentWindowTitle;
                        lastTimePoint = std::chrono::steady_clock::now();
                    }
                }
            }
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

// 添加数据到ListView
void AddDataToListView(HWND hListView, std::map<std::string, long long>& data)
{
    // 清空 ListView
    ListView_DeleteAllItems(hListView);

    // 排序数据
    std::vector<std::pair<std::string, long long>> sortedData(data.begin(), data.end());
    std::sort(sortedData.begin(), sortedData.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    // 插入数据
    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_TEXT;
    for (size_t i = 0; i < sortedData.size(); ++i)
    {
        lvItem.iItem = i;
        lvItem.pszText = const_cast<LPSTR>(sortedData[i].first.c_str());
        ListView_InsertItem(hListView, &lvItem);

        std::string timeStr = FormatTime(sortedData[i].second);
        ListView_SetItemText(hListView, i, 1, const_cast<LPSTR>(timeStr.c_str()));
    }
}

// 格式化时间为字符串
std::string FormatTime(long long milliseconds)
{
    std::ostringstream oss;
    oss << milliseconds;
    return oss.str();
}
