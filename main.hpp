#pragma once
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <fcntl.h>  // _O_U16TEXT 상수가 정의되어 있는 헤더
#include <io.h>
#include <string>
#include <vector>
#include <curl/curl.h>
#include <format>

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
    struct curl_slist *headers;

    ProcessInfo processInfo;

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

            // 2. Content-Type 헤더를 JSON으로 설정
            headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // 3. FastAPI ProcessInfo 모델에 맞는 JSON 데이터 구성
            // pid는 숫자형(int)이므로 값에 큰따옴표(")를 붙이지 않습니다.
            const char *json_data = "{\"name\":\"test_process\",\"pid\":1234}";
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);

            curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

            // 여기를 이제 우리가 정한 Processlist의 변수로 해야함.
            std::string json_str = std::format(R"({{"name":"{}","pid":{}}})", processInfo.exeName, processInfo.pid);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"name\":\"daniel\",\"pid\":1234}");

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
        curl_slist_free_all(headers);

        curl_easy_cleanup(curl);
        
        curl_global_cleanup();
    }
};