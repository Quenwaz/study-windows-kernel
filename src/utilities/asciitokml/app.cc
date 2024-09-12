#include <windows.h>
#include <commctrl.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <locale>
#include <codecvt>

#pragma comment(lib, "comctl32.lib")

// 窗口过程函数声明
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// 控件ID
#define ID_BUTTON_IMPORT 101
#define ID_BUTTON_EXPORT 102
#define ID_EDIT_STATUS 103

// 全局变量
std::vector<std::vector<std::string>> csvData;


std::string wstr_to_utf8(const std::wstring& wstr) {
    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Length <= 0) {
        return "";
    }

    std::string utf8String(utf8Length - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8String[0], utf8Length - 1, nullptr, nullptr);
    return utf8String;
}

// CSV文件解析函数
bool ParseCSV(const std::wstring& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    csvData.clear();
    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        csvData.push_back(row);
    }
    file.close();
    return true;
}

// KML文件生成函数
bool GenerateKML(const std::wstring& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n";
    file << "<Document>\n";

    for (const auto& row : csvData) {
        if (row.size() >= 3) {
            file << "  <Placemark>\n";
            file << "    <name>" << row[0] << "</name>\n";
            file << "    <Point>\n";
            file << "      <coordinates>" << row[1] << "," << row[2] << ",0</coordinates>\n";
            file << "    </Point>\n";
            file << "  </Placemark>\n";
        }
    }

    file << "</Document>\n";
    file << "</kml>\n";
    file.close();
    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 注册窗口类
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName = TEXT("CSVtoKMLConverter");
    RegisterClassEx(&wcex);

    // 创建窗口
    HWND hWnd = CreateWindow(
        TEXT("CSVtoKMLConverter"), TEXT("CSV to KML Converter"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 200,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static HWND hButtonImport, hButtonExport, hEditStatus;

    switch (message) {
    case WM_CREATE:
        hButtonImport = CreateWindow(TEXT("BUTTON"), TEXT("Import CSV"), WS_CHILD | WS_VISIBLE,
            10, 10, 100, 30, hWnd, (HMENU)ID_BUTTON_IMPORT, NULL, NULL);
        hButtonExport = CreateWindow(TEXT("BUTTON"), TEXT("Export KML"), WS_CHILD | WS_VISIBLE,
            120, 10, 100, 30, hWnd, (HMENU)ID_BUTTON_EXPORT, NULL, NULL);
        hEditStatus = CreateWindow(TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | ES_READONLY | ES_MULTILINE,
            10, 50, 370, 100, hWnd, (HMENU)ID_EDIT_STATUS, NULL, NULL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BUTTON_IMPORT:
            {
                TCHAR filename[MAX_PATH] = {0};
                OPENFILENAME ofn = {0};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hWnd;
                ofn.lpstrFilter = TEXT("CSV Files\0*.csv\0All Files\0*.*\0");
                ofn.lpstrFile = filename;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                ofn.lpstrDefExt = TEXT("csv");

                if (GetOpenFileName(&ofn)) {
                    if (ParseCSV(filename)) {
                        SetWindowText(hEditStatus, TEXT("CSV file imported successfully."));
                    } else {
                        SetWindowText(hEditStatus, TEXT("Failed to import CSV file."));
                    }
                }
            }
            break;

        case ID_BUTTON_EXPORT:
            {
                if (csvData.empty()) {
                    SetWindowText(hEditStatus, TEXT("Please import a CSV file first."));
                    break;
                }

                TCHAR filename[MAX_PATH] = {0};
                OPENFILENAME ofn = {0};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hWnd;
                ofn.lpstrFilter = TEXT("KML Files\0*.kml\0All Files\0*.*\0");
                ofn.lpstrFile = filename;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
                ofn.lpstrDefExt = TEXT("kml");

                if (GetSaveFileName(&ofn)) {
                    if (GenerateKML(filename)) {
                        SetWindowText(hEditStatus, TEXT("KML file exported successfully."));
                    } else {
                        SetWindowText(hEditStatus, TEXT("Failed to export KML file."));
                    }
                }
            }
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}