#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <thread>
#include <chrono>

#include "main.hpp"
#include "cpr/api.h"

#include <dwmapi.h>
#include <shellapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

bool Privilege::IsRunningAsAdmin()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;

    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    const bool ok = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size) != FALSE;

    CloseHandle(token);
    return ok && elevation.TokenIsElevated != 0;
}

bool Privilege::RelaunchAsAdmin()
{
    wchar_t exePath[MAX_PATH]{};
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0)
        return false;

    wchar_t currentDir[MAX_PATH]{};
    GetCurrentDirectoryW(MAX_PATH, currentDir);

    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.lpVerb = L"runas";          // UAC 상승 요청
    info.lpFile = exePath;
    info.lpDirectory = currentDir;
    info.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&info))
    {
        // 사용자가 UAC 창에서 '아니요'를 누르면 ERROR_CANCELLED(1223)
        std::cerr << "Relaunch As Admin Failed: " << GetLastError() << '\n';
        return false;
    }

    return true;
}

bool Privilege::EnableDebugPrivilege()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        return false;

    TOKEN_PRIVILEGES privileges{};
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bool ok = LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &privileges.Privileges[0].Luid) != FALSE
        && AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), nullptr, nullptr) != FALSE
        // AdjustTokenPrivileges는 일부만 적용돼도 성공을 반환하므로 따로 확인한다.
        && GetLastError() != ERROR_NOT_ALL_ASSIGNED;

    CloseHandle(token);
    return ok;
}

void Process::RequestAdminPrivileges() 
{
    wchar_t szPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath))) 
    {
        // "runas" 동사를 사용하여 UAC 권한 상승 창 호출
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = szPath;
        sei.hwnd = NULL;
        sei.nShow = SW_NORMAL;

        if (ShellExecuteExW(&sei)) 
        {
            // 권한 상승된 새 프로세스가 실행되었으므로 현재 프로세스 종료
            ExitProcess(0);
        }
    }
}
std::vector<ProcessInfo> Process::GetProcessList()
{
    // 작업 관리자의 '앱'처럼 화면에 보이는 최상위 창을 가진 프로세스의 PID만 모은다.
    std::set<DWORD> appPids;

    EnumWindows([](HWND window, LPARAM param) -> BOOL
    {
        DWORD cloaked = 0;
        DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));

        if (IsWindowVisible(window)
            && GetWindow(window, GW_OWNER) == nullptr
            && GetWindowTextLengthW(window) > 0
            && cloaked == 0
            && !(GetWindowLongPtrW(window, GWL_EXSTYLE) & WS_EX_TOOLWINDOW))
        {
            DWORD pid = 0;
            GetWindowThreadProcessId(window, &pid);
            reinterpret_cast<std::set<DWORD>*>(param)->insert(pid);
        }

        return TRUE;
    }, reinterpret_cast<LPARAM>(&appPids));

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Snapshot Failed: "
                  << GetLastError() << '\n';
        return {};
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    std::vector<ProcessInfo> processList;

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (appPids.contains(entry.th32ProcessID))
            {
                processList.push_back({
                    entry.szExeFile,
                    entry.th32ProcessID
                });
            }
        }
        while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return processList;
}

ProcessHandle::ProcessHandle(DWORD pid, DWORD access)
    : handle(OpenProcess(access, FALSE, pid))
{
}

ProcessHandle::~ProcessHandle()
{
    if (handle != nullptr)
        CloseHandle(handle);
}

bool ProcessHandle::IsValid() const
{
    return handle != nullptr;
}

bool ProcessHandle::Terminate(UINT exitCode) const
{
    return handle != nullptr && TerminateProcess(handle, exitCode) != FALSE;
}

bool Process::KillProcess(const std::wstring& exeName, DWORD pid)
{
    // 서버에서 온 정보를 그대로 믿지 않고, 지금 실제 목록에 있는지 확인한다.
    std::vector<ProcessInfo> processList = GetProcessList();

    for (const ProcessInfo& info : processList)
    {
        if (info.pid != pid || info.exeName != exeName)
            continue;

        ProcessHandle target(pid, PROCESS_TERMINATE);

        if (!target.IsValid())
        {
            std::cerr << "OpenProcess Failed: " << GetLastError() << '\n';
            return false;
        }

        if (!target.Terminate(1))
        {
            std::cerr << "TerminateProcess Failed: " << GetLastError() << '\n';
            return false;
        }

        return true;
    }

    std::cerr << "Process Not Found: PID " << pid << '\n';
    return false;
}

std::string NetWorkSendServer::ToUtf8(const std::wstring& wide)
{
    if (wide.empty()) return {};
    const int length = static_cast<int>(wide.size());
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), length, nullptr, 0, nullptr, nullptr);
    std::string utf8(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), length, utf8.data(), size, nullptr, nullptr);
    return utf8;
}

