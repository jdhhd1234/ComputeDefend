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

std::string NetWorkSendServer::ToUtf8(const std::wstring& wide)
{
    if (wide.empty()) return {};
    const int length = static_cast<int>(wide.size());
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), length, nullptr, 0, nullptr, nullptr);
    std::string utf8(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), length, utf8.data(), size, nullptr, nullptr);
    return utf8;
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

    response = cpr::Post(
        url,
        cpr::Header{{"Content-Type", "application/json"}, {"X-API-Key", api_key}},
        cpr::Body{json_data.dump()}
    );

    // 3. 응답 결과 확인
    std::cout << "Status Code: " << response.status_code << '\n';
    std::cout << "Response Body: " << response.text << '\n';
}

int main()
{
    NetWorkSendServer server_send;

    const std::string server_url = server_send.GetEnvOr("COMPUTER_DEFENDER_URL", "http://127.0.0.1:8000");

    while (true) {
        server_send.SendServerPost(cpr::Url{server_url + "/api/processes"});
        // 10초 동안 스레드를 일시 중지
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    return 0;
}