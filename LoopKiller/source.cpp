#include <vector>
#include <fstream>
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>

using namespace std;

long checkInterval = 1000;
bool ambiguous = true;//Ambiguous Match
bool byAPI = true;//Kiil by API instead of CLI
vector<string> keywords;

//any not 0 or FALSE will treat as TRUE

string defaultConfig = "1000\nTRUE\nTRUE\nnotepad.exe\n";

bool toBool(const string& str)
{
    string tmp;
    bool onlyNum = true;
    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] >= 'A' && str[i] <= 'Z')
            tmp += char(str[i] - 'A' + 'a'), onlyNum = false;
        else if (str[i] >= 'a' && str[i] <= 'z')
            tmp += str[i], onlyNum = false;
        else if (str[i] >= '0' && str[i] <= '9')
            tmp += str[i];
    }
    if (!tmp.empty() && onlyNum) {
        return stol(tmp) != 0;
    }
    else {
        return tmp != "false";
    }
}

bool LoadConfig(string configPath = "")
{
    if (configPath.empty())
        configPath = "config.txt";
    ifstream config(configPath);
    if (!config.is_open()) return false;

    string line;
    int lineNum = 0;
    while (getline(config, line))
    {
        if (line.empty() || line.find_first_not_of(" \t\r\n") == string::npos)
            continue;

        lineNum++;
        if (lineNum == 1)      checkInterval = stol(line);
        else if (lineNum == 2) ambiguous = toBool(line);
        else if (lineNum == 3) byAPI = toBool(line);
        else                   keywords.push_back(line);
    }
    return true;
}
std::wstring toWString(const std::string& str) {
    if (str.empty()) return L"";

    // 1. 获取所需的缓冲区长度
    int size_needed = MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);

    // 2. 分配空间并转换
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string toString(const std::wstring& wstr) {
    if (wstr.empty()) return "";

    // 1. 获取所需的缓冲区长度
    int size_needed = WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);

    // 2. 分配空间并转换
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::string toString(const int& num) {

    return std::to_string(num);

    if (num == 0) return "0";

    int n = num;
    bool isNeg = n < 0;
    std::string res = "";
    if (isNeg) n = -n;
    while (n != 0) {
        res = char('0' + n % 10) + res;
        n /= 10;
    }
    if (isNeg) res = '-' + res;
    return res;
}

std::string toString(const DWORD& num) {

    return std::to_string((unsigned long)num);

    if (num == 0) return "0";

    DWORD n = num;
    std::string res = "";
    while (n != 0) {
        res = char('0' + n % 10) + res;
        n /= 10;
    }
    return res;
}

void Loop()
{
    //Enum Process List
    while (true)
    {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(hSnap, &pe32))
        {
            do
            {
                string procName = toString(pe32.szExeFile);
                for (auto& c : procName) c = (char)tolower((unsigned char)c);
                bool match = false;
                for (const string& keyword : keywords)
                {
                    if (ambiguous)
                    {
                        if (procName.find(keyword) != string::npos)
                        {
                            match = true;
                            break;
                        }
                    }
                    else
                    {
                        if (procName == keyword)
                        {
                            match = true;
                            break;
                        }
                    }
                }
                if (match)
                {
                    if (byAPI)
                    {
                        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                        if (hProc)
                        {
                            TerminateProcess(hProc, 0);
                            CloseHandle(hProc);
                        }
                    }
                    else
                    {
                        //Kill by CLI
                        //system(cmd.c_str());
                        //Make Sure no Window Pop out
                        string args = "/F /PID " + toString((int)pe32.th32ProcessID);
                        ShellExecuteA(NULL, "open", "taskkill.exe", args.c_str(), NULL, SW_HIDE);
                    }
                }
            } while (Process32NextW(hSnap, &pe32));
        }
        CloseHandle(hSnap);
        Sleep(checkInterval);
    }
}

int main()
{
    if (!LoadConfig())
    {
        //Create Default Config
        ofstream config("config.txt");
        config << defaultConfig;
        config.close();
        return 1;
    }

    Loop();

    return 0;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    if (!LoadConfig())
    {
        //Create Default Config
        ofstream config("config.txt");
        config << defaultConfig;
        config.close();
        return 1;
    }

    Loop();

    return 0;
}