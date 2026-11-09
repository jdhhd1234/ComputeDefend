#include <iostream>
#include <thread>
#include <chrono>

#include "main.hpp"
#include "cpr/api.h"

std::vector<ProcessInfo> Process::GetProcessList()
{
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
            processList.push_back({
                entry.szExeFile,
                entry.th32ProcessID
            });
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

void NetWorkSendServer::SendServerPost(cpr::Url url)
{
    std::vector<ProcessInfo> process_list = process.GetProcessList();

    for (const ProcessInfo& process_info : process_list)
    {
        json_data = {
            {"name", ToUtf8(process_info.exeName)},
            {"pid", process_info.pid}
        };

        response = cpr::Post(
            url,
            cpr::Header{{"Content-Type", "application/json"}},
            cpr::Body{json_data.dump()}
        );

        // 3. 응답 결과 확인
        std::cout << "Status Code: " << response.status_code << '\n';
        std::cout << "Response Body: " << response.text << '\n';
    }
}

int main()
{
    NetWorkSendServer server_send;

    while (true) {
        server_send.SendServerPost({"http://127.0.0.1:8000/api/processes"});
        // 10초 동안 스레드를 일시 중지
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return 0;
}