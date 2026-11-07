#pragma once
#include <iostream>
#include <stdexcept>
#include <windows.h>
#include <tlhelp32.h>
#include <fcntl.h>  // _O_U16TEXT 상수가 정의되어 있는 헤더
#include <io.h>
#include <string>
#include <vector>
#include <curl/curl.h>

struct ProcessInfo
{
    std::wstring exeName;
    DWORD pid;
};

class Process
{
private:
    HANDLE hProcessSnap;
    HANDLE hProcess;

    PROCESSENTRY32W pe32;

    DWORD dwPriorityClass;

public:
    std::vector<ProcessInfo> GetProcessList();
};

class NetWorkSend
{
private:
    CURL *curl;

public:
    NetWorkSend(const char* input_url)
    {
        CURLcode result = curl_global_init(CURL_GLOBAL_ALL);
        if(result != CURLE_OK)
        {
            throw std::invalid_argument("curl_global_init Fail...");
        }

        curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, input_url);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

            result = curl_easy_perform(curl);

            if(result == CURLE_OK) 
            {
                const char *ct;
                /* ask for the content-type */
                result = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
            
                if((result == CURLE_OK) && ct)
                {
                    std::cout << "We received Content-Type: " << ct << std::endl;
                }
            }
        }
    }

    ~NetWorkSend()
    {
        curl_easy_cleanup(curl);
        
        curl_global_cleanup();
    }
};