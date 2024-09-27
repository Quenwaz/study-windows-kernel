#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <string>

#include <chrono>
#include <ctime>
#include <Strsafe.h>


#include "core.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// 全局变量
HWND g_hMainWnd, g_hListView;
bool g_expand = false;
LONG_PTR OldEditProc, oldBtnProc;
HMENU hPopMenu;

UINT WM_TRAYICON = RegisterWindowMessage(TEXT("TaskbarCreated"));

#define WM_USER_SHELLICON (WM_USER + 1)

// 定义菜单项ID
const auto global_tick_frequency =  60;
#define ID_TIMER 1001

enum{
    IDM_TRAY_EXIT=1002,
    IDI_TRAYICON,
    IDC_LISTVIEW_TODO,
    IDC_EDIT_TITLE,
    IDC_BOTTON_ADD,
    IDC_BOTTON_EXPAND,
    IDC_EDIT_REMARK,
    IDC_DATETIME_DEADLINE,
    IDC_DATETIME_REMIND,
    IDC_REMIND_CHECKBOX,
    IDC_BOTTON_DEL
};


// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
void FreshListView();
void AddTodoItem();
void FillToMore(LPARAM index);
void ClearMore();

void TimeToNotify();
void ShowNotification(const wchar_t* title, const wchar_t* content);
void CreateTrayMenu();

void DeselectAllItems(HWND hListView) {
    // 获取 ListView 中的项数
    int itemCount = ListView_GetItemCount(hListView);

    // 遍历所有项，取消选择
    for (int i = 0; i < itemCount; i++) {
        ListView_SetItemState(hListView, i, 0, LVIS_SELECTED);
    }
}

