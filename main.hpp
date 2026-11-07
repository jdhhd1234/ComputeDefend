#pragma once
#include <iostream>
#include <curl/curl.h>
#include <windows.h>
#include <tlhelp32.h>
#include <fcntl.h>  // _O_U16TEXT 상수가 정의되어 있는 헤더
#include <io.h>
#include <string>
#include <vector>

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

    std::vector<ProcessInfo> processList;

public:
    ProcessInfo GetProcessList();
};