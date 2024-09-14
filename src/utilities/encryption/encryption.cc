#include <windows.h>
#include <commctrl.h>
#include <string>

#include "core.hpp"
#include <codecvt>
#include <locale>


#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


// 窗口控件ID
#define ID_GENERATE_BUTTON 1001
#define ID_PRIVATE_KEY_EDIT 1002
#define ID_PUBLIC_KEY_EDIT 1003
#define ID_INPUT_EDIT 1004
#define ID_ENCRYPT_BUTTON 1005
#define ID_DECRYPT_BUTTON 1006
#define ID_RESULT_EDIT 1007
#define ID_COMBOBOX 1008

// 全局变量
HWND g_hMainWnd=NULL,g_hGenKeyBtn= NULL,
     g_hPrivateKeyEdit=NULL, g_hPublicKeyEdit=NULL, 
     g_hInputEdit=NULL, g_hResultEdit=NULL,
     g_hEncryptBtn = NULL, g_hDecryptBtn=NULL,
     g_hModeCombo=NULL;

enum class OperateType{
    kMD5,
    kSHA256,
    kAES,
    kRSA
};

OperateType g_current_operate_type = OperateType::kMD5;

std::string wstr_to_utf8(const std::wstring& wstr) {
    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Length <= 0) {
        return "";
    }

    std::string utf8String(utf8Length - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8String[0], utf8Length - 1, nullptr, nullptr);
    return utf8String;
}


std::wstring utf8_to_wstr(const std::string& str) {
    int utf8Length = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (utf8Length <= 0) {
        return L"";
    }

    std::wstring utf8String(utf8Length - 1, '\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &utf8String[0], utf8Length);
    return utf8String;
}


// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void GenerateKeys(HWND hwnd);
void GenerateHashValue(OperateType type);
void EncryptData(HWND hwnd);
void DecryptData(HWND hwnd);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    helper::initialize_openssl();
    // 初始化通用控件
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);
    // 注册窗口类
    const wchar_t CLASS_NAME[] = TEXT("OpenSSL Encryption Window Class");

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // 创建窗口
    g_hMainWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        TEXT("Encryption Demo"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (g_hMainWnd == NULL) {
        return 0;
    }

    ShowWindow(g_hMainWnd, nCmdShow);

    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    helper::uninitialize_openssl();
    return 0;
}

void GetRelativeRect(HWND hwnd, LPRECT rc)
{
    GetWindowRect(hwnd,rc);
    MapWindowPoints(HWND_DESKTOP,g_hMainWnd,(LPPOINT)rc,2);
}