// WinMain 函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 初始化 Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES | ICC_DATE_CLASSES;
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
    g_hMainWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        TEXT("Todo"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 280,  //500
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (g_hMainWnd == NULL) {
        return 0;
    }


    // 创建列表视图（带复选框）
    g_hListView = CreateWindowEx(0, WC_LISTVIEW, TEXT(""), 
        WS_CHILD | WS_VISIBLE | LVS_REPORT  | LVS_NOCOLUMNHEADER , // LVS_EDITLABELS | LVS_OWNERDRAWFIXED 
        10, 0, 280, 200, g_hMainWnd, (HMENU)IDC_LISTVIEW_TODO, hInstance, NULL);
    
    // 设置列表视图扩展样式以包含复选框
    ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_CHECKBOXES | LVS_EX_BORDERSELECT | LVS_EX_GRIDLINES | LVS_EX_INFOTIP); // | LVS_EX_FULLROWSELECT
    
    // 添加列
    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.pszText = const_cast<LPTSTR>(TEXT("Todo Items"));
    lvc.cx = 238;
    ListView_InsertColumn(g_hListView, 0, &lvc);

    lvc.mask = LVCF_IMAGE | LVCF_WIDTH| LVCF_SUBITEM;
    lvc.cx = 25;
    lvc.pszText = const_cast<LPTSTR>(TEXT("Operation"));
    ListView_InsertColumn(g_hListView, 1, &lvc);

    // 创建输入框
    const auto hEdit = CreateWindowEx(0, TEXT("EDIT"), TEXT(""), 
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        10, 210, 200, 25, g_hMainWnd, (HMENU)IDC_EDIT_TITLE, hInstance, NULL);

    OldEditProc = SetWindowLongPtr (hEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);
    // 创建添加按钮
    CreateWindow(TEXT("BUTTON"), TEXT("+"), 
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        220, 210, 25, 25, g_hMainWnd, (HMENU)IDC_BOTTON_ADD, hInstance, NULL);

    // 创建提醒按钮
    CreateWindow(TEXT("BUTTON"), TEXT("v"), 
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        250, 210, 25, 25, g_hMainWnd, (HMENU)IDC_BOTTON_EXPAND, hInstance, NULL);

    CreateWindowEx(
        WS_EX_TRANSPARENT,
        TEXT("STATIC"), TEXT("备注:"), 
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        10, 245, 50, 25, g_hMainWnd,  NULL, hInstance, NULL);

    CreateWindowEx(
                0, L"EDIT",   // predefined class 
                NULL,         // no window title 
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | 
                ES_LEFT | ES_MULTILINE |WS_BORDER| ES_AUTOVSCROLL, 
                10, 270, 260, 75,   // set size in WM_SIZE message 
                g_hMainWnd,         // parent window 
                (HMENU)IDC_EDIT_REMARK,   // edit control ID 
                hInstance, 
                NULL);        // pointer not needed 

    CreateWindowEx(
        WS_EX_TRANSPARENT,
        TEXT("STATIC"), TEXT("截止:"), 
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        10, 360, 40, 25, g_hMainWnd, NULL, hInstance, NULL);

    auto hwndDP = CreateWindowEx(0,
                            DATETIMEPICK_CLASS,
                            TEXT("DateTime"),
                            WS_BORDER|WS_CHILD|WS_VISIBLE |  DTS_SHOWNONE ,
                            50,360,170,25,
                            g_hMainWnd,
                            (HMENU)IDC_DATETIME_DEADLINE,
                            hInstance,
                            NULL);

    DateTime_SetFormat(hwndDP,TEXT("yyyy/MM/dd HH:mm"));
    DateTime_SetSystemtime(hwndDP, GDT_NONE, 0);


    CreateWindowEx(
        WS_EX_TRANSPARENT,
        TEXT("STATIC"), TEXT("提醒:"), 
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        10, 395, 40, 25, g_hMainWnd, NULL, hInstance, NULL);

    hwndDP = CreateWindowEx(0,
                            DATETIMEPICK_CLASS,
                            TEXT("DateTime"),
                            WS_BORDER|WS_CHILD|WS_VISIBLE|  DTS_SHOWNONE,
                            50,395,170,25,
                            g_hMainWnd,
                            (HMENU)IDC_DATETIME_REMIND,
                            hInstance,
                            NULL);

    DateTime_SetFormat(hwndDP,TEXT("yyyy/MM/dd HH:mm"));
    DateTime_SetSystemtime(hwndDP, GDT_NONE, 0);

    const auto hckbox = CreateWindow(
        TEXT("BUTTON"), TEXT("每日"), 
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_OWNERDRAW ,
        225, 395, 50, 25, g_hMainWnd, (HMENU)IDC_REMIND_CHECKBOX, hInstance, NULL);

    SetWindowLongPtr(hckbox, GWLP_USERDATA, (LONG_PTR)BST_CHECKED);

    const auto hdel = CreateWindow(
        TEXT("BUTTON"), TEXT("删除"), 
        WS_CHILD  | BS_PUSHBUTTON | BS_OWNERDRAW,
        10, 430, 260, 25, g_hMainWnd, (HMENU)IDC_BOTTON_DEL, hInstance, NULL);

    SetWindowLongPtr(hdel, GWLP_USERDATA, (LONG_PTR)RGB(255, 0, 0));

    ShowWindow(g_hMainWnd, nCmdShow);
    SetTimer(g_hMainWnd, ID_TIMER, 1000 * global_tick_frequency, NULL);
    FreshListView();
    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LPARAM GetListViewParam(HWND hlsv, int row)
{
    if(row == -1){
        return 0;
    }
    LV_ITEM lvitem = {0};
    lvitem.iItem = row;
    lvitem.mask = LVIF_PARAM;
    ListView_GetItem(hlsv, &lvitem);
    return lvitem.lParam;
}

void GetRelativeRect(HWND hwnd, LPRECT rc)
{
    GetWindowRect(hwnd,rc);
    MapWindowPoints(HWND_DESKTOP,g_hMainWnd,(LPPOINT)rc,2);
}


void show_more(bool expand = true)
{
    RECT rcInput;
    GetWindowRect(GetDlgItem(g_hMainWnd, IDC_EDIT_TITLE),&rcInput);

    const HWND btndel = GetDlgItem(g_hMainWnd, IDC_BOTTON_DEL);
    RECT rcDelete;
    GetWindowRect(btndel,&rcDelete);

    SetDlgItemText(g_hMainWnd, IDC_BOTTON_EXPAND, expand?TEXT("x"):TEXT("v"));

    RECT rcMain;
    GetWindowRect(g_hMainWnd, &rcMain);
    rcMain.bottom = (expand? (IsWindowVisible(btndel)?rcDelete.bottom:rcDelete.top - 10): rcInput.bottom) + 20;
    MoveWindow(g_hMainWnd,rcMain.left, rcMain.top, rcMain.right - rcMain.left, rcMain.bottom - rcMain.top, true);
}

