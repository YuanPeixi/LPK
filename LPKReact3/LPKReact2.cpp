#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <Windows.h>

using namespace std;

std::string ToString(LPCWSTR lpwstr)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, lpwstr, -1, nullptr, 0, nullptr, nullptr);
    if (len == 0)
    {
        return std::string();
    }
    std::string str(len - 1, '\0'); // Exclude null terminator
    WideCharToMultiByte(CP_UTF8, 0, lpwstr, -1, &str[0], len, nullptr, nullptr);
    return str;
}

// Remember to delete the returned LPWSTR after use !
LPCWSTR ToLPCWSTR(const std::string &str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (len == 0)
    {
        return nullptr;
    }
    LPWSTR wstr = new WCHAR[len];
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wstr, len);
    return wstr;
}

bool ToBool(std::string str)
{
    for(size_t i=0;i<str.size();i++)
    {
        str[i]=tolower(str[i]);
    }
    return str != "0" && str != "false";
}

enum OptMode
{
    TERMINATE,
    CLOSE_WND,
    SUSPEND,
    MINIMIZE_WND,
};

bool greedy = true;
vector<string> keywords;
OptMode optMode = TERMINATE;

const string defaultConfig = "TERMINATE(0)/CLOSE_WND(1)/SUSPEND(2)/MINIMIZE_WND(3)\n"
                             "greedy:true/false\n"
                             "keyword1\nkeyword2\nkeyword3\n";

bool LoadConfig(const string &path)
{
    ifstream configFile(path);
    if (!configFile.good())
    {
        configFile.close();
        return false;
    }
    string line;
    bool firstLine = true;
    while (getline(configFile, line))
    {
        if (line.empty())
            continue;
        if (firstLine)
        {
            if (line == "TERMINATE" || line == "TERMINATE(0)")
                optMode = TERMINATE;
            else if (line == "CLOSE_WND" || line == "CLOSE_WND(1)")
                optMode = CLOSE_WND;
            else if (line == "SUSPEND" || line == "SUSPEND(2)")
                optMode = SUSPEND;
            else if (line == "MINIMIZE_WND" || line == "MINIMIZE_WND(3)")
                optMode = MINIMIZE_WND;
            firstLine = false;
        }
        else if (line.rfind("greedy:", 0) == 0)
        {
            string boolStr = line.substr(7);
            greedy = ToBool(boolStr);
        }
        else
        {
            // 过滤处空格不可见字符
            line.erase(std::remove_if(line.begin(), line.end(), [](unsigned char c) {
                return ( (c < 0x20) || (c == 0x7F) ) && c != ' ';
            }), line.end());

            keywords.push_back(line);
        }
    }
    return true;
}

inline bool match(std::string str)
{
    if(keywords.size()==0)
        return false;
    for (auto keyword : keywords)
    {
        if (greedy)
        {
            if (str.find(keyword) != string::npos)
                return true;
        }
        else
        {
            if (str == keyword)
                return true;
        }
    }
    return false;
}

typedef LONG(NTAPI *NtSuspendProcess)(IN HANDLE ProcessHandle);
NtSuspendProcess pfnNtSuspendProcess = nullptr;

void CALLBACK WinEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime)
{
    if (((event == EVENT_OBJECT_NAMECHANGE) || (event == EVENT_OBJECT_CREATE)) && (idObject == OBJID_WINDOW))
    {
        WCHAR buffer[512];
        GetWindowTextW(hwnd, buffer, 512);
            std::string title = ToString(buffer);
        if (match(title))
        {
            switch (optMode)
            {
            case TERMINATE:
            {
                DWORD pid;
                GetWindowThreadProcessId(hwnd, &pid);
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProcess != NULL)
                {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
                break;
            }
            case CLOSE_WND:
            {
                PostMessageA(hwnd, WM_CLOSE, 0, 0);
                break;
            }
            case SUSPEND:
            {
                DWORD pid;
                GetWindowThreadProcessId(hwnd, &pid);
                HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
                if (hProcess != NULL)
                {

                    if (!pfnNtSuspendProcess)
                    {
                        HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
                        pfnNtSuspendProcess = (NtSuspendProcess)GetProcAddress(hNtDll, "NtSuspendProcess");
                    }
                    if (pfnNtSuspendProcess)
                    {
                        pfnNtSuspendProcess(hProcess);
                    }
                    CloseHandle(hProcess);
                }
                break;
            }
            case MINIMIZE_WND:
            {
                ShowWindow(hwnd, SW_MINIMIZE);
                break;
            }
            }
        }
    }
}

string configPath="";

int InternalMain()
{
    if(configPath=="")
    {
        configPath="lpk.txt";
    }
    if (!LoadConfig(configPath))
    {
        ofstream configFile(configPath);
        configFile << defaultConfig;
        configFile.close();
        return 1;
    }
    // 设置 WinEventHook 监听名称更改事件
    HWINEVENTHOOK hook = SetWinEventHook(
        EVENT_OBJECT_CREATE, EVENT_OBJECT_NAMECHANGE,
        NULL, WinEventProc, 0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWinEvent(hook);
    return 0;
}

int main(int argc, char **argv)
{
    if(argc>1)
    {
        configPath=argv[1];
    }

    if(InternalMain()!=0&&argc>1)
    {
        cerr<<"Failed to load config file at "<<configPath<<", creating default config file instead."<<endl;
        return 1;
    }

    return 0;
}

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    int argc = 0;
    // 修正：使用 GetCommandLineW 以获取标准 argv 数组
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argc > 1)
    {
        configPath = ToString(argv[1]);
    }
    LocalFree(argv);

    if (InternalMain() != 0 && !configPath.empty())
    {
        std::cerr << "Failed to load config file at " << configPath << ", creating default config file instead." << std::endl;
        return 1;
    }

    return 0;
}