void SetSecretKeyVisible(OperateType type)
{
    ShowWindow(g_hPrivateKeyEdit,type == OperateType::kAES || type == OperateType::kRSA?SW_SHOW:SW_HIDE);
    ShowWindow(g_hPublicKeyEdit, type == OperateType::kRSA?SW_SHOW:SW_HIDE);
    ShowWindow(g_hGenKeyBtn,type != OperateType::kAES?SW_SHOW:SW_HIDE);
    ShowWindow(g_hEncryptBtn,type == OperateType::kAES || type == OperateType::kRSA?SW_SHOW:SW_HIDE);
    ShowWindow(g_hDecryptBtn,type == OperateType::kAES || type == OperateType::kRSA?SW_SHOW:SW_HIDE);
    SetWindowText(g_hGenKeyBtn, type == OperateType::kRSA?TEXT("生成公私钥"): TEXT("生成散列值"));

    SetWindowText(g_hPrivateKeyEdit, type == OperateType::kRSA?TEXT("私钥"): TEXT("请输入密钥"));
    SetWindowText(g_hPublicKeyEdit, TEXT("公钥"));
    SetWindowText(g_hInputEdit, TEXT("请输入加密内容"));
    SetFocus(g_hPrivateKeyEdit); 
    SendMessage(g_hPrivateKeyEdit, EM_SETSEL, 0, -1);
    SetFocus(g_hPublicKeyEdit); 
    SendMessage(g_hPublicKeyEdit, EM_SETSEL, 0, -1);
    SetFocus(g_hInputEdit); 
    SendMessage(g_hInputEdit, EM_SETSEL, 0, -1);

    RECT rcprivatekey, rcpublickey;
    GetRelativeRect(g_hPrivateKeyEdit,&rcprivatekey);
    GetRelativeRect(g_hPublicKeyEdit,&rcpublickey);
    rcprivatekey.right = type == OperateType::kRSA? rcpublickey.left - 10: rcpublickey.right;
    MoveWindow(g_hPrivateKeyEdit,rcprivatekey.left, rcprivatekey.top, rcprivatekey.right - rcprivatekey.left, rcprivatekey.bottom - rcprivatekey.top, true);

    RECT rcInput;
    GetRelativeRect(g_hInputEdit,&rcInput);
    auto height = rcInput.bottom - rcInput.top; 
    rcInput.top =  IsWindowVisible(g_hPrivateKeyEdit)? rcprivatekey.bottom + 10 : rcprivatekey.top;
    rcInput.bottom = height + rcInput.top;
    MoveWindow(g_hInputEdit,rcInput.left, rcInput.top, rcInput.right - rcInput.left, rcInput.bottom - rcInput.top, true);

    RECT rcMode, rcEncryptBtn, rcDecryptBtn,rcGenKeyBtn;
    GetRelativeRect(g_hGenKeyBtn,&rcGenKeyBtn);
    GetRelativeRect(g_hModeCombo,&rcMode);
    GetRelativeRect(g_hEncryptBtn,&rcEncryptBtn);
    GetRelativeRect(g_hDecryptBtn,&rcDecryptBtn);
    height = rcEncryptBtn.bottom - rcEncryptBtn.top;
    rcGenKeyBtn.top = rcMode.top = rcDecryptBtn.top = rcEncryptBtn.top = rcInput.bottom + 10;
    rcGenKeyBtn.bottom = rcMode.bottom = rcDecryptBtn.bottom = rcEncryptBtn.bottom = rcEncryptBtn.top + height;
    const auto width = rcGenKeyBtn.right - rcGenKeyBtn.left;
    rcGenKeyBtn.left = (IsWindowVisible(g_hDecryptBtn)?rcDecryptBtn.right : rcMode.right) + 10;
    rcGenKeyBtn.right = rcGenKeyBtn.left + width;
    MoveWindow(g_hEncryptBtn,rcEncryptBtn.left, rcEncryptBtn.top, rcEncryptBtn.right - rcEncryptBtn.left, rcEncryptBtn.bottom - rcEncryptBtn.top, true);
    MoveWindow(g_hDecryptBtn,rcDecryptBtn.left, rcDecryptBtn.top, rcDecryptBtn.right - rcDecryptBtn.left, rcDecryptBtn.bottom - rcDecryptBtn.top, true);
    MoveWindow(g_hModeCombo,rcMode.left, rcMode.top, rcMode.right - rcMode.left, rcMode.bottom - rcMode.top, true);
    MoveWindow(g_hGenKeyBtn,rcGenKeyBtn.left, rcGenKeyBtn.top, rcGenKeyBtn.right - rcGenKeyBtn.left, rcGenKeyBtn.bottom - rcGenKeyBtn.top, true);

    RECT rcResult;
    GetRelativeRect(g_hResultEdit,&rcResult);
    height =rcResult.bottom - rcResult.top; 
    rcResult.top =  rcDecryptBtn.bottom + 10;
    rcResult.bottom = height + rcResult.top;
    MoveWindow(g_hResultEdit,rcResult.left, rcResult.top, rcResult.right - rcResult.left, rcResult.bottom - rcResult.top, true);

    RECT rcMain;
    GetWindowRect(g_hMainWnd, &rcMain);
    GetWindowRect(g_hResultEdit, &rcResult);
    rcMain.bottom = rcResult.bottom + 20;
    MoveWindow(g_hMainWnd,rcMain.left, rcMain.top, rcMain.right - rcMain.left, rcMain.bottom - rcMain.top, true);


}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
    {
        g_hPrivateKeyEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_BORDER | ES_MULTILINE | WS_VSCROLL, 10, 10, 230, 100, hwnd, (HMENU)ID_PRIVATE_KEY_EDIT, NULL, NULL);
        g_hPublicKeyEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_BORDER | ES_MULTILINE | WS_VSCROLL, 250, 10, 230, 100, hwnd, (HMENU)ID_PUBLIC_KEY_EDIT, NULL, NULL);
        g_hInputEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER, 10, 120, 470, 30, hwnd, (HMENU)ID_INPUT_EDIT, NULL, NULL);

        // 创建控件
        g_hModeCombo = CreateWindow(
            WC_COMBOBOX, TEXT(""),
            CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
            10, 160, 100, 30,
            hwnd, (HMENU)ID_COMBOBOX, NULL, NULL);
        
        // 添加选项到 ComboBox
        auto itemid = SendMessage(g_hModeCombo, CB_ADDSTRING, 0, (LPARAM)TEXT("MD5"));
        SendMessage(g_hModeCombo, CB_SETITEMDATA, itemid, (LPARAM)OperateType::kMD5);
        itemid = SendMessage(g_hModeCombo, CB_ADDSTRING, 0, (LPARAM)TEXT("SHA256"));
        SendMessage(g_hModeCombo, CB_SETITEMDATA, itemid, (LPARAM)OperateType::kSHA256);
        itemid = SendMessage(g_hModeCombo, CB_ADDSTRING, 0, (LPARAM)TEXT("AES CBC"));
        SendMessage(g_hModeCombo, CB_SETITEMDATA, itemid, (LPARAM)OperateType::kAES);
        itemid = SendMessage(g_hModeCombo, CB_ADDSTRING, 0, (LPARAM)TEXT("RSA"));
        SendMessage(g_hModeCombo, CB_SETITEMDATA, itemid, (LPARAM)OperateType::kRSA);
        // 设置默认选中项
        SendMessage(g_hModeCombo, CB_SETCURSEL, 0, 0);
        
        g_hEncryptBtn = CreateWindow(TEXT("BUTTON"), TEXT("加密"),  WS_CHILD, 120, 200, 100, 30, hwnd, (HMENU)ID_ENCRYPT_BUTTON, NULL, NULL);
        g_hDecryptBtn = CreateWindow(TEXT("BUTTON"), TEXT("解密"),  WS_CHILD, 230, 200, 100, 30, hwnd, (HMENU)ID_DECRYPT_BUTTON, NULL, NULL);
        g_hGenKeyBtn = CreateWindow(TEXT("BUTTON"), TEXT("生成公私钥对"),  WS_VISIBLE | WS_CHILD, 340, 200, 100, 30, hwnd, (HMENU)ID_GENERATE_BUTTON, NULL, NULL);

        g_hResultEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | WS_VSCROLL, 10, 240, 470, 150, hwnd, (HMENU)ID_RESULT_EDIT, NULL, NULL);
        g_hMainWnd = hwnd;
        SetSecretKeyVisible(OperateType::kMD5);
    }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_GENERATE_BUTTON:
            g_current_operate_type==OperateType::kRSA?GenerateKeys(hwnd):GenerateHashValue(g_current_operate_type);
            break;
        case ID_ENCRYPT_BUTTON:
            EncryptData(hwnd);
            break;
        case ID_COMBOBOX:
            if (HIWORD(wParam) == CBN_SELCHANGE) 
            {
                int curitemid = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                wchar_t ListItem[256] = {0};
                SendMessage((HWND)lParam, CB_GETLBTEXT, curitemid, (LPARAM)ListItem);

                g_current_operate_type = OperateType(SendMessage((HWND)lParam, CB_GETITEMDATA, curitemid, 0));
                SetSecretKeyVisible(g_current_operate_type);

                wchar_t szMessage[300] = {0};
                // wsprintf(szMessage,  TEXT("You selected: %s"), ListItem);
                // MessageBox(hwnd, szMessage, TEXT("ComboBox Selection"), MB_OK | MB_ICONINFORMATION);
            }
            break;
        case ID_DECRYPT_BUTTON:
            DecryptData(hwnd);
            break;
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // 在这里绘制窗口内容

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT rect;
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW+1));
            return TRUE;
        }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void GenerateHashValue(OperateType type)
{
    std::string hashvalue;
    TCHAR input[1024] = {0};
    GetWindowText(g_hInputEdit, input, _countof(input));
    std::string inputvalue = wstr_to_utf8(input);
    switch (type)
    {
    case OperateType::kMD5:
        hashvalue = helper::md5(inputvalue);
        break;
    case OperateType::kSHA256:
        hashvalue = helper::sha256(inputvalue);
        break;
    default:
        break;
    }
    SetWindowText(g_hResultEdit, utf8_to_wstr(hashvalue).c_str());
}

