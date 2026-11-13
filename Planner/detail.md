# 2026/11/12

- 오늘할것들
1. 새로고침이나 API Key를 재입력 하지 않고도. 
새로운 프로세스가 추가되면 웹사이트에서 실시간 WebSocket으로 새로고침 없이 동기화할 예정.
2. 웹사이트에서 받은 프로세스 목록에서 특정 프로세스를 종료하라고 명령을 WebSocket으로 내리면. 
C++ CPR이 그 WebSocket 명령을 받고 실제로 특정 프로세스를 C++ WinAPI에서 TerminateProcess로 종료하는것.

# 2026/11/13

오늘할것

- 프로세스 종료 시스탬 만들기.
1. HTML/CSS/JS에서 "프로세스 종료" 버튼을 추가한다.
2. 버튼을 누르면 
- JavaScript Fetch
- Python FastAPI Server
- C++ exe
순으로 전달이 된다.
그리고 전달은 JSON형태로 전달이 된다
```JSON
{
    "name": processname(string),
    "pid": pid(int)
}
```

3. C++ exe파일에서 탐색을 통해 해당 JSON과 일치하는 정보를 std::vector에서 찾고 발견하면 
TerminateProcess를 통해 종료시킨다 단 이 경우에도 아래 규칙을 따라야 한다.

- TerminateProcess를 사용하기 위해 OpenProcess를 사용하며, 메모리 누수를 방지하기 위해 CloseHandle을 사용하고 hpp파일에서 RAII를 사용한다.
- 코드는 복잡하게 operator, 이동생성자, 복사 생성자를 자제해야하며 또한 가급적 기본 생성자(매개변수는 허용)를 써야한다