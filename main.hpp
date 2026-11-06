#pragma once
#include <iostream>
#include <curl/curl.h>
#include <windows.h>
#include <tlhelp32.h>
#include <fcntl.h>  // _O_U16TEXT 상수가 정의되어 있는 헤더
#include <io.h>
#include <string>

struct ProcessList
{
    int pid;
    std::wstring exeName;
};

class Process
{
private:
    HANDLE hProcessSnap;
    HANDLE hProcess;

    PROCESSENTRY32W pe32;

    DWORD dwPriorityClass;

public:
    ProcessList GetProcessList();
};