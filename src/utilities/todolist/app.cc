#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <fstream>
#include <codecvt>
#include <locale>
#include <Strsafe.h>
#include "common_header/json.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using json = nlohmann::json;

// 结构体定义
struct TodoItem {
    std::wstring text;
    bool completed;
};

// 全局变量
HWND hWnd, hList, hEdit, hAddButton, hRemindButton;
LONG_PTR OldEditProc;
HMENU hPopMenu;
std::vector<TodoItem> todos;
UINT WM_TRAYICON = RegisterWindowMessage(TEXT("TaskbarCreated"));
#define ID_TRAYICON 1001
#define ID_TIMER 1002
#define WM_USER_SHELLICON (WM_USER + 1)

// 定义菜单项ID
#define ID_TRAY_EXIT 1001

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void AddTodoItem();
void EditTodoItem(int index, const std::wstring& newTodo);
void ToggleTodoItem(int index);
void SetReminder();
void ShowNotification(const wchar_t* title, const wchar_t* content);
void SaveTodos();
void LoadTodos();
void CreateTrayMenu();

// 将ANSI字符串转换为UTF-8
std::string AnsiToUtf8(const std::string& ansiString) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wideString;
    int len = MultiByteToWideChar(CP_ACP, 0, ansiString.c_str(), -1, NULL, 0);
    wideString.resize(len - 1); // 减去NULL终止符
    MultiByteToWideChar(CP_ACP, 0, ansiString.c_str(), -1, &wideString[0], len);
    return converter.to_bytes(wideString);
}

std::string Utf8ToAnsi(const std::string& utf8String) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wideString = converter.from_bytes(utf8String);
    int len = WideCharToMultiByte(CP_ACP, 0, wideString.c_str(), -1, NULL, 0, NULL, NULL);
    std::string ansiString(len - 1, '\0'); // 减去NULL终止符
    WideCharToMultiByte(CP_ACP, 0, wideString.c_str(), -1, &ansiString[0], len, NULL, NULL);
    return ansiString;
}

std::string wstrToUTF8(const std::wstring& wstr) {
    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Length <= 0) {
        return "";
    }

    std::string utf8String(utf8Length - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8String[0], utf8Length - 1, nullptr, nullptr);
    return utf8String;
}

std::wstring utf8Towstr(const std::string& str) {
    int utf8Length = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (utf8Length <= 0) {
        return L"";
    }

    std::wstring utf8String(utf8Length - 1, '\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &utf8String[0], utf8Length);
    return utf8String;
}

// WinMain 函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 初始化 Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // 注册窗口类
    const wchar_t CLASS_NAME[] = TEXT("Todo List Window Class");
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClass(&wc);

    // 创建窗口
    hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        TEXT("Todo"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 280,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hWnd == NULL) {
        return 0;
    }

    // 创建列表视图（带复选框）
    hList = CreateWindowEx(0, WC_LISTVIEW, TEXT(""), 
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS | LVS_NOCOLUMNHEADER,
        10, 0, 280, 200, hWnd, NULL, hInstance, NULL);
    
    // 设置列表视图扩展样式以包含复选框
    ListView_SetExtendedListViewStyle(hList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    

    // 添加列
    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.cx = 250;
    lvc.pszText = const_cast<LPTSTR>(TEXT("Todo Items"));
    ListView_InsertColumn(hList, 0, &lvc);

    // 创建输入框
    hEdit = CreateWindowEx(0, TEXT("EDIT"), TEXT(""), 
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        10, 210, 200, 25, hWnd, 0, hInstance, NULL);

    OldEditProc = SetWindowLongPtr (hEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);


    // 创建添加按钮
    hAddButton = CreateWindow(TEXT("BUTTON"), TEXT("+"), 
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        220, 210, 25, 25, hWnd, NULL, hInstance, NULL);

    // 创建提醒按钮
    hRemindButton = CreateWindow(TEXT("BUTTON"), TEXT("!!!"), 
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        250, 210, 25, 25, hWnd, NULL, hInstance, NULL);

    // 加载保存的 todo 项目
    LoadTodos();

    ShowWindow(hWnd, nCmdShow);

    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}


LRESULT CALLBACK EditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_KEYDOWN: {
		if (wParam == VK_RETURN) {//以回车消息为例
			AddTodoItem();
		}
		return 0;
	}
	}
    //这里一定要注意，我们只处理我们想要处理的消息，我们不需要处理的消息放给原来的回调函数处理
	return CallWindowProc((WNDPROC)OldEditProc, hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        // 初始化托盘图标
        {
            NOTIFYICONDATA nid = {0};
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwnd;
            nid.uID = ID_TRAYICON;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = WM_USER_SHELLICON;
            nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            StringCchCopy(nid.szTip,_countof(nid.szTip),TEXT("Todo List"));
            Shell_NotifyIcon(NIM_ADD, &nid);
        }

        CreateTrayMenu();
        // 注册热键 (Ctrl + Shift + T)
        RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_SHIFT, 'T');
        return 0;

    case WM_COMMAND:
        if ((HWND)lParam == hAddButton) {
            AddTodoItem();
        }
        else if ((HWND)lParam == hRemindButton) {
            SetReminder();
        }

        // 处理菜单命令
        if (HIWORD(wParam) == 0 && LOWORD(wParam) == ID_TRAY_EXIT) {
            DestroyWindow(hWnd);    
        }
        return 0;


    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->hwndFrom == hList) {
            switch (((LPNMHDR)lParam)->code) {
            case LVN_ITEMCHANGED:
                {
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
                    if (pnmv->uChanged & LVIF_STATE) {
                        if ((pnmv->uNewState & LVIS_STATEIMAGEMASK) != (pnmv->uOldState & LVIS_STATEIMAGEMASK)) {
                            // 复选框状态改变
                            ToggleTodoItem(pnmv->iItem);
                        }
                    }
                }
            case LVN_ENDLABELEDIT:
            {
                NMLVDISPINFO* pdi = (NMLVDISPINFO*)lParam;
                if (pdi->item.pszText != NULL) {
                    EditTodoItem(pdi->item.iItem, pdi->item.pszText);
                }
            }
                return 0;
            }
        }
        break;

    case WM_HOTKEY:
        if (wParam == 1) {
            // 置顶/取消置顶
            HWND hForeground = GetForegroundWindow();
            if (hForeground == hwnd) {
                SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            } else {
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                SetForegroundWindow(hwnd);
            }
        }
        return 0;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        // 保存 todo 项目
        SaveTodos();
        // 移除托盘图标
        {
            NOTIFYICONDATA nid = {0};
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwnd;
            nid.uID = ID_TRAYICON;
            Shell_NotifyIcon(NIM_DELETE, &nid);
        }
        PostQuitMessage(0);
        return 0;

    case WM_TIMER:
        if (wParam == ID_TIMER) {
            ShowNotification(TEXT("Todo Reminder"), TEXT("It's time to do your task!"));
            KillTimer(hwnd, ID_TIMER);
        }
        return 0;

    case WM_USER_SHELLICON:
        if (lParam == WM_LBUTTONDOWN) {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
        } else if (lParam == WM_RBUTTONDOWN) {
            POINT pt;
            GetCursorPos(&pt);
            // SetForegroundWindow(hWnd);
            TrackPopupMenu(hPopMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
                            pt.x, pt.y, 0, hWnd, NULL);
            PostMessage(hWnd, WM_NULL, 0, 0);
        }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 创建托盘菜单