std::wstring NetWorkSendServer::FromUtf8(const std::string& utf8)
{
    if (utf8.empty()) return {};
    const int length = static_cast<int>(utf8.size());
    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), length, nullptr, 0);
    std::wstring wide(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), length, wide.data(), size);
    return wide;
}

std::string NetWorkSendServer::GetEnvOr(const char* name, const std::string& fallback)
{
    char* raw = nullptr;
    size_t length = 0;

    if (_dupenv_s(&raw, &length, name) != 0 || !raw)
        return fallback;

    std::unique_ptr<char, decltype(&std::free)> value(raw, &std::free);

    return std::string(value.get());
}

void NetWorkSendServer::SendServerPost(cpr::Url url)
{
    std::vector<ProcessInfo> process_list = process.GetProcessList();

    const std::string api_key = GetEnvOr("COMPUTER_DEFENDER_API_KEY", "");

    // 전체 목록을 한 번에 보내야 서버가 종료된 앱을 목록에서 뺄 수 있다.
    json_data = nlohmann::json::array();

    for (const ProcessInfo& process_info : process_list)
    {
        json_data.push_back({
            {"name", ToUtf8(process_info.exeName)},
            {"pid", process_info.pid}
        });
    }

    // 목록이 그대로면 보내지 않는다. 서버 메모리가 초기화될 수 있으니 30초마다는 다시 보낸다.
    const auto now = std::chrono::steady_clock::now();
    if (json_data == last_sent_data && now - last_sent_time < std::chrono::seconds(30))
        return;

    response = cpr::Post(
        url,
        cpr::Header{{"Content-Type", "application/json"}, {"X-API-Key", api_key}},
        cpr::Body{json_data.dump()},
        cpr::Timeout{REQUEST_TIMEOUT}
    );

    // 타임아웃이나 연결 실패는 status_code가 0이고 이유는 error에 담긴다.
    if (response.error)
    {
        std::cerr << "POST Failed: " << response.error.message << '\n';
        return;
    }

    // 전송에 실패하면 다음 확인 때 다시 보내도록 기록하지 않는다.
    if (response.status_code == 200)
    {
        last_sent_data = json_data;
        last_sent_time = now;
    }

    // 3. 응답 결과 확인
    std::cout << "Status Code: " << response.status_code << '\n';
    std::cout << "Response Body: " << response.text << '\n';
}

void NetWorkSendServer::ReceiveCommands(cpr::Url url)
{
    const std::string api_key = GetEnvOr("COMPUTER_DEFENDER_API_KEY", "");

    cpr::Response command_response = cpr::Get(
        url,
        cpr::Header{{"X-API-Key", api_key}},
        cpr::Timeout{REQUEST_TIMEOUT}
    );

    if (command_response.error)
    {
        std::cerr << "GET Commands Failed: " << command_response.error.message << '\n';
        return;
    }

    if (command_response.status_code != 200)
        return;

    // 형식이 틀린 응답이 와도 프로그램이 죽지 않도록 예외 없이 파싱한다.
    nlohmann::json commands = nlohmann::json::parse(command_response.text, nullptr, false);
    if (!commands.is_array())
        return;

    for (const nlohmann::json& command : commands)
    {
        if (!command.contains("name") || !command["name"].is_string()
            || !command.contains("pid") || !command["pid"].is_number_unsigned())
            continue;

        const std::wstring name = FromUtf8(command["name"].get<std::string>());
        const DWORD pid = command["pid"].get<DWORD>();

        const bool killed = process.KillProcess(name, pid);
        std::cout << "Kill " << command["name"].get<std::string>()
                  << " (PID: " << pid << "): "
                  << (killed ? "OK" : "FAILED") << '\n';
    }
}

int main()
{
    // 관리자 권한이 없으면 UAC로 다시 실행하고 지금 프로세스는 끝낸다.
    if (!Privilege::IsRunningAsAdmin())
    {
        std::cout << "Not running as administrator. Requesting elevation..." << std::endl;

        if (Privilege::RelaunchAsAdmin())
            return 0;

        std::cerr << "Continuing without administrator rights. Some processes cannot be terminated.\n";
    }
    else if (!Privilege::EnableDebugPrivilege())
    {
        std::cerr << "Enable SeDebugPrivilege Failed: " << GetLastError() << '\n';
    }

    NetWorkSendServer server_send;

    const std::string server_url = server_send.GetEnvOr("COMPUTER_DEFEND_URL", "");

    std::cout << server_url << std::endl;

    while (true) 
    {
        server_send.SendServerPost(cpr::Url{server_url + "/api/processes"});
        server_send.ReceiveCommands(cpr::Url{server_url + "/api/commands"});
        // 1초마다 확인하고, 바뀌었을 때만 실제로 전송한다.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}