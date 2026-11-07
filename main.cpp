#include "main.hpp"
#include <algorithm>

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


/*
void MainCurl::CurlHandle(const char* inputURL)
{
    curl = curl_easy_init();
    if(curl) 
    {
        curl_easy_setopt(curl, CURLOPT_URL, inputURL);

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
        {
            std::cout << stderr << "curl_easy_perform() failed: " << curl_easy_strerror(res);
            return;
        }

        curl_easy_cleanup(curl);
    }
}
*/

ProcessInfo Process::GetProcessList()
{
    ProcessInfo processList;

    hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

    if( hProcessSnap == INVALID_HANDLE_VALUE )
    {
        std::cout << "CreateToolhelp32Snapshot (of processes)" << std::endl;
        return {};
    }

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof( pe32 );

    if( !Process32FirstW( hProcessSnap, &pe32 ) )
    {
        std::cout << "Process32First" << std::endl;
        CloseHandle(hProcessSnap);
        return {};
    }

    do
    {
        /*
        processList.exeName.push_back(pe32.szExeFile);
        processList[1] = pe32.th32ProcessID;
        */

        processList.exeName.push_back(pe32.szExeFile);
        processList.pid

    } while (Process32NextW(hProcessSnap, &pe32));

    return processList;
}

int main()
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    
    Process process;
    ProcessInfo list = process.GetProcessList();

    std::wcout << list.exeName << std::endl;

    return 0;
}
