#pragma once
#include "cpr/cprtypes.h"
#include <windows.h>
#include <chrono>
#include <string>
#include <tlhelp32.h>
#include <vector>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

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

class NetWorkSendServer
{
private:
    cpr::Response response;
    nlohmann::json json_data;
    nlohmann::json last_sent_data;
    std::chrono::steady_clock::time_point last_sent_time{};

    Process process;

public:
    void SendServerPost(cpr::Url url);

    static std::string GetEnvOr(const char* name, const std::string& fallback);
    static std::string ToUtf8(const std::wstring& wide);
};