#include <windows.h>
#include <commctrl.h>
#include <string>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include <vector>
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

// 全局变量
HWND hPrivateKeyEdit=NULL, hPublicKeyEdit=NULL, hInputEdit=NULL, hResultEdit=NULL;
 


std::string RSAEncrypt(const std::string& data, const std::string& public_key);
std::string RSADecrypt(const std::string& data, const std::string& private_key);

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
void EncryptData(HWND hwnd);
void DecryptData(HWND hwnd);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 初始化OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    // 注册窗口类
    const wchar_t CLASS_NAME[] = TEXT("OpenSSL Encryption Window Class");

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // 创建窗口
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        TEXT("Encryption Demo"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理OpenSSL
    EVP_cleanup();
    ERR_free_strings();

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        // 创建控件
        CreateWindow(TEXT("BUTTON"), TEXT("生成公私钥对"), WS_VISIBLE | WS_CHILD, 10, 10, 100, 30, hwnd, (HMENU)ID_GENERATE_BUTTON, NULL, NULL);
        hPrivateKeyEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | WS_VSCROLL, 10, 50, 380, 200, hwnd, (HMENU)ID_PRIVATE_KEY_EDIT, NULL, NULL);
        hPublicKeyEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | WS_VSCROLL, 400, 50, 380, 200, hwnd, (HMENU)ID_PUBLIC_KEY_EDIT, NULL, NULL);
        hInputEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER, 10, 260, 770, 30, hwnd, (HMENU)ID_INPUT_EDIT, NULL, NULL);
        CreateWindow(TEXT("BUTTON"), TEXT("加密"), WS_VISIBLE | WS_CHILD, 10, 300, 100, 30, hwnd, (HMENU)ID_ENCRYPT_BUTTON, NULL, NULL);
        CreateWindow(TEXT("BUTTON"), TEXT("解密"), WS_VISIBLE | WS_CHILD, 120, 300, 100, 30, hwnd, (HMENU)ID_DECRYPT_BUTTON, NULL, NULL);
        hResultEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | WS_VSCROLL, 10, 340, 770, 200, hwnd, (HMENU)ID_RESULT_EDIT, NULL, NULL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_GENERATE_BUTTON:
            GenerateKeys(hwnd);
            break;
        case ID_ENCRYPT_BUTTON:
            EncryptData(hwnd);
            break;
        case ID_DECRYPT_BUTTON:
            DecryptData(hwnd);
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void GenerateKeys(HWND hwnd) {
    

    RSA* rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);
    if (!rsa) {
        MessageBox(hwnd, TEXT("生成RSA密钥对失败."), TEXT("Error"), MB_OK | MB_ICONERROR);
        return;
    }

    BIO* pri = BIO_new(BIO_s_mem());
    BIO* pub = BIO_new(BIO_s_mem());

    PEM_write_bio_RSAPrivateKey(pri, rsa, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_RSAPublicKey(pub, rsa);

    int pri_len = BIO_pending(pri);
    int pub_len = BIO_pending(pub);

    char* pri_key = new char[pri_len + 1];
    char* pub_key = new char[pub_len + 1];

    BIO_read(pri, pri_key, pri_len);
    BIO_read(pub, pub_key, pub_len);

    pri_key[pri_len] = '\0';
    pub_key[pub_len] = '\0';

    SetWindowText(hPrivateKeyEdit, utf8_to_wstr(pri_key).c_str());
    SetWindowText(hPublicKeyEdit, utf8_to_wstr(pub_key).c_str());

    BIO_free(pri);
    BIO_free(pub);
    RSA_free(rsa);
    delete[] pri_key;
    delete[] pub_key;
}


void EncryptData(HWND hwnd) {
    TCHAR input[1024] = {0};
    TCHAR public_key[4096] = {0};
    GetWindowText(hInputEdit, input, _countof(input));
    GetWindowText(hPublicKeyEdit, public_key, _countof(public_key));

    try {
        std::string encrypted = RSAEncrypt(wstr_to_utf8(input).c_str(),wstr_to_utf8( public_key).c_str());
        SetWindowText(hResultEdit, utf8_to_wstr(encrypted).c_str());
    }
    catch (const std::exception& e) {
        MessageBox(hwnd, utf8_to_wstr(e.what()).c_str(), TEXT("Encryption Error"), MB_OK | MB_ICONERROR);
    }
}

void DecryptData(HWND hwnd) {
    TCHAR encrypted[4096] = {0};
    TCHAR private_key[4096] = {0};
    GetWindowText(hResultEdit, encrypted, _countof(encrypted));
    GetWindowText(hPrivateKeyEdit, private_key, _countof(private_key));

    try {
        
        std::string decrypted = RSADecrypt(wstr_to_utf8(encrypted).c_str(), wstr_to_utf8(private_key));
        SetWindowText(hResultEdit, utf8_to_wstr(decrypted).c_str());
    }
    catch (const std::exception& e) {
        MessageBox(hwnd, utf8_to_wstr(e.what()).c_str(), TEXT("Decryption Error"), MB_OK | MB_ICONERROR);
    }
}

std::string RSAEncrypt(const std::string& data, const std::string& public_key) {
    BIO* keybio = BIO_new_mem_buf(public_key.c_str(), -1);
    RSA* rsa = PEM_read_bio_RSAPublicKey(keybio, NULL, NULL, NULL);
    if (!rsa) {
        BIO_free(keybio);
        throw std::runtime_error("加载公钥失败");
    }

    int rsa_size = RSA_size(rsa);
    std::vector<unsigned char> encrypted(rsa_size);
    int result = RSA_public_encrypt(data.length(), (unsigned char*)data.c_str(), encrypted.data(), rsa, RSA_PKCS1_PADDING);

    RSA_free(rsa);
    BIO_free(keybio);

    if (result == -1) {
        throw std::runtime_error("Encryption failed");
    }

    // Convert to hex string
    std::string hex_result;
    for (int i = 0; i < result; i++) {
        char hex[3];
        sprintf(hex, "%02x", encrypted[i]);
        hex_result += hex;
    }

    return hex_result;
}

std::string RSADecrypt(const std::string& data, const std::string& private_key) {
    BIO* keybio = BIO_new_mem_buf(private_key.c_str(), -1);
    RSA* rsa = PEM_read_bio_RSAPrivateKey(keybio, NULL, NULL, NULL);
    if (!rsa) {
        BIO_free(keybio);
        throw std::runtime_error("加载私钥失败");
    }

    // Convert hex string to binary
    std::vector<unsigned char> encrypted;
    for (size_t i = 0; i < data.length(); i += 2) {
        std::string byte = data.substr(i, 2);
        char chr = (char)strtol(byte.c_str(), NULL, 16);
        encrypted.push_back(chr);
    }

    int rsa_size = RSA_size(rsa);
    std::vector<unsigned char> decrypted(rsa_size);
    int result = RSA_private_decrypt(encrypted.size(), encrypted.data(), decrypted.data(), rsa, RSA_PKCS1_PADDING);

    RSA_free(rsa);
    BIO_free(keybio);

    if (result == -1) {
        throw std::runtime_error("Decryption failed");
    }

    return std::string(reinterpret_cast<char*>(decrypted.data()), result);
}
