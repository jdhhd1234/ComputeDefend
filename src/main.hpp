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

// OpenProcess로 연 핸들을 소멸자에서 CloseHandle로 닫는 RAII 래퍼.
// 핸들을 두 번 닫지 않도록 복사하지 말고 지역 변수로만 쓴다.
class ProcessHandle
{
private:
    HANDLE handle;

public:
    ProcessHandle(DWORD pid, DWORD access);
    ~ProcessHandle();

    bool IsValid() const;
    bool Terminate(UINT exitCode) const;
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

    // 현재 목록에서 이름과 PID가 모두 일치하는 프로세스를 찾아 종료한다.
    bool KillProcess(const std::wstring& exeName, DWORD pid);
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

    // 서버에 쌓인 종료 명령을 가져와 실행한다.
    void ReceiveCommands(cpr::Url url);

    static std::string GetEnvOr(const char* name, const std::string& fallback);
    static std::string ToUtf8(const std::wstring& wide);
    static std::wstring FromUtf8(const std::string& utf8);
};