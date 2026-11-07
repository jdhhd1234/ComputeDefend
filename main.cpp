#include "main.hpp"

/*
# Debug: C++20, -O0 -g, -Wall -Wextra
cmake --preset mingw-x64-debug
cmake --build --preset mingw-x64-debug

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

ProcessList Process::GetProcessList()
{
    ProcessList processList;

    hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

    if( hProcessSnap == INVALID_HANDLE_VALUE )
    {
        std::cout << "CreateToolhelp32Snapshot (of processes)" << std::endl;
        return {};
    }

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof( pe32 );

    if( !Process32First( hProcessSnap, &pe32 ) )
    {
        std::cout << "Process32First" << std::endl;
        CloseHandle(hProcessSnap);
        return {};
    }

    do 
    {
        // std::cout << TEXT("PROCESS NAME: ") << pe32.szExeFile << TEXT("PID: ") << pe32.th32ProcessID << std::endl;
        processList.exeName = pe32.szExeFile;
        processList.pid = pe32.th32ProcessID;
    } while ( Process32Next( hProcessSnap, &pe32 ) );

    return processList;
}

int main()
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    
    Process process;

    ProcessList list = process.GetProcessList();

    std::cout << list.exeName << std::endl;
    return 0;
}
