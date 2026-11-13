#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <thread>
#include <chrono>

#include "main.hpp"
#include "cpr/api.h"

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

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

    HANDLE snapshot =
        CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

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
        cpr::Body{json_data.dump()}
    );

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
        cpr::Header{{"X-API-Key", api_key}}
    );

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
    NetWorkSendServer server_send;

    const std::string server_url = server_send.GetEnvOr("COMPUTER_DEFENDER_URL", "http://127.0.0.1:8000");

    while (true) 
    {
        server_send.SendServerPost(cpr::Url{server_url + "/api/processes"});
        server_send.ReceiveCommands(cpr::Url{server_url + "/api/commands"});
        // 1초마다 확인하고, 바뀌었을 때만 실제로 전송한다.
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}