void show_add_button(bool show)
{
    RECT rcInput = {0};
    const HWND edit = GetDlgItem(g_hMainWnd, IDC_EDIT_TITLE);
    GetRelativeRect(edit,&rcInput);

    const HWND btnadd = GetDlgItem(g_hMainWnd, IDC_BOTTON_ADD);
    const HWND btndel = GetDlgItem(g_hMainWnd, IDC_BOTTON_DEL);

    RECT rcBtn = {0};
    GetRelativeRect(btnadd,&rcBtn);

    ShowWindow(btnadd, show? SW_SHOW:SW_HIDE);
    ShowWindow(btndel, show? SW_HIDE:SW_SHOW);
    rcInput.right = show? rcBtn.left - 10 : rcBtn.right;
    MoveWindow(edit,rcInput.left, rcInput.top, rcInput.right - rcInput.left, rcInput.bottom - rcInput.top, true);

    RECT rcMain;
    GetWindowRect(g_hMainWnd, &rcMain);

    RECT rcDelete;
    GetWindowRect(btndel,&rcDelete);

    if (rcMain.bottom > (rcDelete.top - 20)){
        rcMain.bottom = (show? rcDelete.top-10: rcDelete.bottom) + 20;
        MoveWindow(g_hMainWnd,rcMain.left, rcMain.top, rcMain.right - rcMain.left, rcMain.bottom - rcMain.top, true);
    }
}


