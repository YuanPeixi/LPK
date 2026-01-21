#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <Windows.h>
#include <algorithm>

using namespace std;

// --- 全局配置与缓冲区 ---
long checkInterval = 1000;
bool ambiguous = true;
bool killProcess = true;
vector<wstring> keywords;
WCHAR g_titleBuffer[1024]; // 静态全局缓冲区，无需反复申请/释放内存

string defaultConfig = "500\nTRUE\nTRUE\n抖音";

// --- 安全的编码转换函数 (基于 stack 自动管理内存，绝不泄漏) ---
wstring ToWString(const string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
    if (size <= 0) return L"";
    wstring wstr(size - 1, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}

// --- 通用的布尔值转换逻辑 ---
bool SafeToBool(string str) {
    // 转小写并提取数字或字母
    string tmp;
    for (char c : str) {
        if (isalnum((unsigned char)c)) tmp += (char)tolower((unsigned char)c);
    }
    if (tmp.empty()) return false;
    if (isdigit((unsigned char)tmp[0])) return stol(tmp) != 0;
    return tmp != "false";
}

// --- 修正后的配置读取 ---
bool LoadConfig(string configPath = "config.txt") {
    ifstream config(configPath);
    if (!config.is_open()) return false;

    keywords.clear();
    string line;
    int lineNum = 0;
    while (getline(config, line)) {
        // 彻底清理行末的 \r, \n, 空格
        size_t last = line.find_last_not_of(" \t\r\n");
        if (last == string::npos) continue;
        line = line.substr(0, last + 1);

        lineNum++;
        if (lineNum == 1)      checkInterval = stol(line);
        else if (lineNum == 2) ambiguous = SafeToBool(line);
        else if (lineNum == 3) killProcess = SafeToBool(line);
        else {
            keywords.push_back(ToWString(line));
        }
    }
    return !keywords.empty() || lineNum >= 3;
}

// --- 枚举窗口回调函数 ---
BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lParam) {
    // 每次清空缓冲区（其实 GetWindowTextW 会处理结尾 \0，清空是双保险）
    memset(g_titleBuffer, 0, sizeof(g_titleBuffer));

    if (GetWindowTextW(hwnd, g_titleBuffer, 1024) <= 0) return TRUE;

    wstring title(g_titleBuffer);
    bool matched = false;

    for (const wstring& kw : keywords) {
        if (ambiguous) {
            if (title.find(kw) != wstring::npos) { matched = true; break; }
        }
        else {
            if (title == kw) { matched = true; break; }
        }
    }

    if (matched) {
        if (killProcess) {
            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            // 保护：不要把自己给杀了
            if (pid != GetCurrentProcessId()) {
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProc) {
                    TerminateProcess(hProc, 0);
                    CloseHandle(hProc);
                }
            }
        }
        else {
            // 温和关闭窗口（发送点击关闭按钮的消息）
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
    }
    return TRUE; // 返回 TRUE 继续遍历后续窗口
}

void Loop() {
    while (true) {
        EnumWindows(EnumProc, 0);
        Sleep(checkInterval);
    }
}

// 控制台主程序
int main() {
    if (LoadConfig()) {
        Loop();
    }
    else {
        fstream file("config.txt",ios::out);
        file << defaultConfig << endl;
        file.close();
        return 1;
    }
    return 0;
}

// GUI 模式主程序 (隐藏黑色控制台)
int WINAPI WinMain(HINSTANCE h, HINSTANCE p, LPSTR c, int s) {
    if (LoadConfig()) {
        Loop();
    }
    else {
        fstream file("config.txt", ios::out);
        file << defaultConfig << endl;
        file.close();
        return 1;
    }
    return 0;
}