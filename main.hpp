#pragma once
#include <iostream>
#include <curl/curl.h>
#include <Windows.h>

class MainCurl
{
private:
    CURL* curl;
    CURLcode res;

public:
    MainCurl()
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~MainCurl()
    {
        curl_global_cleanup();
    }

    void CurlHandle(const char inputURL);
};