LRESULT CALLBACK EditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_KEYDOWN: {
		if (wParam == VK_RETURN && IsWindowVisible(GetDlgItem(g_hMainWnd, IDC_BOTTON_ADD))) {//以回车消息为例
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
            nid.uID = IDI_TRAYICON;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = WM_USER_SHELLICON;
            nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            StringCchCopy(nid.szTip,_countof(nid.szTip),TEXT("Todo List"));
            Shell_NotifyIcon(NIM_ADD, &nid);
        }

        CreateTrayMenu();
        // 注册热键 (Ctrl + Shift + T)
        RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_SHIFT, 'T');
        break;
    case WM_COMMAND:
        {
            const auto ctrl_hwnd = (HWND)lParam;
            const auto ctrl_id =  LOWORD(wParam);
            const auto notify_type = HIWORD(wParam);
            switch (ctrl_id)
            {
            case IDC_BOTTON_DEL:
                {
                    const auto sel = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
                    if (sel != -1){
                        core::TodoMgr::instance()->remove(GetListViewParam(g_hListView, sel));
                        ListView_DeleteItem(g_hListView, sel);
                    }
                }
                break;
            case IDC_BOTTON_ADD:
                AddTodoItem();
                break;
            case IDC_BOTTON_EXPAND:
                g_expand = !g_expand;
                show_more(g_expand);
                if (!g_expand){
                    DeselectAllItems(g_hListView);
                    ClearMore();
                }
                break;
            case IDC_REMIND_CHECKBOX:
            {
                const auto hckbox = GetDlgItem(g_hMainWnd,IDC_REMIND_CHECKBOX);
                const auto checkstatus = GetWindowLongPtr(hckbox,GWLP_USERDATA) == BST_CHECKED;
                SetWindowLongPtr(hckbox, GWLP_USERDATA, (LONG_PTR)checkstatus?BST_UNCHECKED:BST_CHECKED);
            }
                break;  
            case IDC_EDIT_TITLE:
            {
                if (notify_type == EN_CHANGE)
                {
                    const auto sel = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
                    if (sel != -1){
                        wchar_t buffer[256]={0};
                        GetDlgItemText(g_hMainWnd, IDC_EDIT_TITLE, buffer, _countof(buffer));
                        core::TodoMgr::instance()->update_title(GetListViewParam(g_hListView, sel), buffer);
                        LV_ITEM lvitem = {0};
                        lvitem.mask = LVIF_TEXT;
                        lvitem.iItem = sel;
                        lvitem.iSubItem = 0;
                        lvitem.pszText = (LPTSTR)buffer;
                        ListView_SetItem(g_hListView, &lvitem);
                    }
                }
            }
                break;
            case IDC_EDIT_REMARK:
            {
                if (notify_type == EN_CHANGE)
                {
                    const auto sel = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
                    if (sel != -1){
                        wchar_t buffer[1024]={0};
                        GetDlgItemText(g_hMainWnd, IDC_EDIT_REMARK, buffer, _countof(buffer));
                        core::TodoMgr::instance()->update_remark(GetListViewParam(g_hListView, sel), buffer);
                    }
                }
            }
                break;
            case IDM_TRAY_EXIT:
                DestroyWindow(g_hMainWnd); 
                break;
            default:
                break;
            }

        }
        break;
    case WM_NOTIFY:
    {
        LPNMHDR lpnmhdr = (LPNMHDR)lParam;
        switch (lpnmhdr->idFrom)
        {
        case IDC_LISTVIEW_TODO:
            switch (lpnmhdr->code) 
            {
                case NM_CLICK:
                {
                    // 用户点击了 ListView 项
                    LPNMITEMACTIVATE lpnmia = (LPNMITEMACTIVATE)lParam;

                    LVHITTESTINFO plvhti = {0};
                    plvhti.pt = lpnmia->ptAction;
                    ListView_SubItemHitTest(g_hListView,&plvhti);

                    // 获取点击的行和列
                    int iRow = plvhti.iItem;// lpnmia->iItem;         // 获取行索引
                    int iColumn = plvhti.iSubItem;//  lpnmia->iSubItem;   // 获取列索引

                    if (iRow != -1 && iColumn != -1) {
                        FillToMore(GetListViewParam(lpnmhdr->hwndFrom, iRow));
                    }else{
                        ClearMore();
                    }
                    break;
                }
                case LVN_ITEMCHANGED:
                    {
                        LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
                        if (pnmv->uChanged & LVIF_STATE) {
                            if ((pnmv->uNewState & LVIS_STATEIMAGEMASK) != (pnmv->uOldState & LVIS_STATEIMAGEMASK)) {
                                DeselectAllItems(g_hListView);
                                ClearMore();
                                // 复选框状态改变
                                const auto stat = ((pnmv->uNewState & LVIS_STATEIMAGEMASK) >> 12) ;
                                core::TodoMgr::instance()->update_status(pnmv->lParam, ((pnmv->uNewState & LVIS_STATEIMAGEMASK) >> 12) == 2);
                            }
                        }
                    }
                    break;
                // case LVN_ENDLABELEDIT:
                // {
                //     NMLVDISPINFO* pdi = (NMLVDISPINFO*)lParam;
                //     if (pdi->item.pszText != NULL) {
                //         
                //     }
                // }
                //     break;
                case NM_CUSTOMDRAW:
                {
                    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)lParam;
                    const auto ischecked = ListView_GetCheckState(lpnmhdr->hwndFrom, lvcd->nmcd.dwItemSpec);
                    // Start custom draw
                    switch (lvcd->nmcd.dwDrawStage)
                    {
                    case CDDS_PREPAINT:
                        return CDRF_NOTIFYITEMDRAW; // Ask to receive item-level notifications
                    case CDDS_ITEMPREPAINT:
                        if (ischecked){
                            lvcd->clrText = RGB(150, 150, 150);
                        }
                        return CDRF_NOTIFYSUBITEMDRAW; // 请求接收子项级别的通知
                    case (CDDS_SUBITEM | CDDS_ITEMPREPAINT):

                      if (lvcd->iSubItem == 1) { // pLVCD->nmcd.dwItemSpec == 1 For the second item (index 1)
                            lvcd->clrText = RGB(255, 0, 0); // Set text color to red
                            if(ischecked) lvcd->clrTextBk = RGB(0, 200, 0);
                        }
                        return CDRF_NEWFONT;
                    default:
                        break;
                    }
                    return CDRF_DODEFAULT;  // 默认处理
                }
            }
            break;
        default:
            break;
        }

        switch (lpnmhdr->code)
        {
        case DTN_DATETIMECHANGE:
        {
            const auto sel = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
            if (sel != -1){
                const auto idx = GetListViewParam(g_hListView, sel);
                const auto lpChange = (LPNMDATETIMECHANGE) lParam;
                const auto timestamp= core::SystemTimeToTimestamp(&lpChange->st);
                if(lpnmhdr->idFrom == IDC_DATETIME_DEADLINE)
                    core::TodoMgr::instance()->update_deadline(idx, timestamp);
                else{
                    core::TodoMgr::instance()->update_remaind(idx, timestamp);
                }
            }
        }
            break;
        default:
            break;
        }
        break;
    }
    case WM_DRAWITEM:
    {
           LPDRAWITEMSTRUCT lpDrawItemStruct = (LPDRAWITEMSTRUCT)lParam;
            if (lpDrawItemStruct->CtlType == ODT_BUTTON) { // ID of the checkbox
                // 获取设备上下文
                HDC hdc = lpDrawItemStruct->hDC;

                // 设置背景颜色
                SetBkColor(hdc, RGB(255, 255, 255));
                SetBkMode(hdc, TRANSPARENT);
                HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
                FillRect(hdc, &lpDrawItemStruct->rcItem, hBrush);
                DeleteObject(hBrush);

                // 绘制文本
                SetTextColor(hdc, RGB(50, 50, 50));
                // 绘制文本
                TCHAR szText[256] = {0};
                GetWindowText(lpDrawItemStruct->hwndItem, szText, 256);

                RECT rctext = lpDrawItemStruct->rcItem;

                const auto userdata = GetWindowLongPtr(lpDrawItemStruct->hwndItem, GWLP_USERDATA);
                if (userdata != BST_CHECKED && userdata != BST_UNCHECKED){
                    // DrawFrameControl(hdc, &lpDrawItemStruct->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH  | ((lpDrawItemStruct->itemState & ODS_SELECTED) ? DFCS_PUSHED | DFCS_HOT : DFCS_FLAT));
                    const auto colorbrush = (lpDrawItemStruct->itemState & ODS_SELECTED)?CreateSolidBrush(userdata):GetSysColorBrush(COLOR_BTNFACE);
                    FillRect(hdc,  &lpDrawItemStruct->rcItem,colorbrush);
                    DeleteObject(colorbrush);

                }else{
                    const auto ckbox_offset = 5;
                    const auto ckbox_size = lpDrawItemStruct->rcItem.bottom - ckbox_offset;
                    rctext.left += ckbox_size;
                    // 绘制 Checkbox 方框
                    RECT box = { lpDrawItemStruct->rcItem.left + ckbox_offset, lpDrawItemStruct->rcItem.top + ckbox_offset, lpDrawItemStruct->rcItem.left + ckbox_size, lpDrawItemStruct->rcItem.top + ckbox_size };
                    
                    DrawFrameControl(hdc, &box, DFC_BUTTON, DFCS_BUTTONCHECK  | (BST_CHECKED == userdata)? DFCS_CHECKED |DFCS_FLAT:  DFCS_FLAT);//((lpDrawItemStruct->itemState & ODS_SELECTED) ? DFCS_CHECKED : 0));
                }
                DrawText(hdc, szText, -1, &rctext, DT_CENTER |DT_SINGLELINE | DT_VCENTER);


                // 设置焦点矩形
                if (lpDrawItemStruct->itemState & ODS_FOCUS) {
                    DrawFocusRect(hdc, &lpDrawItemStruct->rcItem);
                }

                return TRUE;
            }
    }
        break;

    case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;
            HWND hControl = (HWND)lParam;

            // 设置文本颜色为黑色
            SetTextColor(hdc, RGB(50, 50, 50));
            // 设置背景为透明
            SetBkMode(hdc, TRANSPARENT);
            // 返回空刷，表示不绘制背景
            return (LRESULT)GetStockObject(NULL_BRUSH);
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
        break;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        break;

    case WM_DESTROY:
        // 移除托盘图标
        {
            NOTIFYICONDATA nid = {0};
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwnd;
            nid.uID = IDI_TRAYICON;
            Shell_NotifyIcon(NIM_DELETE, &nid);
        }
        KillTimer(hwnd, ID_TIMER);
        PostQuitMessage(0);
        break;

    case WM_TIMER:
        if (wParam == ID_TIMER) {
            TimeToNotify();
            
        }
        break;

    case WM_USER_SHELLICON:
        if (lParam == WM_LBUTTONDOWN) {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
        } else if (lParam == WM_RBUTTONDOWN) {
            POINT pt;
            GetCursorPos(&pt);
            // SetForegroundWindow(hWnd);
            TrackPopupMenu(hPopMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
                            pt.x, pt.y, 0, g_hMainWnd, NULL);
            PostMessage(g_hMainWnd, WM_NULL, 0, 0);
        }
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    }
    return 0;
}

