#include <Windows.h>
#include <iostream>

// 全局钩子函数
LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        // 只处理按键按下事件
        if (wParam == WM_KEYDOWN) {
            KBDLLHOOKSTRUCT* pKbdStruct = (KBDLLHOOKSTRUCT*) lParam;
            // 输出按键码和扫描码
            printf("Key Down: vkCode=%d, scanCode=%d\n", pKbdStruct->vkCode, pKbdStruct->scanCode);
        }
    }
    // 继续传递事件给下一个Hook
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
int main() {
    // 安装键盘Hook
    HHOOK hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, NULL, 0);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 卸载Hook
    UnhookWindowsHookEx(hKeyboardHook);

    return 0;
}