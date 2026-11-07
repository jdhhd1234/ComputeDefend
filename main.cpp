#include "main.hpp"
#include <algorithm>
#include <cstddef>
#include <errhandlingapi.h>
#include <vector>

/*
# Debug: C++20, -O0 -g, -Wall -Wextra
cmake --preset mingw-x64-debug
cmake --build --preset mingw-x64-debug


cmake --preset mingw-x64-release -B /home/kwonosuing/win-build
cmake --build /home/kwonosuing/win-build --parallel 4

# Release
cmake --preset mingw-x64-release
cmake --build --preset mingw-x64-release
*/

std::vector<ProcessInfo> Process::GetProcessList()
{
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == NULL)
    {
        std::cerr << "CreateToolhelp32Snapshot Failed..." << GetLastError() << std::endl;
        return {};
    }

    pe32.dwSize = sizeof(PROCESSENTRY32W);

    std::vector<ProcessInfo> processList;

    if (!Process32FirstW(hProcessSnap, &pe32)) 
    {
        std::cerr << "Process32First 실패" << std::endl;
        CloseHandle(hProcessSnap);
        return {};
    }

    if (Process32FirstW(hProcessSnap, &pe32))
    {
        do 
        {   
            processList.push_back({
                pe32.szExeFile,
                pe32.th32ProcessID
            });

        } while (Process32NextW(hProcessSnap, &pe32));
    }

    return processList;
}

int main()
{
    /*
    Process process;
    std::vector<ProcessInfo> processlist = process.GetProcessList();
    
    if (!processlist.empty())
    {
        for (const auto& p : processlist)
        {
            std::wcout << L"ProcessName: " << p.exeName
                    << L" PID: " << p.pid
                    << L'\n';
        }
    } else 
    {
        std::cerr << "Empty!" << std::endl;
        return 1;
    }
    */

    NetWorkSend networkSend("");

    return 0;
}