// 创建托盘菜单
void CreateTrayMenu() {
    hPopMenu = CreatePopupMenu();
    AppendMenu(hPopMenu, MF_STRING, IDM_TRAY_EXIT, TEXT("Exit"));
}

void ClearMore()
{
    SetDlgItemText(g_hMainWnd, IDC_EDIT_TITLE, TEXT(""));
    SetDlgItemText(g_hMainWnd, IDC_EDIT_REMARK, TEXT(""));
    DateTime_SetSystemtime(GetDlgItem(g_hMainWnd, IDC_DATETIME_DEADLINE), GDT_NONE, 0);
    DateTime_SetSystemtime(GetDlgItem(g_hMainWnd, IDC_DATETIME_REMIND), GDT_NONE, 0);
    show_add_button(true);
}

void FillToMore(LPARAM index)
{
    core::TodoItem current_todo = core::TodoMgr::instance()->at(index);
    SetDlgItemText(g_hMainWnd, IDC_EDIT_TITLE, current_todo.text.c_str());
    SetDlgItemText(g_hMainWnd, IDC_EDIT_REMARK, current_todo.remark.c_str());
    DateTime_SetSystemtime(GetDlgItem(g_hMainWnd, IDC_DATETIME_DEADLINE), 
        current_todo.deadline== 0? GDT_NONE:GDT_VALID, &core::TimestampToSystemTime(current_todo.deadline));
    DateTime_SetSystemtime(GetDlgItem(g_hMainWnd, IDC_DATETIME_REMIND), 
        current_todo.remind== 0? GDT_NONE:GDT_VALID, &core::TimestampToSystemTime(current_todo.remind));
    
    show_add_button(false);
}

