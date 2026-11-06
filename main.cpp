#include "main.hpp"

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
    try 
    {
        ProcessList processList;

        hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

        // Set the size of the structure before using it.
        pe32.dwSize = sizeof( pe32 );

        do 
        {
            // std::cout << TEXT("PROCESS NAME: ") << pe32.szExeFile << TEXT("PID: ") << pe32.th32ProcessID << std::endl;
            processList.exeName = pe32.szExeFile;
            processList.pid = pe32.th32ProcessID;
        } while ( Process32NextW( hProcessSnap, &pe32 ) );

        return processList;
    } 
    catch (const std::exception& e) 
    {
        if( hProcessSnap == INVALID_HANDLE_VALUE )
        {
            std::cout << "CreateToolhelp32Snapshot (of processes)" << std::endl;
            return {};
        }

        if( !Process32FirstW( hProcessSnap, &pe32 ) )
        {
            std::cout << "Process32First" << std::endl;
            CloseHandle(hProcessSnap);
            return {};
        }
    }
}

int main()
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    
    Process process;

    ProcessList list = process.GetProcessList();

    std::wcout << list.exeName << std::endl;
    return 0;
}
