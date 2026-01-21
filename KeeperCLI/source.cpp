#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

using namespace std;

// 核心监控函数：支持 Unicode，解决内存泄漏，自动设置工作目录
void Keep(wstring targetPath, DWORD checkInterval)
{
    // 1. 提取工作目录（让子进程在它自己所在的文件夹运行）
    wstring workDir;
    size_t lastSlash = targetPath.find_last_of(L"\\/");
    if (lastSlash != wstring::npos) {
        workDir = targetPath.substr(0, lastSlash);
    }

    // 2. 构造带引号的命令行（防止路径中有空格导致启动失败）
    wstring cmdLine = L"\"" + targetPath + L"\"";

    // CreateProcessW 要求第二个参数必须是可写缓冲区，所以用 vector 转换
    vector<wchar_t> buffer(cmdLine.begin(), cmdLine.end());
    buffer.push_back(0);

    while (true) {
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };

        // 3. 启动进程
        if (CreateProcessW(
            NULL,               // 传 NULL 让系统解析 cmdLine
            buffer.data(),      // 命令行写在缓冲区里
            NULL, NULL, FALSE, 0, NULL,
            workDir.empty() ? NULL : workDir.c_str(), // <--- 设置子进程的工作目录
            &si, &pi
        )) {
            // 启动成功，关闭线程句柄（不需要），等待进程结束
            CloseHandle(pi.hThread);
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess); // 进程结束后关闭进程句柄
        }
        else {
            // 启动失败（如路径错误）发送一条调试日志
            OutputDebugStringW((L"KeeperCLI Error: " + to_wstring(GetLastError())).c_str());
        }

        // 进程崩溃或退出后，等待设定时间再重启
        Sleep(checkInterval);
    }
}

// 统一的参数处理逻辑
int RunInternal(int argc, wchar_t* argv[]) {
    if (argc < 3) {
        // 如果是从命令行运行，输出提示
        wcout << L"Usage: KeeperCLI <Interval_ms> <Program_Path>" << endl;
        // 如果是直接双击运行（没有黑框），弹出对话框提示
        if (!GetConsoleWindow()) {
            MessageBoxW(NULL, L"用法: KeeperCLI <频率ms> <程序完整路径>", L"参数错误", MB_OK | MB_ICONERROR);
        }
        return 1;
    }

    DWORD interval = wcstoul(argv[1], nullptr, 10);
    wstring path = argv[2];

    Keep(path, interval);
    return 0;
}

// 环境适配：控制台模式入口
int wmain(int argc, wchar_t* argv[]) {
    return RunInternal(argc, argv);
}

// 环境适配：GUI模式入口（用于在任务计划中静默后台运行，避免弹出黑框）
int WINAPI wWinMain(HINSTANCE h, HINSTANCE p, LPWSTR c, int s) {
    // 内部调用全局变量获取宽字符参数
    return RunInternal(__argc, __wargv);
}