int  CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    return lParam1 == lParam2?0 : lParam1 < lParam2? -1:1;
}

void FreshListView()
{
    ListView_DeleteAllItems(g_hListView);
    for(auto todo : *core::TodoMgr::instance())
    {
        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE; 
        lvi.stateMask =  LVIS_STATEIMAGEMASK ;
        lvi.state = INDEXTOSTATEIMAGEMASK(todo.status?2:1);
        lvi.pszText = const_cast<LPTSTR>(todo.text.c_str());
        lvi.lParam = todo.id;
        int index = ListView_InsertItem(g_hListView, &lvi);
        ListView_SetItemText(g_hListView, index, 1, TEXT(""));
        ListView_SetCheckState(g_hListView, index, todo.status);
    }

    // ListView_SortItems(g_hListView, CompareFunc, 0);
}

void TimeToNotify()
{
    for(auto& todo : *core::TodoMgr::instance())
    {
        if (todo.remind == 0 || todo.status){
            continue;
        }
        const auto duration = std::chrono::system_clock::now() - std::chrono::system_clock::from_time_t(todo.remind);
        const auto elapsed= std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        if (std::chrono::duration_cast<std::chrono::seconds>(duration).count() <= global_tick_frequency){
            ShowNotification(TEXT("Todo Reminder"), todo.text.c_str());
        }
    }
}

void AddTodoItem() {
    wchar_t buffer[1024] ={0};
    GetDlgItemText(g_hMainWnd, IDC_EDIT_TITLE, buffer, _countof(buffer));
    size_t chsize = 0;
    StringCchLength(buffer,_countof(buffer) ,&chsize);
    if (chsize > 0) {
        core::TodoItem item;
        item.text = buffer;
        ZeroMemory(buffer, sizeof(buffer));
        GetDlgItemText(g_hMainWnd, IDC_EDIT_REMARK, buffer, _countof(buffer));
        item.remark = buffer;
        item.status = false;

        SYSTEMTIME deadline = {0};
        if(GDT_VALID == DateTime_GetSystemtime(GetDlgItem(g_hMainWnd, IDC_DATETIME_DEADLINE),&deadline)){
            item.deadline = core::SystemTimeToTimestamp(&deadline);
        }

        SYSTEMTIME remind = {0};
        if(GDT_VALID == DateTime_GetSystemtime(GetDlgItem(g_hMainWnd, IDC_DATETIME_REMIND),&remind)){
            item.remind = core::SystemTimeToTimestamp(&remind);
        }

        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.pszText = item.text.data();
        lvi.lParam = core::TodoMgr::instance()->add(item);
        int index = ListView_InsertItem(g_hListView, &lvi);
        ListView_SetCheckState(g_hListView, index, FALSE);
        ListView_SetItemText(g_hListView, index, 1, TEXT(""));

        ClearMore();
    }
}


void ShowNotification(const wchar_t* title, const wchar_t* content) {
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = g_hMainWnd;
    nid.uID = IDI_TRAYICON;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    
    StringCchCopy(nid.szInfoTitle,_countof(nid.szInfoTitle), title);
    StringCchCopy(nid.szInfo,_countof(nid.szInfo), content);
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}