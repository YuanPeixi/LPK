#include <windows.h>
#include <iostream>

void CALLBACK WinEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime
) {
    if (event == EVENT_OBJECT_NAMECHANGE && idObject == OBJID_WINDOW) {
        char title[256];
        GetWindowTextA(hwnd, title, sizeof(title));
        std::cout << "窗口标题已更改: " << title << " HWND: " << hwnd << std::endl;
    }
}

int main() {
    // 设置 WinEventHook 监听名称更改
    HWINEVENTHOOK hook = SetWinEventHook(
        EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE,
        NULL, WinEventProc, 0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWinEvent(hook);
    return 0;
}