void GenerateKeys(HWND hwnd) {
    std::string prikey, pubkey;
    if (!helper::generate_key_pairs(pubkey,prikey)) {
        MessageBox(hwnd, TEXT("生成RSA密钥对失败."), TEXT("Error"), MB_OK | MB_ICONERROR);
        return;
    }

    SetWindowText(g_hPrivateKeyEdit, utf8_to_wstr(prikey).c_str());
    SetWindowText(g_hPublicKeyEdit, utf8_to_wstr(pubkey).c_str());
}


void EncryptData(HWND hwnd) {
    TCHAR input[1024] = {0};
    TCHAR keys[4096] = {0};
    GetWindowText(g_hInputEdit, input, _countof(input));
    GetWindowText(g_current_operate_type == OperateType::kRSA?g_hPublicKeyEdit:g_hPrivateKeyEdit, keys, _countof(keys));
    std::string keystr = wstr_to_utf8( keys);
    std::string plaintext = wstr_to_utf8(input);
    try {
        ;
        std::string encrypted = g_current_operate_type == OperateType::kRSA?
            helper::RSAEncrypt(plaintext.c_str(),keystr.c_str()):
            helper::aes_encrypt(plaintext,keystr);
        SetWindowText(g_hResultEdit, utf8_to_wstr(encrypted).c_str());
    }
    catch (const std::exception& e) {
        MessageBox(hwnd, utf8_to_wstr(e.what()).c_str(), TEXT("Encryption Error"), MB_OK | MB_ICONERROR);
    }
}

void DecryptData(HWND hwnd) {
    TCHAR encrypted[4096] = {0};
    TCHAR key[4096] = {0};
    GetWindowText(g_hResultEdit, encrypted, _countof(encrypted));
    GetWindowText(g_hPrivateKeyEdit, key, _countof(key));

    std::string keystr = wstr_to_utf8( key);
    std::string ciphertext = wstr_to_utf8(encrypted);
    try {
        std::string decrypted = g_current_operate_type == OperateType::kRSA?
            helper::RSADecrypt(ciphertext, keystr):
            helper::aes_decrypt(ciphertext,keystr);
        SetWindowText(g_hResultEdit, utf8_to_wstr(decrypted).c_str());
    }
    catch (const std::exception& e) {
        MessageBox(hwnd, utf8_to_wstr(e.what()).c_str(), TEXT("Decryption Error"), MB_OK | MB_ICONERROR);
    }
}