void CreateTrayMenu() {
    hPopMenu = CreatePopupMenu();
    AppendMenu(hPopMenu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));
}

// 编辑待办事项
void EditTodoItem(int index, const std::wstring& newTodo) {
    todos[index].text = newTodo;
    LVITEM lvi;
    lvi.mask = LVIF_TEXT;
    lvi.iItem = index;
    lvi.iSubItem = 0;
    lvi.pszText = (LPTSTR)newTodo.c_str();
    ListView_SetItem(hList, &lvi);
    SaveTodos();
}

void AddTodoItem() {
    wchar_t buffer[256];
    GetWindowText(hEdit, buffer, 256);
    size_t chsize = 0;
    StringCchLength(buffer,_countof(buffer) ,&chsize);
    if (chsize > 0) {
        TodoItem item;
        item.text = buffer;
        item.completed = false;
        todos.push_back(item);

        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.pszText = buffer;
        int index = ListView_InsertItem(hList, &lvi);
        ListView_SetCheckState(hList, index, FALSE);
        SetWindowText(hEdit, TEXT(""));

        SaveTodos();
    }
}

void ToggleTodoItem(int index) {
    if (index >= 0 && index < todos.size()) {
        BOOL checked = ListView_GetCheckState(hList, index);
        todos[index].completed = checked;
        SaveTodos();
    }
}

void SetReminder() {
    // 简单起见，我们设置一个5秒后的提醒
    SetTimer(hWnd, ID_TIMER, 5000, NULL);
}

void ShowNotification(const wchar_t* title, const wchar_t* content) {
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAYICON;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    
    StringCchCopy(nid.szInfoTitle,_countof(nid.szInfoTitle), title);
    StringCchCopy(nid.szInfo,_countof(nid.szInfo), content);
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void SaveTodos() {
    json j;
    for (const auto& item : todos) {
        j.push_back({{"text",wstrToUTF8(item.text)}, {"status", item.completed}});
    }
    std::ofstream o("todos.json");
    o << j << std::endl;
}

void LoadTodos() {
    std::ifstream i("todos.json");
    if (i.good()) {
        json j;
        i >> j;
        todos.clear();
        ListView_DeleteAllItems(hList);
        for (const auto& item : j) {
            TodoItem todo;
            todo.text = utf8Towstr(item["text"]);
            todo.completed = item["status"];
            todos.push_back(todo);

            LVITEM lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.pszText = const_cast<LPTSTR>(todo.text.c_str());
            int index = ListView_InsertItem(hList, &lvi);
            ListView_SetCheckState(hList, index, todo.completed);
        }
    }
}