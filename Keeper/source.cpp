#include <fstream>
#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>

using namespace std;

const string defaultContents = "1000\n\nnotepad.exe\n";
long checkInterval = 1000;
string path;

void Keep()
{
    // 提前计算工作目录
    string workDir;
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != string::npos) {
        workDir = path.substr(0, lastSlash);
    }

    while (true) {
        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };

        // 构造带引号的命令行缓冲区，防止路径空格，且不需要手动 delete
        // 引号包裹路径： "C:\Path With Spaces\a.exe"
        string cmdStr = "\"" + path + "\"";
        vector<char> cmdBuffer(cmdStr.begin(), cmdStr.end());
        cmdBuffer.push_back('\0');

        BOOL success = CreateProcessA(
            NULL,
            cmdBuffer.data(), // 使用 vector 自动管理内存
            NULL, NULL, FALSE, 0, NULL,
            workDir.empty() ? NULL : workDir.c_str(), // 自动设置被启动程序的当前目录
            &si, &pi
        );

        if (success) {
            CloseHandle(pi.hThread);
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
        }
        else {
            cerr << "启动失败，错误代码: " << GetLastError() << endl;
        }

        Sleep(checkInterval);
    }
}

bool LoadConfig()
{
    ifstream config("keep.txt"); // 使用 ifstream 更简洁
    if (!config.is_open()) {
        ofstream out("keep.txt");
        out << defaultContents;
        return false;
    }

    if (!(config >> checkInterval)) {
        checkInterval = 1000;
    }

    config.ignore(65535, '\n'); // 跳过数字后的那个换行符

    string line;
    while (getline(config, line)) {
        // 去除两侧空格或特殊字符
        if (!line.empty() && line.find_first_not_of(" \t\r\n") != string::npos) {
            path = line;
            break;
        }
    }
    return !path.empty();
}

int main()
{
    if (LoadConfig()) {
        Keep();
    }
    return 0;
}

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    if (LoadConfig()) {
        Keep();
    }
    return 0;
}