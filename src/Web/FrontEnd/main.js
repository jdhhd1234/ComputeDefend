const num_list = document.querySelector('#num_list');
const process_list = document.querySelector('#process_list');
const test_button = document.querySelector('#test_button')
const api_key_input = document.querySelector('#api_key');

// Live Server(5500)에서는 로컬 FastAPI로, 배포 환경에서는 같은 도메인으로 연결한다.
const is_live_server = location.port === '5500';
const api_base = is_live_server ? 'http://127.0.0.1:8000' : '';
const is_local = ['127.0.0.1', 'localhost'].includes(location.hostname);

process_list.remove();

let source = null;

function renderProcesses(processes) {
    num_list.replaceChildren();
    for (const { name, pid } of processes) {
        const li = document.createElement('li');

        const label = document.createElement('span');
        label.className = 'process_name';
        label.textContent = `${name} (PID: ${pid})`;

        const kill_button = document.createElement('button');
        kill_button.className = 'btn btn_danger';
        kill_button.textContent = '프로세스 종료';
        kill_button.addEventListener('click', () => killProcess(name, pid, kill_button));

        li.append(label, kill_button);
        num_list.appendChild(li);
    }
}

// 브라우저 → FastAPI → C++ 순으로 {name, pid} JSON이 전달된다.
// 실제로 종료되면 C++이 보내는 새 목록이 SSE로 와서 항목이 사라진다.
async function killProcess(name, pid, button) {
    if (!confirm(`${name} (PID: ${pid}) 프로세스를 종료할까요?`)) return;

    button.disabled = true;
    try {
        const response = await fetch(`${api_base}/api/processes/kill`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'X-API-Key': api_key_input.value,
            },
            body: JSON.stringify({ name, pid }),
        });

        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }
        button.textContent = '종료 요청됨';
    } catch (error) {
        alert(`종료 요청 실패: ${error.message}`);
        button.disabled = false;
    }
}

function showError(message) {
    const li = document.createElement('li');
    li.textContent = message;
    num_list.replaceChildren(li);
}

// 한 번 연결하면 서버가 목록 변경 시마다 푸시해 주므로 새로고침이 필요 없다.
// 끊겨도 EventSource가 알아서 재연결한다.
function connectProcesses() {
    if (source) source.close();

    const key = encodeURIComponent(api_key_input.value);
    source = new EventSource(`${api_base}/api/processes/stream?api_key=${key}`);

    source.onmessage = (event) => renderProcesses(JSON.parse(event.data));
    source.onerror = () => {
        // 401 같은 오류 응답이면 재연결을 멈추고 CLOSED가 된다.
        if (source.readyState === EventSource.CLOSED) {
            showError('SSE 연결 실패 (API 키 또는 서버 확인)');
        }
    };
}

// 로컬은 바로 연결하고, 배포 환경은 API 키 입력이 필요하므로 버튼을 눌렀을 때만 연결한다.
if (is_local) {
    connectProcesses();
}
test_button.addEventListener('click', connectProcesses);
