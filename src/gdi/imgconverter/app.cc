#define _WIN32_IE 0x0600
#define WINVER 0x0500
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <filesystem>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

HWND hSlider, hQualityText, hListView;
std::wstring selectedPath;
int jpegQuality = 75;

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SelectFolder(HWND hWnd);
void ConvertImages(HWND hWnd);
void UpdateQualityText();
void InitListView(HWND hWnd);
void AddListViewItem(int index, const std::wstring& fileName, const std::wstring& status, 
                     const std::wstring& originalSize, const std::wstring& newSize);
void UpdateListViewItem(int index, const std::wstring& status, const std::wstring& newSize);
std::wstring FormatFileSize(uintmax_t size);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    InitCommonControls();

    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"PNGtoJPGConverter";
    RegisterClassEx(&wcex);

    HWND hWnd = CreateWindow(L"PNGtoJPGConverter", L"PNG to JPG Converter",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
        {
            HWND hButton = CreateWindow(L"BUTTON", L"选择目录",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                10, 10, 100, 30, hWnd, (HMENU)1, NULL, NULL);

            CreateWindowEx(0, L"STATIC", L"图片质量:",
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                120, 10, 80, 30, hWnd, (HMENU)3, NULL, NULL);

            hSlider = CreateWindowEx(0, TRACKBAR_CLASS, L"Quality",
                WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
                200, 10, 200, 30, hWnd, (HMENU)2, NULL, NULL);
            SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            SendMessage(hSlider, TBM_SETPOS, TRUE, 75);


            hQualityText = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"75",
                WS_CHILD | WS_VISIBLE | ES_READONLY,
                400, 10, 50, 30, hWnd, (HMENU)3, NULL, NULL);

            InitListView(hWnd);
        }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                SelectFolder(hWnd);
                if (!selectedPath.empty()) {
                    ConvertImages(hWnd);
                }
            }
            break;

        case WM_HSCROLL:
            if ((HWND)lParam == hSlider) {
                jpegQuality = SendMessage(hSlider, TBM_GETPOS, 0, 0);
                UpdateQualityText();
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

void SelectFolder(HWND hWnd) {
    BROWSEINFO bi = {0};
    bi.lpszTitle = L"选择目录";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != 0) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path)) {
            selectedPath = path;
        }
        IMalloc *imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }
    }
}

void ConvertImages(HWND hWnd) {
    std::vector<std::filesystem::path> pngFiles;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(selectedPath)) {
        if (entry.path().extension() == L".png") {
            pngFiles.push_back(entry.path());
        }
    }

    ListView_DeleteAllItems(hListView);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    for (size_t i = 0; i < pngFiles.size(); ++i) {
        std::wstring pngPath = pngFiles[i].wstring();
        std::wstring jpgPath = pngFiles[i].replace_extension(L".jpg").wstring();

        uintmax_t originalSize = std::filesystem::file_size(pngPath);
        std::wstring originalSizeStr = FormatFileSize(originalSize);

        AddListViewItem(i + 1, pngFiles[i].filename().wstring(), L"转换中...", originalSizeStr, L"-");

        Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(pngPath.c_str());
        if (bitmap) {
            CLSID jpgClsid;
            GetEncoderClsid(L"image/jpeg", &jpgClsid);

            Gdiplus::EncoderParameters encoderParameters;
            encoderParameters.Count = 1;
            encoderParameters.Parameter[0].Guid = Gdiplus::EncoderQuality;
            encoderParameters.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
            encoderParameters.Parameter[0].NumberOfValues = 1;
            encoderParameters.Parameter[0].Value = &jpegQuality;

            if (bitmap->Save(jpgPath.c_str(), &jpgClsid, &encoderParameters) == Gdiplus::Ok) {
                uintmax_t newSize = std::filesystem::file_size(jpgPath);
                std::wstring newSizeStr = FormatFileSize(newSize);
                UpdateListViewItem(i, L"已转换", newSizeStr);
            } else {
                UpdateListViewItem(i, L"转换失败", L"-");
            }
            delete bitmap;
        } else {
            UpdateListViewItem(i, L"加载失败", L"-");
        }
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    MessageBox(hWnd, L"转换完成!", L"Info", MB_OK | MB_ICONINFORMATION);
}

void UpdateQualityText() {
    wchar_t qualityStr[4];
    _itow_s(jpegQuality, qualityStr, 10);
    SetWindowText(hQualityText, qualityStr);
}

void InitListView(HWND hWnd) {
    hListView = CreateWindowEx(0, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        10, 50, 760, 480, hWnd, (HMENU)4, NULL, NULL);

    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;

    lvc.iSubItem = 0;
    lvc.cx = 30;
    lvc.pszText = L"#";
    ListView_InsertColumn(hListView, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.cx = 150;
    lvc.pszText = L"文件名";
    ListView_InsertColumn(hListView, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.cx = 80;
    lvc.pszText = L"状态";
    ListView_InsertColumn(hListView, 2, &lvc);

    lvc.iSubItem = 3;
    lvc.cx = 80;
    lvc.pszText = L"原大小";
    ListView_InsertColumn(hListView, 3, &lvc);

    lvc.iSubItem = 4;
    lvc.cx = 80;
    lvc.pszText = L"新大小";
    ListView_InsertColumn(hListView, 4, &lvc);
}

void AddListViewItem(int index, const std::wstring& fileName, const std::wstring& status, 
                     const std::wstring& originalSize, const std::wstring& newSize) {
    LVITEM lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = index - 1;

    // Add index
    wchar_t indexStr[16];
    _itow_s(index, indexStr, 10);
    lvi.iSubItem = 0;
    lvi.pszText = indexStr;
    ListView_InsertItem(hListView, &lvi);

    // Add file name
    lvi.iSubItem = 1;
    lvi.pszText = (LPWSTR)fileName.c_str();
    ListView_SetItem(hListView, &lvi);

    // Add status
    lvi.iSubItem = 2;
    lvi.pszText = (LPWSTR)status.c_str();
    ListView_SetItem(hListView, &lvi);

    // Add original size
    lvi.iSubItem = 3;
    lvi.pszText = (LPWSTR)originalSize.c_str();
    ListView_SetItem(hListView, &lvi);

    // Add new size
    lvi.iSubItem = 4;
    lvi.pszText = (LPWSTR)newSize.c_str();
    ListView_SetItem(hListView, &lvi);
}

void UpdateListViewItem(int index, const std::wstring& status, const std::wstring& newSize) {
    ListView_SetItemText(hListView, index, 2, (LPWSTR)status.c_str());
    ListView_SetItemText(hListView, index, 4, (LPWSTR)newSize.c_str());
}

std::wstring FormatFileSize(uintmax_t size) {
    const wchar_t* units[] = {L"B", L"KB", L"MB", L"GB", L"TB"};
    int unitIndex = 0;
    double adjustedSize = static_cast<double>(size);

    while (adjustedSize >= 1024 && unitIndex < 4) {
        adjustedSize /= 1024;
        unitIndex++;
    }

    std::wostringstream oss;
    oss << std::fixed << std::setprecision(2) << adjustedSize << L" " << units[unitIndex];
    return oss.str();
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;

    Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;

    pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL)
        return -1;

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }

    free(pImageCodecInfo);
    return -1;
}